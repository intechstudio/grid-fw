#ifndef GRID_UI_LCD_H
#define GRID_UI_LCD_H

#include <stdint.h>

#include "grid_protocol.h"
#include "grid_ui.h"

void grid_ui_element_lcd_init(struct grid_ui_element* ele, template_init_t initializer);

void grid_ui_element_lcd_template_parameter_init(struct grid_ui_template_buffer* buf);

#define GRID_LUA_FNC_L_DRAW_SWAP_short "ldsw"
#define GRID_LUA_FNC_L_DRAW_SWAP_human "draw_swap"
#define GRID_LUA_FNC_L_DRAW_SWAP_usage "lcd:draw_swap() Updates the screen with the contents of the background buffer."

#define GRID_LUA_FNC_L_DRAW_PIXEL_short "ldpx"
#define GRID_LUA_FNC_L_DRAW_PIXEL_human "draw_pixel"
#define GRID_LUA_FNC_L_DRAW_PIXEL_usage "lcd:draw_pixel(x, y, {r, g, b}) Draws a pixel at (x, y) with the specified 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_LINE_short "ldl"
#define GRID_LUA_FNC_L_DRAW_LINE_human "draw_line"
#define GRID_LUA_FNC_L_DRAW_LINE_usage "lcd:draw_line(x1, y1, x2, y2, {r, g, b}) Draws a line between (x1, y1) and (x2, y2) points with the specified 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_RECTANGLE_short "ldr"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_human "draw_rectangle"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_usage "lcd:draw_rectangle(x1, y1, x2, y2, {r, g, b}) Draws a rectangle between (x1, y1) and (x2, y2) points with the specified 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_RECTANGLE_FILLED_short "ldrf"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_FILLED_human "draw_rectangle_filled"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_FILLED_usage                                                                                                                                                     \
  "lcd:draw_rectangle_filled(x1, y1, x2, y2, {r, g, b}) Draws a filled rectangle between (x1, y1) and (x2, y2) points with the specified 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_short "ldrr"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_human "draw_rectangle_rounded"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_usage                                                                                                                                                    \
  "lcd:draw_rectangle_rounded(x1, y1, x2, y2, radius, {r, g, b}) Draws a rounded rectangle between (x1, y1) and (x2, y2) points using pixel based radius with the specified 8-bit color "              \
  "channels."

#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_FILLED_short "ldrrf"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_FILLED_human "draw_rectangle_rounded_filled"
#define GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_FILLED_usage                                                                                                                                             \
  "lcd:draw_rectangle_rounded_filled(x1, y1, x2, y2, radius, {r, g, b}) Draws a filled rounded rectangle between (x1, y1) and (x2, y2) points using pixel based radius with the specified 8-bit "      \
  "color channels."

#define GRID_LUA_FNC_L_DRAW_POLYGON_short "ldpo"
#define GRID_LUA_FNC_L_DRAW_POLYGON_human "draw_polygon"
#define GRID_LUA_FNC_L_DRAW_POLYGON_usage "lcd:draw_polygon({x1, x2, x3 ...}, {y1, y2, y3 ...}, {r, g, b}) Draws a polygon using the x and y coordinate pairs with the specified 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_POLYGON_FILLED_short "ldpof"
#define GRID_LUA_FNC_L_DRAW_POLYGON_FILLED_human "draw_polygon_filled"
#define GRID_LUA_FNC_L_DRAW_POLYGON_FILLED_usage                                                                                                                                                       \
  "lcd:draw_polygon_filled({x1, x2, x3 ...}, {y1, y2, y3 ...}, {r, g, b}) Draws a filled polygon using the x and y coordinate pairs with the specified 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_TEXT_short "ldt"
#define GRID_LUA_FNC_L_DRAW_TEXT_human "draw_text"
#define GRID_LUA_FNC_L_DRAW_TEXT_usage "lcd:draw_text('text', x, y, size, {r, g, b}) Draws the specified text at (x, y) with the specified font size and 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_FASTTEXT_short "ldft"
#define GRID_LUA_FNC_L_DRAW_FASTTEXT_human "draw_text_fast"
#define GRID_LUA_FNC_L_DRAW_FASTTEXT_usage "lcd:draw_text_fast('text', x, y, size, {r, g, b}) Draws the specified text at (x, y) with the specified font size and 8-bit color channels."

