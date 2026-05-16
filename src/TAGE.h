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

typedef struct component {
    int cnt_bits; // 3-bits
    int tag; // 9-bits
    int useful_bits; // // 2-bits
} component_t;

class O3_CPU;

class TAGE {
private:
    bitset<131> ghr; // Global History Register
    uint16_t phr; // Path History Register
    
    int clock = 0; // every 256K branches reset msb or lsb of all useful counters
    int clock_flip = 1; // when 1 msb reset, 0 lsb reset
    int altBetterCount = 8;
    int ProviderComp = 4;
    
    uint8_t alt_pred = 0;
    uint8_t prime_pred = 0;

    int table_indices[NUM_TABLES];

    bimodal_t bimodal_table[BIMODAL_ENTRIES]; // (T4)
    component_t tagged_tables[NUM_TABLES][ENTRIES_PER_BANK];

public:
    explicit TAGE(O3_CPU* cpu) { (void)cpu; }
    void initialize_branch_predictor();
    uint8_t predict_branch(uint64_t ip);
    void last_branch_result(uint64_t ip, uint64_t branch_target, uint8_t taken, uint8_t branch_type);

    void bimodal_init();
    uint8_t bimodal_predict(uint64_t pc);
    void bimodal_update(uint64_t pc, uint8_t taken);
    void bank_init(bank_t *table);
    
    int PC_hash(u_int64_t pc);
    int fold_ghr(int length);
    int find_index(uint64_t pc, int index_bank);
    int calc_tag(uint64_t pc, int bank_index);
    int convert_int(int start, bitset<131> source, int mask);
    uint8_t comp_pred(int counter);
    void update_policy();
};

#endif