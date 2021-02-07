CC = g++ 
CFLAGS = -Wall -Wextra -Wpedantic -Wshadow
BINS = httpserver 
OBJS = httpserver.o myqueue.o

all: $(BINS)

httpserver: $(OBJS)
	$(CC) $(CFLAGS) -pthread -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -f *.dSYM $(OBJS) $(BINS) 

spotless:
	rm -f $(SERVER).exe