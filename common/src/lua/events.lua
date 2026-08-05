_events_clear = function()
  for i = 1, #_events_eleidx do
    _events_eleidx[i] = nil
  end
  for i = 1, #_events_evestr do
    _events_evestr[i] = nil
  end
end

_events_process = function(eleidx, evestr)
  for i = 1, #eleidx do
    local element = ele[eleidx[i]]
    local eve = element[evestr[i]]
    if eve then
      eve(element)
    end
  end
  for i = 0, #ele do
    if ele[i].type == "touch" then
      if ele[i]:touch_pop() and ele[i].touch_cb then
        ele[i]:touch_cb(ele[i]:tid(), ele[i]:tev(), ele[i]:txv(), ele[i]:tyv())
      end
    end
  end
  _events_clear()
end
