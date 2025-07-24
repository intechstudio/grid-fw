init_element_midi = function(self)
  self.gms = function(self, ch, cmd, p1, p2)

    if cmd == -1 then
      if event_function_name() == "bc" then
        cmd = 144
      else
        cmd = 176
      end
    end

    if ch == -1 then
      -- ch = (module_position_y()*4+page_current())%16 
      ch = (gmy()*4+gpc())%16
    end

    if p1 == -1 then
      -- note = (32+module_position_x()*16+self:element_index())%128 
      p1 = (32+gmx()*16+self:ind())%128
    end

    if p2 == -1 then
      local event = event_function_name()
      if event == "bc" then
        p2 = self:bva()
      elseif event == "ec" then
        p2 = self:eva()
      elseif event == "epc" then
        p2 = self:epva()
      elseif event == "pc" then
        p2 = self:pva()
      else
        -- system element, lcd element etc
        p2 = 0
      end  
    end

    gms(ch, cmd, p1, p2)
  end
end


init_simple_midi = function()
  for i = 0, #ele do
    init_element_midi(ele[i])
  end
end

