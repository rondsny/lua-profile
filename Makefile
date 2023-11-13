all: linux

linux:
	gcc -shared -o high_precision_time.so -fPIC high_precision_time.c rdtsc.h

clean:
	rm -rf high_precision_time.so