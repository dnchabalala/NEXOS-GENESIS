# NexOS - Makefile | Top-level build system | MIT License

# Cross-compiler detection
ifneq ($(shell which x86_64-elf-gcc 2>/dev/null),)
	CC = x86_64-elf-gcc
	LD = x86_64-elf-ld
else
	CC = gcc
	LD = ld
endif

ASM    = nasm
PYTHON = python3

ROOT   = .
BUILD  = build
KERNEL = kernel
BOOT   = boot
USER   = userspace
TOOLS  = tools
NETSURF_SOURCE_DIR ?= ports/netsurf/src
NETSURF_HOST_PREFIX ?= ports/netsurf/host-prefix
NETSURF_BUILD_SYSTEM := $(abspath $(NETSURF_SOURCE_DIR)/buildsystem)
NETSURF_HOST_PREFIX_ABS := $(abspath $(NETSURF_HOST_PREFIX))
TLS_SOURCE_DIR ?= ports/tls/src/mbedtls-3.6.7
TLS_CONFIG_FILE := $(abspath ports/tls/mbedtls_nexos_config.h)
TLS_LIB_DIR := $(TLS_SOURCE_DIR)/library
TLS_LIBS := $(TLS_LIB_DIR)/libmbedtls.a $(TLS_LIB_DIR)/libmbedx509.a $(TLS_LIB_DIR)/libmbedcrypto.a
LIBGCC := $(shell $(CC) -print-libgcc-file-name)

KERNEL_ELF = $(BUILD)/nexos.kernel
ISO_FILE   = $(BUILD)/nexos.iso

CFLAGS = -std=c11 -ffreestanding -fno-stack-protector -fno-pic \
        -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
        -Wall -Wextra -O2 -m64 -mcmodel=kernel \
        -nostdlib -nostdinc \
        -ffunction-sections -fdata-sections -fno-common \
        -I$(ROOT) -I$(KERNEL) -I$(KERNEL)/include \
        -I$(KERNEL)/gui -I$(KERNEL)/drivers \
        -Iports/tls -Iports/tls/include -I$(TLS_SOURCE_DIR)/include \
        -DMBEDTLS_CONFIG_FILE=\"$(TLS_CONFIG_FILE)\" \
        -Wno-unused-parameter -Wno-unused-variable

ifneq ($(NATIVE_TLS_TEST),)
CFLAGS += -DNEXOS_NATIVE_TLS_TEST
endif

ifneq ($(NATIVE_NETSURF_TEST),)
CFLAGS += -DNEXOS_NATIVE_NETSURF_TEST \
	-Iports/netsurf/src -Iports/netsurf/src/netsurf \
	-Iports/netsurf/src/netsurf/include -Iports/netsurf/src/libwapcaplet/include \
	-Iports/netsurf/src/libparserutils/include -Iports/netsurf/src/libhubbub/include \
	-Iports/netsurf/src/libnsutils/include -Iports/netsurf/src/libcss/include \
	-Iports/netsurf/src/libdom/include -Iports/netsurf/src/libnsfb/include
endif

ASFLAGS = -f elf64

LDFLAGS = -T $(BOOT)/linker.ld -nostdlib -z noexecstack --gc-sections

