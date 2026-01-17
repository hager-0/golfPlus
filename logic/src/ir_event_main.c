#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include "ir_sensor.h"

static volatile sig_atomic_t running = 1;
static IrSensor* g_sensor = NULL;

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

static void print_ts(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    printf("[%ld.%03ld] ", ts.tv_sec, ts.tv_nsec / 1000000);
}

static void usage(const char* prog) {
    printf("Usage:\n");
    printf("  %s [--chip gpiochip0] [--line 17] [--active-low 1|0] [--debounce-ms N]\n", prog);
    printf("\nExamples:\n");
    printf("  %s --chip gpiochip0 --line 17 --active-low 1 --debounce-ms 50\n", prog);
}

static int parse_int_arg(int argc, char** argv, int* i, const char* name, int* out) {
    if (*i + 1 >= argc) {
        fprintf(stderr, "Missing value for %s\n", name);
        return 0;
    }
    *out = atoi(argv[*i + 1]);
    *i += 1;
    return 1;
}

static const char* parse_str_arg(int argc, char** argv, int* i, const char* name) {
    if (*i + 1 >= argc) {
        fprintf(stderr, "Missing value for %s\n", name);
        return NULL;
    }
    *i += 1;
    return argv[*i];
}

int main(int argc, char** argv) {
    const char* chip = "gpiochip0";
    int line = 17;
    int active_low = 1;
    int debounce_ms = 50;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            usage(argv[0]);
            return 0;
        } else if (!strcmp(argv[i], "--chip")) {
            const char* v = parse_str_arg(argc, argv, &i, "--chip");
            if (!v) return 1;
            chip = v;
        } else if (!strcmp(argv[i], "--line")) {
            if (!parse_int_arg(argc, argv, &i, "--line", &line)) return 1;
        } else if (!strcmp(argv[i], "--active-low")) {
            if (!parse_int_arg(argc, argv, &i, "--active-low", &active_low)) return 1;
            active_low = active_low ? 1 : 0;
        } else if (!strcmp(argv[i], "--debounce-ms")) {
            if (!parse_int_arg(argc, argv, &i, "--debounce-ms", &debounce_ms)) return 1;
            if (debounce_ms < 0) debounce_ms = 0;
        } else {
            fprintf(stderr, "Unknown arg: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    printf("Config: chip=%s line=%d active_low=%d debounce_ms=%d\n",
           chip, line, active_low, debounce_ms);

    g_sensor = ir_sensor_open(chip, (unsigned int)line, active_low, debounce_ms);
    if (!g_sensor) {
        fprintf(stderr, "Failed to open IR sensor.\n");
        return 1;
    }

    // Register signal handlers for graceful shutdown
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    printf("Waiting for debounced state changes... (Ctrl+C to exit)\n");

    while (running) {
        bool detected = false;
        int raw = -1;
        if (!ir_sensor_wait_change(g_sensor, &detected, &raw)) {
            if (!running) break;  // Exit was requested
            fprintf(stderr, "Error while waiting for change.\n");
            break;
        }
        print_ts();
        printf("RAW=%d -> %s\n", raw, detected ? "OBJECT DETECTED" : "NO OBJECT");
        fflush(stdout);
    }

    printf("\nShutting down gracefully...\n");
    ir_sensor_close(g_sensor);
    g_sensor = NULL;
    printf("GPIO released. Goodbye!\n");

    return 0;
}
