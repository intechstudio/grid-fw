-- hello.lua
-- the first program in every language

function check_int(thing, min, max)
  if type(thing) == "number" and math.floor(thing) == thing
    then 
      if thing < min 
        then thing = min end
      if thing > max 
        then thing = max end
      print(thing)
    else print("problem")
  end
end

function grid_send_midi(ch, cmd, p1, p2)
  check_int(ch, 0, 16)
  check_int(cmd, 128, 256)

  print(string.format("%02X%02X%02X", cmd, p1, p2))
end

local element = {}
element[0] = 1
element[2] = 2
element[3] = 3
element[4] = 4
element[5] = 5


-- USER CONFIG
function ui_change(element, event)

if element == 1
  then 
--@el1ev0
    grid_send_midi(-10, 10, 45, 100)
elseif  
  then  

end



function encoder_change (element)

  grid_send_midi(0, 10, 45, 100)

end


v

encoder_change(14);
