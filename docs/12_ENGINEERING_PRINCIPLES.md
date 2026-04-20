# 12_ENGINEERING_PRINCIPLES — Code-Review Playbook

**Status:** Canonical · Binding at review
**Audience:** Anyone writing C++ or Rust code for Greywater_Engine · Anyone reviewing a PR
**Enforcement:** Every item below is a gate at Review. A violation without a written justification blocks the merge.

---

## Why this document exists

Greywater's codebase will outlive the first title. Code we write in 2026 will be debugged in 2029 by engineers who weren't in the room. The difference between a codebase that survives that transition and one that rots is **disciplined review, consistently applied, at every commit**.

This playbook is the rubric. It distills the patterns we will enforce on every Greywater pull request, adapted to our stack (C++23 engine + Rust BLD + ImGui editor + Vulkan 1.2 baseline, RX 580 target).

Every rule below has a cost when you break it. We write them down so we pay that cost at most once.

Read this file before you start a Phase 2 card. Re-read it before every Review.

---

## Section A — Memory and Ownership

### A1. No raw `new` / `delete` in engine code

**Rule.** Never write `new T(...)` or `delete ptr` in `engine/`, `editor/`, `sandbox/`, `runtime/`, `gameplay/` unless the code is inside an allocator implementation or the one-line entry-point bootstrap.

**Why.** Raw heap allocation bleeds ownership into the ambient type system — a caller cannot tell whether they are supposed to delete the result or not. Every codebase that allows it eventually has a double-free, a leak, or a use-after-free that is invisible in the diff.

**What to use instead.**
- **Unique ownership:** `std::unique_ptr<T>` (stored on the stack, passed by value to transfer, by reference to loan).
- **Shared ownership, single-threaded:** `std::shared_ptr<T>` — use only when ownership is genuinely shared; see §A4.
- **Shared ownership, multi-threaded across the render thread or asset thread:** the **intrusive reference-counted `gw::Ref<T>`** primitive in `engine/core/`. Prefer this over `std::shared_ptr` because it avoids the separate control block allocation and is the codebase convention; see §C2.
- **Handles over pointers** for resources whose owner is not the caller: `EntityHandle`, `TextureHandle`, `BufferHandle`. Plain 64-bit values with generation, cheap to copy, safe to store.

**The one exception.** The entry point's `main()` may construct and destroy the `Application` object with direct ownership semantics because both the creation and the destruction are visible within a handful of lines in a single function. This exception is *only* granted to code in `sandbox/main.cpp`, `editor/main.cpp`, `runtime/main.cpp`. Anywhere else, it is a review-blocking violation.

```cpp
// FORBIDDEN in engine code:
auto* buffer = new Buffer(size);
use(buffer);
delete buffer;

// REQUIRED:
auto buffer = std::make_unique<Buffer>(size);
use(*buffer);
// (destructor runs at scope exit, no manual delete)
```

### A2. No two-phase construction / destruction on public APIs

**Rule.** Don't expose `init()` and `shutdown()` as public members of a class that a caller is supposed to call in sequence around the constructor and destructor. If a class needs delayed init (e.g., GPU resources that can only be created after the Vulkan device is up), hide it — `init()` and `shutdown()` go **protected or private**, and the owner calls them on behalf of the caller.

**Why.** Public two-phase init creates an invariant no compiler can check: "after you constructed this object, you must also call `init()` before using it." Any caller who forgets gets undefined behaviour. Worse, the object can also get destroyed without `shutdown()`, leaking resources.

**What to use instead.**

- **Prefer RAII.** If resources can be acquired in the constructor, acquire them there. Let the destructor release them.
- **If delayed creation is genuinely required** (and it often is, for GPU resources), express the two-phase nature **structurally**: the object is dormant after construction and only becomes active via a single explicit `activate()` call that is clearly documented and return-value-checked. Pair it with an `is_active()` predicate so callers can guard against misuse, and mark `activate()` protected if only the owner should call it.

