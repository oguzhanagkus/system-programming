main: program.o helper.o svd.o
	gcc program.o helper.o svd.o -o program -Wall -lm

program.o: program.c
	gcc -c program.c -Wall

helper.o: helper.c
	gcc -c helper.c -Wall

svd.o: svd.c
	gcc -c svd.c -Wall

run:
	./program -i ./input -j ./input -n 2

clean:
	rm *.o program
