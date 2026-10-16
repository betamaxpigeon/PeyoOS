NASM ?= $(shell command -v nasm 2>/dev/null || printf '%s' /opt/local/bin/nasm)
CXX = x86_64-elf-g++
LD = x86_64-elf-ld
GRUB_MKRESCUE ?= $(shell command -v grub-mkrescue 2>/dev/null || command -v i686-elf-grub-mkrescue 2>/dev/null || command -v x86_64-elf-grub-mkrescue)

CXXFLAGS = -m64 -O2 -ffreestanding -fno-builtin -fno-exceptions -fno-rtti \
	-fno-stack-protector -fno-pic -fno-pie -fno-unwind-tables \
	-fno-asynchronous-unwind-tables -mno-red-zone -mno-sse -mno-sse2 -mno-mmx

CUSTOM_FONT ?= $(wildcard font.ttf)
ifneq ($(CUSTOM_FONT),)
empty :=
space := $(empty) $(empty)
escaped_font_path = $(subst $(space),\$(space),$(CUSTOM_FONT))
CXXFLAGS += -DPEYO_CUSTOM_FONT
kernel.o: generated_font.h

generated_font.h: $(call escaped_font_path) tools/ttf_to_bitmap.py
	python3 tools/ttf_to_bitmap.py "$(CUSTOM_FONT)" generated_font.h
endif

.PHONY: all iso clean

all: peyoos.iso

boot.o: boot.asm
	$(NASM) -f elf64 $< -o $@

kernel.o: kernel.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

peyoos.bin: boot.o kernel.o linker.ld
	$(LD) -m elf_x86_64 -T linker.ld --build-id=none -o $@ boot.o kernel.o

peyoos.iso: peyoos.bin iso_root/boot/grub/grub.cfg
	cp peyoos.bin iso_root/boot/peyoos.bin
	$(GRUB_MKRESCUE) -o $@ iso_root

iso: peyoos.iso

clean:
	rm -f boot.o kernel.o peyoos.bin peyoos.iso generated_font.h iso_root/boot/peyoos.bin
