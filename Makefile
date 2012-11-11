SOURCES=main.cpp
DEPS=cpplog.hpp
EXECUTABLE=main
INCLUDES=-I/usr/local/include
LIBS=-L/usr/local/lib -lboost_thread-mt -lboost_system-mt

CC=g++
CFLAGS=-c -Wall
OBJECTS=$(SOURCES:.cpp=.o)
DEFINES=-DCPPLOG_THREADING -DCPPLOG_SYSTEM_IDS

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(LIBS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@

test: $(EXECUTABLE)
	./main

clean:
	rm $(OBJECTS) $(EXECUTABLE) *.log

