
## useage

```lua
local Profiler = require "profiler"

profiler.start()
-- test code
profiler.stop()

```

## test

```sh
gcc -shared -DRDTSC -o profiler.so -fPIC lua-profiler.c game_hashtbl.h  game_hashtbl.c

lua test_profiler.lua 2
```

## output

```
0.000432 (0.02%)           1                 print  =[C]:-1
0.000061 (0.00%)           2                     ?  =[C]:-1
0.000003 (0.00%)           1                 clock  =[C]:-1
0.000009 (0.00%)          10                  nono  @test_profiler.lua:50
0.000005 (0.00%)           1                   foo  @test_profiler.lua:36
0.000000 (0.00%)           1                  foo2  @test_profiler.lua:32
0.000063 (0.00%)           2                  loop  @test_profiler.lua:24
0.015758 (0.69%)         200             call_func  @test_profiler.lua:15
2.268523 (99.29%)     8160000                 deep2  @test_profiler.lua:2
```