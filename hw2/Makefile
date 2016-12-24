CC = gcc
CFLAGS = -Wall

SRC = sender.c receiver.c agent.c
TARGET = $(SRC:.c=)
DEPS = packet.h
.PHONY: all clean run

all: $(TARGET)

sender: sender.c $(DEPS)
	$(CC) $(CFLAGS) -o sender sender.c

receiver: receiver.c $(DEPS)
	$(CC) $(CFLAGS) -o receiver receiver.c

agent: agent.c $(DEPS)
	$(CC) $(CFLAGS) -o agent agent.c

run:
	./sender

clean:
	$(RM) $(TARGET)

