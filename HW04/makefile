main: program.o helper.o
	gcc program.o helper.o -o program -lrt -pthread -Wall -Wextra

program.o: program.c
	gcc -c program.c -lrt -pthread -Wall -Wextra

helper.o: helper.c
	gcc -c helper.c  -lrt -pthread -Wall -Wextra

run:
	./program -i ./input

clean:
	rm *.o program
