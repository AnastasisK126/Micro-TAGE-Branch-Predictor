#ifndef TAGE_H
#define TAGE_H

#include <stdio.h>
#include <math.h>
#include <stdint.h>

#define MIN_LEN 5 // bits
#define MAX_LEN 130
#define NUM_TABLES 4


#define PC_LENGTH 14
#define BIMODAL_ENTRIES 16384

#define STRONGLY_NT 0
#define WEAKLY_NT 1
#define WEAKLY_T 2
#define STRONGLY_T 3

#define TAKEN 1
#define NOT_TAKEN 0

#define ENTRIES_PER_BANK 1024

const int geo_lengths[NUM_TABLES] = {130, 44, 15, 5};

class O3_CPU;

class TAGE {
private:
    uint64_t ghr[3]; // Global History Register
    uint16_t phr; // Path History Register

public:
    explicit TAGE(O3_CPU* cpu) { (void)cpu; }
    void initialize_branch_predictor();
    uint8_t predict_branch(uint64_t ip);
    void last_branch_result(uint64_t ip, uint64_t branch_target, uint8_t taken, uint8_t branch_type);
};

typedef struct bimodal {
    int cnt_value;
    int prediction;
} bimodal_t;

typedef struct bank {
    int cnt_bits;
    int tag;
    int useful_bits;
} bank_t;

#endif