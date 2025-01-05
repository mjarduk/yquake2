#include "header/local.h"
#include <lauxlib.h>
#include <lua.h>
#include <stdio.h>
#include <unistd.h>

lua_State *lua_state;

/* Callbacks */

int command_callback = 0;
int join_callback = 0;

/* Globals */

static int LuaPrint(lua_State *L);
static int LuaRegisterCallback(lua_State *L);

/* Util functions */

void AddLuaGlobal(const char* name, lua_CFunction func) {
  lua_newtable(lua_state);
  lua_pushcfunction(lua_state, func);
  lua_setglobal(lua_state, name);
  lua_settop(lua_state, 0);
}

void CallLuaCallback(int callback_reference, callback_info_t info) {
  /* return if the callback aint registered */
  if (callback_reference == 0)
	return;

  int nargs = 0;

  lua_rawgeti(lua_state, LUA_REGISTRYINDEX, callback_reference);

  switch (info.type) {
	case CALL_COMMAND: {
	  lua_pushstring(lua_state, ((edict_t*)info.data)->client->pers.netname);
	  nargs++;
	} break;
  }

  if (lua_pcall(lua_state, nargs, 0, 0 ) != 0) {
	gi.cprintf(NULL, PRINT_ALERT, "Failed to run callback: %s\n", lua_tostring(lua_state, -1));
	return;
  }
}

/* Exported functions */

void InitLua(void)
{
  lua_state = luaL_newstate();

  InitLuaGlobals();

  luaL_loadfile(lua_state, "baseq2/main.lua");
  if (lua_pcall(lua_state, 0, 0, 0) != 0) {
	gi.cprintf(NULL, PRINT_ALERT, "Failed to run script: %s\n", lua_tostring(lua_state, -1));
  }
}

void InitLuaGlobals(void) {
  luaL_openlibs(lua_state);
  AddLuaGlobal("print", LuaPrint);
  AddLuaGlobal("callback", LuaRegisterCallback); 
}

void ReloadLua(void) {

}

void StopLua(void)
{
  lua_close(lua_state);
}

/* Defining them globals */

static int LuaPrint(lua_State* L)
{
  int nargs = lua_gettop(L);
  edict_t* other;

  for (int i = 0; i <= nargs + 1; i++) {
	const char* text;
	int newline = false;

	if (i == nargs + 1) {
	  text = "\n";
	  newline = true;
	} else if (i == 0) {
	  text = "lua:";
	} else {
	  text = lua_tostring(L, i);
	}

	gi.cprintf(NULL, PRINT_CHAT, (newline ? "%s" : "%s "), text);

	for (int j = 1; j <= game.maxclients; j++)
	{
	  other = &g_edicts[j];

	  if (!other->inuse)
	  {
		continue;
	  }

	  if (!other->client)
	  {
		continue;
	  }

	  gi.cprintf(other, PRINT_CHAT, (newline ? "%s" : "%s "), text);
	}
  }

  return 0;
}

static int LuaRegisterCallback(lua_State *L) {
  if (!lua_isstring(L, 1)) {
	luaL_error(L, "Invalid 1st argument. Expected string, function");
  }

  if (!lua_isfunction(L, 2)) {
	luaL_error(L, "Invalid 2nd argument. Expected string, function");
  }

  const char* type = lua_tostring(L, 1);

  if (!strcmp(type, "command"))
	command_callback = luaL_ref(L, LUA_REGISTRYINDEX);
  else if (!strcmp(type, "join"))
	join_callback = luaL_ref(L, LUA_REGISTRYINDEX);
  else {
	luaL_error(L, "Unknown callback %s", type);
  }

  // gi.cprintf(NULL, PRINT_DEVELOPER, "Lua registered callback %s\n", type);

  return 0;
}
