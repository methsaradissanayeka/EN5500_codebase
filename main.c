#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define BLOCK_SIZE 100
#define TOTAL_BLOCKS 5
#define MISSING_MARKER -1
#define CORRUPTED_MARKER -2

/* ---------- data generation ---------- */
int generate_sample(int index) {
    int r = rand() % 110;          /* occasionally out-of-range */
    if (r > 104) return MISSING_MARKER;
    if (r > 100) return CORRUPTED_MARKER;
    return r;
}

/* ---------- block statistics ---------- */
typedef struct {
    int    max_val;
    int    min_val;
    double mean;
    double std_dev;
    int    missing;
    int    corrupted;
    int    valid_count;
} BlockStats;

BlockStats compute_stats(int *block, int size) {
    BlockStats s = {0, 100, 0.0, 0.0, 0, 0, 0};
    long   sum  = 0;
    double sum2 = 0.0;
    int    first_valid = 1;

    for (int i = 0; i < size; i++) {
        if (block[i] == MISSING_MARKER)   { s.missing++;   continue; }
        if (block[i] == CORRUPTED_MARKER) { s.corrupted++; continue; }

        int v = block[i];
        s.valid_count++;
        sum  += v;
        sum2 += (double)v * v;

        if (first_valid) { s.max_val = v; s.min_val = v; first_valid = 0; }
        else {
            if (v > s.max_val) s.max_val = v;
            if (v < s.min_val) s.min_val = v;
        }
    }

    if (s.valid_count > 0) {
        s.mean    = (double)sum / s.valid_count;
        double variance = (sum2 / s.valid_count) - (s.mean * s.mean);
        s.std_dev = sqrt(variance < 0 ? 0 : variance);
    }
    return s;
}

/* ---------- display ---------- */
void print_stats(int block_num, BlockStats *s) {
    printf("\n========================================\n");
    printf("  Block %d Results\n", block_num);
    printf("========================================\n");
    printf("  Valid samples  : %d / %d\n", s->valid_count, BLOCK_SIZE);
    printf("  Missing        : %d\n",  s->missing);
    printf("  Corrupted      : %d\n",  s->corrupted);
    if (s->valid_count > 0) {
        printf("  Maximum        : %d\n",   s->max_val);
        printf("  Minimum        : %d\n",   s->min_val);
        printf("  Mean           : %.2f\n", s->mean);
        printf("  Std Deviation  : %.2f\n", s->std_dev);
    } else {
        printf("  (No valid samples in this block)\n");
    }
    printf("========================================\n");
}

/* ---------- main ---------- */
int main(void) {
    srand((unsigned)time(NULL));

    printf("Block size: %d samples | Blocks: %d\n", BLOCK_SIZE, TOTAL_BLOCKS);

    int block[BLOCK_SIZE];
    int sample_index = 0;

    for (int b = 1; b <= TOTAL_BLOCKS; b++) {
        printf("\n[Block %d] Generating %d samples...\n", b, BLOCK_SIZE);

        for (int i = 0; i < BLOCK_SIZE; i++) {
            block[i] = generate_sample(sample_index++);
        }

        BlockStats stats = compute_stats(block, BLOCK_SIZE);
        print_stats(b, &stats);
    }

    printf("\nProcessing complete.\n");
    return 0;
}