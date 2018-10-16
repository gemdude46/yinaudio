CXX ?= g++
TARGET := ./build/yinaudio.out
CXXFLAGS := -O2 -Wall -Wextra -lasound
OBJECTS := src/main.o src/parser.o src/nodeserializer.o src/socketcontroller.o src/audioframe.o src/audionode.o src/audionodestereomerge.o src/audionodesine.o src/audionodealsainput.o src/audionodealsaoutput.o

all: $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(CXXFLAGS)

run:
	make all
	$(TARGET)

clean:
	rm -- $(OBJECTS) $(TARGET) || true

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)
