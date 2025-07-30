function gsc(seg, enc_val, enc_min, enc_max)
  local s_min = enc_min + (enc_max - enc_min) / 5 * seg
  local s_max = enc_min + (enc_max - enc_min) / 5 * (seg + 1)
  return math.floor(gmaps(enc_val, s_min, s_max, 0, 255) / 1)
end
