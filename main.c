/**
 * Tony Givargis
 * Copyright (C), 2023-2024
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * main.c
 */

#include <signal.h>
#include "system.h"

/**
 * Needs:
 *   signal()
 */

static volatile int done;

static void
_signal_(int signum)
{
    assert(SIGINT == signum);

    done = 1;
}

double
cpu_util(const char *s)
{
    static unsigned sum_, vector_[7];
    unsigned sum, vector[7];
    const char *p;
    double util;
    uint64_t i;

    /*
      user
      nice
      system
      idle
      iowait
      irq
      softirq
    */

    if (!(p = strstr(s, " ")) ||
        (7 != sscanf(p,
                     "%u %u %u %u %u %u %u",
                     &vector[0],
                     &vector[1],
                     &vector[2],
                     &vector[3],
                     &vector[4],
                     &vector[5],
                     &vector[6])))
    {
        return 0;
    }
    sum = 0.0;
    for (i = 0; i < ARRAY_SIZE(vector); ++i)
    {
        sum += vector[i];
    }
    util = (1.0 - (vector[3] - vector_[3]) / (double)(sum - sum_)) * 100.0;
    sum_ = sum;
    for (i = 0; i < ARRAY_SIZE(vector); ++i)
    {
        vector_[i] = vector[i];
    }
    return util;
}

double memory_util()
{
    FILE *file;
    char line[1024];
    double memory_used = 0;
    unsigned long memory_total = 0, memory_buffer = 0, memory_cached = 0, memory_free = 0;

    /* Open the /proc/meminfo file */
    file = fopen("/proc/meminfo", "r");

    if (!file)
    {
        TRACE("File cannot be opened");
    }

    /* Read all the memory parameters to calculate used memory */
    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "MemTotal: %lu kB", &memory_total) == 1)
            continue;
        if (sscanf(line, "MemFree: %lu kB", &memory_free) == 1)
            continue;
        if (sscanf(line, "Buffers: %lu kB", &memory_buffer) == 1)
            continue;
        if (sscanf(line, "Cached: %lu kB", &memory_cached) == 1)
            break;
    }
    fclose(file);

    if (memory_total == 0)
        return -1;

    /* Calculate available memory */
    memory_used = memory_total - (memory_buffer + memory_free + memory_cached);
    return (memory_used / memory_total) * 100;
}

unsigned long interrrupt_count()
{
    FILE *file;
    char line[1024];
    unsigned long total_interrupts = 0;
    unsigned long count = 0;
    unsigned long interrupt_count = 0;
    char *token;
    char *endptr;

    /* Open the /proc/interrupts file */
    if (!(file = fopen("/proc/interrupts", "r")))
    {
        perror("fopen");
        return -1;
    }

    /* Read and parse the file line by line */
    while (fgets(line, sizeof(line), file))
    {
        if (line[0] == ' ' || line[0] == '\n')
            continue; /*Skip empty or irrelevant lines*/

        token = strtok(line, " "); /*Split the line by spaces*/

        /*Skip the interrupt ID (first token)*/
        token = strtok(NULL, " ");

        /*Iterate through CPU interrupt counts*/
        while (token)
        {

            count = strtoul(token, &endptr, 10);
            if (*endptr == '\0') /*If it's a valid number*/
            {
                interrupt_count += count;
            }
            token = strtok(NULL, " ");
        }
        total_interrupts += interrupt_count;
    }
    fclose(file);

    return total_interrupts;
}

int main(int argc, char *argv[])
{
    const char *const PROC_STAT = "/proc/stat";
    char line[1024];
    FILE *file;

    UNUSED(argc);
    UNUSED(argv);

    if (SIG_ERR == signal(SIGINT, _signal_))
    {
        TRACE("signal()");
        return -1;
    }
    while (!done)
    {
        if (!(file = fopen(PROC_STAT, "r")))
        {
            TRACE("fopen()");
            return -1;
        }
        if (fgets(line, sizeof(line), file))
        {
            double cpu = cpu_util(line);
            double memory = memory_util();
            unsigned long interrupts = interrrupt_count();

            printf("\rCPU: %5.1f%%, Memory: %5.1f%%, Interrupts: %lu", cpu, memory, interrupts);
            fflush(stdout);
        }
        us_sleep(500000);
        fclose(file);
    }
    printf("\rDone!   \n");
    return 0;
}
