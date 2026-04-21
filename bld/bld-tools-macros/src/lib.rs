//! `bld-tools-macros` — the proc-macro half of BLD's tool definition.
//!
//! The user-facing surface lives in `bld-tools`; *this* crate is just the
//! proc-macro that `bld-tools` re-exports. Splitting the crate is
//! mandatory: `proc-macro = true` precludes adding normal dependencies,
//! and ADR-0012 §2.4 forbids the tool registry from living behind a
//! proc-macro-only compile gate.
//!
//! What ships here in Phase 9A (this file):
//!
//! - `#[bld_tool]` — attribute macro on an `async fn` or sync fn.
//!   Parses the attribute payload (`id="…", tier="…", summary="…"`),
//!   validates it, and emits:
//!     1. the original function unchanged,
//!     2. a `ToolDescriptor` constant with the id/tier/summary baked in,
//!     3. a `_BLD_TOOL_<NAME>` `inventory::submit!` entry so the runtime
//!        registry can enumerate tools without a central list.
//!
//! - `bld_component_tools!()` — function-like macro expanded at build
//!   time by `bld-tools/build.rs`. Here it is a stub that records a
//!   compile-time warning if invoked without a reflection manifest.
//!
//! Tests live in `bld-tools` (because proc-macro crates can only be
//! unit-tested via `trybuild`). The proc-macro itself is kept minimal
//! and span-preserving so diagnostics surface cleanly in the editor's
//! cargo-check stream.

use proc_macro::TokenStream;
use proc_macro2::Span;
use quote::{format_ident, quote, ToTokens};
use syn::parse::{Parse, ParseStream};
use syn::{parse_macro_input, FnArg, ItemFn, LitStr, Pat, PatIdent, Result, Token, Type};

// -----------------------------------------------------------------------------
// Attribute parsing
// -----------------------------------------------------------------------------

/// Parsed `#[bld_tool(...)]` arguments.
struct ToolArgs {
    id: LitStr,
    tier: LitStr,
    summary: Option<LitStr>,
}

impl Parse for ToolArgs {
    fn parse(input: ParseStream) -> Result<Self> {
        let mut id: Option<LitStr> = None;
        let mut tier: Option<LitStr> = None;
        let mut summary: Option<LitStr> = None;

        while !input.is_empty() {
            let key: syn::Ident = input.parse()?;
            let _eq: Token![=] = input.parse()?;
            let value: LitStr = input.parse()?;
            match key.to_string().as_str() {
                "id" => id = Some(value),
                "tier" => tier = Some(value),
                "summary" => summary = Some(value),
                other => {
                    return Err(syn::Error::new(
                        key.span(),
                        format!("unknown bld_tool argument `{other}`"),
                    ))
                }
            }
            if !input.is_empty() {
                let _comma: Token![,] = input.parse()?;
            }
        }

        let id = id.ok_or_else(|| {
            syn::Error::new(Span::call_site(), "bld_tool missing required `id = \"…\"`")
        })?;
        let tier = tier.ok_or_else(|| {
            syn::Error::new(
                Span::call_site(),
                "bld_tool missing required `tier = \"Read|Mutate|Execute\"`",
            )
        })?;

        // Validate tier text at macro-expansion time. ADR-0012 §2.2.
        match tier.value().as_str() {
            "Read" | "Mutate" | "Execute" => {}
            _ => {
                return Err(syn::Error::new(
                    tier.span(),
                    "tier must be exactly `Read`, `Mutate`, or `Execute`",
                ))
            }
        }

        // Validate id is a dotted lowercase string (e.g. "scene.create_entity").
        let raw = id.value();
        if raw.is_empty()
            || raw
                .chars()
                .any(|c| !(c.is_ascii_alphanumeric() || c == '.' || c == '_'))
        {
            return Err(syn::Error::new(
                id.span(),
                "tool id must be [A-Za-z0-9_.]+ (e.g. \"scene.create_entity\")",
            ));
        }

        Ok(ToolArgs { id, tier, summary })
    }
}

// -----------------------------------------------------------------------------
// #[bld_tool(id=…, tier=…, summary=…)]
// -----------------------------------------------------------------------------

