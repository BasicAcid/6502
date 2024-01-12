build: | bin
	gcc -O2 -Wall -Wextra -Wformat=2 -Wformat-overflow -Wformat-truncation -Wshadow -Wdouble-promotion -Wundef -fno-common -z noexecstack -Wconversion -g -o ./bin/main main.c -lm

bin:
	mkdir -p bin

run:
	./bin/main
tags:
	@etags main.c
