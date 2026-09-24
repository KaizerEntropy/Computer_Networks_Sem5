CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall

# Common objects needed by both sender and receiver
COMMON_OBS = frame.o checksum.o crc.o error.o stop_and_wait.o go_back_n.o selective_repeat.o utils.o

all: sender receiver

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

sender: sender.o $(COMMON_OBS)
	$(CXX) $(CXXFLAGS) sender.o $(COMMON_OBS) -o sender

receiver: receiver.o $(COMMON_OBS)
	$(CXX) $(CXXFLAGS) receiver.o $(COMMON_OBS) -o receiver

clean:
	rm -f sender receiver *.o received_file.txt