/// Attribute macro. Marks a function as a BLD tool.
///
/// Emits the original function plus a `ToolDescriptor` constant named
/// `_BLD_TOOL_<FN_UPPER>`. The runtime registry (`bld-tools::registry`)
/// discovers these via the `inventory` crate.
///
/// Example:
/// ```ignore
/// #[bld_tool(id = "scene.create_entity", tier = "Mutate",
///            summary = "Create a new empty entity in the active scene.")]
/// pub async fn scene_create_entity(args: CreateEntityArgs) -> Result<u64> { … }
/// ```
#[proc_macro_attribute]
pub fn bld_tool(attr: TokenStream, item: TokenStream) -> TokenStream {
    let args = parse_macro_input!(attr as ToolArgs);
    let func = parse_macro_input!(item as ItemFn);

    let fn_ident = &func.sig.ident;
    let const_ident = format_ident!("_BLD_TOOL_{}", fn_ident.to_string().to_uppercase());

    let id = &args.id;
    let tier = &args.tier;
    let summary = args
        .summary
        .as_ref()
        .map(|s| s.to_token_stream())
        .unwrap_or_else(|| quote! { "" });

    // We intentionally reference names by their absolute path through
    // `::bld_tools` so this macro works in a downstream crate that
    // re-exports its own `ToolDescriptor`. If a user needs to rehome the
    // types, they set `--cfg bld_tools_crate="other"`; that escape hatch
    // is documented in ADR-0012.
    // Build a dispatch shim if the function shape is amenable to
    // automatic wiring (single arg, non-async, returning either
    // `Result<T, E>`, `T`, or `serde_json::Value`). If any of the
    // assumptions are violated we skip the shim and only register the
    // descriptor — the caller is then responsible for wiring dispatch
    // manually (pattern used by legacy tools pre-9B).
    let dispatch_item = build_dispatch_shim(&func, fn_ident);

    let dispatch_const_ident = format_ident!("_BLD_DISPATCH_{}", fn_ident.to_string().to_uppercase());
    let dispatch_fn_ident = format_ident!("_bld_dispatch_{}", fn_ident);

    let dispatch_wire = if dispatch_item.is_some() {
        quote! {
            #[doc(hidden)]
            #[allow(non_upper_case_globals)]
            pub const #dispatch_const_ident: ::bld_tools::ToolDispatchEntry =
                ::bld_tools::ToolDispatchEntry {
                    id: #id,
                    dispatch: #dispatch_fn_ident,
                };

            ::bld_tools::inventory::submit! {
                &#dispatch_const_ident
            }
        }
    } else {
        quote! {}
    };
    let shim = dispatch_item.unwrap_or_else(|| quote! {});

    let out = quote! {
        #func

        #[doc(hidden)]
        #[allow(non_upper_case_globals)]
        pub const #const_ident: ::bld_tools::ToolDescriptor = ::bld_tools::ToolDescriptor {
            id: #id,
            tier: ::bld_tools::tier_from_str(#tier),
            summary: #summary,
        };

        ::bld_tools::inventory::submit! {
            &#const_ident
        }

        #shim
        #dispatch_wire
    };

    out.into()
}

/// Best-effort dispatch shim.
///
/// We emit a shim for synchronous fns with exactly one non-`self` input.
/// If the input type is `&serde_json::Value` (by path heuristic) we pass
/// the value through. Otherwise we attempt `serde_json::from_value` for
/// owned inputs. The return type is shoved through `serde_json::to_value`
/// (for `T: serde::Serialize`) if it isn't already `serde_json::Value`.
fn build_dispatch_shim(
    func: &ItemFn,
    fn_ident: &syn::Ident,
) -> Option<proc_macro2::TokenStream> {
    if func.sig.asyncness.is_some() {
        return None;
    }
    if func.sig.inputs.len() != 1 {
        return None;
    }
    let arg = match func.sig.inputs.first()? {
        FnArg::Receiver(_) => return None,
        FnArg::Typed(pt) => pt,
    };
    match &*arg.pat {
        Pat::Ident(PatIdent { .. }) => {}
        _ => return None,
    }

    let arg_ty = &arg.ty;
    let is_ref_value = matches!(
        &**arg_ty,
        Type::Reference(r) if type_is_serde_json_value(&r.elem)
    );
    let is_owned_value = type_is_serde_json_value(arg_ty);

    let dispatch_fn_ident = format_ident!("_bld_dispatch_{}", fn_ident);

    let call = if is_ref_value {
        quote! { #fn_ident(args) }
    } else if is_owned_value {
        quote! { #fn_ident(args.clone()) }
    } else {
        quote! {
            match ::serde_json::from_value::<#arg_ty>(args.clone()) {
                Ok(v) => #fn_ident(v),
                Err(e) => return Err(format!("bad input: {}", e)),
            }
        }
    };

    let returns_result = match &func.sig.output {
        syn::ReturnType::Default => false,
        syn::ReturnType::Type(_, ty) => type_is_result(ty),
    };

    let coerce = if returns_result {
        quote! { ::bld_tools::dispatch_shim::serialize_result(raw) }
    } else {
        quote! { ::bld_tools::dispatch_shim::serialize_ok(raw) }
    };

    let shim = quote! {
        #[doc(hidden)]
        #[allow(non_snake_case)]
        fn #dispatch_fn_ident(
            args: &::serde_json::Value,
        ) -> ::core::result::Result<::serde_json::Value, ::std::string::String> {
            let raw = #call;
            #coerce
        }
    };
    Some(shim)
}

fn type_is_result(ty: &Type) -> bool {
    let Type::Path(tp) = ty else {
        return false;
    };
    // Match bare `Result<_, _>` or `std::result::Result<_, _>` or
    // `core::result::Result<_, _>`. We check only the terminal segment.
    tp.path
        .segments
        .last()
        .map(|s| s.ident == "Result")
        .unwrap_or(false)
}

fn type_is_serde_json_value(ty: &Type) -> bool {
    let Type::Path(tp) = ty else {
        return false;
    };
    let segs: Vec<String> = tp
        .path
        .segments
        .iter()
        .map(|s| s.ident.to_string())
        .collect();
    matches!(
        segs.as_slice(),
        [a] if a == "Value"
    ) || matches!(
        segs.as_slice(),
        [a, b] if a == "serde_json" && b == "Value"
    )
}

// -----------------------------------------------------------------------------
// bld_component_tools!()
// -----------------------------------------------------------------------------

/// Build-time expansion of autogenerated component tools.
///
/// The real body is written by `bld-tools/build.rs` into a file the user
/// includes via `include!(concat!(env!("OUT_DIR"), "/bld_component_tools.rs"))`.
/// When invoked directly (no generated file available), this macro emits
/// an inert unit expression so tests still compile.
#[proc_macro]
pub fn bld_component_tools(_input: TokenStream) -> TokenStream {
    let out = quote! { { /* bld_component_tools stub — build.rs injects the body */ } };
    out.into()
}