# ---- C sources ----
KERNEL_C_SOURCES = \
        $(KERNEL)/kernel.c \
        $(KERNEL)/arch/x86_64/gdt.c \
        $(KERNEL)/arch/x86_64/idt.c \
        $(KERNEL)/arch/x86_64/paging.c \
        $(KERNEL)/arch/x86_64/fpu.c \
        $(KERNEL)/drivers/vga.c \
        $(KERNEL)/drivers/serial.c \
        $(KERNEL)/drivers/keyboard.c \
        $(KERNEL)/drivers/timer.c \
        $(KERNEL)/drivers/ata.c \
        $(KERNEL)/drivers/pci.c \
        $(KERNEL)/drivers/rtl8139.c \
        $(KERNEL)/drivers/rtc.c \
        $(KERNEL)/drivers/wifi.c \
        $(KERNEL)/mm/pmm.c \
        $(KERNEL)/mm/heap.c \
        $(KERNEL)/errno.c \
        $(KERNEL)/mm/vmm.c \
        $(KERNEL)/fs/vfs.c \
        $(KERNEL)/fs/ramfs.c \
        $(KERNEL)/fs/fat32.c \
        $(KERNEL)/fs/procfs.c \
        $(KERNEL)/proc/process.c \
        $(KERNEL)/proc/scheduler.c \
        $(KERNEL)/proc/syscall.c \
        $(KERNEL)/proc/elf.c \
        $(KERNEL)/net/net.c \
        $(KERNEL)/net/netif.c \
        $(KERNEL)/net/ethernet.c \
        $(KERNEL)/net/arp.c \
        $(KERNEL)/net/ip.c \
        $(KERNEL)/net/icmp.c \
        $(KERNEL)/net/udp.c \
        $(KERNEL)/net/dns.c \
        $(KERNEL)/net/dhcp.c \
        $(KERNEL)/net/tcp.c \
        $(KERNEL)/net/socket_compat.c \
        $(KERNEL)/net/http.c \
        $(USER)/init/init.c \
        $(USER)/shell/nsh.c \
        $(USER)/libc/string.c \
        $(KERNEL)/libc.c \
        $(KERNEL)/libc_float.c \
        ports/tls/nexos_tls.c \
        ports/tls/nexos_tls_tcp.c \
        ports/tls/nexos_tls_platform.c \
        $(KERNEL)/drivers/fb.c \
        $(KERNEL)/drivers/font.c \
        $(KERNEL)/drivers/console.c \
        $(KERNEL)/drivers/mouse.c \
        $(KERNEL)/gui/wm.c \
        $(KERNEL)/gui/desktop.c \
        $(KERNEL)/gui/taskbar.c \
        $(KERNEL)/gui/launcher.c \
        $(KERNEL)/gui/term_app.c \
        $(KERNEL)/gui/files_app.c \
        $(KERNEL)/gui/sysinfo_app.c \
        $(KERNEL)/gui/theme_app.c \
        $(KERNEL)/gui/notif.c \
        $(KERNEL)/gui/browser_app.c \
        $(KERNEL)/gui/calc_app.c \
        $(KERNEL)/gui/clock_app.c \
        $(KERNEL)/gui/edit_app.c \
        $(KERNEL)/gui/viz_app.c \
        $(KERNEL)/gui/snake_app.c \
        $(KERNEL)/gui/sysmon_app.c \
        $(KERNEL)/gui/settings_app.c \
        $(KERNEL)/gui/gui.c \
        $(KERNEL)/installer/installer.c \
        $(KERNEL)/pkg/npkg.c \
        $(KERNEL)/pkg/pkg_store.c

# ---- Assembly sources ----
KERNEL_ASM_SOURCES = \
        $(KERNEL)/arch/x86_64/boot.asm \
        $(KERNEL)/arch/x86_64/gdt_flush.asm \
        $(KERNEL)/arch/x86_64/isr.asm \
        $(KERNEL)/arch/x86_64/enter_ring3.asm \
        ports/tls/trust_bundle.asm

KERNEL_C_OBJS   = $(patsubst %.c,   $(BUILD)/%.o,     $(KERNEL_C_SOURCES))
KERNEL_ASM_OBJS = $(patsubst %.asm, $(BUILD)/%.asm.o, $(KERNEL_ASM_SOURCES))
ALL_OBJS        = $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)

# ---- Targets ----
DISK_IMG = $(BUILD)/nexos_disk.img

.PHONY: all kernel iso clean run run-debug run-with-disk disk \
        install install-usb run-vbox run-vmware run-installer check

