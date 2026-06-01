#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define BLOCK_SIZE 100

/* sentinel values */
#define MISSING_MARKER    -1
#define CORRUPTED_MARKER  -2
#define DELAYED_MARKER    -3   /* slot reserved but sample hasn't arrived yet */

typedef struct {
    int    max_val, min_val;
    double mean, std_dev;
    int    missing, corrupted, delayed, valid_count;
} BlockStats;

BlockStats compute_stats(int *block, int size) {
    BlockStats s = {0, 100, 0.0, 0.0, 0, 0, 0, 0};
    long   sum  = 0;
    double sum2 = 0.0;
    int    first = 1;
    for (int i = 0; i < size; i++) {
        if (block[i] == MISSING_MARKER)   { s.missing++;   continue; }
        if (block[i] == CORRUPTED_MARKER) { s.corrupted++; continue; }
        if (block[i] == DELAYED_MARKER)   { s.delayed++;   continue; }
        int v = block[i]; s.valid_count++;
        sum += v; sum2 += (double)v * v;
        if (first) { s.max_val = s.min_val = v; first = 0; }
        else {
            if (v > s.max_val) s.max_val = v;
            if (v < s.min_val) s.min_val = v;
        }
    }
    if (s.valid_count > 0) {
        s.mean = (double)sum / s.valid_count;
        double var = sum2 / s.valid_count - s.mean * s.mean;
        s.std_dev = sqrt(var < 0 ? 0 : var);
    }
    return s;
}

void print_stats(BlockStats *s, const char *label) {
    printf("  [%-22s] valid:%3d  max:%3d  min:%3d  "
           "mean:%5.1f  std:%4.1f  "
           "missing:%2d  corrupted:%2d  delayed:%2d\n",
           label,
           s->valid_count, s->max_val, s->min_val,
           s->mean, s->std_dev,
           s->missing, s->corrupted, s->delayed);
}

/* ── generate a clean sample ─────────────────────────── */
int clean_sample(void) {
    return rand() % 101;   /* always 0-100, no faults */
}

/* ─────────────────────────────────────────────────────────────────
   SCENARIO 1: Effect of missing samples at different rates
   ───────────────────────────────────────────────────────────────── */
void scenario_missing(void) {
    printf("\n╔══════════════════════════════════════════════════╗\n");
    printf("║  Part (c) — Scenario 1: Missing Samples          ║\n");
    printf("╚══════════════════════════════════════════════════╝\n");
    printf("  Handling: missing samples are detected (sentinel −1)\n");
    printf("  and excluded from all calculations.\n\n");

    int missing_rates[] = {0, 5, 20, 50};
    for (int r = 0; r < 4; r++) {
        int miss_pct = missing_rates[r];
        int block[BLOCK_SIZE];
        for (int i = 0; i < BLOCK_SIZE; i++) {
            block[i] = (rand() % 100 < miss_pct) ? MISSING_MARKER : clean_sample();
        }
        BlockStats s = compute_stats(block, BLOCK_SIZE);
        char label[32];
        sprintf(label, "%d%% missing", miss_pct);
        print_stats(&s, label);
    }
    printf("\n  Observation: as missing rate rises, valid_count drops.\n");
    printf("  Mean stays ~50 (still accurate) but is less reliable\n");
    printf("  because it is based on fewer samples.\n");
}

/* ─────────────────────────────────────────────────────────────────
   SCENARIO 2: Effect of corrupted samples — filtered vs unfiltered
   ───────────────────────────────────────────────────────────────── */
