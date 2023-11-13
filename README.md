
## useage

```lua
local Profiler = require "profiler"

profiler.start()
-- test code
profiler.stop()

```

## test

```sh
gcc -shared -o high_precision_time.so -fPIC high_precision_time.c

lua test_profiler.lua
```