.PHONY = clean

hello: hello.cpp
	g++ hello.cpp -o hello -g -O0 -no-pie

loop: loop.c
	gcc loop.c -o loop -g -O0

functions: functions.c
	g++ functions.c -o functions -g -O0

simple_functions: simple_functions.c
	gcc simple_functions.c -o simple_functions -g -O0

clean:
	rm loop
	rm hello
	rm functions
	rm simple_functions