/*
*    Metal Examples
*    Copyright(c) 2019 Matt Guerrette
*
*    This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "example.h"
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

/*----------------------------------------------*/
// Metal View
/*----------------------------------------------*/

@interface MetalView : NSView
@property (nonatomic, assign) CAMetalLayer* metalLayer;
@end

@implementation MetalView

+ (id)layerClass
{
    return [CAMetalLayer class];
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (CALayer*)makeBackingLayer
{
    CAMetalLayer* layer = [CAMetalLayer layer];
    self.metalLayer = layer;
    return self.metalLayer;
}

- (instancetype)initWithFrame:(NSRect)frameRect
{
    if ((self = [super initWithFrame:frameRect])) {
        self.wantsLayer = YES;
        self.metalLayer.device = MTLCreateSystemDefaultDevice();
        self.metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        self.metalLayer.drawableSize = self.bounds.size;
    }
    
    return self;
}

@end

/*-------------------------------------------------*/

/*-------------------------------------------------*/
// Window
/*-------------------------------------------------*/

@interface Window : NSWindow
- (BOOL)canBecomeMainWindow;
- (BOOL)canBecomeKeyWindow;
- (BOOL)acceptsFirstResponder;
- (void)keyDown:(NSEvent*)anEvent;
@end

@implementation Window
- (BOOL)canBecomeMainWindow
{
    return YES;
}
- (BOOL)canBecomeKeyWindow
{
    return YES;
}
- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)ignoresMouseEvents
{
    return NO;
}

- (void)keyDown:(NSEvent*)anEvent
{
    // Disable system 'beep' sound on key press
}
@end

/*-------------------------------------------------*/

/*-------------------------------------------------*/
// Application Delegate
/*-------------------------------------------------*/

@interface AppDelegate : NSObject<NSApplicationDelegate>
@property (strong, nonatomic) NSWindow* window;
@property (strong, nonatomic) MetalView* view;
@property (nonatomic) NSString* title;
@property (nonatomic) BOOL isRunning;
@property (nonatomic) NSRect windowRect;
- (void)_windowWillClose:(NSNotification*)notification;
- (instancetype)initWithTitleAndRect:(NSString*)title :(NSRect)rect;
@end

@implementation AppDelegate

- (instancetype)initWithTitleAndRect:(NSString*)title :(NSRect)rect
{
    self = [super init];
    if (self != nil) {
        [self setTitle:title];
        [self setWindowRect:rect];
    }
    
    return self;
}

- (void)_windowWillClose:(NSNotification *)notification
{
    [self setIsRunning:NO];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    NSRect frame = [self windowRect];
    
    self.view = [[MetalView alloc] initWithFrame:frame];
    
    const int style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
    self.window = [[Window alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:YES];
    [self.window setTitle:_title];
    [self.window setOpaque:YES];
    [self.window setContentView:_view];
    [self.window setAcceptsMouseMovedEvents:YES];
    [self.window makeMainWindow];
    [self.window makeFirstResponder:nil];
    [self.window makeKeyAndOrderFront:nil];
    [self.window center];
    
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self selector:@selector(_windowWillClose:) name:NSWindowWillCloseNotification object:nil];
}

@end

/*-------------------------------------------------*/

/*-------------------------------------------------*/
// Application
/*-------------------------------------------------*/

@interface App : NSApplication

@end

@implementation App

- (void)finishLaunching
{
    [super finishLaunching];

    // Flag application delegate as running to
    // start the application loop
    AppDelegate* delegate = [self delegate];
    [delegate setIsRunning:YES];
}

- (void)sendEvent:(NSEvent*)event
{
    [super sendEvent:event];
}

@end

/*-------------------------------------------------*/

Example::Example(std::string title, uint32_t width, uint32_t height)
    : title_(title)
    , width_(width)
    , height_(height)
    , delegate_(nil)
{

}

Example::~Example()
{
    
}

int Example::run()
{
    NSString* title = [[NSString alloc] initWithUTF8String:title_.c_str()];
    NSRect rect = NSMakeRect(0.0f, 0.0f, static_cast<CGFloat>(width_), static_cast<CGFloat>(height_));
    delegate_ = [[AppDelegate alloc] initWithTitleAndRect:title :rect];
    
    App* application = [App sharedApplication];
    [application setActivationPolicy:NSApplicationActivationPolicyRegular];
    [application activateIgnoringOtherApps:YES];
    [application setDelegate:delegate_];
    [application finishLaunching];
    
    while([((AppDelegate*)delegate_) isRunning])
    {
        // Handle event processing
        NSEvent* event;
        do {
            event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES];
            if (event) {
                // Handle events
                [NSApp sendEvent:event]; //propogate
                
                //processEvent(event);
            }

        } while (event);
        
        // update
        update();
        
        // render
        render();
    }
    
    return EXIT_SUCCESS;
}