```cpp
// FORBIDDEN: public two-phase init
class Application {
public:
    Application(const AppSpec& spec);   // sets members only
    void init();                         // caller must remember
    void shutdown();                     // caller must remember
    ~Application();                      // might or might not also run shutdown
};

// PREFERRED: RAII
class Application {
public:
    Application(const AppSpec& spec);    // acquires everything
    ~Application();                      // releases everything
    // no init/shutdown on public surface
};

// ACCEPTABLE when delayed creation is unavoidable:
class VulkanBuffer {
public:
    VulkanBuffer(const BufferSpec& spec);  // stores spec, no GPU work
    ~VulkanBuffer();                       // idempotent cleanup

    [[nodiscard]] bool is_active() const noexcept { return handle_ != VK_NULL_HANDLE; }

protected:
    friend class Device;
    void activate();                       // called by Device during its init
};
```

### A3. Containers own their contents

**Rule.** A container with a shutdown responsibility owns the objects it contains via `unique_ptr` (or `gw::Ref`). It does **not** hand out raw pointers, have callers iterate and delete, or rely on an external teardown sequence.

**Why.** If a `LayerStack` stores raw pointers and `Application::~Application()` iterates and deletes them, the ownership is implicit and the destruction order is fragile. Move the lifecycle into the container. One owner, clear rules.

```cpp
// FORBIDDEN:
class LayerStack {
    std::vector<Layer*> layers_;
public:
    void push(Layer* layer) { layers_.push_back(layer); }
    // destructor leaves layers_ dangling; caller is expected to delete
};

// REQUIRED:
class LayerStack {
    std::vector<std::unique_ptr<Layer>> layers_;
public:
    void push(std::unique_ptr<Layer> layer) { layers_.push_back(std::move(layer)); }
    // destructor runs and every layer is destroyed in reverse order automatically
    // if shutdown order matters, override destructor and iterate explicitly in reverse
};
```

**Shutdown order note.** When `~Container()` must tear down in a specific order (reverse of insertion, say), write it explicitly. `std::vector` destroys elements in the order they are stored, which is usually not what you want for subsystem dependencies:

```cpp
LayerStack::~LayerStack() {
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
        (*it)->on_detach();
        // unique_ptr destructor runs as the element is erased
    }
    layers_.clear();
}
```

### A4. `unique_ptr` first; `shared_ptr` only when sharing is real

**Rule.** Default to `std::unique_ptr<T>`. Reach for `std::shared_ptr<T>` or `gw::Ref<T>` only when you can name at least two distinct owners whose lifetimes are not nested.

**Why.** `shared_ptr` costs an atomic increment/decrement on every copy. In a render-thread-driven engine, those atomics land on the hot path constantly unless you are careful. `unique_ptr` costs nothing.

**Key wins:**

- A function that takes ownership: `void add(std::unique_ptr<Thing> t)` — caller moves it in, done.
- A function that borrows: `void read(const Thing& t)` or `void read(Thing& t)` — no smart pointer at all.
- A callback queue of `std::move_only_function<void()>` (**C++23**) can hold lambdas that captured `std::unique_ptr` by move. This removes the common historical justification for `shared_ptr` in callback queues.

```cpp
// HISTORICAL ANTI-PATTERN (pre-C++23):
// std::function required copyable lambdas, so capturing a unique_ptr was impossible.
// Teams reached for shared_ptr to make the lambda copyable.
std::queue<std::function<void()>> queue;
auto evt = std::make_shared<Event>(...);
queue.push([evt]{ dispatch(*evt); });    // shared_ptr for copyability only

// MODERN (C++23):
std::queue<std::move_only_function<void()>> queue;
auto evt = std::make_unique<Event>(...);
queue.push([evt = std::move(evt)]() mutable { dispatch(*evt); });
```

### A5. Pass shared ownership wisely

**Rule.** When a function is *not* taking ownership of a `std::shared_ptr<T>` or `gw::Ref<T>`, do not take it by value. Take `const std::shared_ptr<T>&`, or — better — take `const T&` or `T*`.

**Why.** Every pass-by-value of a `shared_ptr` is an atomic increment on entry and an atomic decrement on exit. In a loop or a hot path, this is measurable cost. Taking by value broadcasts "I might keep this" to readers even when the function doesn't keep it.

```cpp
// WRONG for a non-owning use:
void process(std::shared_ptr<Thing> t);              // 2 atomics per call

// BETTER:
void process(const std::shared_ptr<Thing>& t);       // 0 atomics, still clear

// BEST when you don't need the smart-pointer-ness:
void process(const Thing& t);                        // 0 atomics, clear as mud
```

