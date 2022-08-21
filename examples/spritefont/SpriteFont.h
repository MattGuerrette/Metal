
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

@interface SpriteFont : NSObject<NSXMLParserDelegate>

-(instancetype)initWithFile:(NSString*)file
                     device:(id<MTLDevice>)device;

@property (readonly, strong) id<MTLTexture> texture;

@end
