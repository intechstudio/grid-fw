function cp(c)
    local x, y, z = c[1], c[2], c[#c]
    if #c == 1 then
        x = {0, 0, 0, 0}
        y = {z[1] // 2, z[2] // 2, z[3] // 2, z[4] / 2}
    elseif #c == 2 then
        y = {(x[1] + z[1]) // 2, (x[2] + z[2]) // 2, (x[3] + z[3]) // 2, (x[4] + z[4]) / 2}
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
};

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
						local ln = self:lix() + i * self:lof()
            if l == 1 then
								-- button
								if v == nil then int = gmaps(self:bva(), self:bmi(), self:bma(), 0, 255)//1
								else int = v end
						else
								-- rotation
								if v == nil then int = gsc(i, self:epva(), self:epmi(), self:epma())
								else int = gsc(i, v, 0, 255) end
						end
				glp(ln, l, int)
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
            local ln = self:ind() + i * self:lof()
            gln(ln, l, up(x))
            gld(ln, l, up(y))
            glx(ln, l, up(z))
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
						local ln = self:lix() + i * self:lof()
						if l == 1 then
								-- button
								if v == nil then
										int = gmaps(self:bva(), self:bmi(), self:bma(), 0, 255) // 1
								end
						else
								-- rotation
								if v == nil then
										int = gmaps(self:epva(), self:epmi(), self:epma(), 0, 255) // 1
								end
						end
            glp(ln, l, int)
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
            local ln = self:ind() + i * self:lof()
            gln(ln, l, up(x))
            gld(ln, l, up(y))
            glx(ln, l, up(z))
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
				if v == nil then
						glp(self:ind(), l, -1)
				else
						glp(self:ind(), l, v)
				end
		end

    self.glc = function(self, l, c)
        local up = table.unpack
        local i, x, y, z = self:lix(), c[1], c[2], c[#c]
        if #c == 1 then
            x = {0, 0, 0, 0}
            y = {z[1] // 2, z[2] // 2, z[3] // 2, z[4] / 2}
        elseif #c == 2 then
            y = {(x[1] + z[1]) // 2, (x[2] + z[2]) // 2, (x[3] + z[3]) // 2, (x[4] + z[4]) / 2}
        elseif #c == 3 then
        end
				if l == -1 then
						l = event_handler_to_layer[event_function_name()]
						if l == nil then
								return
						end
				end
        gln(i, l, up(x))
        gld(i, l, up(y))
        glx(i, l, up(z))
    end
end

init_simple_color = function()
  for i = 0, #ele do
    if ele[i].type == 'endless' then
      init_endless_color(ele[i])
    else
      init_element_color(ele[i])
    end
  end
end
