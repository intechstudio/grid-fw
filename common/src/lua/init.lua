_decoded_order = {}
_decoded_midi = {}
_decoded_sysex = {}
_decoded_eview = {}
_decoded_rtm = {}

rx_type = { MIDIVOICE = 0, MIDISYSEX = 1, MIDIRTM = 2, EVENTVIEW = 3 }
rx_feat = { FORWARD = 0x01, HANDLE_EXTERNAL = 0x02, HANDLE_INTERNAL = 0x04 }

grxm(rx_type.MIDIVOICE, rx_feat.FORWARD | rx_feat.HANDLE_EXTERNAL)
grxm(rx_type.MIDISYSEX, rx_feat.FORWARD | rx_feat.HANDLE_EXTERNAL)
grxm(rx_type.MIDIRTM, 0)
grxm(rx_type.EVENTVIEW, 0)
if ghaslcd() then
  grxm(rx_type.EVENTVIEW, rx_feat.HANDLE_EXTERNAL | rx_feat.HANDLE_INTERNAL)
end

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

  local path = string.format("%02x/%02x", page, i)
  if os.stat(path) then
    for _, v in ipairs(dirent.list(path)) do
      local caps = string.match(v[1], "(%x%x)%.lua")
      if v[2] == 1 and caps and tonumber("0x" .. caps) then
        local idx = tonumber("0x" .. caps)
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
