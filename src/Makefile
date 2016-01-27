CC=g++
CFLAGS=-c -Wall -Iincludes
LDFLAGS=-lcurl -ljson-c -lstdc++
SOURCES=main.cpp connection.cpp hue/hue.cpp hue/config.cpp hue/light.cpp hue/hub.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=huelights

all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
