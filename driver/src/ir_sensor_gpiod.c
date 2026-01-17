#include "ir_sensor.h"

#include <gpiod.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

static volatile sig_atomic_t g_shutdown_requested = 0;

void ir_sensor_request_shutdown(void) {
    g_shutdown_requested = 1;
}

bool ir_sensor_shutdown_requested(void) {
    return g_shutdown_requested != 0;
}

struct IrSensor {
    struct gpiod_chip* chip;
    struct gpiod_line* line;
    unsigned int gpio_line;
    int active_low;
    int debounce_ms;

    long long last_event_ms;
    int last_reported_detected; // -1 unknown, 0/1 known
};

static long long now_ms_monotonic(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + (ts.tv_nsec / 1000000LL);
}

static int logical_detected(const IrSensor* s, int raw) {
    // active_low: detected when raw==0, else detected when raw==1
    if (s->active_low) return (raw == 0);
    return (raw == 1);
}

IrSensor* ir_sensor_open(const char* chip_name,
                         unsigned int gpio_line,
                         int active_low,
                         int debounce_ms)
{
    if (!chip_name) {
        fprintf(stderr, "ir_sensor_open: chip_name is NULL\n");
        return NULL;
    }
    if (debounce_ms < 0) debounce_ms = 0;

    IrSensor* s = (IrSensor*)calloc(1, sizeof(IrSensor));
    if (!s) {
        perror("calloc");
        return NULL;
    }

    s->gpio_line = gpio_line;
    s->active_low = active_low ? 1 : 0;
    s->debounce_ms = debounce_ms;
    s->last_event_ms = -1000000;
    s->last_reported_detected = -1;

    s->chip = gpiod_chip_open_by_name(chip_name);
    if (!s->chip) {
        perror("gpiod_chip_open_by_name");
        free(s);
        return NULL;
    }

    s->line = gpiod_chip_get_line(s->chip, (unsigned int)gpio_line);
    if (!s->line) {
        perror("gpiod_chip_get_line");
        gpiod_chip_close(s->chip);
        free(s);
        return NULL;
    }

    // Request both edge events for interrupt-style changes
    if (gpiod_line_request_both_edges_events(s->line, "ir_sensor") < 0) {
        perror("gpiod_line_request_both_edges_events");
        gpiod_chip_close(s->chip);
        free(s);
        return NULL;
    }

    return s;
}

void ir_sensor_close(IrSensor* s) {
    if (!s) return;
    if (s->line) gpiod_line_release(s->line);
    if (s->chip) gpiod_chip_close(s->chip);
    free(s);
}

bool ir_sensor_wait_change(IrSensor* s, bool* detected_out, int* raw_out) {
    if (!s || !detected_out) {
        errno = EINVAL;
        return false;
    }

    struct gpiod_line_event event;

    while (1) {
        int ret = gpiod_line_event_wait(s->line, NULL); // wait forever
        if (g_shutdown_requested) {
            errno = EINTR;
            return false;
        }
        if (ret < 0) {
            perror("gpiod_line_event_wait");
            return false;
        }
        if (ret == 0) {
            continue;
        }

        if (gpiod_line_event_read(s->line, &event) < 0) {
            perror("gpiod_line_event_read");
            return false;
        }

        long long t = now_ms_monotonic();
        long long dt = t - s->last_event_ms;

        // Debounce: ignore edges too close
        if (dt < s->debounce_ms) {
            continue;
        }
        s->last_event_ms = t;

        // settling delay (helps with noisy sensors)
        usleep(5 * 1000);

        int raw = gpiod_line_get_value(s->line);
        if (raw < 0) {
            perror("gpiod_line_get_value");
            return false;
        }

        int detected = logical_detected(s, raw);

        // only emit on actual logical state change
        if (detected == s->last_reported_detected) {
            continue;
        }
        s->last_reported_detected = detected;

        if (raw_out) *raw_out = raw;
        *detected_out = detected ? true : false;
        return true;
    }
}
