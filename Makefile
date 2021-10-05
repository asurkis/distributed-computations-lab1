build:
	clang -std=c99 -Wall -pedantic *.c

run: build
	./a.out -p 3

clean:
	rm a.out *.log
