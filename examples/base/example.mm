/*
*    Metal Examples
*    Copyright(c) 2019 Matt Guerrette
*
*    This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "example.h"
#import <GameController/GameController.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <atomic>
#include <cassert>
#include <chrono>

using namespace std::chrono_literals;

constexpr std::chrono::nanoseconds timestep(16ms);

#ifdef SYSTEM_MACOS

/*----------------------------------------------*/
// Metal View
/*----------------------------------------------*/

@interface                                  MetalView : NSView
@property (nonatomic, assign) CAMetalLayer* metalLayer;
@end

@implementation MetalView

- (BOOL)isFlipped
{
    return YES;
}

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
    self.metalLayer     = layer;
    return self.metalLayer;
}

- (instancetype)initWithFrame:(NSRect)frameRect
{
    if ((self = [super initWithFrame:frameRect])) {
        self.wantsLayer              = YES;
        self.metalLayer.device       = MTLCreateSystemDefaultDevice();
        self.metalLayer.pixelFormat  = MTLPixelFormatBGRA8Unorm;
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
// View Controller
/*-------------------------------------------------*/

@interface ViewController : GCEventViewController

@end

@implementation ViewController

- (void)loadView
{
    /* Do nothing */
}

- (void)mouseMoved:(NSEvent *)event
{
    NSPoint event_location = [event locationInWindow];
    //NSPoint local_point = [self.view convertPoint:event_location fromView:nil];
    if(NSMouseInRect(event_location, [self.view frame], YES))
    {
        //NSLog(@"Mouse X: %ld Y: %ld", (long)event_location.x, (long)event_location.y);
    }
}

@end

/*-------------------------------------------------*/

/*-------------------------------------------------*/
// Application Delegate
/*-------------------------------------------------*/

@interface AppDelegate : NSObject <NSApplicationDelegate> {
@public
    Example* example;
}
@property (strong, nonatomic) NSWindow*         window;
@property (strong, nonatomic) MetalView*        view;
@property (strong, nonatomic) ViewController*   viewController;
@property (atomic) BOOL                         needsDisplay;
@property (nonatomic, nonnull) CVDisplayLinkRef displayLink;
@property (nonatomic) NSString*                 title;
@property (nonatomic) BOOL                      isRunning;
@property (nonatomic) NSRect                    windowRect;
- (void)_windowWillClose:(NSNotification*)notification;
- (void)_controllerDidConnect:(NSNotification*)notification;
- (instancetype)initWithTitleAndRect:(NSString*)title:(NSRect)rect;
@end

@implementation AppDelegate

- (instancetype)initWithTitleAndRect:(NSString*)title:(NSRect)rect
{
    self = [super init];
    if (self != nil) {
        [self setTitle:title];
        [self setWindowRect:rect];
    }

    return self;
}

- (void)_windowWillClose:(NSNotification*)notification
{
    [self setIsRunning:NO];
}

- (void)_controllerDidConnect:(NSNotification *)notification
{
    GCController* controller = [notification object];
    NSLog(@"Controller: %@", [controller vendorName]);
    
    GCExtendedGamepad* extended_gamepad = [controller extendedGamepad];
    if(extended_gamepad != nil)
    {
        [[extended_gamepad rightThumbstick] setValueChangedHandler:^(GCControllerDirectionPad * _Nonnull dpad, float xValue, float yValue) {
            NSLog(@"X: %.2f, Y: %.2f", xValue, yValue);
            example->onRightThumbstick(xValue, yValue);
        }];
        
        [[extended_gamepad buttonA] setValueChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed) {
            NSLog(@"%@", pressed ? @"Pressed" : @"Released");
        }];
    }
}

// Display link callback
// Handles signaling render should occur
CVReturn displayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    AppDelegate* delegate = (__bridge AppDelegate*)displayLinkContext;
    if (delegate) {
        [delegate setNeedsDisplay:YES];
    }

    return kCVReturnSuccess;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    NSRect frame = [self windowRect];

    self.viewController = [[ViewController alloc] initWithNibName:nil bundle:nil];
    self.view = [[MetalView alloc] initWithFrame:frame];
    [_viewController setView:_view];

    const int style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
    self.window     = [[Window alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:YES];
    [self.window setTitle:_title];
    [self.window setOpaque:YES];
    [self.window setAcceptsMouseMovedEvents:YES];
    [self.window setContentViewController:self.viewController];
    [self.window makeMainWindow];
    [self.window makeFirstResponder:nil];
    [self.window makeKeyAndOrderFront:nil];
    [self.window center];

    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self selector:@selector(_windowWillClose:) name:NSWindowWillCloseNotification object:nil];
    [center addObserver:self selector:@selector(_controllerDidConnect:) name:GCControllerDidConnectNotification object:nil];

    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    CVDisplayLinkSetOutputCallback(_displayLink, displayLinkCallback, (__bridge_retained void*)self);
    CVDisplayLinkSetCurrentCGDisplay(_displayLink, 0);
    CVDisplayLinkStart(_displayLink);

    if (example) {
        example->init();
    }
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

#else

/*----------------------------------------------*/
// Metal View
/*----------------------------------------------*/

@interface                                  MetalView : UIView
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

//- (CALayer*)makeBackingLayer
//{
//    CAMetalLayer* layer = [CAMetalLayer layer];
//    self.metalLayer     = layer;
//    return self.metalLayer;
//}

- (instancetype)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) {
        self.metalLayer = (CAMetalLayer*)self.layer;
        self.metalLayer.device       = MTLCreateSystemDefaultDevice();
        self.metalLayer.pixelFormat  = MTLPixelFormatBGRA8Unorm;
        self.metalLayer.drawableSize = self.bounds.size;
    }

    return self;
}

