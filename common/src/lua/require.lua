function gr(n)
  local ok, m = pcall(require, string.format("/%02X/", gpc()) .. n)
  if ok then
    return m
  end
  print(string.format("/%02X/", gpc()) .. n .. ".lua missing")
end
