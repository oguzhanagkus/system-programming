main: program.o helper.o
	gcc helper.o program.o -o program -lm -Wall

program.o: program.c
	gcc -c program.c -Wall

helper.o: helper.c
	gcc -c helper.c -Wall

run:
	./program -i ./input -o ./output

clean:
	rm *.o program output