### A6. Iterate `shared_ptr` / `Ref` containers by `const auto&`

**Rule.** Range-based `for` over a container of `shared_ptr` or `gw::Ref` must use `const auto&` (or `auto&` if mutating), never plain `auto`.

**Why.** `for (auto x : vec)` copies each `shared_ptr` — two atomics per iteration. Over 10k entities, this is millions of unnecessary atomic ops per frame.

```cpp
// WRONG:
for (auto entity : entities)   { ... }     // copy every iteration

// RIGHT:
for (const auto& entity : entities) { ... }
```

### A7. Move assignment must release existing resources

**Rule.** Move-assignment and move-constructor operators **must** release any resources the destination object currently holds before taking ownership of the source.

**Why.** A move assignment is not "copy the pointer and call it a day." If the destination already owns a resource, failing to release it leaks. This is one of the most common bugs in move-enabled classes, and it hides for months because it only leaks — nothing crashes.

```cpp
// BUG (leaks m_data on every move-assignment onto an existing object):
Animation& operator=(Animation&& other) noexcept {
    m_data = other.m_data;            // overwrites the existing m_data — leak
    other.m_data = nullptr;
    return *this;
}

// CORRECT:
Animation& operator=(Animation&& other) noexcept {
    if (this == &other) return *this;
    release();                         // dispose of our current data
    m_data = other.m_data;
    other.m_data = nullptr;
    return *this;
}
```

**Corollary.** When using `unique_ptr`/`gw::Ref`, move assignment works correctly by default because they release their current resource in their own assignment operator. Rule A7 is about raw-pointer members and custom owning types.

---

## Section B — Casts

### B1. No C-style casts

**Rule.** Do not write `(T)expr` or `(T*)expr` in C++ code. Use `static_cast`, `reinterpret_cast`, `const_cast`, or `dynamic_cast`.

**Why.** C-style casts silently select whichever cast compiles. That's exactly the opposite of what a review reader wants — they want to know what the author *intended*.

```cpp
// FORBIDDEN:
auto* p = (u32*)buffer.data;

// REQUIRED (and readable):
auto* p = reinterpret_cast<u32*>(buffer.data);
```

### B2. `static_cast` when the compiler can verify, `reinterpret_cast` only when it can't

- `static_cast<float>(i)` — numeric conversion, derived-to-base pointer, explicit conversion that the language knows about. Safe.
- `reinterpret_cast<u32*>(void_ptr)` — bit-level reinterpretation, no conversion code emitted, you the programmer are asserting the memory is a `u32`. **The burden of correctness is on you.**
- `const_cast` — only for interoperating with mis-const'd APIs. Flag for review.
- `dynamic_cast` — forbidden in hot paths (RTTI). Permitted in one-off tools.

### B3. Reinterpreting bytes — know the UB exposure

**Rule.** When you reinterpret a byte buffer as a typed pointer (`reinterpret_cast<u32*>(bytes)`), you are on thin ice with strict aliasing rules. Use **`std::start_lifetime_as<T>`** (C++23, when available) or `std::memcpy` for correctness. At minimum, leave a comment explaining the invariant.

**Why.** Dereferencing a `u32*` that was never the result of a live `u32` object is technically undefined behaviour under the C++ abstract machine. In practice, compilers tolerate it for the byte-buffer case, but aliasing optimisations can bite at `-O2`.

```cpp
// SURVIVES IN PRACTICE BUT UB IN THEORY:
auto* words = reinterpret_cast<u32*>(buffer.data);
u32 value = words[0];

// C++23 CORRECT:
auto* words = std::start_lifetime_as_array<u32>(buffer.data, buffer.size / sizeof(u32));
u32 value = words[0];

// PORTABLE / NEVER UB:
u32 value;
std::memcpy(&value, buffer.data, sizeof(value));
```

---

## Section C — Smart Pointers and `gw::Ref`

### C1. Prefer `gw::Ref<T>` over `std::shared_ptr<T>` in the engine

**Rule.** Inside `engine/`, when shared ownership is genuinely required across threads (render thread, asset thread), use the intrusive `gw::Ref<T>` rather than `std::shared_ptr`.