.PHONY: netsurf-sources netsurf-config netsurf-deps netsurf-adapter-check netsurf-native-objects netsurf-native-deps netsurf-native-frontend netsurf-native-link-probe netsurf-native-kernel-link tls-source tls-check tls-host-test \
		tls-freestanding native-tls test-tcp netsurf-native-kernel-test netsurf-native-run netsurf-native-gui-object

netsurf-sources:
	@chmod +x $(TOOLS)/netsurf_fetch_sources.sh
	@$(TOOLS)/netsurf_fetch_sources.sh

netsurf-config: netsurf-sources
	@cp ports/netsurf/Makefile.config $(NETSURF_SOURCE_DIR)/netsurf/Makefile.config
	@chmod +x tools/netsurf_apply_nexos_fetcher.sh
	@NETSURF_SOURCE_DIR=$(NETSURF_SOURCE_DIR) tools/netsurf_apply_nexos_fetcher.sh
	@command -v gperf >/dev/null 2>&1 || \
		{ echo "Missing host tool: gperf (required to generate libhubbub parser tables)." >&2; exit 127; }
	@echo "NetSurf first-milestone configuration installed"

netsurf-deps: netsurf-config
	@$(MAKE) -C $(NETSURF_SOURCE_DIR)/libwapcaplet \
		PREFIX=$(NETSURF_HOST_PREFIX_ABS) NSSHARED=$(NETSURF_BUILD_SYSTEM) install
	@$(MAKE) -C $(NETSURF_SOURCE_DIR)/libparserutils \
		PREFIX=$(NETSURF_HOST_PREFIX_ABS) NSSHARED=$(NETSURF_BUILD_SYSTEM) \
		PKG_CONFIG_PATH=$(NETSURF_HOST_PREFIX_ABS)/lib/pkgconfig install
	@$(MAKE) -C $(NETSURF_SOURCE_DIR)/libhubbub \
		PREFIX=$(NETSURF_HOST_PREFIX_ABS) NSSHARED=$(NETSURF_BUILD_SYSTEM) \
		PKG_CONFIG_PATH=$(NETSURF_HOST_PREFIX_ABS)/lib/pkgconfig install
	@$(MAKE) -C $(NETSURF_SOURCE_DIR)/libnsutils \
		PREFIX=$(NETSURF_HOST_PREFIX_ABS) NSSHARED=$(NETSURF_BUILD_SYSTEM) \
		PKG_CONFIG_PATH=$(NETSURF_HOST_PREFIX_ABS)/lib/pkgconfig install
	@$(MAKE) -C $(NETSURF_SOURCE_DIR)/libcss \
		PREFIX=$(NETSURF_HOST_PREFIX_ABS) NSSHARED=$(NETSURF_BUILD_SYSTEM) \
		PKG_CONFIG_PATH=$(NETSURF_HOST_PREFIX_ABS)/lib/pkgconfig install
	@$(MAKE) -C $(NETSURF_SOURCE_DIR)/libdom \
		PREFIX=$(NETSURF_HOST_PREFIX_ABS) NSSHARED=$(NETSURF_BUILD_SYSTEM) \
		PKG_CONFIG_PATH=$(NETSURF_HOST_PREFIX_ABS)/lib/pkgconfig install
	@$(MAKE) -C $(NETSURF_SOURCE_DIR)/libnsfb \
		PREFIX=$(NETSURF_HOST_PREFIX_ABS) NSSHARED=$(NETSURF_BUILD_SYSTEM) \
		PKG_CONFIG_PATH=$(NETSURF_HOST_PREFIX_ABS)/lib/pkgconfig install
	@echo "NetSurf locked host dependencies built and installed"

netsurf-adapter-check: netsurf-sources
	@chmod +x tools/check_netsurf_adapter.sh
	@NETSURF_SOURCE_DIR=$(NETSURF_SOURCE_DIR) tools/check_netsurf_adapter.sh

