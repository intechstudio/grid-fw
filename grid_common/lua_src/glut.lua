function glut (a, ...)
  local t = table.pack(...)
  for i = 1, t.n//2*2 do
    if i%2 == 1 then
      if t[i] == a then
        return t[i+1]
      end
    end
  end
  return nil
end
