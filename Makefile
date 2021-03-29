default: assemble simulate test

simulate: simulate.c
	gcc -o simulate simulate.c

assemble: test.as
	./assembler test.as test.mc

test: simulate.c test.mc
	./simulate test.mc > test.txt