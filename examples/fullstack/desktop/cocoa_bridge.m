// ╔══════════════════════════════════════════════════════════════════════════════╗
// ║  Nova Desktop — Cocoa FFI Bridge  v2.0                                    ║
// ║  Complete macOS native window with event loop, views, and controls        ║
// ╚══════════════════════════════════════════════════════════════════════════════╝

#import <Cocoa/Cocoa.h>
#include <stdio.h>
#include <string.h>

// ── Application Delegate ────────────────────────────────────────────────────

@interface NovaAppDelegate : NSObject <NSApplicationDelegate>
@property(nonatomic, strong) NSWindow *mainWindow;
@end

@implementation NovaAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
  printf("🚀 Nova App: applicationDidFinishLaunching\n");
  [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:
    (NSApplication *)sender {
  return YES;
}

@end

// ── Window Delegate ─────────────────────────────────────────────────────────

@interface NovaWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation NovaWindowDelegate

- (void)windowWillClose:(NSNotification *)notification {
  printf("🔒 Nova App: Window closing\n");
  [NSApp terminate:nil];
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize {
  printf("📐 Nova App: Resize to %.0fx%.0f\n", frameSize.width,
         frameSize.height);
  return frameSize;
}

@end

// ── Button Action Target ────────────────────────────────────────────────────

typedef void (*nova_button_callback_t)(int button_id);

@interface NovaButtonTarget : NSObject
@property(nonatomic) int buttonId;
@property(nonatomic) nova_button_callback_t callback;
@end

@implementation NovaButtonTarget

- (void)buttonClicked:(id)sender {
  printf("🔘 Nova App: Button %d clicked\n", self.buttonId);
  if (self.callback) {
    self.callback(self.buttonId);
  }
}

@end

// ═══════════════════════════════════════════════════════════════════════════════
// FFI EXPORTS — Functions callable from Nova via foreign "C"
// ═══════════════════════════════════════════════════════════════════════════════

static NovaAppDelegate *g_app_delegate = nil;
static NovaWindowDelegate *g_window_delegate = nil;
static NSMutableArray *g_button_targets = nil;

// ── App Lifecycle ──────────────────────────────────────────────────────────

void nova_app_init(void) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    g_app_delegate = [[NovaAppDelegate alloc] init];
    [NSApp setDelegate:g_app_delegate];

    g_window_delegate = [[NovaWindowDelegate alloc] init];
    g_button_targets = [[NSMutableArray alloc] init];

    // Create menu bar
    NSMenu *menuBar = [[NSMenu alloc] init];
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    [menuBar addItem:appMenuItem];

    NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@"Nova App"];
    [appMenu addItemWithTitle:@"About Nova"
                       action:@selector(orderFrontStandardAboutPanel:)
                keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit"
                       action:@selector(terminate:)
                keyEquivalent:@"q"];
    [appMenuItem setSubmenu:appMenu];

    [NSApp setMainMenu:menuBar];

    printf("✅ Nova App initialized (Cocoa)\n");
  }
}

void nova_app_run(void) {
  @autoreleasepool {
    printf("🏃 Nova App: Starting event loop...\n");
    [NSApp run];
  }
}

void nova_app_quit(void) {
  @autoreleasepool {
    [NSApp terminate:nil];
  }
}

// ── Window Management ──────────────────────────────────────────────────────

void *nova_window_create(double width, double height, const char *title) {
  __block NSWindow *window = nil;

  void (^createBlock)(void) = ^{
    @autoreleasepool {
      NSRect frame = NSMakeRect(0, 0, width, height);

      window = [[NSWindow alloc]
          initWithContentRect:frame
                    styleMask:(NSWindowStyleMaskTitled |
                               NSWindowStyleMaskClosable |
                               NSWindowStyleMaskMiniaturizable |
                               NSWindowStyleMaskResizable)
                      backing:NSBackingStoreBuffered
                        defer:NO];

      [window setTitle:[NSString stringWithUTF8String:title]];
      [window center];
      [window setDelegate:g_window_delegate];

      // Dark appearance
      if (@available(macOS 10.14, *)) {
        [window setAppearance:[NSAppearance
                                  appearanceNamed:NSAppearanceNameDarkAqua]];
      }

      // Set minimum size
      [window setMinSize:NSMakeSize(400, 300)];

      // Background color
      [window setBackgroundColor:[NSColor colorWithRed:0.08
                                                 green:0.08
                                                  blue:0.12
                                                 alpha:1.0]];

      g_app_delegate.mainWindow = window;

      printf("✅ Window created: \"%s\" (%.0fx%.0f)\n", title, width, height);
    }
  };

  if ([NSThread isMainThread]) {
    createBlock();
  } else {
    dispatch_sync(dispatch_get_main_queue(), createBlock);
  }

  return (__bridge_retained void *)window;
}

