#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
int main() {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device) {
        printf("Metal device: %s
", [[device name] UTF8String]);
        return 0;
    } else {
        printf("No Metal device
");
        return 1;
    }
}
