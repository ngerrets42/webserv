TARGET := server.out

SOURCES := main.cpp Connection.cpp
HEADERS := Server.h Connection.h

CXX = clang++

.PHONY: all clean re
all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) -std=c++11 $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)

re: clean all
