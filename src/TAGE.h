#ifndef TAGE_H
#define TAGE_H

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <bitset>
using namespace std;

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

const int geo_lengths[NUM_TABLES] = {131, 44, 15, 5};

typedef struct bimodal {
    int cnt_value;
    int prediction;
} bimodal_t;

typedef struct bank {
    bitset<3> cnt_bits;
    bitset<9> tag;
    bitset<2> useful_bits;
} bank_t;

class O3_CPU;

class TAGE {
private:
    bitset<131> ghr; // Global History Register
    uint16_t phr; // Path History Register
    int clock = 0;
    int clock_flip = 1;
    bitset<4> altBetterCount = 8;

    bimodal_t bimodal_table[BIMODAL_ENTRIES];

    bank_t T0[ENTRIES_PER_BANK];
    bank_t T1[ENTRIES_PER_BANK];
    bank_t T2[ENTRIES_PER_BANK];
    bank_t T3[ENTRIES_PER_BANK];

public:
    explicit TAGE(O3_CPU* cpu) { (void)cpu; }
    void initialize_branch_predictor();
    uint8_t predict_branch(uint64_t ip);
    void last_branch_result(uint64_t ip, uint64_t branch_target, uint8_t taken, uint8_t branch_type);

    void bimodal_init();
    int bimodal_predict(uint64_t pc);
    void bimodal_update(uint64_t pc, uint8_t taken);
    void bank_init(bank_t *table);
    
    u_int64_t PC_hash(u_int64_t pc);
    bitset<131> fold_ghr(int length);
    bitset<131> find_index(uint64_t pc, int index_bank);
    bitset<9> TAGE::calc_tag(uint64_t pc, int bank_index);
};

#endif TAGE_H