#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <linux/input-event-codes.h>
#include <libevdev-1.0/libevdev/libevdev.h>

// TODO: The device name is hardcoded for now.
// The name is not guaranteed to stay the same and
// some kind of device detection would be preferable.
#define DEVICE "/dev/input/event8"

// Threshold after a button becomes a hold-button (milliseconds)
#define THRESHOLD 250

// Set this to fit the longest name in the buttons struct below
#define MAX_NAME_LEN 5

typedef struct Button {
    uint16_t code;
    char name[MAX_NAME_LEN + 1];
    bool pressed;
    int64_t time_pressed;
    int64_t time_released;
} Button;

// Use evtest to find which events are supported by a controller.
// Buttons not listed here are ignored.
#define CONFIRM_BUTTON BTN_BASE4
#define ABORT_BUTTON BTN_BASE3
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
    { .code = BTN_BASE6,   .name = "BASE6", .pressed = false }
};

static const size_t n_buttons = sizeof(buttons) / sizeof(Button);

static struct timeval start_time;

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

// Print a controller combination to stdout
// arg Button *b: The released button that concludes the combination
static void print_combination(Button *b) {
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
        strcat(buffer, " ");
        strcat(buffer, b->name);
    }
    printf("%s;", buffer);
    fflush(stdout);
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
            print_combination(button);
        }
    }
}

int main () {
    gettimeofday(&start_time, NULL);

    int rc = 1;
    int fd = open(DEVICE, O_RDONLY|O_NONBLOCK);

    struct libevdev *device = NULL;
    rc = libevdev_new_from_fd(fd, &device);
    if (rc < 0) {
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
        return 1;
    }

    // Make sure that at least the abort button is available to avoid locking in.
    if (!libevdev_has_event_code(device, EV_KEY, ABORT_BUTTON)) {
        libevdev_free(device);
        fprintf(stderr, "No suitable controller device available\n");
        return 1;
    }

    do {
        struct input_event ev;
        rc = libevdev_next_event(device, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == 0) {
            switch (ev.type) {
                case EV_KEY:
                    if (ev.value == 0) {
                        if (ev.code == CONFIRM_BUTTON) {
                            libevdev_free(device);
                            return 0;
                        }
                        else if (ev.code == ABORT_BUTTON) {
                            libevdev_free(device);
                            fprintf(stderr, "\nController input aborted by user\n");
                            return 1;
                        }
                    }
                    handle_button(&ev);
                    break;
                case EV_ABS:
                    // Idea: Handle D-Pad directions like buttons
                    break;
            }
        }
    } while (rc == 1 || rc == 0 || rc == -EAGAIN);

    libevdev_free(device);
    fprintf(stderr, "%s\n", strerror(-rc));
    return 1;
}
