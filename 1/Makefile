all: compile test_gen run

run: prog
	./prog -i .rand_file -o cypher.txt -x 4212 -a 84589 -c 45989 -m 217728
	./prog -i cypher.txt -o decypher.txt -x 4212 -a 84589 -c 45989 -m 217728
	diff .rand_file decypher.txt

test_gen:
	head -c 10M < /dev/urandom > .rand_file

compile: main.c
	gcc main.c -o prog

prof:
	gcc -pg main.c -o prog 
	./prog -i .rand_file -o cypher.txt -x 4212 -a 84589 -c 45989 -m 217728
	gprof prog 


