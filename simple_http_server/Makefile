CC = g++ 
CFLAGS =  -std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow
TARGET = httpserver

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp 

clean:
	rm -f $(TARGET) $(TARGET).o 

spotless:
	rm -f $(TARGET).exe
