#include "example.h"

class Triangle : public Example
{
public:
    Triangle();
    
    ~Triangle();
    
    void update() override;
    void render() override;
    
private:
};

Triangle::Triangle()
    : Example("Triangle", 1280, 720)
{
    
}

Triangle::~Triangle()
{
    
}

void Triangle::update()
{
    
}

void Triangle::render()
{
    
}

int main(int argc, char** argv)
{
    Triangle example;
    
    return example.run();
}
