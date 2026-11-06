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
      while ele[i]:touch_pop() and ele[i].touch_cb do
        ele[i]:touch_cb(ele[i]:tid(), ele[i]:tev(), ele[i]:txv(), ele[i]:tyv())
      end
      while ele[i]:gesture1_pop() and ele[i].gesture1_cb do
        ele[i]:gesture1_cb(ele[i]:g1id(), ele[i]:g1ev(), ele[i]:g1xv(), ele[i]:g1yv())
      end
    end
  end
  _events_clear()
end
