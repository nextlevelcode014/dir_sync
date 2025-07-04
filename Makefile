CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -D_XOPEN_SOURCE=500 -D_DEFAULT_SOURCE
TARGET = dir_sync
SRCS = src/main.c src/utils.c src/watcher.c src/config.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

docs:
	doxygen Doxyfile

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
