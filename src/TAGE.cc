#include "TAGE.h"
#include "instruction.h"

bimodal_t bimodal_table[BIMODAL_ENTRIES];

bank_t T0[ENTRIES_PER_BANK];
bank_t T1[ENTRIES_PER_BANK];
bank_t T2[ENTRIES_PER_BANK];
bank_t T3[ENTRIES_PER_BANK];

// Initializes bimodal table
void bimodal_init() {

    for(int i = 0; i < BIMODAL_ENTRIES; i++) {
        bimodal_table[i].cnt_value = WEAKLY_T;
        bimodal_table[i].prediction = TAKEN;
    }
}

// Returns Bimodal table prediction
int bimodal_predict(uint64_t pc) {
    int index = pc % BIMODAL_ENTRIES;

    return bimodal_table[index].prediction;
}

// Updates the table after the branch is evaluated,
// It is known if the prediction was correct or not
void bimodal_update(uint64_t pc, uint8_t taken) {
    int index = pc % BIMODAL_ENTRIES;

    // The prediction was correct
    if (taken == bimodal_table[index].prediction) {
        bimodal_table[index].cnt_value++;
        if (bimodal_table[index].cnt_value > WEAKLY_NT) {
            bimodal_table[index].prediction = TAKEN;
        }
    }
    // prediction was wrong
    else {
        bimodal_table[index].cnt_value--;
        if (bimodal_table[index].cnt_value < WEAKLY_T) {
            bimodal_table[index].prediction = NOT_TAKEN;
        }
    }

}

// Initialized tagged tables
void bank_init(bank_t *table) {
    for (int i = 0;  i < ENTRIES_PER_BANK; i++) {
        table[i].cnt_bits = 0;
        table[i].tag = 0;
        table[i].useful_bits = 0;
    }
}

void TAGE::initialize_branch_predictor() {
    bimodal_init();
    bank_init(T0);
    bank_init(T1);
    bank_init(T2);
    bank_init(T3);
}

int fold_history() {
    
}

int find_index(uint64_t pc, uint64_t ghr[3], int index_bank); {
    int index = 0;
    int mask = 0x3FF;
    
    switch (index_bank) {
    case 0:
        // index PC[9:0] ^ PC[19:10] ^ GHR[9:0]
        index = (pc & mask) ^ ((pc >> 10) & mask) ^ (ghr[3] & mask);        
        break;
    case 1:
    
        break;
    case 2:
        break;
    case 3:
        break;
    default:
        break;
    }
}

uint8_t TAGE::predict_branch(uint64_t ip) {
    return (bimodal_predict(ip) == TAKEN) ? 1 : 0;
}

void TAGE::last_branch_result(uint64_t ip, uint64_t branch_target, uint8_t taken, uint8_t branch_type) {
    if (branch_type == BRANCH_CONDITIONAL) {
        bimodal_update(ip, taken);
        
        // Make space for the new taken bit.        
        ghr[2] = (ghr[2] << 1) | (ghr[1] >> 63);
        ghr[1] = (ghr[1] << 1) | (ghr[0] >> 63);
        ghr[0] = (ghr[0] << 1) | taken;
        ghr[2] &= 0x7; 
        
        phr = ((phr << 1) ^ (ip & 0xFFFF)) & 0xFFFF;
    }
}

