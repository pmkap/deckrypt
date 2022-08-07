// This library can be used to "type" ASCII characters with a virtual input device
// using the uinput kernel module.
// Only meant for console/tty because it types based on the kernel keymap.

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <linux/keyboard.h>
#include <libevdev/libevdev-uinput.h>
#include <keymap.h>

#include "uitype.h"

static struct libevdev *device;
static struct libevdev_uinput *udevice;

// "Reverse" keymap that maps ascii characters to event codes. The first
// column is the modifier mask and the second column is the input event code.
// Can be read as "Which modifiers and key do I need to press to produce a
// certain ASCII character?".
static uint8_t chrmap[CHRMAP_LEN_X][CHRMAP_LEN_Y];

static const uint8_t modmap[] = {
    KEY_LEFTSHIFT,
    KEY_RIGHTALT,
    KEY_LEFTCTRL,
};

static void press(uint8_t mod, uint32_t code) {
    // Press modifiers
    for (size_t i = 0; i < ARRLEN(modmap); i++) {
        if (((int)(pow(2.0, (double)i)) & mod) > 0) {
            libevdev_uinput_write_event(udevice, EV_KEY, modmap[i], 1);
            libevdev_uinput_write_event(udevice, EV_SYN, SYN_REPORT, 0);
        }
    }

    // Press key
    libevdev_uinput_write_event(udevice, EV_KEY, code, 1);
    libevdev_uinput_write_event(udevice, EV_SYN, SYN_REPORT, 0);
    libevdev_uinput_write_event(udevice, EV_KEY, code, 0);
    libevdev_uinput_write_event(udevice, EV_SYN, SYN_REPORT, 0);

    // Release modifiers
    for (size_t i = 0; i < ARRLEN(modmap); i++) {
        if (((int)(pow(2.0, (double)i)) & mod) > 0) {
            libevdev_uinput_write_event(udevice, EV_KEY, modmap[i], 0);
            libevdev_uinput_write_event(udevice, EV_SYN, SYN_REPORT, 0);
        }
    }
}

// The kernel keymap maps event codes and modifiers to symbols.
// We want to "reverse" this to map ASCII characters to event codes.
// To make this work with different keyboard layouts, we use the libkeymap API
// to check out the current keymap and generate our map from that.
// Note that the kernel keymap is independent of the Xorg/Wayland keyboard layout
// and only relevant on console/tty.
static int genchrmap() {
    char *tty = ttyname(fileno(stdin));
    int fd;
    if (tty == NULL) {
        fd = open("/dev/console", O_RDONLY);
    }
    else if (strncmp(tty, "/dev/tty", 8) == 0) {
        fd = open(tty, O_RDONLY);
    }
    else if (getuid() == 0) {
        fd = open("/dev/tty0", O_RDONLY);
    }
    else {
        fprintf(stderr, "Needs to run on tty/console or as root to read the kernel keymap. Using default US keymap.\n");
        memcpy(chrmap, default_chrmap, CHRMAP_LEN_X * CHRMAP_LEN_Y);
        return 0;
    }

	struct lk_ctx *ctx = lk_init();
	if (!ctx) {
        fprintf(stderr, "kbd initialization failed\n");
        return 1;
	}

	lk_kernel_keymap(ctx, fd);

    // Traverse the kernel keymap to look for ASCII characters and store the
    // respective event codes and modifiers in our map.
    for (int j = 0; j <= 2; j++) {
        for (int i = 1; i <= 57; i++) {
            int code = lk_get_key(ctx, j, i);
            int ktyp = KTYP(code);
            int kval = KVAL(code);
            if ((ktyp == KT_LATIN || ktyp == KT_LETTER) && chrmap[kval][1] == 0) {
                chrmap[kval][1] = (uint8_t)i;
                chrmap[kval][0] = (uint8_t)j;
            }
            else if (code == 513 && chrmap['\n'][1] == 0) {
                chrmap['\n'][1] = (uint8_t)i;
                chrmap['\n'][0] = (uint8_t)j;
            }
        }
    }

    lk_free(ctx);
    return 0;
}

// Returns 0 on success. In this case the caller is responsible to call uitype_deinit
int uitype_init() {
    if (genchrmap() != 0) {
        return 1;
    }
    device = libevdev_new();
    libevdev_set_name(device, "virtual keyboard");
    libevdev_enable_event_type(device, EV_KEY);
    for (uint8_t i = 0; i < 255; i++) {
        libevdev_enable_event_code(device, EV_KEY, i, NULL);
    }

    int rc = libevdev_uinput_create_from_device(device, LIBEVDEV_UINPUT_OPEN_MANAGED, &udevice);
    if (rc != 0) {
        libevdev_free(device);
        fprintf(stderr, "Error: Couldn't create libevdev uinput device\n");
        return rc;
    }

    return 0;
}

void uitype_deinit() {
    libevdev_uinput_destroy(udevice);
    libevdev_free(device);
}

void uitype_type(char *string) {
    for (size_t i = 0; i < strlen(string); i++) {
        press(chrmap[(size_t)string[i]][0], chrmap[(size_t)string[i]][1]);
        // Give the consumer a bit time to process before the next key comes.
        usleep(1000);
    }
}

void uitype_enter() {
    press(0, KEY_ENTER);
}

void uitype_ctrlu() {
    press(4, KEY_U);
}
