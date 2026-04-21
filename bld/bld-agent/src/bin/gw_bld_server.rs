//! `gw_bld_server` — the MCP-over-stdio entry-point.
//!
//! Wires the pure-Rust MCP transport in `bld-mcp` to the RAG-backed
//! tool dispatcher in `bld-agent::tools_impl`. Intended use:
//!
//! ```bash
//! cargo run --features server -p bld-agent --bin gw_bld_server -- \
//!   --project-root /path/to/project
//! ```
//!
//! The server reads JSON-RPC frames from stdin, writes responses to
//! stdout, and logs diagnostics to stderr. The editor launches this
//! binary as a child process (ADR-0011 §2.1) and speaks MCP over the
//! inherited stdio pipes.

use std::path::PathBuf;
use std::sync::Arc;

use bld_agent::{FsFileLoader, RagDispatcher, ToolDispatcher};
use bld_mcp::schema::Tool;
use bld_mcp::server::{Handler, McpServer, ServerError};
use bld_tools::registry;
use serde_json::{json, Value};

/// Parsed command-line flags.
#[derive(Debug)]
struct Cli {
    project_root: PathBuf,
    preindex_exts: Vec<String>,
}

impl Cli {
    fn parse() -> Result<Self, String> {
        let mut args = std::env::args().skip(1);
        let mut project_root: Option<PathBuf> = None;
        let mut preindex_exts: Vec<String> =
            vec!["md".into(), "rs".into(), "cpp".into(), "hpp".into(), "h".into()];

        while let Some(a) = args.next() {
            match a.as_str() {
                "--project-root" => {
                    let v = args
                        .next()
                        .ok_or_else(|| "--project-root needs a value".to_owned())?;
                    project_root = Some(PathBuf::from(v));
                }
                "--preindex-exts" => {
                    let v = args
                        .next()
                        .ok_or_else(|| "--preindex-exts needs a value".to_owned())?;
                    preindex_exts = v.split(',').map(|s| s.trim().to_owned()).collect();
                }
                "--help" | "-h" => {
                    eprintln!(
                        "gw_bld_server [--project-root PATH] [--preindex-exts rs,md,cpp]"
                    );
                    std::process::exit(0);
                }
                other => return Err(format!("unknown flag: {other}")),
            }
        }

        let project_root = project_root
            .unwrap_or_else(|| std::env::current_dir().unwrap_or_else(|_| PathBuf::from(".")));
        Ok(Cli {
            project_root,
            preindex_exts,
        })
    }
}

/// Bridges `bld-mcp::Handler` (sync JSON in / JSON out) to the
/// `RagDispatcher`. Every tool the dispatcher knows about is routed;
/// anything else returns `ToolError`-style content so the agent can see
/// the error and retry.
struct McpHandler {
    dispatcher: Arc<dyn ToolDispatcher>,
}

impl Handler for McpHandler {
    fn call(&self, tool_id: &str, args: &Value) -> Result<Value, ServerError> {
        self.dispatcher
            .dispatch(tool_id, args)
            .map_err(ServerError::Tool)
    }

    fn list(&self) -> Vec<Tool> {
        registry::all()
            .map(|d| Tool {
                name: d.id.to_owned(),
                description: d.summary.to_owned(),
                input_schema: json!({ "type": "object", "additionalProperties": true }),
                tier: Some(format!("{}", d.tier)),
            })
            .collect()
    }
}

fn install_tracing() {
    // stderr-only, so we don't corrupt the MCP stdio stream on stdout.
    let _ = tracing_subscriber::fmt()
        .with_writer(std::io::stderr)
        .with_target(false)
        .with_env_filter("info,bld=debug")
        .try_init();
}

#[tokio::main(flavor = "current_thread")]
async fn main() -> std::io::Result<()> {
    let cli = match Cli::parse() {
        Ok(c) => c,
        Err(e) => {
            eprintln!("arg error: {e}");
            std::process::exit(2);
        }
    };
    install_tracing();

    tracing::info!(
        project = %cli.project_root.display(),
        exts = ?cli.preindex_exts,
        "starting gw_bld_server"
    );

    let loader = Box::new(FsFileLoader::new(cli.project_root.clone()));
    let dispatcher = Arc::new(RagDispatcher::new(loader));
    // Fire-and-forget bootstrap so `tools/list` returns immediately even
    // if the project has thousands of files.
    let bootstrap_dispatcher = Arc::clone(&dispatcher);
    let exts = cli.preindex_exts.clone();
    tokio::task::spawn_blocking(move || {
        let exts_ref: Vec<&str> = exts.iter().map(String::as_str).collect();
        let n = bootstrap_dispatcher.bootstrap_from_disk(&exts_ref);
        tracing::info!(indexed = n, "initial RAG bootstrap complete");
    });

    let handler = Box::new(McpHandler {
        dispatcher: dispatcher as Arc<dyn ToolDispatcher>,
    });
    let server = McpServer::new(handler);
    let stdin = tokio::io::stdin();
    let stdout = tokio::io::stdout();
    bld_mcp::server::stdio::run(&server, stdin, stdout).await?;
    tracing::info!("gw_bld_server shutdown clean");
    Ok(())
}

