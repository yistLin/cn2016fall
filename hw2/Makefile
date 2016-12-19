CC = gcc
CFLAGS = -Wall

SRC = sender.c receiver.c agent.c
TARGET = $(SRC:.c=)
.PHONY: all clean run

all: $(TARGET)

sender: sender.c
	$(CC) $(CFLAGS) -o sender sender.c

receiver: receiver.c
	$(CC) $(CFLAGS) -o receiver receiver.c

agent: agent.c
	$(CC) $(CFLAGS) -o agent agent.c

run:
	./sender

clean:
	$(RM) $(TARGET)

