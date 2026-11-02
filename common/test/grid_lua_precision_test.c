#include "test.h"

#include <stdbool.h>
#include <stdio.h>

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

// Evaluate a Lua expression and return its result as a Lua number.
// On error, returns NAN so the caller's numeric assertion fails.
static lua_Number lua_eval_number(lua_State* L, const char* expr) {

  char src[256];
  snprintf(src, sizeof(src), "return (%s)", expr);

  if (luaL_dostring(L, src) != LUA_OK) {
    lua_pop(L, 1);
    return (lua_Number)(0.0 / 0.0);
  }

  int isnum = 0;
  lua_Number val = lua_tonumberx(L, -1, &isnum);
  lua_pop(L, 1);

  return isnum ? val : (lua_Number)(0.0 / 0.0);
}

// Evaluate a Lua boolean expression.
static bool lua_eval_bool(lua_State* L, const char* expr) {

  char src[256];
  snprintf(src, sizeof(src), "return (%s)", expr);

  if (luaL_dostring(L, src) != LUA_OK) {
    lua_pop(L, 1);
    return false;
  }

  bool val = lua_toboolean(L, -1);
  lua_pop(L, 1);

  return val;
}

// Boot a Lua VM and verify its floating-point arithmetic carries full
// IEEE-754 double precision (53-bit mantissa), not single-precision float.
TEST_DECL(grid_lua_precision) {

  lua_State* L = luaL_newstate();
  TEST_ASSERT(L);

  luaL_openlibs(L);

  // NOTE: this test asserts that Lua numbers are single-precision float32.
  // The firmware builds Lua with LUA_FLOAT_DOUBLE, so every assertion below
  // is EXPECTED TO FAIL — it exists to demonstrate the failing (red) case.

  // In float32 the mantissa runs out at 2^24, so 2^24 + 1 = 16777217 rounds
  // back down to 16777216. In double it stays exact, so this is false.
  TEST_ASSERT(lua_eval_bool(L, "16777216.0 + 1.0 == 16777216.0"));

  // The classic float tell: 0.1 + 0.2 == 0.3 holds only when both round to
  // the same float32; in double, 0.1 + 0.2 = 0.30000000000000004 != 0.3.
  TEST_ASSERT(lua_eval_bool(L, "0.1 + 0.2 == 0.3"));

  // float32 epsilon is ~1.19e-7, so adding 1e-8 to 1.0 is lost to rounding.
  // A double keeps the perturbation, making this comparison false.
  TEST_ASSERT(lua_eval_bool(L, "1.0 + 1e-8 == 1.0"));

  // Read a value back across the C boundary: 16777217 must have been
  // truncated to 16777216 at parse time if the VM is single precision.
  lua_Number big = lua_eval_number(L, "16777217.0");
  TEST_ASSERT(big == 16777216.0);

  lua_close(L);

  return TEST_SUCCESS;
}
