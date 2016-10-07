##@author by Sebastian WIEDEMANN 1425647
CC = g++
DEFS = ##-D_XOPEN_SOURCE=500 -D_BSD_SOURCE
CFLAGS = -g ##-Wall -g -std=c99 -pedantic $(DEFS)
LDFLAGS = -g
OBJECTFILES = IrcBot.o main.o
EXECS = main 
.PHONY: all clean

all: main

main: $(OBJECTFILES)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c  $<

clean:
	rm -f $(OBJECTFILES) $(EXECS)
