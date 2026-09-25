CC = g++
CFLAGS = -std=c++11 -Wall

all: channel station

channel: channel.cpp utils.hpp
	$(CC) $(CFLAGS) channel.cpp -o channel

station: station.cpp 1_persistent.cpp non_persistent.cpp p_persistent.cpp csma.hpp utils.hpp
	$(CC) $(CFLAGS) station.cpp 1_persistent.cpp non_persistent.cpp p_persistent.cpp -o station

clean:
	rm -f channel station
