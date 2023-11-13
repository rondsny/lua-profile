local HighPrecisionTime = require "high_precision_time"

local _gettime = HighPrecisionTime.get_high_precision_time
local getinfo = debug.getinfo

local M = {}

local map_record
local lst_stack

local function _func_key(stack)
    -- local name = stack.name
    local line = stack.line
    local source = stack.source
    return source .. line
end

local function _func_title(stack)
    local name = stack.name or "anonymous"
    local line = stack.line or 0
    local source = stack.source or 'c_func'
    return string.format("%-30s: %s:%s", name, source, line)
end

local function _func_item(stack)
    local key = _func_key(stack)
    local item = map_record[key]
    if not item then
        item = {
            call_count = 0,
            total_time = 0,
            name = stack.name,
            line = stack.line,
            source = stack.source,
        }
        map_record[key] = item
    end
    return item
end

-- local trace_use_time = 0
function M.start()
    print("======== profiler start ========")
    map_record = {}
    lst_stack = {}

    debug.sethook(function(hook_type)
        local now = _gettime()
        local func_info = getinfo(2, 'nSl')
        if hook_type == 'call' or hook_type == 'tail call' then
            local stack = {
                name         = func_info.name or "anonymous",
                line         = func_info.linedefined or 0,
                source       = func_info.short_src or "c_func",
                call_time    = now,
                sub_cost     = 0,
                is_tail_call = hook_type == "tail call",
            }
            table.insert(lst_stack, stack)
            local record = _func_item(stack)
            record.call_count = record.call_count + 1
        elseif hook_type == 'return' then
            local is_tail_call = true
            while is_tail_call and #lst_stack > 0 do
                local len = #lst_stack
                local total_time  = now - lst_stack[len].call_time - lst_stack[len].sub_cost
                local record      = _func_item(lst_stack[len])
                record.total_time = record.total_time + total_time
                is_tail_call = lst_stack[len] and lst_stack[len].is_tail_call
                if lst_stack[len-1] then
                    lst_stack[len-1].sub_cost = lst_stack[len-1].sub_cost + (now - lst_stack[len].call_time)
                end
                lst_stack[len] = nil
            end
        end
        -- trace_use_time = trace_use_time + (_gettime()-now)
    end, 'cr', 0)
end

function M.stop()
    debug.sethook()
    local total_time = 0
    local lst_record = {}
    for _,v in pairs(map_record) do
        total_time = total_time + v.total_time
        table.insert(lst_record, v)
    end
    if total_time == 0 then
        total_time = 0.00001
    end
    table.sort(lst_record, function(ta, tb)
        return ta.total_time > tb.total_time
    end)

    print("========================================")
    print(string.format("%12s, %6s, %7s, %s", "time", "percent", "count", "function"))
    print("----------------------------------------")
    for _,record in pairs(lst_record) do
        local percent = record.total_time / total_time * 100
        if percent < 0 then
            break
        end
        print(string.format("%6.10f, %6.2f%%, %7d, %s", record.total_time/2000000000,
            percent, record.call_count, _func_title(record)))
    end
    -- print("======== trace_use_time ==========", trace_use_time)
    print("======== profiler end total_time ==========", total_time)
end

function M.start_trace()
    print("======== profiler start_trace ========")
    debug.sethook(function(hook_type)
        local func_info = getinfo(2)
        if hook_type == 'call' then
            local name = func_info.name
            local source = func_info.short_src
            if name and source then
                print(string.format("%-30s: %s: %s", name, source, func_info.linedefined or 0))
            end
        end
    end, 'cr', 0)
end

function M.stop_trace()
    debug.sethook()
    print("======== profiler stop_trace ========")
end

return M