// Cocoa FFI Bridge - Objective-C implementation
#import <Cocoa/Cocoa.h>
#include <stdio.h>

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
        
        printf("✅ NSWindow created: %p\n", (void*)window);
        return (__bridge_retained void*)window;
    }
}

void NSWindow_show(void* window_ptr) {
    @autoreleasepool {
        NSWindow *window = (__bridge NSWindow*)window_ptr;
        [window makeKeyAndOrderFront:nil];
        printf("✅ Window shown\n");
    }
}
