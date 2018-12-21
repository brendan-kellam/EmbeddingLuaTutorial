
#include "ArenaAllocator.h"
#include "AutomatedBinding.h"
#include "lua.hpp"
#include <string.h>
#include <cstdio>
#include <rttr/registration>

/**
 Note: We have a extremly week coupling between AutomatedBinding.cpp and TestRegistrations.cpp
 Due to RTTR. Note that there is no header file for TestRegistrations
**/


const char* LUA_SCRIPT = R"(
-- lua script
Global.HelloWorld()
Global.HelloWorld2()
Global.Test()
)";


int CallGlobalFromLua(lua_State* L)
{
    // Grab type information from up-value
    rttr::method* m = (rttr::method*) lua_touserdata(L, lua_upvalueindex(1));
    
    // Invoke method
    rttr::method& methodToInvoke(*m);
    methodToInvoke.invoke({});
    
    return 0;
}

void AutomatedBindingTutorial()
{
    printf("---- Automated binding using RTTR ---\n");
    
    // 20KB of memory on stack
    constexpr int POOL_SIZE = 1024 * 20;
    char memory[POOL_SIZE];
    
    ArenaAllocator pool(memory, &memory[POOL_SIZE - 1]);
    
    // Open the lua state using memory pool
    lua_State* L = lua_newstate(ArenaAllocator::l_alloc, &pool);
    
    // Push new table and name it "Global"
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setglobal(L, "Global");                                         // 1
    lua_pushvalue(L, -1);
    
    // Bind global methods!
    for (auto& method : rttr::type::get_global_methods())
    {
        // Push name of method
        lua_pushstring(L, method.get_name().to_string().c_str());       // 2
        
        lua_pushlightuserdata(L, (void*) &method);
        lua_pushcclosure(L, CallGlobalFromLua, 1);                      // 3
        
        // Set the table
        lua_settable(L, -3);                                            //1[2] = 3
    }
  
    
    // Execute lua script
    int res = luaL_dostring(L, LUA_SCRIPT);
    if (res != LUA_OK)
    {
        printf("Error: %s\n", lua_tostring(L, -1));
    }
    
    lua_close(L);
}