netsurf-native-objects: netsurf-deps netsurf-adapter-check
	@chmod +x tools/build_netsurf_native_objects.sh
	@NETSURF_SOURCE_DIR=$(NETSURF_SOURCE_DIR) NETSURF_HOST_PREFIX=$(NETSURF_HOST_PREFIX) \
		 tools/build_netsurf_native_objects.sh

netsurf-native-deps: netsurf-deps
	@chmod +x tools/build_netsurf_native_deps.sh
	@NETSURF_SOURCE_DIR=$(NETSURF_SOURCE_DIR) NETSURF_HOST_PREFIX=$(NETSURF_HOST_PREFIX) \
		 tools/build_netsurf_native_deps.sh

netsurf-native-frontend: netsurf-native-deps
	@chmod +x tools/build_netsurf_native_frontend.sh
	@NETSURF_SOURCE_DIR=$(NETSURF_SOURCE_DIR) NETSURF_HOST_PREFIX=$(NETSURF_HOST_PREFIX) \
		 tools/build_netsurf_native_frontend.sh

netsurf-native-link-probe: netsurf-native-objects netsurf-native-frontend
	@chmod +x tools/link_netsurf_native_probe.sh
	@tools/link_netsurf_native_probe.sh

netsurf-native-kernel-link: netsurf-native-link-probe netsurf-native-gui-object $(ALL_OBJS) $(TLS_LIBS)
	@echo "[LD] build/nexos.kernel.netsurf"
	$(LD) $(LDFLAGS) -o build/nexos.kernel.netsurf $(ALL_OBJS) $(TLS_LIBS) \
		--whole-archive build/netsurf-native/libnetsurf-native.a \
		build/netsurf-native/libnetsurf-native-frontend.a \
		build/netsurf-native/libwapcaplet-native.a \
		build/netsurf-native/libparserutils-native.a \
		build/netsurf-native/libhubbub-native.a \
		build/netsurf-native/libnsutils-native.a \
		build/netsurf-native/libcss-native.a \
		build/netsurf-native/libdom-native.a \
		build/netsurf-native/libnsfb-native.a \
		--no-whole-archive $(LIBGCC)

netsurf-native-gui-object:
	@mkdir -p build/kernel/gui
	@echo "[CC] kernel/gui/gui.c (native NetSurf target)"
	$(CC) $(CFLAGS) -c kernel/gui/gui.c -o build/kernel/gui/gui.o

netsurf-native-kernel-test:
	@$(MAKE) NATIVE_NETSURF_TEST=1 netsurf-native-kernel-link

netsurf-native-run: netsurf-native-kernel-link
	@KERNEL=build/nexos.kernel.netsurf tools/build_iso.sh
	@DISPLAY_MODE=none tools/run_qemu.sh

tls-source:
	@chmod +x tools/tls_fetch_source.sh
	@TLS_SOURCE_DIR=$(dir $(TLS_SOURCE_DIR)) tools/tls_fetch_source.sh

tls-check: tls-source
	@gcc -std=c11 -ffreestanding -fno-stack-protector -fno-pic \
		-mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -O2 \
		-m64 -mcmodel=kernel -nostdlib -nostdinc \
		-Iports/tls/include -I. -Ikernel -Ikernel/include -Iports/tls \
		-I$(TLS_SOURCE_DIR)/include -Wno-unused-parameter \
		-Wno-unused-variable -fsyntax-only ports/tls/nexos_tls.c \
		ports/tls/nexos_tls_tcp.c

tls-freestanding: tls-source
	@chmod +x tools/build_mbedtls_nexos.sh
	@TLS_SOURCE_DIR=$(TLS_SOURCE_DIR) TLS_CONFIG_FILE=$(TLS_CONFIG_FILE) \
		 tools/build_mbedtls_nexos.sh

