local profiler = require "profiler"

local function deep2(c)
    if c < 0 then
        return
    end
    local a = 11
    local i = 22
    a = a + i
    a = a * i
    a = a / i
    deep2(c - 1)
    return 111
end

local function call_func(a, b)
    for i=1, 400 do
        a = a + i
        a = a * i
        a = a / i
        deep2(100)
    end
end

local function loop(sec)
    local a = 0
    for i=1, 100 do
        call_func(i)
    end
end


local function foo2(a, b)
    return a + b
end

local function foo(a, b)
    loop(1)
    local c = a + b + foo2(a, b)
    loop(1)
    return c
end

local function bar(a)
    if a <= 0 then
        return 0
    end
    return bar(a - 1)
end

local function nono(a, b)
    local c = a + b / a
    return bar(a)
end

local start_time = os.clock()
profiler.start()

foo(11, 22)
for i=1,10 do
    nono(11, 22)
end

profiler.stop()

print("total time is", os.clock() - start_time)