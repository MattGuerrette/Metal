
#include <SDL2/SDL.h>

class Example
{
public:
    Example(const char* title, uint32_t width, uint32_t height);
    
    virtual ~Example();
    
    int Run(int argc, char** argv);
    
    uint32_t GetFrameWidth() const;
    uint32_t GetFrameHeight() const;
    
    virtual bool Load() = 0;
    virtual void Update(float elapsed) = 0;
    virtual void Render(float elapsed) = 0;
    
protected:
    SDL_MetalView View;
private:
    SDL_Window* Window;
    uint32_t Width;
    uint32_t Height;
    float LastClockTimestamp;
    float CurrentClockTimestamp;
    bool Running;
};