tls-host-test: tls-source
	@$(MAKE) -C $(TLS_SOURCE_DIR) clean >/dev/null
	@$(MAKE) -C $(TLS_SOURCE_DIR) -j2 lib
	@gcc -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -Werror \
		-I$(TLS_SOURCE_DIR)/include -Iports/tls \
		tests/https_mbedtls_host_test.c ports/tls/nexos_tls.c \
		$(TLS_SOURCE_DIR)/library/libmbedtls.a \
		$(TLS_SOURCE_DIR)/library/libmbedx509.a \
		$(TLS_SOURCE_DIR)/library/libmbedcrypto.a -o /tmp/nexos-https-test
	@/tmp/nexos-https-test example.com ports/tls/trust/cacert.pem

native-tls: clean
	@$(MAKE) NATIVE_TLS_TEST=1 iso
	@DISPLAY_MODE=none tools/run_qemu.sh

all: kernel iso

kernel: check-host-tools $(KERNEL_ELF)

iso: kernel
	@chmod +x $(TOOLS)/build_iso.sh
	@$(TOOLS)/build_iso.sh

run: iso
	@chmod +x $(TOOLS)/run_qemu.sh
	@$(TOOLS)/run_qemu.sh

run-debug: iso
	@chmod +x $(TOOLS)/run_qemu_debug.sh
	@$(TOOLS)/run_qemu_debug.sh

disk:
	@mkdir -p $(BUILD)
	@chmod +x $(TOOLS)/create_disk.sh
	@$(TOOLS)/create_disk.sh

run-with-disk: iso disk
	@chmod +x $(TOOLS)/run_qemu.sh
	@NEXOS_USE_DISK=1 $(TOOLS)/run_qemu.sh

packages:
	@echo "[npkg] Building packages..."
	@$(PYTHON) $(TOOLS)/npkg_build.py genstore
	@echo "[npkg] Done. Re-run 'make all' to embed packages into the kernel."

.PHONY: packages

install-usb: iso
	@$(PYTHON) $(TOOLS)/install_usb.py

# -- Full installer TUI (USB / QEMU / VirtualBox / VMware) ------------------
install: iso
	@$(PYTHON) $(TOOLS)/installer.py

# -- Boot NexOS in QEMU with installer mode (nexos.install=1) ---------------
run-installer: iso
	@chmod +x $(TOOLS)/run_qemu.sh
	@NEXOS_INSTALL=1 QEMU_EXTRA="-append nexos.install=1" $(TOOLS)/run_qemu.sh

# -- VirtualBox quick-launch -------------------------------------------------
run-vbox: iso
	@chmod +x $(TOOLS)/run_vbox.sh
	@$(TOOLS)/run_vbox.sh

# -- VMware quick-launch -----------------------------------------------------
run-vmware: iso
	@chmod +x $(TOOLS)/run_vmware.sh
	@$(TOOLS)/run_vmware.sh

check:
	$(CC) $(CFLAGS) -fsyntax-only $(KERNEL_C_SOURCES)
	@echo "Syntax check passed."

check-host-tools:
	@command -v $(ASM) >/dev/null 2>&1 || \
		{ echo "Missing host tool: $(ASM) (required to assemble the kernel)." >&2; exit 127; }
	@command -v $(PYTHON) >/dev/null 2>&1 || \
		{ echo "Missing host tool: $(PYTHON)." >&2; exit 127; }

test-tcp:
	@tests/tcp_reliability_test.sh

clean:
	rm -rf $(BUILD)

# ---- Link ----
$(KERNEL_ELF): $(ALL_OBJS) $(BOOT)/linker.ld $(TLS_LIBS)
	@mkdir -p $(dir $@)
	@echo "[LD] $@"
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS) $(TLS_LIBS) $(LIBGCC)

$(TLS_LIBS): tls-freestanding

# ---- Compile C ----
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel/libc_float.o: CFLAGS += -msse -msse2 -mfpmath=sse

# ---- Assemble ----
$(BUILD)/%.asm.o: %.asm
	@mkdir -p $(dir $@)
	@echo "[ASM] $<"
	$(ASM) $(ASFLAGS) $< -o $@

-include $(ALL_OBJS:.o=.d)
