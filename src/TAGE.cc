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
uint8_t TAGE::bimodal_predict(uint64_t pc) {
    int index = pc % BIMODAL_ENTRIES;

    return ((uint8_t) (bimodal_table[index].prediction));
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
void TAGE::bank_init(component_t *table) {
    for (int i = 0;  i < ENTRIES_PER_BANK; i++) {
        table[i].cnt_bits = 0;
        table[i].tag = 0;
        table[i].useful_bits = 0;
    }
}

// Folds 20 bits of PC to 10 bits
// PC[9:0] ^ PC[19:10]
int TAGE::PC_hash(u_int64_t pc) {
    u_int64_t mask = 0x3FF; // 10 lsb are 1

    return ((int)((pc & mask) ^ ((pc >> 10) & mask))); 
}

// Converts 131 bit to 9 bit (int)
int TAGE::convert_int(int start, bitset<131> source, int mask) {
    return (static_cast<int>((source >> start).to_ulong() & mask));
}

// folds length bits of ghr to 10 bits
int TAGE::fold_ghr(int length) {
    
    int mask = 0x3FF; // 10 lsb are 1
    int folded_history = 0;
    int temp_ghr;
    
    int times_fold = length / 10;
    int start = 0;

    for (int i=0; i < times_fold; i++) {
        temp_ghr = convert_int(start, ghr, mask);
        folded_history ^= (temp_ghr & mask);
        start+=10;  
    }

    return folded_history;
}

// Finds the index of each bank to be accessed.
int TAGE::find_index(uint64_t pc, int index_bank) {

    int pc_hash = PC_hash(pc);
    int folded_history = fold_ghr(geo_lengths[index_bank]);    
    
    return (pc_hash ^ folded_history); 
}

int TAGE::calc_tag(uint64_t pc, int bank_index) {
    uint16_t temp_phr = phr;
    int folded_history = 0;

    // mask 0x1FF 9 lsb are 1 
    // folds 18 bits of pc to 9 bits
    int folded_pc = (int) ((pc & 0x1FF) ^ ((pc >> 9) & 0x1FF));

    // fold ghr to 9 bits
    int times_fold = geo_lengths[bank_index] / 9;
    int start = 0;
    int mask = 0x1FF;

    for (int i=0; i < times_fold; i++) {
        int temp_ghr = convert_int(start, ghr, mask);
        folded_history ^= (temp_ghr & mask);
        start+=9;  
    }

    int folded_phr = (int) ((temp_phr & 0x1FF) ^ ((temp_phr >> 9) & 0x1FF));

    return (folded_pc ^ folded_history ^ folded_phr);
}

// translates 3-bit counter of bank entry to T/NT
uint8_t TAGE::comp_pred(int counter) {
    return((uint8_t) (counter >= 4 ? 1 : 0));
}

void TAGE::update_policy(uint8_t is_misspred, uint8_t taken) {
    component_t *prov_comp = &tagged_tables[provider_comp][table_indices[provider_comp]];

    // update prediction counter of provider bank
    if(is_misspred) {
        if (prov_comp->cnt_bits > 0) {
            prov_comp->cnt_bits--;
        }  
    }
    else {
        if (prov_comp->cnt_bits < 7) {
            prov_comp->cnt_bits++;
        } 
    }
    
    // update useful counter 
    if (prime_pred != alt_pred) {
        if(is_misspred) {
            if (prov_comp->useful_bits > 0) 
                prov_comp->useful_bits--;
        }
        else {
            if (prov_comp->useful_bits < 3) {
                prov_comp->useful_bits++;
            } 
        }
    }

    int msb_mask = 0b01;
    int lsb_mask = 0b10;
    clock++;

    // Useful bits reset, every 256K branches
    if(clock == 262144) {
        // Reset MSBs
        if(clock_flip) {
            for (int i=0; i < NUM_TABLES; i++) {
                for (int j=0; j < ENTRIES_PER_BANK; j++) 
                    tagged_tables[i][j].useful_bits &= msb_mask; 
            }
            clock_flip = 0;
        }
        // Reset LSBs
        else {
            for (int i=0; i < NUM_TABLES; i++) {
                for (int j=0; j < ENTRIES_PER_BANK; j++)
                    tagged_tables[i][j].useful_bits &= lsb_mask; 
            }
            clock_flip = 1;
        }
    
        clock = 0;
    }

    int eviction_candidates = 0;
    int canditates_table[4];
    int random_val = rand() % 100;
    int target_bank = 0;

    // Allocate a new entry in case of missprediction
    if (is_misspred) {
        for (int i = provider_comp-1; i >= 0; i--) {
            if (!tagged_tables[i][table_indices[i]].useful_bits) {
                canditates_table[eviction_candidates] = i;
                eviction_candidates++;
            }
        }

        // No useful bits set to 0
        if (!eviction_candidates) {
            // Decrement all useful bits
            for (int i = provider_comp-1; i <= 0; i--)   
                tagged_tables[i][table_indices[i]].useful_bits--;
        } 
        else if (eviction_candidates == 1) {
            // Allocate entry
            int index = provider_comp-1;
            component_t *target_component = &tagged_tables[index][table_indices[index]];
            target_component->useful_bits = 0;
            target_component->tag = computed_tags[index];
            target_component->cnt_bits = taken ? 3:4;
        }
        // More than 1 
        else {
            if(random_val > 33 && random_val <= 99) {
                target_bank = canditates_table[(eviction_candidates-1)];
            }
            else {
                target_bank = canditates_table[(eviction_candidates-2)];
            }
            
            component_t *target_component = &tagged_tables[target_bank][table_indices[target_bank]];
            target_component->useful_bits = 0;
            target_component->tag = computed_tags[target_bank];
            target_component->cnt_bits = taken ? 3:4;
        }
    }

    // alt_pred was used
    if (!which_pred) {
        if(is_misspred) {
            if (alt_better > 0) 
                alt_better--;
        } 
        else {
            if (alt_better < 15) 
                alt_better++;
        } 
    } 

}

/* Functions Called from ChampSim */
void TAGE::initialize_branch_predictor() {
    bimodal_init();
    
    bank_init(tagged_tables[0]);
    bank_init(tagged_tables[1]);
    bank_init(tagged_tables[2]);
    bank_init(tagged_tables[3]);

    ghr.reset();
    phr = 0;
}

uint8_t TAGE::predict_branch(uint64_t ip) {
    uint8_t prediction;
    uint8_t bimodal_pred;

    bimodal_pred = bimodal_predict(ip);

    // Get hash for current pc  
    table_indices[3] = find_index(ip, 3);
    computed_tags[3] = TAGE::calc_tag(ip, 3);
    
    // Prediction logic
    if (computed_tags[3] == tagged_tables[3][table_indices[3]].tag) {
        prediction = comp_pred(tagged_tables[3][table_indices[3]].cnt_bits);
        prime_pred = prediction;
        alt_pred = bimodal_pred;
        provider_comp = 3;
    } else {
        prediction = bimodal_pred;
        prime_pred = bimodal_pred;
        provider_comp = 4; 
    }
    
    table_indices[2] = find_index(ip, 2);
    computed_tags[2] = TAGE::calc_tag(ip, 2);
    
    if (computed_tags[2] == tagged_tables[2][table_indices[2]].tag) {
        prediction = comp_pred(tagged_tables[2][table_indices[2]].cnt_bits);
        prime_pred = prediction;
        alt_pred = prime_pred;
        provider_comp = 2;
    } 

    table_indices[1] = find_index(ip, 1);
    computed_tags[1] = TAGE::calc_tag(ip, 1);
    
    if (computed_tags[1] == tagged_tables[1][table_indices[1]].tag) {
        prediction = comp_pred(tagged_tables[1][table_indices[1]].cnt_bits);
        prime_pred = prediction;
        alt_pred = prime_pred;
        provider_comp = 1;
    } 

    table_indices[0] = find_index(ip, 0);
    computed_tags[0] = TAGE::calc_tag(ip, 0);
    
    if (computed_tags[0] == tagged_tables[0][table_indices[0]].tag) {
        prediction = comp_pred(tagged_tables[0][table_indices[0]].cnt_bits);
        prime_pred = prediction;
        alt_pred = prime_pred;
        provider_comp = 0;
    }

    // Newly allocated optimization
    component_t *prov_comp = &tagged_tables[provider_comp][table_indices[provider_comp]]; 
    if (prov_comp->useful_bits == 0) {
        // Alt_pred maybe better
        if(alt_better > 7) {
            prediction = alt_pred;
            which_pred = 0;
        }
        else {
            which_pred = 1;
        }
    }

    return (prediction);
}

void TAGE::last_branch_result(uint64_t ip, uint64_t branch_target, uint8_t taken, uint8_t branch_type) {
    if (branch_type == BRANCH_CONDITIONAL) {
        bimodal_update(ip, taken);
        
        uint8_t is_misspred = (taken != (which_pred ? prime_pred : alt_pred)) ? 1:0;

        update_policy(is_misspred, taken);

        ghr <<= 1;
        ghr.set(0, taken);
        phr = ((phr << 1) ^ (ip & 0xFFFF)) & 0xFFFF;
    }
}

