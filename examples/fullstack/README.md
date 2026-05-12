# Nova AI Studio — Fullstack Demo

Full-stack application demonstrating Nova's **FFI**, **SFC**, and **self-compilation** capabilities.

## 🏗️ Architecture

```
fullstack/
├── desktop/                    # macOS Desktop App (Cocoa FFI)
│   ├── cocoa_bridge.m          # Objective-C FFI bridge (NSWindow, views, events)
│   ├── cocoa_bridge.h          # C header for FFI declarations
│   └── desktop_app.zn          # Nova source — calls Cocoa via foreign "C"
│
├── web/                        # Web App (static fallback)
│   └── index.html              # Preview of what SFC renders
│
├── build.sh                    # Build script (desktop + web + test)
├── build/                      # Build artifacts
│   └── nova_desktop            # Compiled macOS binary
│
└── README.md                   # This file
```

### Nova SFC (Web)
The real web application lives in the compiler frontend:
```
zn/src/compiler/frontend/web/
├── ai_studio_sfc.zn            # ← AI Studio SFC (template + style + script)
├── sfc_demo.zn                 # Original Nova website SFC
├── web_framework.zn            # NovaServer, Router, Signal<T>, View trait
├── web_codegen.zn              # WASM code generation
├── ssr_support.zn              # Server-side rendering
└── dom_bindings.zn             # Browser DOM bindings
```

## 🚀 Quick Start

### Desktop App (macOS)
```bash
# Build & run
bash examples/fullstack/build.sh desktop
./examples/fullstack/build/nova_desktop
```

### Web App (SFC)
```bash
# Using Nova CLI
nova web dev    # Starts SFC dev server at http://localhost:8080
```

### Self-Compile Test
```bash
# Test that Nova can compile its own .zn files
bash examples/fullstack/build.sh test
```

## 🔗 FFI Bridge

The desktop app uses `foreign "C"` blocks to call Cocoa APIs:

```zn
foreign "C" {
    fn nova_app_init();
    fn nova_window_create(w: f64, h: f64, title: *const i8) -> *mut u8;
    fn nova_window_show(window: *mut u8);
    fn nova_app_run();
}

expose fn main() {
    nova_app_init();
    let window = nova_window_create(900.0, 650.0, "Nova AI Studio");
    nova_window_show(window);
    nova_app_run();
}
```

The bridge provides:
- **App lifecycle**: `nova_app_init`, `nova_app_run`, `nova_app_quit`
- **Window**: `nova_window_create`, `nova_window_show`, `nova_window_set_title`
- **UI**: labels, buttons, text inputs, panels, progress bars
- **Events**: button callbacks, timer callbacks
- **Dialogs**: `nova_alert`

## 🌐 SFC (Single File Component)

Web apps use Vue.js-like SFC pattern:

```zn
use web::framework::{NovaServer, View, Signal};

data AIStudioPage {
    open stats: CompilerStats,
}

impl AIStudioPage for View {
    fn render(&self) -> String {
        format!("<h1>{}</h1>", self.stats.lines_parsed.get())
    }
    fn style(&self) -> String {
        yield stylesheet! { h1 { color: #4488ff; } }
    }
}

fn main() {
    let server = NovaServer::new(8080)
        .register("studio", Box::new(AIStudioPage::new()))
        .get("/", "studio");
    server.start();
}
```

## 📊 Build Results

| Component      | Status | Notes                              |
|---------------|--------|-------------------------------------|
| Desktop App   | ✅     | Cocoa FFI, 900x650 dark window      |
| Web SFC       | ✅     | Compiled with znc                    |
| FFI Bridge    | ✅     | 35+ Cocoa function calls             |
| Self-Compile  | ✅     | All .zn files parsed successfully    |
| WASM Target   | ⏳     | Defined, needs LLVM WASM backend     |
