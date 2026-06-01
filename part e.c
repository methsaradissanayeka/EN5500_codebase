#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* ── NO hardcoded block size — it comes from the user at runtime ── */
#define MISSING_MARKER   -1
#define CORRUPTED_MARKER -2

typedef struct {
    int    max_val, min_val;
    double mean, std_dev;
    int    missing, corrupted, valid_count;
} BlockStats;

/* ── same compute_stats as before, but size is now a parameter ── */
BlockStats compute_stats(int *block, int size) {
    BlockStats s = {0, 100, 0.0, 0.0, 0, 0, 0};
    long   sum  = 0;
    double sum2 = 0.0;
    int    first = 1;
    for (int i = 0; i < size; i++) {
        if (block[i] == MISSING_MARKER)   { s.missing++;   continue; }
        if (block[i] == CORRUPTED_MARKER) { s.corrupted++; continue; }
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

int generate_sample(void) {
    int r = rand() % 110;
    if (r > 104) return MISSING_MARKER;
    if (r > 100) return CORRUPTED_MARKER;
    return r;
}

void print_stats(int block_num, int block_size, BlockStats *s) {
    printf("  Block %2d | valid:%3d/%-3d | max:%3d min:%3d "
           "mean:%5.1f std:%4.1f | missing:%d corrupted:%d\n",
           block_num, s->valid_count, block_size,
           s->max_val, s->min_val,
           s->mean, s->std_dev,
           s->missing, s->corrupted);
}

/* ════════════════════════════════════════════════════════════════
   KEY CHANGE 1: main() now accepts command-line arguments
   KEY CHANGE 2: block array is dynamically allocated with malloc()
   instead of a fixed-size array on the stack
   ════════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));

    /* ── read block size from command line, default to 100 ── */
    int block_size = 100;
    if (argc >= 2) {
        block_size = atoi(argv[1]);
        if (block_size <= 0 || block_size > 10000) {
            fprintf(stderr, "Invalid block size '%s'. "
                    "Must be 1-10000. Using default: 100\n", argv[1]);
            block_size = 100;
        }
    }

    /* number of blocks to run — fixed at 3 for demonstration */
    int num_blocks = 3;
    if (argc >= 3) {
        num_blocks = atoi(argv[2]);
        if (num_blocks <= 0) num_blocks = 3;
    }

    printf("EN5500 - Part (e): Configurable Block Size\n");
    printf("Block size : %d samples\n", block_size);
    printf("Num blocks : %d\n\n", num_blocks);

    /* ── KEY CHANGE 2: dynamic allocation ── */
    int *block = malloc(block_size * sizeof(int));
    if (block == NULL) {
        fprintf(stderr, "Error: could not allocate block of %d ints\n",
                block_size);
        return 1;
    }

    /* ── same processing loop as original sensor.c ── */
    for (int b = 1; b <= num_blocks; b++) {
        for (int i = 0; i < block_size; i++)
            block[i] = generate_sample();

        BlockStats s = compute_stats(block, block_size);
        print_stats(b, block_size, &s);
    }

    /* ── KEY CHANGE 3: always free dynamically allocated memory ── */
    free(block);

    printf("\nDone.\n");
    return 0;
}

