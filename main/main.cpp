#include "ArenaAllocator.h"
#include "lua.hpp"
#include <iostream>
#include <assert.h>
#include <string.h>
#include <cstdio>
#include <new>

// Lua types:
// nil
// boolean
// light userdata - c pointer
// number - fixed point, floating point, int etc.
// string (Garbage collected by lua)
// table - Complex data type. Ex: x = { "foo", "bar" }
// function - all functions are lamdas (can be passed to another method, be stored in a table etc.)
// userdata - create your own types
// thread - coroutines 

// == What has been done ==
// Create and destroy lua state
// get global numbers from lua
// How to use the lua stack from c api
// call lua functions from c
// bind and call c functions from lua

int main()
{
	// Intro
	{
		lua_State* L = luaL_newstate();
		luaL_dostring(L, "x = 42");
		// Pushes the x var to the LUA stack
		lua_getglobal(L, "x");
		lua_Number x = lua_tonumber(L, 1);
		printf("lua says x = %d\n", (int)x);
		lua_close(L);
	}
	
	// Pushing values onto the lua stack
	{
		lua_State* L = luaL_newstate();
		lua_pushnumber(L, 42);
		lua_pushnumber(L, 52);
		lua_pushnumber(L, 62);

		// 42 - 1 or -3
		// 52 - 2 or -2
		// 62 - 3 or -1

		lua_Number x = lua_tonumber(L, -2);
		printf("lua says x = %d\n", (int)x);

		lua_remove(L, -2);

		// 42 - 1 or -2
		// 62 - 2 or -1

		x = lua_tonumber(L, -2);
		printf("lua says x = %d\n", (int)x);

		lua_close(L);
	}

	// Call LUA Functions
	{
        const char* LUA_FILE = R"(
		function Pythagoras(a, b)
			return (a*a) + (b*b), a, b
		end
		)";

		lua_State* L = luaL_newstate();
		luaL_dostring(L, LUA_FILE);

		// Pushes the function onto the stack
		lua_getglobal(L, "Pythagoras");

		// Check if top of stack is function
		if (lua_isfunction(L, -1))
		{
			// Push params to Pythagoras
			lua_pushnumber(L, 3);
			lua_pushnumber(L, 4);

			// Expects the last element on the stack to be a function
			constexpr int NUM_ARGS = 2;
			constexpr int NUM_RETURNS = 3;
			lua_pcall(L, NUM_ARGS, NUM_RETURNS, 0);
			lua_Number x = lua_tonumber(L, -3);
			lua_Number a = lua_tonumber(L, -2);
			lua_Number b = lua_tonumber(L, -1);

			printf("Result: %d | %d, %d\n", (int)x, (int)a, (int)b);
		}

		lua_close(L);
	}

	// Call C function
	{

		// Return number of vals leaving on stack
		auto NativePythagoras = [](lua_State* L) -> int
		{
			// Read numbers of stack
			lua_Number a = lua_tonumber(L, -2);
			lua_Number b = lua_tonumber(L, -1);

			// Compute
			lua_Number csqr = (a * a) + (b * b);

			// Push result
			lua_pushnumber(L, csqr);
			return 1;
		};

		const char* LUA_FILE = R"(
		function Pythagoras(a, b)
			csqr = NativePythagoras(a, b)
			return csqr, a, b
		end
	
		)";

		lua_State* L = luaL_newstate();

		// Push function pointer onto stack and bound
		lua_pushcfunction(L, NativePythagoras);
		lua_setglobal(L, "NativePythagoras");
		
		luaL_dostring(L, LUA_FILE);

		// Pushes the function onto the stack
		lua_getglobal(L, "Pythagoras");

		// Check if top of stack is function
		if (lua_isfunction(L, -1))
		{
			// Push params to Pythagoras
			lua_pushnumber(L, 3);
			lua_pushnumber(L, 4);

			// Expects the last element on the stack to be a function
			constexpr int NUM_ARGS = 2;
			constexpr int NUM_RETURNS = 3;
			lua_pcall(L, NUM_ARGS, NUM_RETURNS, 0);
			lua_Number x = lua_tonumber(L, -3);
			lua_Number a = lua_tonumber(L, -2);
			lua_Number b = lua_tonumber(L, -1);

			printf("Result: %d | %d, %d\n", (int)x, (int)a, (int)b);
		}

		lua_close(L);
	}

	// User data
	{

		// Our own type
		struct Sprite
		{
			int x;
			int y;

			void Move(int velX, int velY)
			{
				x += velX;
				y += velY;
			}
		};

		auto CreateSprite = [](lua_State* L) -> int
		{
			// Get lua to create a new sprite (and manage the memory!)
			Sprite* sprite = (Sprite*) lua_newuserdata(L, sizeof(Sprite));
			sprite->x = 0;
			sprite->y = 0;
			return 1;
		};

		auto MoveSprite = [](lua_State* L) -> int
		{
			Sprite* sprite = (Sprite*)lua_touserdata(L, -3);
			lua_Number velX = lua_tonumber(L, -2);
			lua_Number velY = lua_tonumber(L, -1);
			sprite->Move((int) velX, (int) velY);
			return 0;
		};

		const char* LUA_FILE = R"(
		sprite = CreateSprite()
		MoveSprite(sprite, 5, 7)
		)";

		lua_State* L = luaL_newstate();
		lua_pushcfunction(L, CreateSprite);
		lua_setglobal(L, "CreateSprite");
		lua_pushcfunction(L, MoveSprite);
		lua_setglobal(L, "MoveSprite");
		luaL_dostring(L, LUA_FILE);
		lua_getglobal(L, "sprite");

		if (lua_isuserdata(L, -1))
		{
			printf("Sprite!!\n");
			Sprite* sprite = (Sprite*) lua_touserdata(L, -1);
			printf("(%d,%d)\n", sprite->x, sprite->y);
		}
		else
		{
			printf("No sprite :(\n");
		}

		lua_close(L);
	}

	printf("---- Tables ---- \n");
	{
		const char* LUA_FILE = R"(
		x = {dave = "busy", ian = "idle" }
		)";

		lua_State* L = luaL_newstate();
		luaL_dostring(L, LUA_FILE);

		// Push table onto stack
		lua_getglobal(L, "x");

		// Push key onto stack
		lua_pushstring(L, "dave");
		
		// Get value from table
		lua_gettable(L, -2);

		// Note: Pointer to a string. Lua will garbage collectl
		const char* dave = lua_tostring(L, -1);
		printf("Dave is: %s\n", dave);

		// Simpler way of grabbing variable from table
		lua_getglobal(L, "x");
		lua_getfield(L, -1, "ian");
		const char* ian = lua_tostring(L, -1);
		printf("Ian is: %s\n", ian);

		// Push value into table
		lua_getglobal(L, "x");
		// Value we want to put in
		lua_pushstring(L, "sleeping");
		lua_setfield(L, -2, "john");

		// Simpler way of grabbing variable from table
		lua_getglobal(L, "x");
		lua_getfield(L, -1, "john");
		const char* john = lua_tostring(L, -1);
		printf("John is: %s\n", john);


		lua_close(L);
	}

	printf("---- metatables and metamethod(s) ----\n");
	{
		// Meta-table: A table that allows you to add "special fields". You can attach a metatable onto other tables or
		// user datum to change behavior.

		struct Vec
		{
			static int CreateVector2D(lua_State* L)
			{
				// Create new table
				lua_newtable(L);
				lua_pushstring(L, "x");		// key
				lua_pushnumber(L, 0);		// value
				lua_settable(L, -3);

				lua_pushstring(L, "y");		// key
				lua_pushnumber(L, 0);		// value
				lua_settable(L, -3);

				// Assign metatable
				luaL_getmetatable(L, "VectorMetaTable");
				lua_setmetatable(L, -2);

				return 1;
			}

			// Meta method for add operation
			static int __add(lua_State* L)
			{
				assert(lua_istable(L, -2));  // left table
				assert(lua_istable(L, -1));	 // right table

				lua_pushstring(L, "x");
				lua_gettable(L, -3);
				lua_Number xleft = lua_tonumber(L, -1);
				lua_pop(L, 1);

				lua_pushstring(L, "x");
				lua_gettable(L, -2);
				lua_Number xright = lua_tonumber(L, -1);
				lua_pop(L, 1);

				lua_Number xAdded = xleft + xright;
				printf("__add was called: %d\n", (int)xAdded);

				Vec::CreateVector2D(L);
				lua_pushstring(L, "x");
				lua_pushnumber(L, xAdded);
				lua_rawset(L, -3);			// Equivalent  of lua_setTable, except that it won't invoke the meta methods. Prevents infinite loops

				return 1;
			}
		};

		const char* LUA_FILE = R"(
		v1 = CreateVector() -- table
		v2 = CreateVector() -- table
		v1.x = 11
		v2.x = 42
		v3 = v1 + v2
		result = v3.x
		)";

		lua_State* L = luaL_newstate();
		lua_pushcfunction(L, Vec::CreateVector2D);
		lua_setglobal(L, "CreateVector");

		// Create new metatable
		luaL_newmetatable(L, "VectorMetaTable");
		lua_pushstring(L, "__add");				// Push special add operator to stack
		lua_pushcfunction(L, Vec::__add);
		lua_settable(L, -3);

		int err = luaL_dostring(L, LUA_FILE);
		if (err != LUA_OK)
		{
			printf("Error: %s\n", lua_tostring(L, -1));
		}

		lua_getglobal(L, "result");
		lua_Number result = lua_tonumber(L, -1);
		printf("Result = %d\n", (int) result);


		lua_close(L);
	}

	printf("---- C++ Constructors and destructors ----\n");
	{
		static int numberOfSpritesExisting = 0;

		// Our own type
		struct Sprite
		{
			int x;
			int y;

			Sprite() : x(0), y(0)
			{
				numberOfSpritesExisting++;
			}

			~Sprite()
			{
				numberOfSpritesExisting--;
			}

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

		auto CreateSprite = [](lua_State* L) -> int
		{
			// Get lua to create a new sprite (and manage the memory!)
			void* pointerToSprite = lua_newuserdata(L, sizeof(Sprite));

			// Placement new (calls Sprite's constructor without allocating memory)
			new (pointerToSprite) Sprite();

			// (Only one instance of this sprite metatable)
			luaL_getmetatable(L, "SpriteMetaTable");
			assert(lua_istable(L, -1));
			lua_setmetatable(L, -2);		// Set metatable to user datum Sprite. This command pops metatable off stack

			return 1;
		};

		auto DestroySprite = [](lua_State* L) -> int
		{
			// Call deconstructor
			Sprite* sprite = (Sprite*)lua_touserdata(L, -1);
			sprite->~Sprite();
			return 0;
		};

		auto MoveSprite = [](lua_State* L) -> int
		{
			Sprite* sprite = (Sprite*)lua_touserdata(L, -3);
			lua_Number velX = lua_tonumber(L, -2);
			lua_Number velY = lua_tonumber(L, -1);
			sprite->Move((int)velX, (int)velY);
			return 0;
		};

		auto DrawSprite = [](lua_State* L) -> int
		{
			Sprite* sprite = (Sprite*)lua_touserdata(L, -1);
			sprite->Draw();
			return 0;
		};

		const char* LUA_FILE = R"(
		sprite = CreateSprite()
		MoveSprite(sprite, 5, 7)
		DrawSprite(sprite)
		MoveSprite(sprite, 1, 2)
		DrawSprite(sprite)
		)";

		lua_State* L = luaL_newstate();

		// We can attach a meta-tables to our Sprite to call the deconstruction upon GC!
		// We will only need 1 meta-table for all Sprites
		luaL_newmetatable(L, "SpriteMetaTable");
		lua_pushstring(L, "__gc");				// Only available for user datum
		lua_pushcfunction(L, DestroySprite);
		lua_settable(L, -3);					// Set meta table

		lua_pushcfunction(L, CreateSprite);
		lua_setglobal(L, "CreateSprite");
		lua_pushcfunction(L, MoveSprite);
		lua_setglobal(L, "MoveSprite");
		lua_pushcfunction(L, DrawSprite);
		lua_setglobal(L, "DrawSprite");

		int err = luaL_dostring(L, LUA_FILE);
		if (err == LUA_OK)
		{
			printf("Ok.");
		}
		else
		{
			printf("Error: %s\n", lua_tostring(L, -1));
		}

		lua_close(L);

		assert(numberOfSpritesExisting == 0);


	}

	// : -> syntaxic sugar
	printf("---- Object Oriented access ----\n");
	{
		static int numberOfSpritesExisting = 0;

		// Our own type
		struct Sprite
		{
			int x;
			int y;

			Sprite() : x(0), y(0)
			{
				numberOfSpritesExisting++;
			}

			~Sprite()
			{
				numberOfSpritesExisting--;
			}

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

		auto CreateSprite = [](lua_State* L) -> int
		{
			// Get lua to create a new sprite (and manage the memory!)
			void* pointerToSprite = lua_newuserdata(L, sizeof(Sprite));

			// Placement new (calls Sprite's constructor without allocating memory)
			new (pointerToSprite) Sprite();

			// (Only one instance of this sprite metatable)
			luaL_getmetatable(L, "SpriteMetaTable");
			assert(lua_istable(L, -1));
			lua_setmetatable(L, -2);		// Set metatable to user datum Sprite. This command pops metatable off stack

			return 1;
		};

		auto DestroySprite = [](lua_State* L) -> int
		{
			// Call deconstruction
			Sprite* sprite = (Sprite*)lua_touserdata(L, -1);
			sprite->~Sprite();
			return 0;
		};

		auto MoveSprite = [](lua_State* L) -> int
		{
			Sprite* sprite = (Sprite*)lua_touserdata(L, -3);
			lua_Number velX = lua_tonumber(L, -2);
			lua_Number velY = lua_tonumber(L, -1);
			sprite->Move((int)velX, (int)velY);
			return 0;
		};

		auto DrawSprite = [](lua_State* L) -> int
		{
			Sprite* sprite = (Sprite*)lua_touserdata(L, -1);
			sprite->Draw();
			return 0;
		};

		const char* LUA_FILE = R"(
		sprite = Sprite.new()
		sprite:Move(5, 7)			-- Syntax sugar for: Sprite.Move(sprite, 5, 7)
		sprite:Draw()
		sprite:Move(1, 2)
		sprite:Draw()

		-- sprite -> sprite is a userdatum
		--		has a metatable called SpriteMetaTable
		--			dont have Move(), use the __index metamethod
		--				__index metamethod is a table which is Sprite
		--				Sprite has a field called Move(), invoke that
		--				Move() is a c function
		--				Invoke, pass the userdatum as the first parameter.
		)";

		lua_State* L = luaL_newstate();

		// Create new Sprite table
		// Reduce # of globals / name conflicts
		lua_newtable(L);
		int spriteTableIdx = lua_gettop(L);			// Get top of stack
		lua_pushvalue(L, spriteTableIdx);			// Push table onto the stack again
		lua_setglobal(L, "Sprite");					// set table name. Will pop top table off, leaving one.

		lua_pushcfunction(L, CreateSprite);
		lua_setfield(L, -2, "new");
		lua_pushcfunction(L, MoveSprite);
		lua_setfield(L, -2, "Move");
		lua_pushcfunction(L, DrawSprite);
		lua_setfield(L, -2, "Draw");

		// We can attach a meta-tables to our Sprite to call the deconstruction upon GC!
		// We will only need 1 meta-table for all Sprites
		luaL_newmetatable(L, "SpriteMetaTable");
		lua_pushstring(L, "__gc");				// Only available for user datum
		lua_pushcfunction(L, DestroySprite);
		lua_settable(L, -3);					// Set meta table

		// Add meta method for handling ":" sugar.
		// __index is invoked when you try and do something and the operation doesn't exist
		lua_pushstring(L, "__index");				
		lua_pushvalue(L, spriteTableIdx);			// Index meta-method points to sprite table
		lua_settable(L, -3);


		int err = luaL_dostring(L, LUA_FILE);
		if (err == LUA_OK)
		{
			printf("Ok.");
		}
		else
		{
			printf("Error: %s\n", lua_tostring(L, -1));
		}

		lua_close(L);

		assert(numberOfSpritesExisting == 0);


	}


	printf("---- Reading Object Properties ----\n");
	{
		static int numberOfSpritesExisting = 0;

		// Our own type
		struct Sprite
		{
			int x; // bad encaspulation
			int y;

			Sprite() : x(0), y(0)
			{
				numberOfSpritesExisting++;
			}

			~Sprite()
			{
				numberOfSpritesExisting--;
			}

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

		auto CreateSprite = [](lua_State* L) -> int
		{
			// Get lua to create a new sprite (and manage the memory!)
			void* pointerToSprite = lua_newuserdata(L, sizeof(Sprite));

			// Placement new (calls Sprite's constructor without allocating memory)
			new (pointerToSprite) Sprite();

			// (Only one instance of this sprite metatable)
			luaL_getmetatable(L, "SpriteMetaTable");
			assert(lua_istable(L, -1));
			lua_setmetatable(L, -2);		// Set metatable to user datum Sprite. This command pops metatable off stack

			return 1;
		};

		auto DestroySprite = [](lua_State* L) -> int
		{
			// Call deconstruction
			Sprite* sprite = (Sprite*)lua_touserdata(L, -1);
			sprite->~Sprite();
			return 0;
		};

		auto MoveSprite = [](lua_State* L) -> int
		{
			Sprite* sprite = (Sprite*)lua_touserdata(L, -3);
			lua_Number velX = lua_tonumber(L, -2);
			lua_Number velY = lua_tonumber(L, -1);
			sprite->Move((int)velX, (int)velY);
			return 0;
		};

		auto DrawSprite = [](lua_State* L) -> int
		{
			Sprite* sprite = (Sprite*)lua_touserdata(L, -1);
			sprite->Draw();
			return 0;
		};

		auto SpriteIndex = [](lua_State* L) -> int
		{
			assert(lua_isuserdata(L, -2));
			assert(lua_isstring(L, -1));		// Index we are accessing "x"

			Sprite* sprite = (Sprite*)lua_touserdata(L, -2);
			const char* index = lua_tostring(L, -1);
			
			// Properties
			if (strcmp(index, "x") == 0)
			{
				lua_pushnumber(L, sprite->x);
				return 1;
			}
			else if (strcmp(index, "y") == 0)
			{
				lua_pushnumber(L, sprite->y);
				return 1;
			}
			else
			{
				lua_getglobal(L, "Sprite");
				lua_pushstring(L, index);
				lua_rawget(L, -2);				// Get method
				return 1;
			}

		};

		const char* LUA_FILE = R"(
		sprite = Sprite.new()
		sprite:Move(5, 7)			-- Syntax sugar for: Sprite.Move(sprite, 5, 7)
		sprite:Draw()
		temp_x = sprite.x
		)";

		lua_State* L = luaL_newstate();

		// Create new Sprite table
		// Reduce # of globals / name conflicts
		lua_newtable(L);
		int spriteTableIdx = lua_gettop(L);			// Get top of stack
		lua_pushvalue(L, spriteTableIdx);			// Push table onto the stack again
		lua_setglobal(L, "Sprite");					// set table name. Will pop top table off, leaving one.

		lua_pushcfunction(L, CreateSprite);
		lua_setfield(L, -2, "new");
		lua_pushcfunction(L, MoveSprite);
		lua_setfield(L, -2, "Move");
		lua_pushcfunction(L, DrawSprite);
		lua_setfield(L, -2, "Draw");


		// We can attach a meta-tables to our Sprite to call the deconstruction upon GC!
		// We will only need 1 meta-table for all Sprites
		luaL_newmetatable(L, "SpriteMetaTable");
		lua_pushstring(L, "__gc");				// Only available for user datum
		lua_pushcfunction(L, DestroySprite);
		lua_settable(L, -3);					// Set meta table

		// Add meta method for handling ":" sugar.
		// __index is invoked when you try and do something and the operation doesn't exist
		lua_pushstring(L, "__index");
		lua_pushcfunction(L, SpriteIndex);
		lua_settable(L, -3);


		int err = luaL_dostring(L, LUA_FILE);
		if (err == LUA_OK)
		{
			printf("Ok.");
		}
		else
		{
			printf("Error: %s\n", lua_tostring(L, -1));
		}

		lua_getglobal(L, "temp_x");
		lua_Number temp_x = lua_tonumber(L, -1);
		assert(temp_x == 5);

		lua_close(L);

		assert(numberOfSpritesExisting == 0);


	}

	printf("---- Writing Object Properties + User values ----\n");
	{
		static int numberOfSpritesExisting = 0;

		// Our own type
		struct Sprite
		{
			int x; // bad encaspulation
			int y;

			Sprite() : x(0), y(0)
			{
				numberOfSpritesExisting++;
			}

			~Sprite()
			{
				numberOfSpritesExisting--;
			}

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

		auto CreateSprite = [](lua_State* L) -> int
		{
			// Get lua to create a new sprite (and manage the memory!)
			void* pointerToSprite = lua_newuserdata(L, sizeof(Sprite));

			// Placement new (calls Sprite's constructor without allocating memory)
			new (pointerToSprite) Sprite();

			// (Only one instance of this sprite metatable)
			luaL_getmetatable(L, "SpriteMetaTable");
			assert(lua_istable(L, -1));
			lua_setmetatable(L, -2);		// Set metatable to user datum Sprite. This command pops metatable off stack

			// USER TABLE:
			// Stores any additional value to the native object that we didn't have at compile time
			lua_newtable(L);
			lua_setuservalue(L, 1);		// Associate this userdatum with the non-native table

			return 1;
		};

		auto DestroySprite = [](lua_State* L) -> int
		{
			// Call deconstruction
			Sprite* sprite = (Sprite*)lua_touserdata(L, -1);
			sprite->~Sprite();
			return 0;
		};

		auto MoveSprite = [](lua_State* L) -> int
		{
			Sprite* sprite = (Sprite*)lua_touserdata(L, -3);
			lua_Number velX = lua_tonumber(L, -2);
			lua_Number velY = lua_tonumber(L, -1);
			sprite->Move((int)velX, (int)velY);
			return 0;
		};

		auto DrawSprite = [](lua_State* L) -> int
		{
			Sprite* sprite = (Sprite*)lua_touserdata(L, -1);
			sprite->Draw();
			return 0;
		};

		auto SpriteIndex = [](lua_State* L) -> int
		{
			assert(lua_isuserdata(L, -2));	//1
			assert(lua_isstring(L, -1));	//2

			Sprite* sprite = (Sprite*)lua_touserdata(L, -2);
			const char* index = lua_tostring(L, -1);
			if (strcmp(index, "x") == 0)
			{
				lua_pushnumber(L, sprite->x);
				return 1;
			}
			else if (strcmp(index, "y") == 0)
			{
				lua_pushnumber(L, sprite->y);
				return 1;
			}
			else
			{
				lua_getuservalue(L, 1);
				lua_pushvalue(L, 2);
				lua_gettable(L, -2);
				if (lua_isnil(L, -1))
				{
					lua_getglobal(L, "Sprite");
					lua_pushstring(L, index);
					lua_rawget(L, -2);
				}
				return 1;
			}
		};
		auto SpriteNewIndex = [](lua_State* L) -> int
		{
			assert(lua_isuserdata(L, -3)); // 1
			assert(lua_isstring(L, -2));   // 2 - Index we are accessing
										   // 3 - Value we want to set

			Sprite* sprite = (Sprite*)lua_touserdata(L, -3);
			const char* index = lua_tostring(L, -2);

			// Properties
			if (strcmp(index, "x") == 0)
			{
				sprite->x = (int) lua_tonumber(L, -1);
			}
			else if (strcmp(index, "y") == 0)
			{
				sprite->y = (int) lua_tonumber(L, -1);
			}
			else
			{
				// Get user value table associated with this user datum
				lua_getuservalue(L, 1);	// 1 - table
				lua_pushvalue(L, 2);	// 2 - index
				lua_pushvalue(L, 3);	// 3 - value
				lua_settable(L, -3);
			}

			return 0;
		};

		const char* LUA_FILE = R"(
		sprite = Sprite.new()
		sprite:Move( 6, 7 )		-- Sprite.Move( sprite, 6, 7 )
		-- sprite:Draw()
		sprite.y = 10
		sprite.zzz = 99
		sprite.x = sprite.zzz
		temp_x = sprite.x
		-- sprite:Draw()
		)";

        
        // 20KB of memory on stack
        constexpr int POOL_SIZE = 1024 * 20;
        char memory[POOL_SIZE];
        
        // Heapless allocation
        ArenaAllocator pool(memory, &memory[POOL_SIZE - 1]);
        
        for (int i = 0; i < 50000; i++)
        {
            pool.Reset();
            lua_State* L = lua_newstate(ArenaAllocator::l_alloc, &pool);
            //lua_State* L = luaL_newstate();

            // Create new Sprite table
            // Reduce # of globals / name conflicts
            lua_newtable(L);
            int spriteTableIdx = lua_gettop(L);			// Get top of stack
            lua_pushvalue(L, spriteTableIdx);			// Push table onto the stack again
            lua_setglobal(L, "Sprite");					// set table name. Will pop top table off, leaving one.

            lua_pushcfunction(L, CreateSprite);
            lua_setfield(L, -2, "new");
            lua_pushcfunction(L, MoveSprite);
            lua_setfield(L, -2, "Move");
            lua_pushcfunction(L, DrawSprite);
            lua_setfield(L, -2, "Draw");


            // We can attach a meta-tables to our Sprite to call the deconstruction upon GC!
            // We will only need 1 meta-table for all Sprites
            luaL_newmetatable(L, "SpriteMetaTable");
            lua_pushstring(L, "__gc");				// Only available for user datum
            lua_pushcfunction(L, DestroySprite);
            lua_settable(L, -3);					// Set meta table

            // Add meta method for handling ":" sugar.
            // __index is invoked when you try and do something and the operation doesn't exist
            lua_pushstring(L, "__index");
            lua_pushcfunction(L, SpriteIndex);
            lua_settable(L, -3);

            // __newindex -> for writing
            lua_pushstring(L, "__newindex");
            lua_pushcfunction(L, SpriteNewIndex);
            lua_settable(L, -3);

            int err = luaL_dostring(L, LUA_FILE);
            if (err == LUA_OK)
            {
                //printf("Ok.\n");
            }
            else
            {
                printf("Error: %s\n", lua_tostring(L, -1));
            }

            lua_close(L);
        }

		assert(numberOfSpritesExisting == 0);


	}

    printf("---- Lua memory allocator ----\n");
    {
        
        // 20KB of memory on stack
        constexpr int POOL_SIZE = 1024 * 10;
        char memory[POOL_SIZE];
        
        // Heapless allocation
        ArenaAllocator pool(memory, &memory[POOL_SIZE - 1]);
        
        // *ud: called for every allocation (good for debug allocation tracking ect.)
        lua_State* L = lua_newstate(ArenaAllocator::l_alloc, &pool);
        
        
        lua_close(L);
        
    }
    
    printf("---- Lua aligned memory allocator ----\n");
    {
        
        // 20KB of memory on stack
        constexpr int POOL_SIZE = 1024 * 10;
        char memory[POOL_SIZE];
        
        // Heapless allocation
        ArenaAllocator pool(memory, &memory[POOL_SIZE - 1]);
        
        // *ud: called for every allocation (good for debug allocation tracking ect.)
        lua_State* L = lua_newstate(ArenaAllocator::l_alloc, &pool);
        
        // Enforces this struct has to have a 8-bit allignment
        struct alignas(8) Thing
        {
            float x;
            float z;
        };
        
        Thing* t = (Thing*) lua_newuserdata(L, sizeof(Thing));
        assert((uintptr_t) t % alignof(Thing) == 0); // Checks if we are aligned
        
        lua_close(L);
        
    }
    
	return 0;
}