#define GRID_LUA_FNC_L_DRAW_AREA_FILLED_short "ldaf"
#define GRID_LUA_FNC_L_DRAW_AREA_FILLED_human "draw_area_filled"
#define GRID_LUA_FNC_L_DRAW_AREA_FILLED_usage "lcd:draw_area_filled(x1, y1, x2, y2, {r, g, b}) Fills an area with the specified color, without alpha blending."

#define GRID_LUA_FNC_L_DRAW_DEMO_short "ldd"
#define GRID_LUA_FNC_L_DRAW_DEMO_human "draw_demo"
#define GRID_LUA_FNC_L_DRAW_DEMO_usage "lcd:draw_demo(n) Draws the n-th iteration of the demo."

// clang-format off

#define GRID_LUA_FNC_ASSIGN_META_DRAW(key, val) \
  key " = function (self, ...) " \
  "local screen_idx = self:" GRID_LUA_FNC_L_SCREEN_INDEX_short "(); " \
  val "(screen_idx, ...) end"

// clang-format on

// clang-format off

#define GRID_LUA_L_TYPE "Lcd"

extern const luaL_Reg GRID_LUA_L_INDEX_META[];

#define GRID_LUA_L_META_init \
  GRID_LUA_L_TYPE " = { __index = {" \
  \
  "type = 'lcd', "\
  \
  "post_init_cb = function (self) " \
  "self:"GRID_LUA_FNC_A_INIT_short"() " \
  "end," \
  \
  GRID_LUA_FNC_ASSIGN_META_PAR1_RET("gen", GRID_LUA_FNC_G_ELEMENTNAME_short) "," \
  \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_SWAP_short, GRID_LUA_FNC_G_GUI_DRAW_SWAP_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_PIXEL_short, GRID_LUA_FNC_G_GUI_DRAW_PIXEL_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_LINE_short, GRID_LUA_FNC_G_GUI_DRAW_LINE_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_RECTANGLE_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_RECTANGLE_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_FILLED_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_RECTANGLE_ROUNDED_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_RECTANGLE_ROUNDED_FILLED_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_POLYGON_short, GRID_LUA_FNC_G_GUI_DRAW_POLYGON_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_POLYGON_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_POLYGON_FILLED_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_TEXT_short, GRID_LUA_FNC_G_GUI_DRAW_TEXT_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_FASTTEXT_short, GRID_LUA_FNC_G_GUI_DRAW_FASTTEXT_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_AREA_FILLED_short, GRID_LUA_FNC_G_GUI_DRAW_AREA_FILLED_short) "," \
  GRID_LUA_FNC_ASSIGN_META_DRAW(GRID_LUA_FNC_L_DRAW_DEMO_short, GRID_LUA_FNC_G_GUI_DRAW_DEMO_short) "," \
  \
  "}}"

#define GRID_ACTIONSTRING_LCD_INIT \
  "<?lua --[[@cb]] glsb(255)pi,s,c,self.f,self.v,self.id=math.pi,64,{{0,0,0},{255,255,255},{glr(),glg(),glb()}},1,{27,0,100},'VSN1'd={[1]='Linear',[2]='Encoder',[3]='Button',[7]='Endless'}xc,yc,p=160,120,s*5/8;self.eventrx_cb=function(self,hdr,e,v,n)self.v=v;if#n==0 then n=d[e[3]]..e[2]end;self.id=string.sub(n,1,(self:lsw()/(s/2)-1)//1)self.f=1 end;self:ldaf(0,0,319,239,c[1])self:ldrr(3,3,317,237,10,c[2]) ?>"

#define GRID_ACTIONSTRING_LCD_DRAW \
  "<?lua --[[@cb]] if self.f>0 then self.f=self.f-1;local a,xo=gmaps(self.v[1],self.v[2],self.v[3],0.1,1),#tostring(self.v[1])/2*s/2-#tostring(self.v[1])-s//32;self:ldaf(10,10,310,230,c[1])self:ldrr(xc-p//1-1,yc-p//1-1,xc+p//1+1,yc+p//1+1,s,c[2])self:ldrrf(xc-p*a//1,yc-p*a//1,xc+p*a//1,yc+p*a//1,s,c[3])self:ldft(self.v[1],xc-xo,yc+s,s/2,c[2])local xn=(#self.id*(s/2))/2-s//32;self:ldft(self.id,xc-xn,yc-1.5*s,s/2,c[2])self:ldsw()end ?>"

// clang-format on

#endif /* GRID_UI_LCD_H */
