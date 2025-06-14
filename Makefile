CC = aarch64-none-elf-gcc
LD = aarch64-none-elf-ld
OBJCOPY = aarch64-none-elf-objcopy
CFLAGS = -Wall -O0 -nostdlib -nostartfiles -ffreestanding
# -Wall			Enable all common warnings
# -O0			No optimizations — easier to debug
# -nostdlib		Don’t link the standard C library (no OS!)
# -nostartfiles	Don’t use the normal startup code (crt0.o) — use (start.S)
# -ffreestandingWe're in a freestanding environment — not Linux or Windows. 
# 				This disables assumptions about the environment like availability of main() or printf()

all: kernel8.img

kernel8.img: start.o main.o scheduler.o task.o context.o timer.o irq.o vectors.o mm.o mutex.o semaphore.o
	$(LD) -T linker.ld -o kernel.elf $^
	$(OBJCOPY) kernel.elf -O binary $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.elf kernel8.img
