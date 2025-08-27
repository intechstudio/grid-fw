init_element_value = function(self)
  self.get_auto_value = function(self)
    local event = event_function_name()
    local value = 0

    if event == "bc" then
      value = self:bva()
    elseif event == "ec" then
      value = self:eva()
    elseif event == "epc" then
      value = self:epva()
    elseif event == "pc" then
      value = self:pva()
    else
      -- system element, lcd element etc
      value = 0
    end

    return value
  end

  self.get_auto_mode = function(self)
    local event = event_function_name()
    local mode = 0

    if event == "bc" then
      mode = 0
    elseif event == "ec" then
      if self:emo() > 0 then
        mode = 1
      else
        mode = 0
      end
    elseif event == "epc" then
      if self:epmo() > 0 then
        mode = 1
      else
        mode = 0
      end
    elseif event == "pc" then
      mode = 0
    else
      -- system element, lcd element etc
      mode = 0
    end

    return mode
  end
end

init_auto_value = function()
  for i = 0, #ele do
    init_element_value(ele[i])
  end
end
