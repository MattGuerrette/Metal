
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>

#import <SDL2/SDL.h>

@interface Example : NSObject

- (instancetype)initTitleWithDimensions:(NSString*)title
                                       :(NSUInteger)width
                                       :(NSUInteger)height;

- (NSInteger)run:(int)argc
                :(const char**)argv;

- (BOOL)load;

- (void)update:(double)elapsed;

- (void)render:(double)elasped;

@property (readonly) CAMetalLayer* metalLayer;

@end

//#include <chrono>
//#include <string>
//
//#include <SDL.h>
//#include <QuartzCore/QuartzCore.h>
//
//class Example
//{
// public:
//    Example(const char* title, uint32_t width, uint32_t height);
//
//    virtual ~Example();
//
//    virtual void Init() = 0;
//
//    virtual void Update(double elapsed) = 0;
//
//    virtual void Render(double elapsed) = 0;
//
//    // Run loop
//    int Run(int argc, char* argv[]);
//
//    CAMetalLayer* GetMetalLayer() const;
//
// private:
//    static int EventFilter(void* self, SDL_Event* e);
//
//    static void FrameTick(void* self);
//
//    std::string   Title;
//    bool          Running;
//    uint32_t      Width;
//    uint32_t      Height;
//    SDL_MetalView MetalView;
//    SDL_Window* Window;
//    uint64_t CurrentClockTime = 0;
//    uint64_t LastClockTime = 0;
//};
