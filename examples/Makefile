.PHONY = clean

hello: hello.cpp
	g++ hello.cpp -o hello -g -O0 -no-pie

loop: loop.c
	gcc loop.c -o loop -g -O0

clean:
	rm loop
	rm hello