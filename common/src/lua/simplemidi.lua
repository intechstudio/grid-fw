midi_auto_ch = function(self)
  return (gmy() * 4 + gpc()) % 16
end

midi_auto_cmd = function(self)
  if event_function_name() == "bc" then
    return 144
  else
    return 176
  end
end

midi_auto_p1 = function(self)
  return (32 + gmx() * 16 + self:ind()) % 128
end

midi_auto_p2 = function(self)
  local event = event_function_name()
  if event == "bc" then
    return self:bva()
  elseif event == "ec" then
    return self:eva()
  elseif event == "epc" then
    return self:epva()
  elseif event == "pc" then
    return self:pva()
  else
    return 0
  end
end

init_element_midi = function(self)
  self.gms = function(self, ch, cmd, p1, p2, mode)
    if cmd == -1 then
      cmd = midi_auto_cmd(self)
    end

    if ch == -1 then
      ch = midi_auto_ch(self)
    end

    if p1 == -1 then
      p1 = midi_auto_p1(self)
    end

    if p2 == -1 then
      p2 = midi_auto_p2(self)
    end

    if mode == nil or mode == 0 then
      gms(ch, cmd, p1, p2)
    elseif mode == 1 then
      gms(ch, 0xB0, p1, p2 // 128)
      gms(ch, 0xB0, p1 + 32, p2 % 128)
    elseif mode == 2 then
      gms(ch, 0xB0, 99, p1 // 128)
      gms(ch, 0xB0, 98, p1 % 128)
      gms(ch, 0xB0, 6, p2)
    elseif mode == 3 then
      gms(ch, 0xB0, 99, p1 // 128)
      gms(ch, 0xB0, 98, p1 % 128)
      gms(ch, 0xB0, 6, p2 // 128)
      gms(ch, 0xB0, 38, p2 % 128)
    end
  end

  self.midirx_register = function(self, ev, ch, cmd, p1, features)
    midirx_cb_register(self, ev, ch, cmd, p1, features)
  end
end

init_simple_midi = function()
  for i = 0, #ele do
    init_element_midi(ele[i])
  end
end

midirx_cb_register = function(self, ev, ch, cmd, p1, features)
  if ev == -1 then
    ev = event_function_name():sub(1, -2)
  end

  if cmd == -1 then
    cmd = midi_auto_cmd(self)
  end
  if ch == -1 then
    ch = midi_auto_ch(self)
  end
  if p1 == -1 then
    p1 = midi_auto_p1(self)
  end

  local rx_set_value, rx_set_led = table.unpack(features)

  self.midirx_cb = function(self, header, event)
    for name, fn in pairs(self) do
      if type(fn) == "function" and name:match("_midirx_cb") then
        fn(self, header, event)
      end
    end
  end
  self[ev .. "_midirx_cb"] = function(self, header, event)
    if event[2] == 128 then
      event[2] = 144
    end
    local v = event[4]
    if
      event[1] == ch
      and event[2] == cmd
      and event[3] == p1
      and header[1] == 13
    then
      if rx_set_value then
        self[ev .. "va"](self, v)
      end
      if rx_set_led then
        local l = { ep = 2, e = 2, p = 1, b = 1 }
        self:glp(
          l[ev],
          gmaps(v, self[ev .. "mi"](self), self[ev .. "ma"](self), 0, 255) // 1
        )
      end
    end
  end
end
