function gmaps (x, in_min, in_max, o_min, o_max)
	local n = (x - in_min) * (o_max - o_min) / (in_max - in_min) + o_min
  local o_max2, o_min2 = o_max, o_min
  if o_min > o_max then
    o_max2, o_min2 = o_min, o_max
  end
	if n > o_max2 then
		return o_max2
	elseif n < o_min2 then
		return o_min2
	else
		return n
	end
end
