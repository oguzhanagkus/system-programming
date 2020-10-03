main: a b

a: a.c
	gcc -c a.c -Wall
	gcc -o a a.o

b: b.c
	gcc -c b.c -Wall
	gcc -o b b.o

run:
	./a -i ./input_1.txt -o ./output_0.txt -t 25 &
	./a -i ./input_2.txt -o ./output_0.txt -t 25 &
	./b -i ./output_0.txt -o ./output_1.txt -t 25

clean:
	rm *.o a b