**Why.**
- Intrusive counting: the count lives in the object, so there is no separate control-block allocation.
- Debuggability: a single place (the intrusive base) to drop tracking, ID assignment, or per-type instrumentation.
- Type filtering: templates + concepts let you restrict `gw::Ref` to derived types of a particular base, giving better compile-time gates than `std::shared_ptr<Base>`.
- Consistency: the whole engine uses it, so contributors don't have to remember which pointer type to use.

`std::shared_ptr` remains fine in code outside `engine/` (editor tools, test harnesses, tooling CLIs) where the atomic cost doesn't land on the frame.

### C2. Intrusive ref counting is specifically for cross-thread lifetime

**Rule.** Reach for `gw::Ref<T>` when a resource may be read or freed by a thread other than the one that created it — typically the render thread or an asset-loading thread. **Do not** use it as a general "this might be shared" hedge.

**Why.** If lifetime is fully bounded to one thread, `std::unique_ptr` or a plain owning reference is cheaper and simpler. The whole justification for reference counting is *indeterminate inter-thread lifetime*.

### C3. Captures into cross-thread lambdas use `instance = ref<T>(this)`

**Rule.** When a `gw::Ref`-counted class submits work to another thread (the render thread, typically), the lambda must capture a strong reference to itself:

```cpp
void VulkanVertexBuffer::set_data(const void* data, u32 size) {
    auto instance = gw::ref(this);
    gw::renderer::submit([instance, data, size]() {
        instance->rt_set_data(data, size);    // executes on render thread
    });
}
```

**Why.** The lambda outlives the original caller's scope. Without a strong reference, the buffer may be destroyed on the main thread before the render thread executes. Capturing `this` by raw pointer is a dangling reference waiting to happen.

**Idiom.** In Greywater C++, methods intended to run on the render thread are prefixed `rt_`. The outer function (without the prefix) submits; the inner (`rt_`) executes on the render thread.

---

## Section D — Lambda Captures

### D1. Capture narrowly

**Rule.** `[=]` and `[&]` are forbidden in engine code. Capture each variable explicitly.

**Why.** Broad captures obscure the closure's dependencies, hide expensive copies, and in C++20+ will warn (capturing `this` implicitly via `[=]` is deprecated).

```cpp
// FORBIDDEN:
auto fn = [=]() { use(m_field); };      // captures `this` implicitly, deprecated

// REQUIRED:
auto fn = [this]() { use(m_field); };
// or:
auto fn = [&value]() { use(value); };
// or, for moved ownership:
auto fn = [buf = std::move(buffer)]() mutable { use(buf); };
```

### D2. Prefer `member = member_` over `this`-capture when only one field is needed

**Rule.** If a lambda only needs one or two members, capture them specifically rather than capturing `this`.

**Why.** It's clearer to the reader, it doesn't create a lifetime dependency on the entire enclosing object, and it prevents dangling-`this` bugs when the lambda is stored.

```cpp
// OK but captures the whole object:
auto fn = [this]() { imgui_layer_->render(); };

// BETTER:
auto fn = [layer = imgui_layer_]() { layer->render(); };
```

### D3. For cross-thread capture, capture the ref primitive, not `this`

See §C3.

---

## Section E — Strings and C APIs

### E1. `std::string_view` is not null-terminated

**Rule.** Never pass a `std::string_view::data()` to a C API that expects a null-terminated string. C APIs will read beyond the end.

**Why.** `string_view` is a (pointer, length) pair. The pointer may be mid-buffer, unterminated, or terminated only by accident.

```cpp
// BUG:
void log_label(std::string_view name) {
    ::SomeWin32API(name.data());   // expects a C-string — undefined behavior
}

// FIX — take a `const std::string&` if you must pass to a C API:
void log_label(const std::string& name) {
    ::SomeWin32API(name.c_str());
}

// OR — make a local null-terminated copy (slower, but the contract is clear):
void log_label(std::string_view name) {
    std::string owned(name);
    ::SomeWin32API(owned.c_str());
}
```

### E2. Heterogeneous lookup in hash maps (C++20)

**Rule.** When a map is keyed by `std::string` and you want to look up by `std::string_view` or `const char*` without allocating, opt into transparent comparators.

