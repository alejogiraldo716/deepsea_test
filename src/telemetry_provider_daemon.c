/**
 * @file telemetry_provider_daemon.c
 * @brief Telemetry Provider Daemon - reads system metrics and broadcasts via D-Bus.
 *
 * Reads CPU utilization, RAM utilization, and SoC temperature from the Linux
 * pseudo-filesystem, then emits them as a D-Bus signal on the System Bus at the
 * highest achievable rate toward the 2 kHz target.
 */
// #define _POSIX_C_SOURCE 200809L // for nanosleep and clock_gettime

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

/* ── D-Bus constants ─────────────────────────────────────────────────── */
#define DBUS_BUS_NAME "com.deepsea.TelemetryProvider"
#define DBUS_OBJECT_PATH "/com/deepsea/Telemetry"
#define DBUS_INTERFACE "com.deepsea.Telemetry"
#define DBUS_SIGNAL_NAME "MetricsBroadcast"

/* ── Sampling configuration ──────────────────────────────────────────── */
/*
 * Target: 2000 Hz → 500 us period.
 * In practice, D-Bus round-trip overhead on the Pi Zero W (~1–3 ms per call)
 * makes 2 kHz unachievable at the IPC layer.  The provider samples and emits
 * as fast as the bus allows; latency profiling in the dashboard will expose the
 * real throughput to the customer.
 */
#define SAMPLE_PERIOD_NS 500000L /* 500 us → 2 000 Hz nominal */

/* ── /proc/stat snapshot ─────────────────────────────────────────────── */
typedef struct
{
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
} cpu_stat_t;

/* ── Hardware readers ────────────────────────────────────────────────── */

/**
 * @brief Read a single CPU stat snapshot from /proc/stat.
 * @param[out] out  Destination struct.
 * @return 0 on success, -1 on failure.
 */
static int read_cpu_stat(cpu_stat_t *out)
{
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp)
        return -1;

    int ret = fscanf(fp, "cpu  %lu %lu %lu %lu %lu %lu %lu",
                     &out->user, &out->nice, &out->system,
                     &out->idle, &out->iowait, &out->irq, &out->softirq);
    fclose(fp);
    return (ret == 7) ? 0 : -1;
}

/**
 * @brief Calculate CPU utilization (%) between two snapshots.
 * @param prev  Earlier snapshot.
 * @param curr  Later snapshot.
 * @return CPU usage in [0.0, 100.0].
 */
static double calc_cpu_usage(const cpu_stat_t *prev, const cpu_stat_t *curr)
{
    uint64_t prev_idle = prev->idle + prev->iowait;
    uint64_t curr_idle = curr->idle + curr->iowait;

    uint64_t prev_total = prev->user + prev->nice + prev->system +
                          prev->idle + prev->iowait + prev->irq + prev->softirq;
    uint64_t curr_total = curr->user + curr->nice + curr->system +
                          curr->idle + curr->iowait + curr->irq + curr->softirq;

    uint64_t delta_total = curr_total - prev_total;
    uint64_t delta_idle = curr_idle - prev_idle;

    if (delta_total == 0)
        return 0.0;
    return 100.0 * (double)(delta_total - delta_idle) / (double)delta_total;
}

/**
 * @brief Read RAM utilization (%) from /proc/meminfo.
 * @return RAM usage in [0.0, 100.0], or -1.0 on error.
 */
static double read_ram_usage(void)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp)
        return -1.0;

    uint64_t mem_total = 0, mem_available = 0;
    char line[128];

    while (fgets(line, sizeof(line), fp))
    {
        if (sscanf(line, "MemTotal: %lu kB", &mem_total) == 1)
            continue;
        if (sscanf(line, "MemAvailable: %lu kB", &mem_available) == 1)
            continue;
        if (mem_total && mem_available)
            break;
    }
    fclose(fp);

    if (mem_total == 0)
        return -1.0;
    return 100.0 * (double)(mem_total - mem_available) / (double)mem_total;
}

/**
 * @brief Read SoC core temperature (°C) from the thermal subsystem.
 * @return Temperature in °C, or -1.0 on error.
 */
static double read_temperature(void)
{
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!fp)
        return -1.0;

    int raw = 0;
    fscanf(fp, "%d", &raw);
    fclose(fp);
    return (double)raw / 1000.0;
}

/* ── Time helpers ────────────────────────────────────────────────────── */

/**
 * @brief Return current monotonic time as nanoseconds.
 */
static int64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

/* ── Main ────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("Telemetry Provider Daemon — starting...\n");

    int64_t t0 = now_ns();

    cpu_stat_t prev_stat, curr_stat;
    if (read_cpu_stat(&prev_stat) != 0)
    {
        fprintf(stderr, "Cannot read /proc/stat\n");
        return EXIT_FAILURE;
    }

    /* Small delay so the first delta is meaningful */
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000L};
    nanosleep(&ts, NULL);

    read_cpu_stat(&curr_stat);
    double cpu_pct = calc_cpu_usage(&prev_stat, &curr_stat);
    double ram_pct = read_ram_usage();
    double temp_c = read_temperature();

    int64_t elapsed = now_ns() - t0;

    printf("CPU  : %.2f%%\n", cpu_pct);
    printf("RAM  : %.2f%%\n", ram_pct);
    printf("Temp : %.2f C (%.2f if -1.0 means no thermal sensor)\n",
           temp_c, temp_c);
    printf("Elapsed : %.3f ms\n", (double)elapsed / 1000000.0);

    /* TODO: connect to D-Bus System Bus and start sampling loop */

    return EXIT_SUCCESS;
}