void scenario_corrupted(void) {
    printf("\n╔══════════════════════════════════════════════════╗\n");
    printf("║  Part (c) — Scenario 2: Corrupted Samples        ║\n");
    printf("╚══════════════════════════════════════════════════╝\n");
    printf("  Two approaches compared: filter out vs keep corrupted.\n\n");

    /* build one block with ~15%% corrupted */
    int block_filtered[BLOCK_SIZE];
    int block_unfiltered[BLOCK_SIZE];

    for (int i = 0; i < BLOCK_SIZE; i++) {
        if (rand() % 100 < 15) {
            block_filtered[i]   = CORRUPTED_MARKER; /* filtered out */
            block_unfiltered[i] = rand() % 900 + 101; /* raw bad value: 101-1000 */
        } else {
            int v = clean_sample();
            block_filtered[i]   = v;
            block_unfiltered[i] = v;
        }
    }

    /* filtered stats — use compute_stats which skips CORRUPTED_MARKER */
    BlockStats sf = compute_stats(block_filtered, BLOCK_SIZE);
    print_stats(&sf, "Filtered (correct)");

    /* unfiltered: treat out-of-range as valid — compute manually */
    long sum = 0; double sum2 = 0; int n = 0;
    int mn = block_unfiltered[0], mx = block_unfiltered[0];
    for (int i = 0; i < BLOCK_SIZE; i++) {
        int v = block_unfiltered[i]; n++;
        sum += v; sum2 += (double)v * v;
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }
    double mean_u = (double)sum / n;
    double std_u  = sqrt(sum2 / n - mean_u * mean_u);
    printf("  [%-22s] valid:%3d  max:%3d  min:%3d  "
           "mean:%5.1f  std:%4.1f  (no filtering!)\n",
           "Unfiltered (wrong)", n, mx, mn, mean_u, std_u);

    printf("\n  Observation: unfiltered data has a severely inflated\n");
    printf("  mean and std dev due to out-of-range outliers.\n");
    printf("  Filtering is ESSENTIAL for correct statistics.\n");
}

/* ─────────────────────────────────────────────────────────────────
   SCENARIO 3: Delayed samples — timeout-based partial block
   ───────────────────────────────────────────────────────────────── */
void scenario_delayed(void) {
    printf("\n╔══════════════════════════════════════════════════╗\n");
    printf("║  Part (c) — Scenario 3: Delayed Samples          ║\n");
    printf("╚══════════════════════════════════════════════════╝\n");
    printf("  Timeout = 80 samples received. Block processed early.\n\n");

    /* simulate: only 72 of 100 samples arrived before timeout */
    int block[BLOCK_SIZE];
    int arrived = 72;

    /* fill arrived samples with real data */
    for (int i = 0; i < arrived; i++)
        block[i] = clean_sample();

    /* remaining slots marked DELAYED — timeout fired */
    for (int i = arrived; i < BLOCK_SIZE; i++)
        block[i] = DELAYED_MARKER;

    BlockStats s = compute_stats(block, BLOCK_SIZE);

    printf("  Approach A — wait for all 100 (no timeout):\n");
    printf("    System stalls indefinitely. No result produced.\n\n");

    printf("  Approach B — process at timeout with %d/%d samples:\n",
           arrived, BLOCK_SIZE);
    print_stats(&s, "Partial block (72/100)");

    printf("\n  Approach C — process with 100%% of slots:\n");
    /* fill remaining with interpolated midpoint (50) */
    int block_interp[BLOCK_SIZE];
    for (int i = 0; i < arrived; i++) block_interp[i] = block[i];
    for (int i = arrived; i < BLOCK_SIZE; i++) block_interp[i] = 50; /* interpolated */
    BlockStats si = compute_stats(block_interp, BLOCK_SIZE);
    print_stats(&si, "Interpolated (50 fill)");

    printf("\n  Observation: Approach B gives honest stats on real data\n");
    printf("  but with reduced sample size. Approach C fills gaps with\n");
    printf("  interpolated values — pulls mean toward 50, reduces std.\n");
    printf("  Both are better than stalling (Approach A).\n");
}

/* ── main ──────────────────────────────────────────────── */
int main(void) {
    srand((unsigned)time(NULL));
    printf("EN5500 - Part (c): Handling Missing, Corrupted & Delayed Data\n");
    printf("Block size: %d samples\n", BLOCK_SIZE);

    scenario_missing();
    scenario_corrupted();
    scenario_delayed();

    printf("\nDone.\n");
    return 0;
}