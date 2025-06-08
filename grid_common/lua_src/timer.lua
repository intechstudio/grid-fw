-- Updated infinite loop for the `setTimeout` example
local timers = {}
local timersNextId = 0

function set_timeout(callback, delay)
    local timerId = timersNextId
    timersNextId = timersNextId + 1
    local targetTime = os.clock() + delay
    timers[timerId] = {id = timerId, time = targetTime, interval = delay, callback = callback}
    return timerId
end

function clear_timeout(timerId)
    if timers[timerId] then
        timers[timerId] = nil
    end
end

function check_timers()
    local now = os.clock()
    for timerId, timer in pairs(timers) do
        if now >= timer.time then
            print("Servicing timer"..timerId.." now")
            local retrigger = timer.callback(timer)
            if retrigger == true then
              print("Retriggering timer"..timerId.." now")
              timers[timerId].time = timers[timerId].time + timers[timerId].interval
            else
              print("Deleting timer"..timerId.." now")
              timers[timerId] = nil
            end
        end
    end
end
