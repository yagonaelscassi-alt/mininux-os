BUILD_DIR := build
ISO_ROOT := $(BUILD_DIR)/iso
KERNEL := $(BUILD_DIR)/mininux.elf
ISO := $(BUILD_DIR)/mininux.iso
GRUB_IMAGE := grub/splash.png
WALLPAPER_IMAGE := assets/minux-wallpaper.png
WALLPAPER_RGB := $(BUILD_DIR)/minux-wallpaper.rgb
ARRIUNE_IMAGE := assets/wallpaper-arriune.png
ARRIUNE_RGB := $(BUILD_DIR)/wallpaper-arriune.rgb
CURSOR_IMAGE := assets/ps2-cursor.png
CURSOR_RGBA := $(BUILD_DIR)/ps2-cursor.rgba
APP_ICON_NAMES := calculator settings browser terminal paint
APP_ICON_RGBA_FILES := $(addprefix $(BUILD_DIR)/app-,$(addsuffix .rgba,$(APP_ICON_NAMES)))
ICON_NAMES := carpeta descargar documentos imagen imagens musica nigun musicales textos video videos
ICON_RGBA_FILES := $(addprefix $(BUILD_DIR)/icon-,$(addsuffix .rgba,$(ICON_NAMES)))
DOOM_SCRIPT := games/doom.py

CC := gcc
LD := ld
NASM := nasm
PYTHON := python3

CFLAGS := -std=gnu11 -O2 -Wall -Wextra -Werror \
	-ffreestanding -fno-builtin -fno-pie -fno-stack-protector \
	-Iinclude -m64 -mno-red-zone -mcmodel=small
LDFLAGS := -m elf_x86_64 -nostdlib -z max-page-size=0x1000 \
	-T linker.ld

DRIVER_OBJS := $(BUILD_DIR)/framebuffer.o $(BUILD_DIR)/ps2.o
KERNEL_OBJS := $(BUILD_DIR)/entry.o $(BUILD_DIR)/main.o \
	$(BUILD_DIR)/donut3d.o $(BUILD_DIR)/wallpaper.o $(DRIVER_OBJS)

.PHONY: all check-tools verify run copilot clean
.SECONDARY: $(WALLPAPER_RGB) $(ARRIUNE_RGB) $(CURSOR_RGBA) $(ICON_RGBA_FILES) $(APP_ICON_RGBA_FILES)

all: check-tools $(ISO)

check-tools:
	@set -e; for tool in $(CC) $(LD) $(NASM) $(PYTHON) grub-mkrescue grub-file; do \
		command -v "$$tool" >/dev/null 2>&1 || { \
			echo "Falta la herramienta requerida: $$tool" >&2; exit 1; \
		}; \
	done
	@command -v xorriso >/dev/null 2>&1 || { \
		echo "Falta xorriso (requerido por grub-mkrescue)." >&2; exit 1; \
	}

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/entry.o: boot/entry.asm | $(BUILD_DIR)
	$(NASM) -f elf64 $< -o $@

$(WALLPAPER_RGB): $(WALLPAPER_IMAGE) scripts/png_to_rgb.py | $(BUILD_DIR)
	$(PYTHON) scripts/png_to_rgb.py $(WALLPAPER_IMAGE) $@

$(ARRIUNE_RGB): $(ARRIUNE_IMAGE) scripts/png_to_rgb.py | $(BUILD_DIR)
	$(PYTHON) scripts/png_to_rgb.py $(ARRIUNE_IMAGE) $@

$(CURSOR_RGBA): $(CURSOR_IMAGE) scripts/png_to_rgb.py | $(BUILD_DIR)
	$(PYTHON) scripts/png_to_rgb.py $(CURSOR_IMAGE) $@ --rgba-key-white

$(BUILD_DIR)/icon-%.rgba: assets/icons/%.png scripts/png_to_rgb.py | $(BUILD_DIR)
	$(PYTHON) scripts/png_to_rgb.py $< $@ --rgba-key-white

$(BUILD_DIR)/app-%.rgba: assets/apps/%.png scripts/png_to_rgb.py | $(BUILD_DIR)
	$(PYTHON) scripts/png_to_rgb.py $< $@ --rgba-key-white

$(BUILD_DIR)/wallpaper.o: boot/wallpaper.asm $(WALLPAPER_RGB) $(ARRIUNE_RGB) $(CURSOR_RGBA) $(ICON_RGBA_FILES) $(APP_ICON_RGBA_FILES) | $(BUILD_DIR)
	$(NASM) -f elf64 $< -o $@

$(BUILD_DIR)/main.o: kernel/main.c include/donut3d.h include/framebuffer.h include/ps2.h $(BUILD_DIR)/wallpaper.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/donut3d.o: kernel/donut3d.c include/donut3d.h include/framebuffer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/framebuffer.o: drivers/framebuffer.c include/framebuffer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ps2.o: drivers/ps2.c include/ps2.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(KERNEL_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

verify: $(KERNEL)
	grub-file --is-x86-multiboot2 $(KERNEL)

$(ISO): $(KERNEL) kernel/donut3d.c include/donut3d.h grub/grub.cfg $(GRUB_IMAGE) $(WALLPAPER_IMAGE) $(ARRIUNE_IMAGE) $(DOOM_SCRIPT)
	mkdir -p $(ISO_ROOT)/boot/grub
	mkdir -p $(ISO_ROOT)/kernel $(ISO_ROOT)/include
	cp $(KERNEL) $(ISO_ROOT)/boot/mininux.elf
	cp kernel/donut3d.c $(ISO_ROOT)/kernel/donut3d.c
	cp include/donut3d.h $(ISO_ROOT)/include/donut3d.h
	cp $(DOOM_SCRIPT) $(ISO_ROOT)/doom.py
	cp grub/grub.cfg $(ISO_ROOT)/boot/grub/grub.cfg
	@if [ -f $(GRUB_IMAGE) ]; then cp $(GRUB_IMAGE) $(ISO_ROOT)/boot/grub/splash.png; fi
	grub-mkrescue -o $@ $(ISO_ROOT)

run: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -m 128M

copilot:
	python3 scripts/copilot.py

clean:
	rm -rf $(BUILD_DIR)
