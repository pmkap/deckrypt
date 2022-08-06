#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <libevdev/libevdev.h>

#include "uitype.h"

// Threshold after a button becomes a hold-button (milliseconds)
#define THRESHOLD 250

// Set this to fit the longest name in the buttons struct below
#define MAX_NAME_LEN 5

typedef struct Button {
    // linux event code
    uint16_t code;
    char name[MAX_NAME_LEN + 1];
    bool pressed;
    int64_t time_pressed;
    int64_t time_released;
} Button;

typedef struct Axis {
    // linux event code
    uint16_t code;
    // An axis can be "translated" to two buttons. E.g. the horizontal D-pad axis
    // to buttons for left and right. These two buttons correspond to X and Y here.
    char nameX[MAX_NAME_LEN + 1];
    char nameY[MAX_NAME_LEN + 1];
    // press
    int16_t valueX;
    int16_t valueY;
    // release
    int16_t value0;
} Axis;

// Use evtest to find which events are supported by a controller.
// Buttons not listed here are ignored.
#define ENTER_BUTTON BTN_BASE4
#define CLEAR_BUTTON BTN_BASE3
static Button buttons[] = {
    { .code = BTN_TRIGGER, .name = "TRIG",  .pressed = false },
    { .code = BTN_THUMB,   .name = "THMB",  .pressed = false },
    { .code = BTN_THUMB2,  .name = "THMB2", .pressed = false },
    { .code = BTN_TOP,     .name = "TOP",   .pressed = false },
    { .code = BTN_TOP2,    .name = "TOP2",  .pressed = false },
    { .code = BTN_PINKIE,  .name = "PINK",  .pressed = false },
    { .code = BTN_BASE,    .name = "BASE",  .pressed = false },
    { .code = BTN_BASE2,   .name = "BASE2", .pressed = false },
    { .code = BTN_BASE5,   .name = "BASE5", .pressed = false },
    { .code = BTN_BASE6,   .name = "BASE6", .pressed = false },
    // These "buttons" are translated from the axes below, names have to match
    { .code = 0xffff,      .name = "D-RI",  .pressed = false },
    { .code = 0xffff,      .name = "D-LE",  .pressed = false },
    { .code = 0xffff,      .name = "D-UP",  .pressed = false },
    { .code = 0xffff,      .name = "D-DO",  .pressed = false },
    { .code = 0xffff,      .name = "L-RI",  .pressed = false },
    { .code = 0xffff,      .name = "L-LE",  .pressed = false },
    { .code = 0xffff,      .name = "L-UP",  .pressed = false },
    { .code = 0xffff,      .name = "L-DO",  .pressed = false },
    { .code = 0xffff,      .name = "R-RI",  .pressed = false },
    { .code = 0xffff,      .name = "R-LE",  .pressed = false },
    { .code = 0xffff,      .name = "R-UP",  .pressed = false },
    { .code = 0xffff,      .name = "R-DO",  .pressed = false },
};

static Axis axes[] = {
    {
        .code = ABS_HAT0X,
        .nameX = "D-LE", .nameY = "D-RI",
        .valueX = -1, .value0 = 0, .valueY = 1
    },
    {
        .code = ABS_HAT0Y,
        .nameX = "D-UP", .nameY = "D-DO",
        .valueX = -1, .value0 = 0, .valueY = 1
    },
    {
        .code = ABS_X,
        .nameX = "L-LE", .nameY = "L-RI",
        .valueX = 0, .value0 = 128, .valueY = 255
    },
    {
        .code = ABS_Y,
        .nameX = "L-UP", .nameY = "L-DO",
        .valueX = 0, .value0 = 128, .valueY = 255
    },
    {
        .code = ABS_Z,
        .nameX = "R-LE", .nameY = "R-RI",
        .valueX = 0, .value0 = 128, .valueY = 255
    },
    {
        .code = ABS_RZ,
        .nameX = "R-UP", .nameY = "R-DO",
        .valueX = 0, .value0 = 128, .valueY = 255
    },
};

static const size_t n_buttons = sizeof(buttons) / sizeof(Button);
static const size_t n_axes = sizeof(axes) / sizeof(Axis);

static struct timeval start_time;

// Convert the timestamp of an event to milliseconds since the program started
static int64_t time2millis(struct timeval t) {
    int64_t secs_diff = t.tv_sec - start_time.tv_sec;
    int64_t usecs_diff;
    if (t.tv_usec >= start_time.tv_usec) {
        usecs_diff = t.tv_usec - start_time.tv_usec;
    }
    else {
        usecs_diff = 1000000 - (start_time.tv_usec - t.tv_usec);
        secs_diff--;
    }
    return secs_diff * 1000 + usecs_diff / 1000;
}

static Button* button_from_code(uint16_t code) {
    for (size_t i = 0; i < n_buttons; i++) {
        if (code == buttons[i].code) {
            return &buttons[i];
        }
    }
    return NULL;
}