```cpp
// Old (allocates a std::string on every lookup):
std::unordered_map<std::string, Thing> map;
map.find(std::string(sv));

// Modern (no allocation on lookup):
struct StringHash {
    using is_transparent = void;
    size_t operator()(std::string_view s) const noexcept { return std::hash<std::string_view>{}(s); }
    size_t operator()(const std::string& s) const noexcept { return std::hash<std::string_view>{}(s); }
    size_t operator()(const char* s) const noexcept { return std::hash<std::string_view>{}(s); }
};
struct StringEq {
    using is_transparent = void;
    bool operator()(std::string_view a, std::string_view b) const noexcept { return a == b; }
};
std::unordered_map<std::string, Thing, StringHash, StringEq> map;
map.find(sv);   // no allocation
```

### E3. Windows wide strings — don't assume ASCII round-trip

**Rule.** Do **not** construct a `std::wstring` by iterating `std::string` characters and casting each `char` to `wchar_t`. Use `MultiByteToWideChar(CP_UTF8, ...)` (Windows) to correctly convert UTF-8 to UTF-16.

**Why.** The single-char copy only works for 7-bit ASCII. Anything else — emoji, accented characters, Cyrillic — produces garbage. `std::wstring_convert` is deprecated in C++17 and removed in C++26; do not use it.

```cpp
// FORBIDDEN:
std::wstring w;
for (char c : s) w.push_back(static_cast<wchar_t>(c));   // UTF-8 becomes garbage

// REQUIRED (Windows platform code only, inside engine/platform/):
int size_needed = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
std::wstring w(size_needed, L'\0');
::MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), size_needed);
```

### E4. Prefer `const std::string&` in interfaces that feed C APIs

**Rule.** If a function is going to pass its input to a C API (via `.c_str()` or similar), accept `const std::string&`, not `std::string_view`.

**Why.** It removes an entire class of bug (§E1) at the API boundary. The minor cost of a `std::string` construction at the call site is paid once and exactly once.

### E5. Parse once, access many

**Rule.** If settings are loaded from TOML/JSON/etc., **parse them into their final typed form at load time.** Do not store `std::string` and parse-to-float on every access.

**Why.** Every `settings.get_float("gravity")` that does `std::stof` is an allocation and a parse every frame. Store `settings.gravity` as `float` once; read it forever.

```cpp
// FORBIDDEN for runtime-hot reads:
float g = settings.get_float("gravity");    // string lookup + parse, every frame

// REQUIRED:
struct PhysicsSettings { float gravity; float fixed_step; };
auto physics_settings = load_physics_settings();
// thereafter:
float g = physics_settings.gravity;    // direct access, zero cost
```

---

## Section F — Event Dispatch and std::function

### F1. Do not heap-allocate transient event objects

**Rule.** For synchronous event dispatch (e.g., input events, window events), construct the event on the stack and pass it by reference. Do not `make_shared` or `make_unique` for a transient event.

```cpp
// FORBIDDEN:
auto evt = std::make_shared<KeyPressedEvent>(key_code, is_repeat);
callback(*evt);       // heap allocation for a stack-shaped thing

// REQUIRED:
KeyPressedEvent evt(key_code, is_repeat);
callback(evt);        // zero allocations
```

### F2. `std::function` may heap-allocate captured state

**Rule.** Know that `std::function<void()>` may allocate on the heap when the captured state exceeds the small-buffer optimisation. This is not a "may" in a theoretical sense — it is measurable in hot paths.

**Mitigations:**
- C++23 `std::move_only_function` — allows move-only captures (including `unique_ptr`), may or may not heap-allocate depending on state size.
- For the engine's per-frame work queues, use a custom erased-callable type with a known inline buffer size to eliminate heap allocation entirely.

### F3. Event queues of lambdas: prefer move-only

**Rule.** The engine's main-thread and render-thread work queues use `std::move_only_function<void()>`. Not `std::function`. Do not change this without an ADR.

---

## Section G — Ranges and Modern Loops

### G1. Prefer `std::ranges` over hand-written iterator loops (unless it genuinely reads worse)

**Rule.** For linear find/filter/transform operations, use `std::ranges::find_if`, `std::ranges::transform`, etc. They express intent more clearly and compose better.

```cpp
// OLDER STYLE (still readable, still fine):
auto it = std::find_if(bones_.begin(), bones_.end(),
                       [&](const Bone& b) { return b.name == name; });
return (it != bones_.end()) ? static_cast<u32>(std::distance(bones_.begin(), it)) : 0xFFFFFFFFu;

// RANGES STYLE:
auto it = std::ranges::find_if(bones_, [&](const Bone& b) { return b.name == name; });
return (it != bones_.end()) ? static_cast<u32>(std::ranges::distance(bones_.begin(), it))
                            : 0xFFFFFFFFu;
```

