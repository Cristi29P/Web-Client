CC=gcc
CFLAGS=-I.

client: client_http.c requests.c helpers.c buffer.c parson.c
	$(CC) -o client client_http.c requests.c helpers.c buffer.c parson.c -Wall -Wextra

run: client
	./client

clean:
	rm -f *.o client
