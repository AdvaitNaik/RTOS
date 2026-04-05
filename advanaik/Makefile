CROSS = aarch64-none-elf-
CC = $(CROSS)gcc
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy

CFLAGS = -Wall -O2 -ffreestanding -nostdlib -mcpu=cortex-a76 -I.

OBJS = start.o main.o

all: kernel8.img

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel8.img: $(OBJS) linker.ld
	$(LD) -T linker.ld -o kernel.elf $(OBJS)
	$(OBJCOPY) kernel.elf -O binary kernel8.img

clean:
	del /Q *.o *.elf *.img 2>nul || exit 0
