-- elementname
function gen(a, b)
  if b == nil then
    local name = ggen(a, b)
    if name == nil then
      return ""
    else
      return name
    end
  else
    gsen(a, b)
    gens(a, b)
  end
end
