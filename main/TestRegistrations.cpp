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

void Test()
{
    printf("Test!\n");
}


// Register our native types
// Creates method before main gets called
RTTR_REGISTRATION
{
    rttr::registration::method("HelloWorld", &HelloWorld);
    rttr::registration::method("HelloWorld2", &HelloWorld2);
    rttr::registration::method("Test", &Test);
}

