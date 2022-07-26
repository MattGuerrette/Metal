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

#ifdef SYSTEM_MACOS
#import <AppKit/AppKit.h>
#endif

#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

constexpr std::chrono::nanoseconds timestep(16ms);

/*----------------------------------------------*/
// Metal View
/*----------------------------------------------*/

@interface MetalView : NSView
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
// View Controller
/*-------------------------------------------------*/

@interface ViewController : GCEventViewController {
@public
    Example* example;
}

@end

@implementation ViewController

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

- (void)loadView
{
    /* Do nothing */
}

- (void)keyDown:(NSEvent*)event
{
    const unsigned short code = [event keyCode];
    example->onKeyDown((Key)code);
}

- (void)keyUp:(NSEvent*)event
{
    const unsigned short code = [event keyCode];
    example->onKeyUp((Key)code);
}

- (void)mouseMoved:(NSEvent*)event
{
    NSPoint event_location = [event locationInWindow];
    //NSPoint local_point = [self.view convertPoint:event_location fromView:nil];
    if (NSMouseInRect(event_location, [self.view frame], YES)) {
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
@property (strong, nonatomic) NSWindow* window;
@property (strong, nonatomic) MetalView* view;
@property (strong, nonatomic) ViewController* viewController;
@property (atomic) BOOL needsDisplay;
@property (nonatomic, nonnull) CVDisplayLinkRef displayLink;
@property (nonatomic) NSString* title;
@property (nonatomic) BOOL isRunning;
@property (nonatomic) NSRect windowRect;
- (void)_windowWillClose:(NSNotification*)notification;
- (void)_controllerDidConnect:(NSNotification*)notification;
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

- (void)_windowWillClose:(NSNotification*)notification
{
    [self setIsRunning:NO];
}

- (void)_controllerDidConnect:(NSNotification*)notification
{
    GCController* controller = [notification object];
    NSLog(@"Controller: %@", [controller vendorName]);

    GCExtendedGamepad* extended_gamepad = [controller extendedGamepad];
    if (extended_gamepad != nil) {
        [[extended_gamepad rightThumbstick] setValueChangedHandler:^(GCControllerDirectionPad* _Nonnull dpad, float xValue, float yValue) {
            //NSLog(@"X: %.2f, Y: %.2f", xValue, yValue);
            self->example->onRightThumbstick(xValue, yValue);
        }];

        [[extended_gamepad leftThumbstick] setValueChangedHandler:^(GCControllerDirectionPad* _Nonnull dpad, float xValue, float yValue) {
            //NSLog(@"X: %.2f, Y: %.2f", xValue, yValue);
            self->example->onLeftThumbstick(xValue, yValue);
        }];

        [[extended_gamepad buttonA] setValueChangedHandler:^(GCControllerButtonInput* _Nonnull button, float value, BOOL pressed) {
            //NSLog(@"%@", pressed ? @"Pressed" : @"Released");
        }];
    }
}

// Display link callback
// Handles signaling render should occur
CVReturn displayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    AppDelegate* delegate = (__bridge AppDelegate*)displayLinkContext;
    if (delegate) {
        delegate->example->render();
        //[delegate setNeedsDisplay:YES];
    }

    return kCVReturnSuccess;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    NSRect frame = [self windowRect];

    self.viewController = [[ViewController alloc] initWithNibName:nil bundle:nil];
    self.view = [[MetalView alloc] initWithFrame:frame];
    [_viewController setView:_view];

    const int style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    self.window = [[Window alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:YES];
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
        _viewController->example = example;
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

/*-------------------------------------------------*/

Example::Example(std::string title, uint32_t width, uint32_t height)
    : title_(title)
    , width_(width)
    , height_(height)
//, delegate_(nil)
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
    return metalLayer_; //((AppDelegate*)delegate_).view.metalLayer;
#endif
}

int Example::run(int argc, char* argv[])
{
    using clock = std::chrono::high_resolution_clock;

    NSString* title = [[NSString alloc] initWithUTF8String:title_.c_str()];

    NSRect rect = NSMakeRect(0.0f, 0.0f, static_cast<CGFloat>(width_), static_cast<CGFloat>(height_));
    delegate_ = [[AppDelegate alloc] initWithTitleAndRect:title:rect];

    App* application = [App sharedApplication];
    [application setActivationPolicy:NSApplicationActivationPolicyRegular];
    [application activateIgnoringOtherApps:YES];
    [application setDelegate:delegate_];
    [application finishLaunching];

    auto start_time = clock::now();
    std::chrono::nanoseconds lag(0ns);

    AppDelegate* delegate = (AppDelegate*)delegate_;
    delegate->example = this;
    while ([delegate isRunning]) {
        auto current_time = clock::now();
        auto delta_time = current_time - start_time;
        start_time = current_time;

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
            [delegate setNeedsDisplay:NO];
        }
        
        // Sleep for 8ms to avoid thrashing the CPU
        // TODO: Calculate sleep time
        std::this_thread::sleep_for(std::chrono::milliseconds(8ms));
    }

    return EXIT_SUCCESS;
}
