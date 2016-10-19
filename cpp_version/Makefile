##@author by Sebastian WIEDEMANN 1425647
CC = g++
DEFS = ##-D_XOPEN_SOURCE=500 -D_BSD_SOURCE
CFLAGS = -g -std=c++11# -Wall -pedantic $(DEFS)
LDFLAGS = -g
OBJECTFILES = IrcBot.o main.o Match.o Admins.o
EXECS = main 
.PHONY: all clean

all: main

main: $(OBJECTFILES)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c  $<

clean:
	rm -f $(OBJECTFILES) $(EXECS)
