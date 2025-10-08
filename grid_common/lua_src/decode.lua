_decode_clear = function()
	for i = 1, #_decoded_order do _decoded_order[i] = nil end
	for i = 1, #_decoded_midi do _decoded_midi[i] = nil end
	for i = 1, #_decoded_sysex do _decoded_sysex[i] = nil end
	for i = 1, #_decoded_eview do _decoded_eview[i] = nil end
end

pass_midi = function(el, x)
	if el.midirx_cb then
		el:midirx_cb({ x[1], x[2], x[3], x[4] }, { x[5], x[6], x[7] })
	end
end

pass_sysex = function(el, x)
	if el.sysexrx_cb then
		el:sysexrx_cb(x[1], { x[2], x[3], x[4] })
	end
end

pass_event = function(el, x)
	if el.eventrx_cb then
		el:eventrx_cb(
			{ x[1], x[2], x[3] },
			{ x[4], x[5], x[6] },
			{ x[7], x[8], x[9] },
			x[10]
		)
	end
end

_decode_process = function(midi, sysex, event)

	local order = _decoded_order

	local funs = { pass_midi, pass_sysex, pass_event }
	local srcs = { midi, sysex, event }
	local idxs = { 1, 1, 1 }

	for i = 1, #order do
		local src = order[i]
		for j = 0, #ele do
			funs[src](ele[j], srcs[src][idxs[src]])
		end
		idxs[src] = idxs[src] + 1
	end

	_decode_clear()

end

