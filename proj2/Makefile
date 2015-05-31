CFLAGS         = -g -std=gnu99 -Wall -Wextra -Werror -pedantic -lpthread -lrt

CC              = gcc


###########################################

all: server.o client.o


server.o:
	$(CC) server.c $(CFLAGS) -o ipkperfserver


client.o:
	$(CC) client.c $(CFLAGS) -o ipkperfclient

clean:
	rm -rf ipkperfclient ipkperfserver ipkperf-* xnechu01.tgz

pack: clean
	tar -cvzf xnechu01.tgz client.c server.c Makefile README

test: all
	@echo "Starting server"
	@{ ./ipkperfserver -p 9393 & echo $$! > server.PID; }
	@echo "Starting client -p 9393 -i 20 -r 15 -t 20 -s 93 localhost"
	@./ipkperfclient -p 9393 -i 20 -r 15 -t 20 -s 93 localhost > stdout.test
	@echo "Stoping server"
	@kill `cat server.PID`
	@rm server.PID ipkperf-127.0.0.1-93-15 stdout.test
	@echo "OK"
	@echo "Starting client with bad host xfhk,gclgvkgckgh.cz"
	-./ipkperfclient -p 9393 xfhk,gclgvkgckgh.cz
	@echo "OK"




