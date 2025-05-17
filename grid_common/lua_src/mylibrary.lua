local my_library = {}

function my_library.greet(name)
    return "Hello, " .. (name or "world") .. "!"
end

function my_library.add(a, b)
    return a + b
end

return my_library
