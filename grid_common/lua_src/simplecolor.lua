function cp(c)
  for i = 1, #c do
    if c[i][1] == -1 then
      c[i][1] = glr()
    end
    if c[i][2] == -1 then
      c[i][2] = glg()
    end
    if c[i][3] == -1 then
      c[i][3] = glb()
    end
  end

  local x, y, z = c[1], c[2], c[#c]

  if #c == 1 then
    x = {
      z[1] // 20,
      z[2] // 20,
      z[3] // 20,
      z[4],
    }
    y = {
      z[1] // 2,
      z[2] // 2,
      z[3] // 2,
      z[4],
    }
  elseif #c == 2 then
    y = {
      (x[1] + z[1]) // 2,
      (x[2] + z[2]) // 2,
      (x[3] + z[3]) // 2,
      (x[4] + z[4]) // 2,
    }
  elseif #c == 3 then
  end
  return x, y, z
end

event_handler_to_layer = {
  ["ini"] = nil,
  ["ec"] = 2,
  ["bc"] = 1,
  ["pc"] = 1,
  ["tim"] = nil,
  ["map"] = nil,
  ["mrx"] = nil,
  ["epc"] = 2,
  ["ld"] = nil,
}

init_endless_color = function(self)
  self.glp = function(self, l, v)
    if l == -1 then
      l = event_handler_to_layer[event_function_name()]
      if l == nil then
        return
      end
    end
    for i = 0, 4, 1 do
      local int = 0
      if l == 1 then
        -- button
        if v == nil or v == -1 then
          int = gmaps(self:bva(), self:bmi(), self:bma(), 0, 255) // 1
        else
          int = v
        end
      else
        -- rotation
        if v == nil or v == -1 then
          int = gsc(i, self:epva(), self:epmi(), self:epma())
        else
          int = gsc(i, v, 0, 255)
        end
      end
      local lix = glag(self:ind(), i)
      glp(lix, l, int)
    end
  end

  self.glc = function(self, l, c)
    local up = table.unpack
    local x, y, z = cp(c)
    if l == -1 then
      l = event_handler_to_layer[event_function_name()]
      if l == nil then
        return
      end
    end
    for i = 0, 4, 1 do
      local lix = glag(self:ind(), i)
      gln(lix, l, up(x))
      gld(lix, l, up(y))
      glx(lix, l, up(z))
    end
  end
end

init_endless_nosegment_color = function(self)
  self.glp = function(self, l, v)
    if l == -1 then
      l = event_handler_to_layer[event_function_name()]
      if l == nil then
        return
      end
    end
    for i = 0, 4, 1 do
      local int = v
      if l == 1 then
        -- button
        if v == nil or v == -1 then
          int = gmaps(self:bva(), self:bmi(), self:bma(), 0, 255) // 1
        end
      else
        -- rotation
        if v == nil or v == -1 then
          int = gmaps(self:epva(), self:epmi(), self:epma(), 0, 255) // 1
        end
      end
      local lix = glag(self:ind(), i)
      glp(lix, l, int)
    end
  end

  self.glc = function(self, l, c)
    local up = table.unpack
    local x, y, z = cp(c)
    if l == -1 then
      l = event_handler_to_layer[event_function_name()]
      if l == nil then
        return
      end
    end
    for i = 0, 4, 1 do
      local lix = glag(self:ind(), i)
      gln(lix, l, up(x))
      gld(lix, l, up(y))
      glx(lix, l, up(z))
    end
  end
end

init_element_color = function(self)
  self.glp = function(self, l, v)
    if l == -1 then
      l = event_handler_to_layer[event_function_name()]
      if l == nil then
        return
      end
    end
    local lix = glag(self:ind(), 0)
    local int = v
    if v == nil or v == -1 then
      if event_function_name() == "bc" then
        int = gmaps(self:bva(), self:bmi(), self:bma(), 0, 255) // 1
      elseif event_function_name() == "pc" then
        int = gmaps(self:pva(), self:pmi(), self:pma(), 0, 255) // 1
      elseif event_function_name() == "ec" then
        int = gmaps(self:eva(), self:emi(), self:ema(), 0, 255) // 1
      end
    end

    glp(lix, l, int)
  end

  self.glc = function(self, l, c)
    local up = table.unpack
    local x, y, z = cp(c)

    if l == -1 then
      l = event_handler_to_layer[event_function_name()]
      if l == nil then
        return
      end
    end
    local lix = glag(self:ind(), 0)
    gln(lix, l, up(x))
    gld(lix, l, up(y))
    glx(lix, l, up(z))
  end
end

init_simple_color = function()
  for i = 0, #ele - 1 do
    if ele[i].type == "endless" then
      init_endless_color(ele[i])
    else
      init_element_color(ele[i])
    end
  end
end