static Button* button_from_name(char *name) {
    for (size_t i = 0; i < n_buttons; i++) {
        if (strncmp(name, buttons[i].name, MAX_NAME_LEN) == 0) {
            return &buttons[i];
        }
    }
    return NULL;
}

// Argument Button *b: The released button that concludes the combination
static void combination(Button *b) {
    char buffer[n_buttons * (MAX_NAME_LEN + 1)];
    buffer[0] = '\0';
    // Check for hold-buttons
    // A button becomes a hold-button if it
    // * is pressed for at least the threshold time, and
    // * was pressed before the released button b.
    for (size_t i = 0; i < n_buttons; i++) {
        if (buttons[i].pressed &&
                b->time_released - buttons[i].time_pressed > THRESHOLD &&
                b->time_pressed > buttons[i].time_pressed) {
            if (strnlen(buffer, 1) == 0) {
                strcat(buffer, buttons[i].name);
            }
            else {
                strcat(buffer, "+");
                strcat(buffer, buttons[i].name);
            }
        }
    }
    if (strnlen(buffer, 1) == 0) {
        strcat(buffer, b->name);
    }
    else {
        strcat(buffer, "+");
        strcat(buffer, b->name);
    }

    strcat(buffer, ";");
    uitype_type(buffer);
}

static void handle_button(struct input_event *ev) {
    Button *button = button_from_code(ev->code);
    if (button == NULL) return;

    if (ev->value == 1) {
        button->pressed = true;
        button->time_pressed = time2millis(ev->time);
    }
    else if (ev->value == 0) {
        button->pressed = false;
        button->time_released = time2millis(ev->time);
        if (button->time_released - button->time_pressed < THRESHOLD) {
            combination(button);
        }
    }
}

static void handle_axis(struct input_event *ev) {
    for (size_t i = 0; i < n_axes; i++) {
        if (ev->code == axes[i].code) {
            Button *button;
            if (ev->value == axes[i].valueX) {
                button = button_from_name(axes[i].nameX);
                button->pressed = true;
                button->time_pressed = time2millis(ev->time);
            } else if (ev->value == axes[i].valueY) {
                button = button_from_name(axes[i].nameY);
                button->pressed = true;
                button->time_pressed = time2millis(ev->time);
            } else if (ev->value == axes[i].value0) {
                // One of the two "buttons" of the axis was released
                button = button_from_name(axes[i].nameX);
                if (button->pressed) {
                    button->pressed = false;
                    button->time_released = time2millis(ev->time);
                    if (button->time_released - button->time_pressed < THRESHOLD) {
                        combination(button);
                        return;
                    }
                }

                button = button_from_name(axes[i].nameY);
                button->pressed = false;
                button->time_released = time2millis(ev->time);
                if (button->time_released - button->time_pressed < THRESHOLD) {
                    combination(button);
                }
            }
        }
    }
}

static int is_event_device(const struct dirent *dir) {
    return strncmp("event", dir->d_name, 5) == 0;
}

static bool find_device(struct libevdev** device) {
    struct dirent **namelist;
    int n_devices = scandir("/dev/input/", &namelist, is_event_device, NULL);
    for (int i = 0; i < n_devices; i++) {
        char *device_path = calloc(strlen("/dev/input/") + namelist[i]->d_reclen + 1,
                sizeof(char));
        sprintf(device_path, "%s%s", "/dev/input/", namelist[i]->d_name);

        int fd = open(device_path, O_RDONLY|O_NONBLOCK);
        free(device_path);

        if (libevdev_new_from_fd(fd, device) == 0) {
            if (libevdev_has_event_code(*device, EV_KEY, ENTER_BUTTON)) {
                for (int j = i; j < n_devices; j++) {
                    free(namelist[j]);
                }
                free(namelist);
                return true;
            }
            else {
                libevdev_free(*device);
                *device = NULL;
            }
        }
        close(fd);
        free(namelist[i]);
    }
    free(namelist);
    return false;
}


static volatile bool terminate = false;
static void signal_handler(int signum) { terminate = true; }

int main() {
    gettimeofday(&start_time, NULL);

    if (uitype_init() != 0) {
        return 1;
    };

    struct libevdev *device = NULL;
    while (true) {
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        if (terminate) break;

        if (device == NULL) {
            find_device(&device);
        }
        else {
            struct input_event ev;
            int rc = libevdev_next_event(device, LIBEVDEV_READ_FLAG_NORMAL, &ev);
            if (rc == 0) {
                switch (ev.type) {
                    case EV_KEY:
                        if (ev.value == 0) {
                            if (ev.code == ENTER_BUTTON) {
                                uitype_enter();
                            }
                            else if (ev.code == CLEAR_BUTTON) {
                                uitype_ctrlu();
                            }
                        }
                        handle_button(&ev);
                        break;
                    case EV_ABS:
                        handle_axis(&ev);
                        break;
                }
            }
        }
    }

    // Cleanup
    if (device != NULL) {
        libevdev_free(device);
    }
    uitype_deinit();

    return 0;
}
