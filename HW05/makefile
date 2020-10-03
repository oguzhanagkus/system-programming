main: program.o helper.o queue.o
	gcc program.o helper.o queue.o -o program -pthread -Wall -Wextra

program.o: program.c
	gcc -c program.c -pthread -Wall -Wextra

helper.o: helper.c
	gcc -c helper.c -pthread -Wall -Wextra

queue.o: queue.c
	gcc -c queue.c -Wall -Wextra

run:
	./program -i ./data

clean:
	rm *.o program

leak_test:
	valgrind -v --tool=memcheck --leak-check=full --show-leak-kinds=all ./program -i ./data
