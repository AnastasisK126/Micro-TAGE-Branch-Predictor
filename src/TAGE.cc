#include "TAGE.h"
#include "instruction.h"

// Initializes bimodal table
void TAGE::bimodal_init() {

    for(int i = 0; i < BIMODAL_ENTRIES; i++) {
        bimodal_table[i].cnt_value = WEAKLY_T;
        bimodal_table[i].prediction = TAKEN;
    }
}

// Returns Bimodal table prediction
int TAGE::bimodal_predict(uint64_t pc) {
    int index = pc % BIMODAL_ENTRIES;

    return bimodal_table[index].prediction;
}

// Updates the table after the branch is evaluated,
// It is known if the prediction was correct or not
void TAGE::bimodal_update(uint64_t pc, uint8_t taken) {
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
void TAGE::bank_init(bank_t *table) {
    for (int i = 0;  i < ENTRIES_PER_BANK; i++) {
        table[i].cnt_bits = 0;
        table[i].tag = 0;
        table[i].useful_bits = 0;
    }
}

// Folds 20 bits of PC to 10 bits
// PC[9:0] ^ PC[19:10]
u_int64_t TAGE::PC_hash(u_int64_t pc) {
    u_int64_t mask = 0x3FF; // 10 lsb are 1

    return ((pc & mask) ^ ((pc >> 10) & mask)); 
}

// folds length bits of ghr to 10 bits
bitset<131> TAGE::fold_ghr(int length) {
    
    bitset<131> mask = 0x3FF; // 10 lsb are 1
    bitset<131> folded_history;
    bitset<131> temp_ghr = ghr;
    
    int times_fold = length / 10;

    for (int i=0; i < times_fold; i++) {
        folded_history ^= (temp_ghr & mask);
        temp_ghr >> 10;  
    }

    return folded_history;
}

// Finds the index of each bank to be accessed.
bitset<131> TAGE::find_index(uint64_t pc, int index_bank) {

    bitset<131> pc_hash = (bitset<131>) PC_hash(pc);
    bitset<131> folded_history = fold_ghr(geo_lengths[index_bank]);    
    
    return (pc_hash ^ folded_history); 
}

bitset<9> TAGE::calc_tag(uint64_t pc, int bank_index) {
    bitset<131> temp_ghr = ghr;
    uint16_t temp_phr = phr;
    bitset<131> folded_ghr;

    // mask 0x1FF 9 lsb are 1 
    // folds 18 bits of pc to 9 bits
    uint64_t folded_pc = (pc & 0x1FF) ^ ((pc >> 9) & 0x1FF);

    // fold ghr to 9 bits
    int times_fold = geo_lengths[bank_index] / 9;
    for (int i=0; i < times_fold; i++) {
        folded_ghr ^= (temp_ghr & (bitset<131>) 0x1FF);
        temp_ghr >> 9;  
    }

    uint16_t folded_phr = (temp_phr & 0x1FF) ^ ((temp_phr >> 9) & 0x1FF);

    return ((bitset<9>)folded_pc ^ (bitset<9>)folded_ghr ^ (bitset<9>)folded_phr);
}

/* Functions Called from ChampSim */
void TAGE::initialize_branch_predictor() {
    bimodal_init();
    
    bank_init(T0);
    bank_init(T1);
    bank_init(T2);
    bank_init(T3);

    ghr.reset();
    phr = 0;
}

uint8_t TAGE::predict_branch(uint64_t ip) {
    return (bimodal_predict(ip) == TAKEN) ? 1 : 0;
}

void TAGE::last_branch_result(uint64_t ip, uint64_t branch_target, uint8_t taken, uint8_t branch_type) {
    if (branch_type == BRANCH_CONDITIONAL) {
        bimodal_update(ip, taken);
        
        ghr <<= 1;
        ghr.set(0, taken);
        phr = ((phr << 1) ^ (ip & 0xFFFF)) & 0xFFFF;
    }
}

