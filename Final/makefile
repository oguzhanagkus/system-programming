main: server.o client.o common.o helper.o queue.o graph.o
	gcc server.o common.o helper.o queue.o graph.o -o server -pthread -Wall -Wextra
	gcc client.o common.o -o client -Wall -Wextra

server.o: server.c
	gcc -c server.c -Wall -Wextra

client.o: client.c
	gcc -c client.c -Wall -Wextra

common.o: common.c
	gcc -c common.c -Wall -Wextra

helper.o: helper.c
	gcc -c helper.c -Wall -Wextra

queue.o: queue.c
	gcc -c queue.c -Wall -Wextra

graph.o: graph.c
	gcc -c graph.c -Wall -Wextra

run_server: server.c
	./server -i /home/oguzhan/Desktop/Final/input.txt -p 8080 -o /home/oguzhan/Desktop/Final/log.txt -s 4 -x 24

run_client:
	./client -a 127.0.0.1 -p 8080 -s 3 -d 2414

clean:
	rm *.o server client

leak_test_server:
	valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes ./server -i /home/oguzhan/Desktop/Final/input.txt -p 8080 -o /home/oguzhan/Desktop/Final/log.txt -s 4 -x 24

leak_test_client:
	valgrind --leak-check=full --show-leak-kinds=all ./client -a 127.0.0.1 -p 8080 -s 3 -d 2414

delete_log:
	rm /home/oguzhan/Desktop/Final/log.txt
