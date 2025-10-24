CC      ?= cc
CFLAGS  ?= -O2 -Wall
LDLIBS  ?=

TARGET = mkar

all: $(TARGET)

$(TARGET): mkar.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(TARGET)
