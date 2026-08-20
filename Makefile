
SRCS = $(wildcard ./src/*.c)

OBJS = $(SRCS:%.c=%.o)

CFLAGS = -Wall -Wextra -flto -O3
LDFLAGS = -lbfd -lz -ldl -lopcodes -flto -O3
%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

rvemu.elf: $(OBJS)
	gcc $(OBJS) $(LDFLAGS) -o rvemu.elf

.PHONY: test clean all

test:
	$(MAKE) -C ./test all

clean:
	rm -f $(OBJS) rvemu.elf

all: clean test rvemu.elf