void nova_window_show(void *window_ptr) {
  @autoreleasepool {
    NSWindow *window = (__bridge NSWindow *)window_ptr;
    [window makeKeyAndOrderFront:nil];
    printf("✅ Window shown\n");
  }
}

void nova_window_set_title(void *window_ptr, const char *title) {
  @autoreleasepool {
    NSWindow *window = (__bridge NSWindow *)window_ptr;
    [window setTitle:[NSString stringWithUTF8String:title]];
  }
}

void nova_window_set_size(void *window_ptr, double width, double height) {
  @autoreleasepool {
    NSWindow *window = (__bridge NSWindow *)window_ptr;
    NSRect frame = [window frame];
    frame.size = NSMakeSize(width, height);
    [window setFrame:frame display:YES animate:YES];
  }
}

// ── View Components ────────────────────────────────────────────────────────

void *nova_label_create(void *window_ptr, const char *text, double x, double y,
                        double w, double h) {
  @autoreleasepool {
    NSWindow *window = (__bridge NSWindow *)window_ptr;
    NSTextField *label =
        [[NSTextField alloc] initWithFrame:NSMakeRect(x, y, w, h)];

    [label setStringValue:[NSString stringWithUTF8String:text]];
    [label setBezeled:NO];
    [label setDrawsBackground:NO];
    [label setEditable:NO];
    [label setSelectable:NO];
    [label setTextColor:[NSColor colorWithRed:0.9
                                        green:0.9
                                         blue:0.95
                                        alpha:1.0]];
    [label setFont:[NSFont systemFontOfSize:16 weight:NSFontWeightMedium]];

    [[window contentView] addSubview:label];

    return (__bridge_retained void *)label;
  }
}

void *nova_title_label_create(void *window_ptr, const char *text, double x,
                              double y, double w, double h) {
  @autoreleasepool {
    NSWindow *window = (__bridge NSWindow *)window_ptr;
    NSTextField *label =
        [[NSTextField alloc] initWithFrame:NSMakeRect(x, y, w, h)];

    [label setStringValue:[NSString stringWithUTF8String:text]];
    [label setBezeled:NO];
    [label setDrawsBackground:NO];
    [label setEditable:NO];
    [label setSelectable:NO];
    [label setAlignment:NSTextAlignmentCenter];
    [label setTextColor:[NSColor colorWithRed:0.6
                                        green:0.7
                                         blue:1.0
                                        alpha:1.0]];
    [label setFont:[NSFont systemFontOfSize:28 weight:NSFontWeightBold]];

    [[window contentView] addSubview:label];

    return (__bridge_retained void *)label;
  }
}

void nova_label_set_text(void *label_ptr, const char *text) {
  @autoreleasepool {
    NSTextField *label = (__bridge NSTextField *)label_ptr;
    [label setStringValue:[NSString stringWithUTF8String:text]];
  }
}

void *nova_button_create(void *window_ptr, const char *title, double x,
                         double y, double w, double h, int button_id,
                         nova_button_callback_t callback) {
  @autoreleasepool {
    NSWindow *window = (__bridge NSWindow *)window_ptr;
    NSButton *button = [[NSButton alloc] initWithFrame:NSMakeRect(x, y, w, h)];

    [button setTitle:[NSString stringWithUTF8String:title]];
    [button setBezelStyle:NSBezelStyleRounded];

    NovaButtonTarget *target = [[NovaButtonTarget alloc] init];
    target.buttonId = button_id;
    target.callback = callback;
    [g_button_targets addObject:target];

    [button setTarget:target];
    [button setAction:@selector(buttonClicked:)];

    [[window contentView] addSubview:button];

    return (__bridge_retained void *)button;
  }
}