**But — prefer the form that a junior reader will understand faster.** If a simple `for` loop is clearer than a chain of `views::`, write the `for` loop. Readability beats style.

### G2. `using enum` (C++20) in a switch

**Rule.** When a switch handles every value of an enum, open the scope with `using enum`.

```cpp
// VERBOSE:
switch (type) {
    case DataType::Bool:   return 1;
    case DataType::Int32:  return 4;
    case DataType::Float:  return 4;
    case DataType::Vec3:   return 12;
}

// SHORTER, SAME SAFETY:
switch (type) {
    using enum DataType;
    case Bool:   return 1;
    case Int32:  return 4;
    case Float:  return 4;
    case Vec3:   return 12;
}
```

### G3. Range-based `for` — `const auto&` is almost always what you want

**Rule.** When iterating, default to `const auto&`. Deviate only if you must mutate (`auto&`) or the element is trivially copyable and copying is actually clearer (`auto`).

See also §A6 for the specific `shared_ptr` / `Ref` case.

---

## Section H — Interfaces and API Shape

### H1. Narrow public surface

**Rule.** A class should only expose what callers are supposed to use. Internal helpers go **private** or **protected**. Two-phase init methods that shouldn't be called externally go **protected**.

**Why.** Exposing a method is a commitment. Every caller who uses it becomes a client you can never break. Every reviewer has to spend cycles understanding which methods are "real" API and which are implementation leaks. The more you expose, the slower the codebase moves.

### H2. Pure stateless helpers — namespace + free functions, not static-member classes

**Rule.** If a "class" consists entirely of `static` members and holds no state, it should be a namespace of free functions instead.

```cpp
// UNNECESSARY CEREMONY:
class StringUtils {
public:
    static std::string trim(const std::string&);
    static std::string lower(const std::string&);
private:
    StringUtils() = delete;
};

// CLEANER:
namespace gw::strings {
    std::string trim(const std::string&);
    std::string lower(const std::string&);
}
```

**Exception:** if a "static class" is genuinely a thin wrapper over a singleton stateful backend (e.g., `Renderer::draw_mesh(...)` wraps the current `render::api` instance for ergonomics), that's fine — document it as such and keep the wrapper stateless at the wrapper level.

### H3. Non-owning parameters: express non-ownership in the type

**Rule.** When a function borrows but does not take ownership:

- If the parameter cannot be null and is not optional: `const T&` or `T&`.
- If the parameter may be null: `const T*` or `T*` — raw pointers in modern C++ **mean non-owning**. Do not use raw pointers for owning parameters.
- If the parameter may be null and ownership is unclear: use `std::optional<T&>` only when C++26 lands it; until then, pointer with a documented `// nullable` comment.

```cpp
// non-owning, required:
void compile(const Shader& shader);

// non-owning, optional:
void attach(Shader* shader);   // may be nullptr

// owning, transferred in:
void store(std::unique_ptr<Shader> shader);
```

### H4. Thread-safety requirements are part of the signature

**Rule.** Document thread safety explicitly for every non-trivial method. If a method is safe to call from any thread, say so. If it must be called from the main thread only, say so.

**Greywater convention:**
- Methods safe to call from any thread: no prefix, documented `// thread-safe`.
- Methods that execute on the render thread: `rt_` prefix (see §C3).
- Methods that must be called from the main thread: no prefix, documented `// main-thread only`.

---

## Section I — Threading

### I1. No `std::async`, no raw `std::thread`

See `00_SPECIFICATION_Claude_Opus_4.7.md` §2. All work goes through `engine/jobs/`.

### I2. Thread affinity masks are bit masks, not integers

**Rule.** If you call a platform API that takes a thread-affinity mask, verify the call site is expressing a mask (bits) and not an index.

**Example.** `SetThreadAffinityMask(thread, 8)` does not mean "run on 8 cores" or "run on core 8's affinity tier" — it means "the only CPU this thread is allowed on is CPU with bit index 3 (value 8 = `0b1000`)." A thread pinned to a single core is almost never what you want for engine threads; the scheduler normally does better.

