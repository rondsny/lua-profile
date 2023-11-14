all: linux

linux:
	gcc -shared -DRDTSC -o high_precision_time.so -fPIC high_precision_time.c
	gcc -shared -DRDTSC -o profiler.so -fPIC lua-profiler.c game_hashtbl.h  game_hashtbl.c

clean:
	rm -rf high_precision_time.so profiler.so