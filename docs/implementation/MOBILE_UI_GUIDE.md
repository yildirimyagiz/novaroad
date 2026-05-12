# Nova Mobile UI Framework Guide

## Overview

Nova's mobile UI framework provides a **cross-platform, declarative UI system** for iOS and Android, with a syntax inspired by SwiftUI and Jetpack Compose.

**Features:**
- ✅ **Declarative UI** - SwiftUI-like syntax
- ✅ **Cross-platform** - Single codebase for iOS + Android
- ✅ **Native performance** - Direct bindings to UIKit/Compose
- ✅ **Reactive state** - Automatic UI updates
- ✅ **Platform APIs** - Camera, Location, Notifications
- ✅ **Type-safe** - Full compile-time checking

---

## Quick Start

### Hello World

```nova
use nova::ui::mobile::*;

fn main() {
    let app = Column::new(vec![
        Box::new(Text::new("Hello, World!".to_string()).font_size(24.0)),
        Box::new(Button::new("Tap Me".to_string())),
    ]);
    
    run_app(app);
}
```

### Counter App

```nova
shape CounterApp {
    count: ViewState<i32>,
}

apply View for CounterApp {
    fn build(&self, ctx: BuildContext) -> Element {
        Column::new(vec![
            Box::new(Text::new(format!("Count: {}", self.count.get()))),
            Box::new(
                Button::new("+".to_string())
                    .on_tap(Box::new(|| {
                        self.count.set(self.count.get() + 1);
                    }))
            ),
        ]).build(ctx)
    }
}
```

---

## Core Widgets

### Text

```nova
Text::new("Hello".to_string())
    .font_size(24.0)
    .color(Color::blue())
    .bold()
```

### Button

```nova
Button::new("Click Me".to_string())
    .background(Color::blue())
    .on_tap(Box::new(|| {
        println!("Tapped!");
    }))
```

### TextField

```nova
TextField::new("Enter text...".to_string())
    .on_change(Box::new(|text| {
        println!("Input: {}", text);
    }))
```

### Image

```nova
// Network image
Image::network("https://example.com/image.png".to_string())

// Asset image
Image::asset("logo.png".to_string())
```

---

## Layout

### Column (Vertical)

```nova
Column::new(vec![
    Box::new(Text::new("Top")),
    Box::new(Text::new("Middle")),
    Box::new(Text::new("Bottom")),
])
.spacing(10.0)
```

### Row (Horizontal)

```nova
Row::new(vec![
    Box::new(Button::new("Left")),
    Box::new(Spacer),
    Box::new(Button::new("Right")),
])
.spacing(20.0)
```

### Stack (Layered)

```nova
Stack::new(vec![
    Box::new(Image::asset("background.png")),
    Box::new(Text::new("Overlay Text")),
])
.align(Alignment::Center)
```

### Padding

```nova
Padding::all(
    Box::new(Text::new("Padded")),
    20.0
)

// Or
text.padding(EdgeInsets::symmetric(10.0, 20.0))
```

---

## State Management

### ViewState

```nova
// Create state
let count = ViewState::new(0);

// Read value
let value = count.get();

// Update value
count.set(42);

// Subscribe to changes
count.subscribe(Box::new(|new_value| {
    println!("Count changed to: {}", new_value);
}));
```

### Reactive UI

```nova
shape MyApp {
    name: ViewState<String>,
}

apply View for MyApp {
    fn build(&self, ctx: BuildContext) -> Element {
        Column::new(vec![
            Box::new(Text::new(format!("Hello, {}", self.name.get()))),
            Box::new(
                TextField::new("Enter name".to_string())
                    .on_change(Box::new(|text| {
                        self.name.set(text); // UI auto-updates!
                    }))
            ),
        ]).build(ctx)
    }
}
```

---

## Platform APIs

### Camera

```nova
use nova::ui::mobile::platform::camera::*;

// Request permission
Camera::request_permission()?;

// Capture photo
let camera = Camera::new().front_camera();
let photo = camera.capture_photo()?;

println!("Captured {}x{}", photo.width, photo.height);
```

### Location

```nova
use nova::ui::mobile::platform::location::*;

// Request permission
LocationService::request_permission()?;

// Get current location
let service = LocationService::new();
let location = service.current_location()?;

println!("Lat: {}, Lon: {}", location.latitude, location.longitude);
```

### Notifications

```nova
use nova::ui::mobile::platform::notifications::*;

let service = NotificationService::new();

// Show immediate notification
service.show(Notification::new(
    "Hello".to_string(),
    "This is a notification".to_string()
))?;

// Schedule notification
service.schedule(
    Notification::new("Reminder".to_string(), "Time to check!".to_string()),
    3600 // 1 hour delay
)?;
```

---

## Platform-Specific Code

### iOS

```nova
#[cfg(target_os = "ios")]
{
    use nova::ui::mobile::platform::ios::*;
    
    let vc = iOSViewController::new(Box::new(app))
        .title("My App".to_string());
}
```

