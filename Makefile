TARGET = timer
CFLAGS = -Wall -g

all:$(TARGET)

timer:timer.cpp
	g++ $(CFLAGS) -o $@ $< -ljansson -lpthread

clean:
	rm -rf $(TARGET) *.o
