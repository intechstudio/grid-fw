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

init_endless_color = function(self)
    self.glp = function(self, l, v)
        for i = 0, 4, 1 do
            local ln = self:lix() + i * self:lof()
            local int = gsc(i, self:epva(), self:epmi(), self:epma())
            if l == 1 then
                local min, max, value = self:bmi(), self:bma(), self:bva()
		int = gmaps(value, min, max, 0, 255)//1
            end
            glp(ln, l, int)
        end
    end
    self.glc = function(self, l, c)
        local up = table.unpack
        local x, y, z = cp(c)
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
        for i = 0, 4, 1 do
            local ln = self:lix() + i * self:lof()
            local min, max, value = self:epmi(), self:epma(), self:epva()
            if l == 1 then
                min, max, value = self:bmi(), self:bma(), self:bva()
            end
            local int = gmaps(value, min, max, 0, 255) // 1
            glp(ln, l, int)
        end
    end
    self.glc = function(self, l, c)
        local up = table.unpack
        local x, y, z = cp(c)
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
        glp(self:ind(), l, -1)
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
        gln(i, l, up(x))
        gld(i, l, up(y))
        glx(i, l, up(z))
    end
end
