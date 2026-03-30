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
  self.gms = function(self, ch, cmd, p1, p2)
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

    gms(ch, cmd, p1, p2)
  end
end

init_simple_midi = function()
  for i = 0, #ele do
    init_element_midi(ele[i])
  end
end
