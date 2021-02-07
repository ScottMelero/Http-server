CC = g++ 
CFLAGS =  -std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow
TARGET = httpserver

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CC) -o $(TARGET) $(TARGET).cpp $(CFLAGS)

clean:
	rm -f $(TARGET) $(TARGET).o 0123456789

spotless:
	rm -f $(TARGET).exe
