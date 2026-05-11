#include "TAGE.h"
#include "instruction.h"

bimodal_t bimodal_table[BIMODAL_ENTRIES];

void bimodal_init() {

    for(int i = 0; i < BIMODAL_ENTRIES; i++) {
        bimodal_table[i].cnt_value = WEAKLY_T;
        bimodal_table[i].prediction = TAKEN;
    }
}

int bimodal_predict(uint64_t pc) {
    int index = pc % BIMODAL_ENTRIES;

    return bimodal_table[index].prediction;
}

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

void TAGE::initialize_branch_predictor() {
    bimodal_init();
}

uint8_t TAGE::predict_branch(uint64_t ip) {
    return (bimodal_predict(ip) == TAKEN) ? 1 : 0;
}

void TAGE::last_branch_result(uint64_t ip, uint64_t branch_target, uint8_t taken, uint8_t branch_type) {
    if (branch_type == BRANCH_CONDITIONAL) {
        bimodal_update(ip, taken);
    }
}

