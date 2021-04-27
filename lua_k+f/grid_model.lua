element = {}

function grid_ui_init(e, c)
  for i=0, c-1 do
    e[i] = {}
    e[i].event = {}
    e[i].index = i
    e[i].T = {}
    e[i].T[0] = 2
    e[i].T[1] = 3
    e[i].T[2] = 4
  end
end

function elapsed_time(e)

  return e.T[1]

end

grid_ui_init(element , 16)

local this = element[3]

print(this.index, elapsed_time(this))
print(element[3].index, element[3].T[0])

