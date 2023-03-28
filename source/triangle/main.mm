

#import <Foundation/Foundation.h>
#import <TargetConditionals.h>

#import <DXMathKit/DXMathKit.h>

#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#else
#import <UIKit/UIKit.h>
#import "iOS/AppDelegate.h"
#endif

int main(int argc, char** argv)
{
    if (!DXMVerifyCPUSupport())
    {
        NSLog(@"Unsupported CPU");
        return EXIT_FAILURE;
    }
    
#if TARGET_OS_IOS
    NSString* appDelegateClass;
    @autoreleasepool {
        appDelegateClass = NSStringFromClass([AppDelegate class]);
    }
    return UIApplicationMain(argc, argv, nil, appDelegateClass);
#else
    return NSApplicationMain(argc, (const char**)argv);
#endif
}
