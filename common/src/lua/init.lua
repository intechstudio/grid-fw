init_simple_color()
init_simple_midi()
init_auto_value()

local page = gpn() == 0 and 3 or gpn() - 1
for i = 0, #ele do
  local eve = getmetatable(ele[i]).eve
  for j = 1, #eve do
    local path = string.format("%02d/%02d/%02d.lua", page, i, eve[j])
    gas(i, eve[j], os.stat(path) and path or "")
  end
  collectgarbage("collect")
end

ele[#ele]:post_init_cb()
for i = 0, #ele - 1 do
  collectgarbage("collect")
  ele[i]:post_init_cb()
end