**Rule of practice:** do not set thread affinity unless you have measured a specific problem that affinity solves. An unmotivated affinity mask is technical debt.

### I3. Render thread ownership — strong refs or nothing

See §C3. Never send raw `this` into the render thread's work queue.

---

## Section J — Review Checklist

Use this checklist at every Review. If any item fails, request changes. If the author has a written justification for the deviation, it can land — otherwise, it is blocked.

### Correctness
- [ ] No `new` / `delete` in engine code (§A1)
- [ ] No two-phase init on public APIs (§A2)
- [ ] Containers own their contents (§A3)
- [ ] Move-assignment releases existing resources (§A7)
- [ ] No C-style casts (§B1)
- [ ] `reinterpret_cast` has a `// SAFETY:`-style comment if it's in production code (§B3)
- [ ] `string_view` is never passed to a C API (§E1)
- [ ] Windows wide-string conversions use the platform API (§E3)
- [ ] Thread-affinity calls are intentional and documented (§I2)

### Performance
- [ ] `shared_ptr` / `gw::Ref` is not copied unnecessarily — check pass-by-value vs. `const &` (§A5)
- [ ] Range loops over ref containers use `const auto&` (§A6)
- [ ] No per-frame `std::string` allocation for data that was parsed at load (§E5)
- [ ] No `std::function` heap allocation on the hot path (§F2)

### Discipline
- [ ] Lambda captures are explicit (§D1)
- [ ] Cross-thread lambdas capture strong refs, not `this` (§C3)
- [ ] Public API surface is minimal — helpers are private/protected (§H1)
- [ ] Non-owning parameters are `const T&` or `T*`, not smart pointers (§H3)
- [ ] Thread-safety is documented (§H4)

### Style
- [ ] `clang-format`, `clang-tidy`, `rustfmt`, `clippy` all clean
- [ ] Names follow `00_SPECIFICATION` §3
- [ ] No commented-out code left behind
- [ ] Commit messages follow Conventional Commits

### Rust (BLD)
- [ ] No `unwrap`/`expect` in `bld-ffi` or `bld-governance` (§`04_LANGUAGE_RESEARCH_GUIDE` §3.3)
- [ ] `unsafe` blocks have `// SAFETY:` comments
- [ ] FFI types are `#[repr(C)]`, no generics across the boundary
- [ ] Miri runs clean on any change to `bld-ffi`

### Greywater-game code (anything under `engine/world/` or touching universe / planetary / atmospheric / survival / ecosystem code)
- [ ] World-space positions are `f64` (§K1)
- [ ] Cached world positions are updated on origin-shift event (§K2)
- [ ] Procedural generation is deterministic — seed + coord in, content out; no clocks, no `random_device` (§K3)
- [ ] Chunk generation respects the 10 ms per-step budget and time-slices across frames (§K4)
- [ ] Rollback-critical code is a pure function of game state; no logging or telemetry inside the window (§K5)
- [ ] Structural-integrity recomputation completes in the same frame it's triggered (§K6)
- [ ] No hardcoded planet/faction/resource/scenario names in `engine/world/` (§K7)

---

## Section K — Greywater-game-specific rules

The following rules apply to code under `engine/world/` and to any gameplay code that crosses into the Greywater game's universe / planetary / atmospheric / survival / ecosystem subsystems. They are additive to Sections A–J.

### K1. World-space positions are `f64`. Period.

**Rule.** Any variable representing a world-space position — across the planet, the star system, or the universe — is `f64` (i.e. `double` / `Vec3_f64`). Single-precision `float` is permitted only for local-to-chunk math and for GPU vertex attributes *after* subtracting the floating camera origin.

**Why.** Planetary scale breaks `float32`. A location 10 km from the origin has millimeter precision only in `f64`; in `f32` it has ~1 mm error at best and compounds under accumulation. Jitter is not "a quirk"; it is a correctness bug.

```cpp
// FORBIDDEN — single-precision world-space:
struct Transform {
    Vec3 position;        // f32 — breaks at planet scale
};

// REQUIRED:
struct Transform {
    Vec3_f64 position;    // f64 — correct to mm at 10 000 km
    Quat     rotation;
    Vec3     scale;       // scale is unitless; f32 is fine
};
```

### K2. Floating-origin events are mandatory for cached positions.

