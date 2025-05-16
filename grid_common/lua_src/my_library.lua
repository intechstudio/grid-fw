-- Define a local table to hold the library
local my_library = {}

-- Add a simple function
function my_library.greet(name)
    return "Hello, " .. (name or "world") .. "!"
end

-- Add another example function
function my_library.add(a, b)
    return a + b
end

-- Return the table to be used as the module
return my_library
