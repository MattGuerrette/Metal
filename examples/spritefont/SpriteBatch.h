
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import "graphics_math.h"

@interface SpriteBatch : NSObject

- (instancetype)initWithDevice:(id<MTLDevice>)device;

- (void)begin:(id<MTLRenderCommandEncoder>)encoder;

- (void)draw:(id<MTLTexture>)texture position:(DirectX::XMFLOAT2)position sourceRectangle:(Rect)rect;

- (void)end;

@end