**Rule.** Any subsystem that caches world-space positions must subscribe to `world::origin_shift_event` and translate its cached positions when the event fires.

**Why.** When the floating origin recenters, the entire world translates by a large offset. Caches that miss this event diverge silently. Audio sources ping at the wrong location, particle emitters drift, navmesh nodes become invalid.

### K3. Procedural generation is deterministic. No exceptions.

**Rule.** Any code path that contributes to universe, planetary, or chunk content:
- Takes the seed and the coordinate as explicit arguments.
- Uses only a seeded RNG derived from (seed, coordinate).
- Does not read `std::chrono::steady_clock::now()`, `std::random_device{}()`, or any system-state input.
- Does not branch on hardware-reported values that can vary between machines (e.g., GPU SKU, driver timings).

**Why.** The "infinite without storage" principle requires that the same (seed, coordinate) always produces the same output. If two players walk to the same procedurally-generated location and see different continents, the simulation is broken. If a rollback resimulation produces different chunk content, the simulation is broken.

```cpp
// FORBIDDEN:
f32 height(Vec3_f64 dir) {
    std::mt19937 rng{std::random_device{}()};   // non-deterministic seed
    return noise(dir, rng());
}

// REQUIRED:
f32 height(Vec3_f64 dir, u64 seed) noexcept {
    std::mt19937_64 rng(seed);
    return noise(dir, rng());
}
```

### K4. Chunk generation must be time-sliced.

**Rule.** No chunk-generation step may block the main thread for more than the frame-time budget minus its safety margin (for 60 Hz: ~14 ms budget, ~4 ms safety margin, so ~10 ms hard cap per chunk step). Generation runs on the job system, split across multiple frames for large chunks.

**Why.** The "no stutter on chunk stream-in" requirement in `00_SPECIFICATION` §4 is non-negotiable. A single long chunk-generation call that spikes to 30 ms destroys the perception of a seamless world. Time-slicing with explicit yield points is the only acceptable pattern.

### K5. Rollback-critical code is a pure function of game state.

**Rule.** Any code that runs inside the rollback window (see `docs/games/greywater/17_MULTIPLAYER.md`) must be a pure function of the game state passed in. No global state reads. No side effects other than mutation of the game state. No calls into non-deterministic subsystems (logging, telemetry, file I/O) from within the rolled-back region.

**Why.** Rollback resimulates previous frames with corrected inputs. Any side effect that happened in the original simulation happens *again* during resimulation — duplicated logs, doubled telemetry, corrupted saves. Pure functions are the only safe substrate.

### K6. Structural-integrity updates are deterministic and complete.

**Rule.** When a building piece changes (added, removed, damaged), the integrity-graph recomputation runs to completion before the next frame. Partial recomputation that leaves the graph in an inconsistent state is forbidden.

**Why.** A piece with a stale support value can cause spurious collapses or invincible floating structures. Users will find every edge case we don't handle.

### K7. The engine never hardcodes Greywater-game content.

**Rule.** Code in `engine/world/` must not contain:
- Specific planet names (e.g., "Homeworld", "Nephros-7").
- Specific faction names (e.g., "Red Reavers", "Trader Collective").
- Specific resource names (e.g., "Neutronite", "Dreamshard").
- Specific scenario scripts (e.g., "tutorial mission", "main quest").

Names and scenarios live in `gameplay/` data files and scripts. The engine provides the mechanisms; the game provides the content.

**Why.** Without this separation, the engine becomes un-reusable and the game becomes un-redesignable. The engine is a set of capabilities; the content is the art of arranging them.

---

## Section L — Things this playbook does not cover

This document is not a style guide. It is not a formatting guide. It is not a C++ tutorial. It is also not exhaustive — it is the **bite list**: the mistakes we have written down so we never pay for them twice.

For style, see `.clang-format`, `.clang-tidy`, `rustfmt.toml`.
For idioms, see `04_LANGUAGE_RESEARCH_GUIDE.md`.
For the philosophy behind the rules, see `01_ENGINE_PHILOSOPHY.md`.
For the specification of the project, see `00_SPECIFICATION_Claude_Opus_4.7.md`.
For the Greywater-game subsystem details, see `docs/games/greywater/*`.

---

*Reviewed deliberately. Every rule here is a scar. We honor the scar by not getting cut twice.*
