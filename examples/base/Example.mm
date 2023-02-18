

#import "Example.h"


@interface Example ()
{
@public
    SDL_Window* _window;
    SDL_MetalView _view;
    BOOL _running;
    double _lastClockTime;
    double _currentClockTime;
    Example* _hold;
}

@end


int eventFilter(void* data, SDL_Event* e)
{
    Example* example = (__bridge Example*)data;

    switch (e->type)
    {
        case SDL_APP_WILLENTERFOREGROUND:
        {
            break;
        }
        case SDL_APP_WILLENTERBACKGROUND:
        {
            break;
        }

            
        case SDL_APP_TERMINATING:
        {
            example->_hold = nil;
            break;
        }

    default:
        break;
    }

    return 1;
}


void frameTick(void* data)
{
    Example* example = (__bridge Example*)data;

    example->_lastClockTime = example->_currentClockTime;
    example->_currentClockTime = SDL_GetPerformanceCounter();

    double elapsed = static_cast<double>(example->_currentClockTime - example->_lastClockTime) / static_cast<double>(SDL_GetPerformanceFrequency());

    // Update
    [example update:elapsed];

    [example render:elapsed];
}


@implementation Example

- (instancetype)initTitleWithDimensions:(NSString *)title :(NSUInteger)width :(NSUInteger)height {
    self = [super init];
    
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        NSLog(@"Failed to initialize SDL.");
        abort();
    }
    
    _hold = self;
    
    int flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_METAL;
#if defined(__IPHONEOS__) || defined(__TVOS__)
    flags |= SDL_WINDOW_FULLSCREEN;
#endif
    _window = SDL_CreateWindow([title UTF8String], SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)width, (int)height, flags);
    _view = SDL_Metal_CreateView(_window);
    _running = YES;

    
    return self;
}

- (CAMetalLayer *)metalLayer {
    return (__bridge CAMetalLayer*)SDL_Metal_GetLayer(_view);
}


- (void)dealloc {
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

- (BOOL)load {
    NSAssert(NO, @"Subclasses must implement this method.");
    return NO;
}

- (void)update:(double)elapsed {
    NSAssert(NO, @"Subclasses must implement this method.");
}

- (void)render:(double)elasped {
    NSAssert(NO, @"Subclasses must implement this method.");
}

- (NSInteger)run:(int)argc :(const char **)argv {
    [self load];
    #if defined(__IPHONEOS__) || defined(__TVOS__)
        SDL_SetEventFilter(eventFilter, (__bridge void*)self);
    
        // Use application callback for iOS/iPadOS
        SDL_iPhoneSetAnimationCallback(_window, 1, frameTick, (__bridge void*)self);
    #else
    
        // Run normal application loop
        while (_running)
        {
            SDL_Event e;
            SDL_PollEvent(&e);
            if (e.type == SDL_QUIT)
            {
                _running = NO;
                break;
            }
            if(e.type == SDL_WINDOWEVENT)
            {
                switch(e.window.event)
                {
                    case SDL_WINDOWEVENT_SHOWN:
                    {
                        break;
                    }
                }
            }
    
            _lastClockTime = _currentClockTime;
            _currentClockTime = SDL_GetPerformanceCounter();
    
            double elapsed = static_cast<double>((_currentClockTime - _lastClockTime))
                             / static_cast<double>(SDL_GetPerformanceFrequency());
    
            [self update:elapsed];
            
            [self render:elapsed];
        }
    #endif
    return EXIT_SUCCESS;
}

@end



//Example::Example(const char* title, uint32_t width, uint32_t height)
//    : Title(title), Width(width), Height(height)
//{
//    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
//    {
//        NSLog(@"Failed to initialize SDL.");
//    }
//
//    Window =
//        SDL_CreateWindow(Title.c_str(), SDL_WINDOWPOS_CENTERED,
//                         SDL_WINDOWPOS_CENTERED, width, height,
//                         SDL_WINDOW_ALLOW_HIGHDPI | /*SDL_WINDOW_BORDERLESS |*/
//                         /*SDL_WINDOW_FULLSCREEN_DESKTOP |*/ SDL_WINDOW_METAL);
//    MetalView = SDL_Metal_CreateView(Window);
//    Running   = true;
//}
//
//Example::~Example()
//{
//    SDL_DestroyWindow(Window);
//    SDL_Quit();
//}
//
//CAMetalLayer* Example::GetMetalLayer() const
//{
//    return (__bridge CAMetalLayer*)SDL_Metal_GetLayer(MetalView);
//}
//
//int Example::EventFilter(void* self, SDL_Event* e)
//{
//    Example* example = static_cast<Example*>(self);
//
//    switch (e->type)
//    {
//    case SDL_APP_WILLENTERBACKGROUND:
//    {
//        break;
//    }
//
//    case SDL_APP_WILLENTERFOREGROUND:
//    {
//        break;
//    }
//
//    default:
//        break;
//    }
//
//    return 1;
//}
//
//void Example::FrameTick(void* self)
//{
//    Example* example = static_cast<Example*>(self);
//
//    example->LastClockTime = example->CurrentClockTime;
//    example->CurrentClockTime = SDL_GetPerformanceCounter();
//
//    double elapsed = static_cast<double>(example->CurrentClockTime - example->LastClockTime) / static_cast<double>(SDL_GetPerformanceFrequency());
//
//    // Update
//    example->Update(elapsed);
//
//    // Render
//    example->Render(elapsed);
//}
//
//int Example::Run(int argc, char* argv[])
//{
//    Init();
//#if defined(__IPHONEOS__) || defined(__TVOS__)
//    SDL_SetEventFilter(EventFilter, this);
//
//    // Use application callback for iOS/iPadOS
//    SDL_iPhoneSetAnimationCallback(Window, 1, FrameTick, this);
//#else
//
//    // Run normal application loop
//    while (Running)
//    {
//        SDL_Event e;
//        SDL_PollEvent(&e);
//        if (e.type == SDL_QUIT)
//        {
//            Running = false;
//            break;
//        }
//        if (e.type == SDL_DROPFILE)
//        {      // In case if dropped file
//            char* dropped_filedir = e.drop.file;
//            // Shows directory of dropped file
//            SDL_ShowSimpleMessageBox(
//                SDL_MESSAGEBOX_INFORMATION,
//                "File dropped on window",
//                dropped_filedir,
//                Window
//            );
//            SDL_free(dropped_filedir);    // Free dropped_filedir memory
//            break;
//        }
//
//        LastClockTime = CurrentClockTime;
//        CurrentClockTime = SDL_GetPerformanceCounter();
//
//        double elapsed = static_cast<double>((CurrentClockTime - LastClockTime))
//                         / static_cast<double>(SDL_GetPerformanceFrequency());
//
//        Update(elapsed);
//
//        Render(elapsed);
//
//    }
//#endif
//    return EXIT_SUCCESS;
//}