void *nova_text_input_create(void *window_ptr, const char *placeholder,
                             double x, double y, double w, double h) {
  @autoreleasepool {
    NSWindow *window = (__bridge NSWindow *)window_ptr;
    NSTextField *input =
        [[NSTextField alloc] initWithFrame:NSMakeRect(x, y, w, h)];

    [input setPlaceholderString:[NSString stringWithUTF8String:placeholder]];
    [input setBezeled:YES];
    [input setBezelStyle:NSTextFieldRoundedBezel];

    [[window contentView] addSubview:input];

    return (__bridge_retained void *)input;
  }
}

const char *nova_text_input_get_value(void *input_ptr) {
  @autoreleasepool {
    NSTextField *input = (__bridge NSTextField *)input_ptr;
    return [[input stringValue] UTF8String];
  }
}

// ── Drawing / Custom View ──────────────────────────────────────────────────

@interface NovaCanvasView : NSView
@property(nonatomic) NSColor *fillColor;
@property(nonatomic) CGFloat cornerRadius;
@end

@implementation NovaCanvasView

- (void)drawRect:(NSRect)dirtyRect {
  [super drawRect:dirtyRect];

  NSBezierPath *path =
      [NSBezierPath bezierPathWithRoundedRect:self.bounds
                                      xRadius:self.cornerRadius
                                      yRadius:self.cornerRadius];
  [self.fillColor setFill];
  [path fill];
}

@end

void *nova_panel_create(void *window_ptr, double x, double y, double w,
                        double h, double r, double g, double b, double a,
                        double corner_radius) {
  @autoreleasepool {
    NSWindow *window = (__bridge NSWindow *)window_ptr;
    NovaCanvasView *panel =
        [[NovaCanvasView alloc] initWithFrame:NSMakeRect(x, y, w, h)];

    panel.fillColor = [NSColor colorWithRed:r green:g blue:b alpha:a];
    panel.cornerRadius = corner_radius;

    [[window contentView] addSubview:panel];

    return (__bridge_retained void *)panel;
  }
}

// ── Progress Indicator ─────────────────────────────────────────────────────

void *nova_progress_create(void *window_ptr, double x, double y, double w,
                           double h) {
  @autoreleasepool {
    NSWindow *window = (__bridge NSWindow *)window_ptr;
    NSProgressIndicator *progress =
        [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(x, y, w, h)];

    [progress setStyle:NSProgressIndicatorStyleBar];
    [progress setMinValue:0.0];
    [progress setMaxValue:100.0];
    [progress setDoubleValue:0.0];
    [progress setIndeterminate:NO];

    [[window contentView] addSubview:progress];

    return (__bridge_retained void *)progress;
  }
}

void nova_progress_set_value(void *progress_ptr, double value) {
  @autoreleasepool {
    NSProgressIndicator *progress =
        (__bridge NSProgressIndicator *)progress_ptr;
    [progress setDoubleValue:value];
  }
}

// ── Timer / Animation ──────────────────────────────────────────────────────

typedef void (*nova_timer_callback_t)(void *user_data);

@interface NovaTimerTarget : NSObject
@property(nonatomic) nova_timer_callback_t callback;
@property(nonatomic) void *userData;
@end

@implementation NovaTimerTarget
- (void)timerFired:(NSTimer *)timer {
  if (self.callback) {
    self.callback(self.userData);
  }
}
@end

void *nova_timer_create(double interval_seconds, nova_timer_callback_t callback,
                        void *user_data) {
  @autoreleasepool {
    NovaTimerTarget *target = [[NovaTimerTarget alloc] init];
    target.callback = callback;
    target.userData = user_data;

    NSTimer *timer =
        [NSTimer scheduledTimerWithTimeInterval:interval_seconds
                                         target:target
                                       selector:@selector(timerFired:)
                                       userInfo:nil
                                        repeats:YES];

    return (__bridge_retained void *)timer;
  }
}

void nova_timer_stop(void *timer_ptr) {
  @autoreleasepool {
    NSTimer *timer = (__bridge NSTimer *)timer_ptr;
    [timer invalidate];
  }
}

// ── Dialog ─────────────────────────────────────────────────────────────────

int nova_alert(const char *title, const char *message) {
  @autoreleasepool {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:[NSString stringWithUTF8String:title]];
    [alert setInformativeText:[NSString stringWithUTF8String:message]];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];

    NSModalResponse response = [alert runModal];
    return (response == NSAlertFirstButtonReturn) ? 1 : 0;
  }
}
