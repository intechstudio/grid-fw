#pragma once

#ifndef GRID_LUA_API_H_INCLUDED
#define GRID_LUA_API_H_INCLUDED

// only for uint definitions
#include  <stdint.h>
// only for malloc
#include  <stdlib.h>



#include "lua-5.4.3/src/lua.h"
#include "lua-5.4.3/src/lualib.h"
#include "lua-5.4.3/src/lauxlib.h"


// GRID LOOKUP TABLE
#define GRID_LUA_GLUT_source \
"function glut (a, ...)      \
 local t = table.pack(...)   \
 for i = 1, t.n//2*2 do      \
  if i%2 == 1 then           \
   if t[i] == a then         \
    return t[i+1]            \
   end                       \
  end                        \
 end                         \
 return nil                  \
end"

// GRID LIMIT
#define GRID_LUA_GLIM_source  \
"function glim (a, min, max)  \
 if a>max then return max end \
 if a<min then return min end \
 return a \
end"

// GRID ELEMENT NAME
#define GRID_LUA_GEN_source  \
"function gen (a, b)  \
 if b==nil then \
  if ele[a].sn==nil then\
   return ''\
  else\
   return ele[a].sn \
  end\
 else \
  ele[a].sn=b \
  gens(a,b) \
 end \
end"


#endif /* GRID_LUA_API_H_INCLUDED */