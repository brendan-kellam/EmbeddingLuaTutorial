#include <rttr/registration>
#include <cstdio>


// Contains the stuff we are going to register
// with RTTR and will be found to Lua

void HelloWorld()
{
    printf("Hello, World!\n");
}

void HelloWorld2()
{
    printf("Hello, World 2!\n");
}

void Test(int x, short y, short z)
{
    printf("Test! %d, %d, %d\n", x, y, z);
}

short Add(short x, short y)
{
    return x + y;
}

int Mul(int x, int y)
{
    return x * y;
}

// Our own type
struct Sprite
{
    int x; // bad encaspulation
    int y;
    
    Sprite() : x(0), y(0)
    { }
    
    ~Sprite()
    { }
    
    void Move(int velX, int velY)
    {
        x += velX;
        y += velY;
    }
    
    void Draw()
    {
        printf("sprite(%p): x = %d, y = %d\n", this, x, y);
    }
};


// Register our native types
// Creates method before main gets called
RTTR_REGISTRATION
{
    rttr::registration::method("HelloWorld", &HelloWorld);
    rttr::registration::method("HelloWorld2", &HelloWorld2);
    rttr::registration::method("Test", &Test);
    rttr::registration::method("Add", &Add);
    rttr::registration::method("Mul", &Mul);
    
    // Register Sprite class
    rttr::registration::class_<Sprite>("Sprite")
        .constructor()
        .method("Move", &Sprite::Move)
        .method("Draw", &Sprite::Draw)
        .property("x", &Sprite::x)
        .property("y", &Sprite::y);
}