//- (instancetype)initWithFrame:(NSRect)frameRect
//{
//    if ((self = [super initWithFrame:frameRect])) {
//        self.wantsLayer              = YES;
//        self.metalLayer.device       = MTLCreateSystemDefaultDevice();
//        self.metalLayer.pixelFormat  = MTLPixelFormatBGRA8Unorm;
//        self.metalLayer.drawableSize = self.bounds.size;
//    }
//
//    return self;
//}

@end

@interface ViewController : GCEventViewController
- (void)startAnimation;
- (void)stopAnimation;
- (void)setAnimationCallback:(int)interval
                    callback:(void (*)(void*))callback
               callbackParam:(void*)callbackParam;
- (void)doLoop:(CADisplayLink*)sender;
@end

@implementation ViewController {
    CADisplayLink* displayLink;
    int            animationInterval;
    void (*animationCallback)(void*);
    void* animationCallbackParam;
}

- (void)setAnimationCallback:(int)interval
                    callback:(void (*)(void*))callback
               callbackParam:(void*)callbackParam
{
    [self stopAnimation];

    animationInterval      = interval;
    animationCallback      = callback;
    animationCallbackParam = callbackParam;

    if (animationCallback) {
        [self startAnimation];
    }
}

- (void)startAnimation
{
    displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(doLoop:)];

    if ([displayLink respondsToSelector:@selector(preferredFramesPerSecond)]
        && [[UIScreen mainScreen] respondsToSelector:@selector(maximumFramesPerSecond)]) {
        displayLink.preferredFramesPerSecond = [UIScreen mainScreen].maximumFramesPerSecond / animationInterval;
    }

    [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
}

- (void)stopAnimation
{
    [displayLink invalidate];
    displayLink = nil;
}

- (void)doLoop:(CADisplayLink*)sender
{
    animationCallback(animationCallbackParam);
}

- (void)loadView
{
    // Do nothing
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    NSLog(@"Touches began...");
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    NSLog(@"Touches moved...");
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    NSLog(@"Touches ended...");
}

@end

#ifndef SYSTEM_MACOS
static void runCallback(void* data)
{
    using clock = std::chrono::high_resolution_clock;
    
    Example* example = static_cast<Example*>(data);
    if(example != nullptr)
    {
        auto current_time = clock::now();
        auto delta_time   = current_time - example->start_time;
        example->start_time        = current_time;

        example->lag += std::chrono::duration_cast<std::chrono::nanoseconds>(delta_time);
        
        // Fixed update loop at 'timestep' defined
        // as 16ms per frame.
        while (example->lag >= timestep) {

            example->update();

            example->lag -= timestep;
        }
        
        example->render();
    }
}
#endif

@interface AppDelegate : NSObject <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) ViewController *controller;
@property (strong, nonatomic) MetalView *view;
+ (NSString*)getAppDelegateClassName;
- (void)_controllerDidConnect:(NSNotification*)notification;
@end

@implementation AppDelegate

static Example* example;


