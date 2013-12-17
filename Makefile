TARGET = timer parse
CFLAGS = -Wall -g

all:$(TARGET)

timer:timer.cpp
	g++ $(CFLAGS) -o $@ $< -ljansson -lpthread

parse:parse.cpp
	g++ $(CFLAGS) -o $@ $< -ljansson -lpthread -lgtest -std=c++11

clean:
	rm -rf $(TARGET) *.o
