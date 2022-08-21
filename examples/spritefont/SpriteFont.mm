
#import "SpriteFont.h"
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <vector>


struct Info
{
    NSString* face;
    int size;
    BOOL bold;
    BOOL italic;
    BOOL antiAliased;
    BOOL smooth;
    int stretchH;
    int antiAliasing;
    int padX;
    int padY;
    int padW;
    int padH;
    int spacingX;
    int spacingY;
};

struct Character
{
    
    NSString* letter;
};

@interface SpriteFont ()
{
    id<MTLDevice> _device;
    MTKTextureLoader* _textureLoader;
    Info _info;
    std::vector<Character> _characters;
    NSMutableArray* _textures;
}


@end


@implementation SpriteFont

- (instancetype)initWithFile:(NSString *)file
                      device:(id<MTLDevice>)device {
    self = [super init];
    _device = device;
    _textures = [[NSMutableArray alloc] init];
    
    _textureLoader = [[MTKTextureLoader alloc] initWithDevice:_device];
    
    NSData* fontData = [[NSData alloc] initWithContentsOfFile:file];
    NSXMLParser* parser = [[NSXMLParser alloc] initWithData:fontData];
    [parser setDelegate:self];
    [parser parse];
    
    _textureLoader = nil;
    
    
    return self;
}

- (id<MTLTexture>)texture {
    return [_textures objectAtIndex:0];
}

- (void) parserDidStartDocument:(NSXMLParser *)parser {
    //NSLog(@"parserDidStartDocument");
}

- (void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName attributes:(NSDictionary *)attributeDict {
    //NSLog(@"didStartElement --> %@", elementName);
    
    if([elementName isEqual:@"info"])
    {
        _info.face = [attributeDict valueForKey:@"face"];
        _info.size = [[attributeDict valueForKey:@"size"] integerValue];
        _info.bold = [attributeDict valueForKey:@"bold"];
        _info.italic = [attributeDict valueForKey:@"italic"];
        _info.stretchH = [[attributeDict valueForKey:@"stetchH"] integerValue];
        _info.antiAliased = [attributeDict valueForKey:@"aa"];
        
        NSArray* padding = [[attributeDict valueForKey:@"padding"] componentsSeparatedByString:@","];
        _info.padX = [[padding objectAtIndex:0] integerValue];
        _info.padY = [[padding objectAtIndex:1] integerValue];
        _info.padW = [[padding objectAtIndex:2] integerValue];
        _info.padH = [[padding objectAtIndex:3] integerValue];
        
        NSArray* spacing = [[attributeDict valueForKey:@"spacing"] componentsSeparatedByString:@","];
        _info.spacingX = [[padding objectAtIndex:0] integerValue];
        _info.spacingY = [[padding objectAtIndex:1] integerValue];
        
    }
        
    if ([elementName isEqual: @"char"])
    {
        Character c;
        c.letter = [attributeDict valueForKey:@"letter"];
        _characters.push_back(c);
    }
    
    if ([elementName isEqual:@"page"])
    {
        NSString* textureName = [attributeDict valueForKey:@"file"];
        NSString* texturePath = [[[NSBundle mainBundle] resourcePath]
            stringByAppendingPathComponent:textureName];
        NSURL* url = [[NSURL alloc] initFileURLWithPath:texturePath];
        
        NSDictionary *textureLoaderOptions = @{
              MTKTextureLoaderOptionSRGB: @NO,
              MTKTextureLoaderOptionTextureUsage       : @(MTLTextureUsageShaderRead),
              MTKTextureLoaderOptionTextureStorageMode : @(MTLStorageModePrivate)
            };
        id<MTLTexture> texture = [_textureLoader newTextureWithContentsOfURL:url options:textureLoaderOptions error:nil];
        
        [_textures addObject:texture];
    }
}

-(void) parser:(NSXMLParser *)parser foundCharacters:(NSString *)string {
    //NSLog(@"foundCharacters --> %@", string);
}

- (void)parser:(NSXMLParser *)parser didEndElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName {
    //NSLog(@"didEndElement   --> %@", elementName);
}

- (void) parserDidEndDocument:(NSXMLParser *)parser {
    //NSLog(@"parserDidEndDocument");
}

@end
