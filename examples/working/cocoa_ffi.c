// Cocoa FFI Bridge for Nova
// Provides C functions to create macOS windows

#import <Cocoa/Cocoa.h>
#include <stdio.h>

// Create a new NSWindow
void* NSWindow_new(double width, double height, const char* title) {
    @autoreleasepool {
        NSRect frame = NSMakeRect(100, 100, width, height);
        
        NSWindow *window = [[NSWindow alloc] 
            initWithContentRect:frame
            styleMask:(NSWindowStyleMaskTitled | 
                      NSWindowStyleMaskClosable | 
                      NSWindowStyleMaskMiniaturizable | 
                      NSWindowStyleMaskResizable)
            backing:NSBackingStoreBuffered
            defer:NO];
        
        [window setTitle:[NSString stringWithUTF8String:title]];
        [window center];
        
        printf("✅ NSWindow created: %p\n", window);
        return (__bridge_retained void*)window;
    }
}

// Show the window
void NSWindow_show(void* window_ptr) {
    @autoreleasepool {
        NSWindow *window = (__bridge NSWindow*)window_ptr;
        [window makeKeyAndOrderFront:nil];
        printf("✅ Window shown\n");
    }
}

// Set window title
void NSWindow_setTitle(void* window_ptr, const char* title) {
    @autoreleasepool {
        NSWindow *window = (__bridge NSWindow*)window_ptr;
        [window setTitle:[NSString stringWithUTF8String:title]];
    }
}

// Start Cocoa application
int NSApplicationMain_wrapper(int argc, const char** argv) {
    return NSApplicationMain(argc, argv);
}