### Android

```nova
#[cfg(target_os = "android")]
{
    use nova::ui::mobile::platform::android::*;
    
    let activity = AndroidActivity::new(Box::new(app))
        .title("My App".to_string());
}
```

---

## Complete Examples

### Todo App

See `examples/mobile/todo_app.zn`

Key features:
- State management with `ViewState`
- Dynamic list rendering
- Input handling
- Toggle functionality

### Camera App

See `examples/mobile/camera_app.zn`

Key features:
- Camera permission request
- Photo capture
- Image preview
- Front/back camera switching

### Counter App

See `examples/mobile/counter_app.zn`

Key features:
- Simple state management
- Button interactions
- Number formatting

---

## Building for Mobile

### iOS

```bash
# Build for iOS
znc --target aarch64-apple-ios counter_app.zn

# Create Xcode project
zn mobile init-ios MyApp

# Run on simulator
zn mobile run-ios
```

### Android

```bash
# Build for Android
znc --target aarch64-linux-android counter_app.zn

# Create Android project
zn mobile init-android MyApp

# Run on emulator
zn mobile run-android
```

---

## Widget Reference

| Widget | Description | Platforms |
|--------|-------------|-----------|
| `Text` | Display text | iOS, Android |
| `Button` | Tappable button | iOS, Android |
| `TextField` | Text input | iOS, Android |
| `Image` | Display images | iOS, Android |
| `Column` | Vertical layout | All |
| `Row` | Horizontal layout | All |
| `Stack` | Layered layout | All |
| `Padding` | Add padding | All |
| `Spacer` | Flexible space | All |
| `List` | Scrollable list | iOS, Android |

---

## Styling

### Colors

```nova
Color::rgb(1.0, 0.0, 0.0)      // Red
Color::rgba(0.0, 0.0, 1.0, 0.5) // Semi-transparent blue
Color::black()
Color::white()
Color::red()
Color::blue()
```

### Text Styles

```nova
Text::new("Styled Text")
    .font_size(24.0)
    .color(Color::blue())
    .bold()
```

### Button Styles

```nova
Button::new("Styled Button")
    .background(Color::blue())
    .foreground(Color::white())
```

---

## Best Practices

### 1. Use State Wisely

```nova
// ✅ Good - state for dynamic data
let count = ViewState::new(0);

// ❌ Bad - state for constants
let title = ViewState::new("App Title"); // Just use String
```

### 2. Modular Components

```nova
// ✅ Good - reusable components
shape UserCard {
    name: String,
    avatar: String,
}

apply View for UserCard { /* ... */ }

// Use it
Box::new(UserCard { name: "Alice", avatar: "avatar.png" })
```

### 3. Platform Detection

```nova
// ✅ Good - detect at compile time
#[cfg(target_os = "ios")]
{ /* iOS-specific code */ }

// ❌ Bad - runtime detection (slower)
if detect_platform() == Platform::iOS { /* ... */ }
```

---

## Performance Tips

1. **Minimize state updates** - Only update when necessary
2. **Use builders efficiently** - Chain modifiers
3. **Lazy loading** - Load images/data on demand
4. **List optimization** - Use keys for list items

---

## Troubleshooting

### Permission Issues

```nova
// Always request permissions first
Camera::request_permission()?;
LocationService::request_permission()?;
```

### State Not Updating

```nova
// ❌ Wrong
self.count.get() + 1; // Doesn't update

// ✅ Correct
self.count.set(self.count.get() + 1);
```

### Build Errors

```bash
# Clean build
zn clean
zn build --release

# Check target
zn target list
```

---

## API Coverage

### iOS (UIKit)
- ✅ UIButton
- ✅ UILabel
- ✅ UITextField
- ✅ UIImageView
- ✅ UITableView
- ✅ UINavigationController
- ✅ Camera (AVFoundation)
- ✅ Location (CoreLocation)
- ✅ Notifications (UserNotifications)

### Android (Jetpack Compose)
- ✅ Button
- ✅ TextView
- ✅ EditText
- ✅ ImageView
- ✅ RecyclerView
- ✅ Camera (CameraX)
- ✅ Location (FusedLocation)
- ✅ Notifications (NotificationManager)

---

## Roadmap

### Completed ✅
- Core UI framework
- iOS/Android bindings
- Declarative syntax
- Platform APIs (camera, location, notifications)
- State management
- Example apps

### Coming Soon 🚧
- Hot reload
- More widgets (Picker, Slider, Switch)
- Animations
- Gestures (pan, pinch, rotate)
- Navigation framework
- Persistent storage

---

## Resources

- **Examples:** `nova/examples/mobile/`
- **Source:** `nova/zn/stdlib/ui/mobile/`
- **Issues:** Report bugs on GitHub
- **Discord:** Join Nova community

---

**Last Updated:** March 2, 2026
