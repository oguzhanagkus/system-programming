main: program.o helper.o
	gcc program.o helper.o -o program -Wall -lrt -pthread

program.o: program.c
	gcc -c program.c -Wall -lrt -pthread

helper.o: helper.c
	gcc -c helper.c -Wall -lrt -pthread

run_1:
	./program -N 3 -T 5 -S 5 -L 5 -U 7 -G 3 -F ./supplies_1

run_2:
	./program -N 3 -T 5 -S 4 -L 13 -U 10 -G 2 -F ./supplies_2

clean:
	rm *.o program
