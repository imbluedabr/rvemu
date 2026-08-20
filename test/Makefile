
ASRCS = $(wildcard sys/*.s) $(wildcard drv/*.s) $(wildcard usr/*.s)
OBJS = $(ASRCS:%.s=%.o)

CFLAGS = -O2 -march=rv32i_zicsr -mabi=ilp32 -ffreestanding -Wall -Wextra -Wno-unused-parameter -g
ASFLAGS = -march=rv32i_zicsr -mabi=ilp32 -ffreestanding -g
LDFLAGS = -march=rv32i_zicsr -mabi=ilp32 -nostdlib -nostartfiles -static -T ./linker.ld


%.o: %.s
	riscv64-unknown-elf-gcc $(ASFLAGS) -c $< -o $@

test.elf: $(OBJS)
	riscv64-unknown-elf-gcc $(LDFLAGS) $(OBJS) -o $@
	riscv64-unknown-elf-readelf -S test.elf

test.bin: test.elf
	riscv64-unknown-elf-objcopy -O binary test.elf test.bin

clean:
	rm -f $(OBJS) *.elf *.bin

all: clean test.bin

