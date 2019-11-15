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

using namespace std::chrono_literals;

constexpr std::chrono::nanoseconds timestep(16ms);

/*----------------------------------------------*/
// Metal View
/*----------------------------------------------*/

@interface MetalView : UIView
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

- (instancetype)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) {
        self.metalLayer = (CAMetalLayer*)self.layer;
        self.metalLayer.device = MTLCreateSystemDefaultDevice();
        self.metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        self.metalLayer.drawableSize = self.bounds.size;
    }

    return self;
}

@end

@interface ViewController : GCEventViewController {
@public
    Example* example;
}
- (void)startAnimation;
- (void)stopAnimation;
- (void)setAnimationCallback:(int)interval
                    callback:(void (*)(void*))callback
               callbackParam:(void*)callbackParam;
- (void)doLoop:(CADisplayLink*)sender;
@end

@implementation ViewController {
    CADisplayLink* displayLink;
    int animationInterval;
    void (*animationCallback)(void*);
    void* animationCallbackParam;
}

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
    if (example != nil) {
        example->onSizeChange(size.width, size.height);
    }
}

- (void)setAnimationCallback:(int)interval
                    callback:(void (*)(void*))callback
               callbackParam:(void*)callbackParam
{
    [self stopAnimation];

    animationInterval = interval;
    animationCallback = callback;
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

static void runCallback(void* data)
{
    using clock = std::chrono::high_resolution_clock;

    Example* example = static_cast<Example*>(data);
    if (example != nullptr) {
        auto current_time = clock::now();
        auto delta_time = current_time - example->start_time;
        example->start_time = current_time;

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

@interface AppDelegate : NSObject <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) ViewController* controller;
@property (strong, nonatomic) MetalView* view;
+ (NSString*)getAppDelegateClassName;
- (void)_controllerDidConnect:(NSNotification*)notification;
@end

@implementation AppDelegate

static Example* example;

- (void)_controllerDidConnect:(NSNotification*)notification
{
    GCController* controller = [notification object];
    NSLog(@"Controller: %@", [controller vendorName]);

    GCExtendedGamepad* extended_gamepad = [controller extendedGamepad];
    if (extended_gamepad != nil) {
        [[extended_gamepad rightThumbstick] setValueChangedHandler:^(GCControllerDirectionPad* _Nonnull dpad, float xValue, float yValue) {
            example->onRightThumbstick(xValue, yValue);
        }];

        [[extended_gamepad leftThumbstick] setValueChangedHandler:^(GCControllerDirectionPad* _Nonnull dpad, float xValue, float yValue) {
            example->onLeftThumbstick(xValue, yValue);
        }];

        [[extended_gamepad buttonA] setValueChangedHandler:^(GCControllerButtonInput* _Nonnull button, float value, BOOL pressed) {

        }];
    }
}

- (BOOL)application:(UIApplication*)application willFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id>*)launchOptions
{
    self.controller = [[ViewController alloc] initWithNibName:nil bundle:nil];
    self.view = [[MetalView alloc] initWithFrame:[UIScreen mainScreen].bounds];
    [self.controller setView:self.view];
    _controller->example = example;

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
    return metalLayer_; //((AppDelegate*)delegate_).view.metalLayer;
}

int Example::run(int argc, char* argv[])
{
    example = this;

    width_ = [UIScreen mainScreen].bounds.size.width;
    height_ = [UIScreen mainScreen].bounds.size.height;
    UIApplicationMain(argc, argv, nil, [AppDelegate getAppDelegateClassName]);

    return EXIT_SUCCESS;
}
