#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define BLOCK_SIZE     100
#define BUFFER_CAPACITY 150   /* finite buffer to simulate real memory limit */

#define MISSING_MARKER   -1
#define CORRUPTED_MARKER -2

/* ── circular buffer ─────────────────────────────────────────── */
typedef struct {
    int  data[BUFFER_CAPACITY];
    int  head, tail, count;
    int  total_dropped;
    int  total_arrived;
} CircularBuffer;

void cb_init(CircularBuffer *cb) {
    cb->head = cb->tail = cb->count = cb->total_dropped = cb->total_arrived = 0;
}

int cb_push(CircularBuffer *cb, int val) {
    cb->total_arrived++;
    if (cb->count >= BUFFER_CAPACITY) {
        cb->total_dropped++;
        return 0;   /* drop */
    }
    cb->data[cb->tail] = val;
    cb->tail = (cb->tail + 1) % BUFFER_CAPACITY;
    cb->count++;
    return 1;
}

int cb_pop(CircularBuffer *cb, int *val) {
    if (cb->count == 0) return 0;
    *val = cb->data[cb->head];
    cb->head = (cb->head + 1) % BUFFER_CAPACITY;
    cb->count--;
    return 1;
}

/* ── sample generator ────────────────────────────────────────── */
int generate_sample(void) {
    int r = rand() % 110;
    if (r > 104) return MISSING_MARKER;
    if (r > 100) return CORRUPTED_MARKER;
    return r;
}

/* ── statistics ──────────────────────────────────────────────── */
typedef struct {
    int    max_val, min_val, missing, corrupted, valid_count;
    double mean, std_dev;
} BlockStats;

BlockStats compute_stats(int *block, int size) {
    BlockStats s = {0, 100, 0, 0, 0, 0.0, 0.0};
    long   sum  = 0;
    double sum2 = 0.0;
    int    first = 1;
    for (int i = 0; i < size; i++) {
        if (block[i] == MISSING_MARKER)   { s.missing++;   continue; }
        if (block[i] == CORRUPTED_MARKER) { s.corrupted++; continue; }
        int v = block[i]; s.valid_count++;
        sum += v; sum2 += (double)v * v;
        if (first) { s.max_val = s.min_val = v; first = 0; }
        else { if (v > s.max_val) s.max_val = v;
               if (v < s.min_val) s.min_val = v; }
    }
    if (s.valid_count > 0) {
        s.mean = (double)sum / s.valid_count;
        double var = sum2 / s.valid_count - s.mean * s.mean;
        s.std_dev = sqrt(var < 0 ? 0 : var);
    }
    return s;
}

/* ── scenario runner ─────────────────────────────────────────── */
void run_scenario(const char *label, int arrive_per_tick,
                  int process_per_tick, int ticks) {

    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  Scenario: %-41s║\n", label);
    printf("║  Arrival: %3d/tick   Processing: %3d/tick            ║\n",
           arrive_per_tick, process_per_tick);
    printf("╚══════════════════════════════════════════════════════╝\n");

    CircularBuffer cb;
    cb_init(&cb);

    int block[BLOCK_SIZE];
    int block_pos     = 0;
    int blocks_done   = 0;
    int latency_ticks = 0;   /* ticks the buffer was non-empty */

    for (int t = 1; t <= ticks; t++) {

        /* --- producer: push 'arrive_per_tick' new samples --- */
        for (int i = 0; i < arrive_per_tick; i++)
            cb_push(&cb, generate_sample());

        /* --- consumer: process 'process_per_tick' samples --- */
        int processed_this_tick = 0;
        int val;
        while (processed_this_tick < process_per_tick && cb_pop(&cb, &val)) {
            block[block_pos++] = val;
            processed_this_tick++;

            if (block_pos == BLOCK_SIZE) {
                blocks_done++;
                BlockStats s = compute_stats(block, BLOCK_SIZE);
                printf("  [tick %3d] Block %d | Max:%3d Min:%3d "
                       "Mean:%5.1f Std:%4.1f "
                       "Missing:%d Corrupted:%d\n",
                       t, blocks_done,
                       s.max_val, s.min_val,
                       s.mean, s.std_dev,
                       s.missing, s.corrupted);
                block_pos = 0;
            }
        }

        if (cb.count > 0) latency_ticks++;

        /* print buffer status every 10 ticks */
        if (t % 10 == 0) {
            int cpu_pct = (int)((double)arrive_per_tick /
                                process_per_tick * 100);
            if (cpu_pct > 100) cpu_pct = 100;
            printf("  [tick %3d] Buffer:%3d/%-3d  CPU~%3d%%  "
                   "Dropped so far:%d\n",
                   t, cb.count, BUFFER_CAPACITY, cpu_pct,
                   cb.total_dropped);
        }
    }

    /* ── summary ── */
    printf("\n  ── Summary ──────────────────────────────────────\n");
    printf("  Total arrived   : %d\n", cb.total_arrived);
    printf("  Blocks completed: %d\n", blocks_done);
    printf("  Dropped samples : %d  (%.1f%%)\n",
           cb.total_dropped,
           cb.total_arrived > 0
               ? 100.0 * cb.total_dropped / cb.total_arrived : 0.0);
    printf("  Ticks with backlog (latency indicator): %d / %d\n",
           latency_ticks, ticks);

    if (cb.total_dropped == 0 && latency_ticks < ticks / 4)
        printf("  Result: HEALTHY - system kept up with data rate\n");
    else if (cb.total_dropped == 0)
        printf("  Result: WARNING - latency building but no data loss yet\n");
    else
        printf("  Result: OVERLOADED - data loss occurred!\n");
}

/* ── main ────────────────────────────────────────────────────── */
int main(void) {
    srand((unsigned)time(NULL));

    printf("EN5500 - Part (b): High Data Rate Performance Analysis\n");
    printf("Buffer capacity: %d samples | Block size: %d\n",
           BUFFER_CAPACITY, BLOCK_SIZE);

    /*  arrive_per_tick, process_per_tick, ticks  */
    run_scenario("NORMAL RATE  (arrival < processing)",  5, 10, 30);
    run_scenario("HIGH RATE    (arrival = processing)", 10, 10, 30);
    run_scenario("OVERLOAD     (arrival > processing)", 18, 10, 30);
    run_scenario("SEVERE OVERLOAD (2x arrival rate)",   20,  5, 30);

    return 0;
}