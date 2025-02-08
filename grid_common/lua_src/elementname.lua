-- elementname
function gen (a, b)
  if b==nil then
    if ele[a].sn==nil then
      return ''
    else
      return ele[a].sn
    end
  else
    ele[a].sn=b
    gens(a,b)
  end
end
