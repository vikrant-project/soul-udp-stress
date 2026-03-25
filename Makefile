CC = gcc
CFLAGS = -Wall -Wextra -O3 -march=native -pthread -std=gnu11
LDFLAGS = -pthread

TARGET = soul

SRCS = main.c \
       config.c \
       socket_wrapper.c \
       packet_builder.c \
       memory.c \
       ring_buffer.c \
       thread_pool.c \
       stats.c \
       signals.c \
       utils.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Build complete: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
	@echo "Clean complete"
