CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2

COMMON_OBJS = frame.o checksum.o crc.o error.o utils.o

all: sender receiver

sender: sender.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o sender sender.o $(COMMON_OBJS)

receiver: receiver.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o receiver receiver.o $(COMMON_OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o sender receiver received_output.txt received_output.bits raw_terminal_output.txt
