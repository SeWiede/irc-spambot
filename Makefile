##@author by Sebastian WIEDEMANN 1425647
CC = gcc
DEFS = -D_XOPEN_SOURCE=500 -D_BSD_SOURCE
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
LDFLAGS =
OBJECTFILES = bot.o admin.o match.o
EXECS = bot 
.PHONY: all clean

all: bot 

bot: $(OBJECTFILES)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTFILES) $(EXECS)
