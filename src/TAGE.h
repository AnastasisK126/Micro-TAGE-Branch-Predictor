#ifndef TAGE_H
#define TAGE_H

#include <stdio.h>
#include <math.h>
#include <stdint.h>

class O3_CPU;

class TAGE {
public:
    explicit TAGE(O3_CPU* cpu) { (void)cpu; }
    void initialize_branch_predictor();
    uint8_t predict_branch(uint64_t ip);
    void last_branch_result(uint64_t ip, uint64_t branch_target, uint8_t taken, uint8_t branch_type);
};

#define MIN_LEN 5 // bits
#define MAX_LEN 130
#define NUM_TABLES 4

const int geo_lengths[NUM_TABLES] = {5, 15, 44, 130};

#define PC_LENGTH 14
#define BIMODAL_ENTRIES 16384

#define STRONGLY_NT 0
#define WEAKLY_NT 1
#define WEAKLY_T 2
#define STRONGLY_T 3

#define TAKEN 1
#define NOT_TAKEN 0

typedef struct bimodal {
    int cnt_value;
    int prediction;
} bimodal_t;

#endif