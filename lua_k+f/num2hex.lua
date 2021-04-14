p2x = function(num) 
  local a
  local b

  if num%16 < 10 then 
    a = string.char(48+num%16) 
  else 
    a = string.char(97+num%16-10) 
  end 

  if num//16%16 < 10 then 
    b = string.char(48+num//16%16) 
  else 
    b = string.char(97+(num//16)-10) 
  end 

  return a .. b

end


print(p2x(4))
print(p2x(11))
print(p2x(255))
