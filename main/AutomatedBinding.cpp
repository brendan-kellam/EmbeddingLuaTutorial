
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
local c = Global.Mul(42, 43)
Global.Test(c, 22, 10)
)";

union PassByValue
{
    int intVal;
    short shortVal;
};

template <typename T>
T& selectMember(PassByValue& u)
{
    return *((T*)&u);
}

template <typename T>
void doSomething(PassByValue& a, lua_Number b)
{
    selectMember<T>(a) = (T) b;
}

int CallGlobalFromLua(lua_State* L)
{
    // Grab type information from up-value
    rttr::method* m = (rttr::method*) lua_touserdata(L, lua_upvalueindex(1));
    
    // Invoke method
    rttr::method& methodToInvoke(*m);
    rttr::array_range<rttr::parameter_info> nativeParams = methodToInvoke.get_parameter_infos();
    
    // Top of stack index = number of arguments passed
    int numLuaArgs = lua_gettop(L);
    int numNativeArgs = (int) nativeParams.size();
    
    printf("Number of lua args: %d\n", numLuaArgs);
    printf("Number of native args: %d\n", numNativeArgs);
    
    if (numLuaArgs != numNativeArgs)
    {
        printf("Error calling native function '%s', wrong number of args, expected %d, got %d\n",
               methodToInvoke.get_name().to_string().c_str(), numNativeArgs, numLuaArgs);
        assert(numLuaArgs == numNativeArgs);
    }
    
    
    
    // For arguments passed by value, they will go out of scope! We need to have references to them before calling invoke_variadic
    std::vector<PassByValue> pbv(numNativeArgs);
    
    // Native args to past to rttr invoke
    std::vector<rttr::argument> nativeArgs(numNativeArgs);
    auto nativeParamsIt = nativeParams.begin();
    
    // Loops all params and gets both native/lua type
    for (int i = 0; i < numLuaArgs; i++, nativeParamsIt++)
    {
        const rttr::type nativeParamType = nativeParamsIt->get_type();
        int luaArgIdx = i + 1;
        
        // Gets type
        int luaType = lua_type(L, luaArgIdx);
        
        // RTTR must have exact types
        switch (luaType)
        {
            case LUA_TNUMBER:
                
                // Check type
                if (nativeParamType == rttr::type::get<int>())
                {
                    // Instead of putting it on the stack, we are storing the lua_tonumber in our vector
                    doSomething<int>(pbv[i], lua_tonumber(L, luaArgIdx));
                    nativeArgs[i] = pbv[i].intVal;
                }
                else if (nativeParamType == rttr::type::get<short>())
                {
                    pbv[i].shortVal = (short) lua_tonumber(L, luaArgIdx);
                    nativeArgs[i] = pbv[i].shortVal;
                }
                else
                {
                    printf("Unrecognised parameter type '%s'\n", nativeParamType.get_name().to_string().c_str());
                    assert(false);
                }
                
                break;
                
            default:
                assert(false); // Don't know type!
                break;
        }
    }
    
    rttr::variant result = methodToInvoke.invoke_variadic({}, nativeArgs);
    int numberOfReturnValues = 0;
    if (result.is_valid() == false)
    {
        luaL_error(L, "Unable to invoke '%s'\n", methodToInvoke.get_name().to_string().c_str());
    }
    // If there is a return
    else if (result.is_type<void>() == false)
    {
        // Handle int return
        if (result.is_type<int>())
        {
            lua_pushnumber(L, result.get_value<int>());
            numberOfReturnValues++;
        }
        else if (result.is_type<short>())
        {
            lua_pushnumber(L, result.get_value<short>());
            numberOfReturnValues++;
        }
        
        else
        {
            luaL_error(L,
                       "Unhandled return type '%s' from native method '%s'\n",
                       result.get_type().get_name().to_string().c_str(),
                       methodToInvoke.get_name().to_string().c_str());
        }
        
    }
    
    return numberOfReturnValues;
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