- (void)_controllerDidConnect:(NSNotification *)notification
{
    GCController* controller = [notification object];
    NSLog(@"Controller: %@", [controller vendorName]);
    
    GCExtendedGamepad* extended_gamepad = [controller extendedGamepad];
    if(extended_gamepad != nil)
    {
        [[extended_gamepad rightThumbstick] setValueChangedHandler:^(GCControllerDirectionPad * _Nonnull dpad, float xValue, float yValue) {
            NSLog(@"X: %.2f, Y: %.2f", xValue, yValue);
            example->onRightThumbstick(xValue, yValue);
        }];
        
        [[extended_gamepad buttonA] setValueChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed) {
            NSLog(@"%@", pressed ? @"Pressed" : @"Released");
        }];
    }
}

- (BOOL)application:(UIApplication *)application willFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey,id> *)launchOptions
{
    self.controller  = [[ViewController alloc] initWithNibName:nil bundle:nil];
    self.view = [[MetalView alloc] initWithFrame:[UIScreen mainScreen].bounds];
    [self.controller setView:self.view];

    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    [self.window setRootViewController:self.controller];
    [self.window makeKeyAndVisible];
    
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self selector:@selector(_controllerDidConnect:) name:GCControllerDidConnectNotification object:nil];
    
    
    example->start_time = std::chrono::high_resolution_clock::now();
    example->lag = 0ns;
    example->metalLayer_ = self.view.metalLayer;
    example->init();
    [self.controller setAnimationCallback:1 callback:runCallback callbackParam:example];
    
    return YES;
}

- (void)applicationDidFinishLaunching:(UIApplication*)application
{
    NSLog(@"Finished launching...");
}

+ (NSString*)getAppDelegateClassName
{
    return @"AppDelegate";
}

- (void)applicationWillTerminate:(UIApplication*)application
{
    NSLog(@"Will terminate...");
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application
{
    NSLog(@"Received memory warning");
}

- (void)applicationWillResignActive:(UIApplication*)application
{
    NSLog(@"Will resign active...");
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
    NSLog(@"Entered background");
}

- (void)applicationWillEnterForeground:(UIApplication*)application
{
    NSLog(@"Entering foreground...");
}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
    NSLog(@"Became active");
}

@end

#endif

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

CAMetalLayer* Example::metalLayer()
{

#ifdef SYSTEM_MACOS
    return ((AppDelegate*)delegate_).view.metalLayer;
#else
    return metalLayer_;//((AppDelegate*)delegate_).view.metalLayer;
#endif
}

int Example::run(int argc, char* argv[])
{
#ifdef SYSTEM_MACOS    
    using clock = std::chrono::high_resolution_clock;

    NSString* title = [[NSString alloc] initWithUTF8String:title_.c_str()];
    
    NSRect rect = NSMakeRect(0.0f, 0.0f, static_cast<CGFloat>(width_), static_cast<CGFloat>(height_));
    delegate_   = [[AppDelegate alloc] initWithTitleAndRect:title:rect];

    App* application = [App sharedApplication];
    [application setActivationPolicy:NSApplicationActivationPolicyRegular];
    [application activateIgnoringOtherApps:YES];
    [application setDelegate:delegate_];
    [application finishLaunching];

    auto                     start_time = clock::now();
    std::chrono::nanoseconds lag(0ns);

    AppDelegate* delegate = (AppDelegate*)delegate_;
    delegate->example     = this;
    while ([delegate isRunning]) {
        auto current_time = clock::now();
        auto delta_time   = current_time - start_time;
        start_time        = current_time;

        lag += std::chrono::duration_cast<std::chrono::nanoseconds>(delta_time);

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

        // Fixed update loop at 'timestep' defined
        // as 16ms per frame.
        while (lag >= timestep) {

            update();

            lag -= timestep;
        }

        // render
        if ([delegate needsDisplay]) {
            render();
        }
    }
#else
//
//    AppDelegate* delegate = [[AppDelegate alloc] init];
//    this->delegate_ = delegate;
//    delegate->example = this;
//    [[UIApplication sharedApplication] setDelegate:delegate];
    example = this;
    
    width_ = [UIScreen mainScreen].bounds.size.width;
    height_ = [UIScreen mainScreen].bounds.size.height;
    UIApplicationMain(argc, argv, nil, [AppDelegate getAppDelegateClassName]);

#endif

    return EXIT_SUCCESS;
}
