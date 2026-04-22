init_simple_color()
init_simple_midi()
init_auto_value()

local page = gpn() == 0 and 3 or gpn() - 1
for i = 0, #ele do
  local eve = getmetatable(ele[i]).eve

  local custom = {}
  for j = 1, #eve do
    custom[j] = false
  end

  local path = string.format("%02d/%02d", page, i)
  if os.stat(path) then
    for _, v in ipairs(dirent.list(path)) do
      local caps = string.match(v[1], "(%d%d)%.lua")
      if v[2] == 1 and caps and tonumber(caps) then
        local idx = tonumber(caps)
        for j = 1, #eve do
          if eve[j] == idx then
            gas(i, eve[j], path .. "/" .. v[1])
            collectgarbage("collect")
            custom[j] = true
          end
        end
      end
    end
  end

  for j = 1, #eve do
    if custom[j] == false then
      gas(i, eve[j], "")
      collectgarbage("collect")
    end
  end
end

ele[#ele]:post_init_cb()
for i = 0, #ele - 1 do
  collectgarbage("collect")
  ele[i]:post_init_cb()
end
