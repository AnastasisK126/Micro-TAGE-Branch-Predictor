#include "TAGE.h"
#include "instruction.h"

// Initializes bimodal table
void TAGE::bimodal_init() {

    for(int i = 0; i < BIMODAL_ENTRIES; i++) {
        bimodal_table[i].cnt_value = WEAKLY_NT;
    }
}

// Returns Bimodal table prediction
bool TAGE::bimodal_predict(uint64_t pc) {
    int index = pc % BIMODAL_ENTRIES;

    return ((bool) (bimodal_table[index].cnt_value >= WEAKLY_T ? 1:0));
}

// Updates the table after the branch is evaluated,
// It is known if the prediction was correct or not
void TAGE::bimodal_update(uint64_t pc, uint8_t taken) {
    int index = pc % BIMODAL_ENTRIES;

    if (taken) {
        if (bimodal_table[index].cnt_value < STRONGLY_T) 
            bimodal_table[index].cnt_value++;
    } else {
        if (bimodal_table[index].cnt_value > STRONGLY_NT) 
            bimodal_table[index].cnt_value--;
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
int TAGE::convert_int(int start, bitset<131> source, int width) { 
    int result = 0;

    // Safely extract bit by bit to avoid 64-bit overflow limits
    for (int i = 0; i < width && (start + i) < 131; i++) {
        if (source[start + i]) {
            result |= (1 << i);
        }
    }
    return result;
}

// folds length bits of ghr to 10 bits
int TAGE::fold_ghr(int length) {
    int folded_history = 0;
    int times_fold = length / 10;
    int start = 0;

    // Fold in 10-bit chunks
    for (int i=0; i < times_fold; i++) {
        int temp_ghr = convert_int(start, ghr, 10);
        folded_history ^= temp_ghr;
        start += 10;  
    }

    int remainder = length % 10;
    if (remainder != 0) {
        int temp_ghr = convert_int(start, ghr, remainder);
        folded_history ^= temp_ghr;
    }

    return folded_history & 0x3FF;
}

// Finds the index of each bank to be accessed.
int TAGE::find_index(uint64_t pc, int index_bank) {
    int pc_hash = PC_hash(pc);
    int folded_history = fold_ghr(geo_lengths[index_bank]);    
    
    // Severe Aliasing 
    // Shift the history based on the bank to separate table mappings
    int index = pc_hash ^ folded_history ^ (folded_history >> (index_bank + 1));
    
    // Mix in the path history to further scramble the index
    index ^= (phr & 0x3FF) ^ ((phr >> 3) & 0x3FF); 
    
    // Ensure the index never goes out of bounds
    return index & 0x3FF;
}

int TAGE::calc_tag(uint64_t pc, int bank_index) {
    uint16_t temp_phr = phr;
    int folded_history = 0;
    int length = geo_lengths[bank_index];

    // folds 18 bits of pc to 9 bits
    int folded_pc = (int) ((pc & 0x1FF) ^ ((pc >> 9) & 0x1FF));

    // fold ghr to 9 bits
    int times_fold = length / 9;
    int start = 0;

    for (int i=0; i < times_fold; i++) {
        int temp_ghr = convert_int(start, ghr, 9);
        folded_history ^= temp_ghr;
        start += 9;  
    }

    // Catch the remaining bits
    int remainder = length % 9;
    if (remainder != 0) {
        int temp_ghr = convert_int(start, ghr, remainder);
        folded_history ^= temp_ghr;
    }

    int folded_phr = (int) ((temp_phr & 0x1FF) ^ ((temp_phr >> 9) & 0x1FF));

    // Better tag mixing to prevent aliasing
    int tag = folded_pc ^ folded_history ^ (folded_history >> 1) ^ folded_phr;
    
    return tag & 0x1FF;
}

// translates 3-bit counter of bank entry to T/NT
bool TAGE::comp_pred(int counter) {
    return((bool) (counter >= 4 ? 1 : 0));
}

void TAGE::update_policy(uint8_t is_misspred, uint8_t taken) {

    // update prediction counter of provider bank
    if (provider_comp < NUM_TABLES) {
        component_t *prov_comp = &tagged_tables[provider_comp][table_indices[provider_comp]];

        // update prediction counter of provider bank
        if (taken) {
            if (prov_comp->cnt_bits < 7) prov_comp->cnt_bits++;
        } else {
            if (prov_comp->cnt_bits > 0) prov_comp->cnt_bits--;
        }        
        
        // update useful counter 
        if (prime_pred != alt_pred) {
            if(is_misspred) {
                if (prov_comp->useful_bits > 0) prov_comp->useful_bits--;
            } else {
                if (prov_comp->useful_bits < 3) prov_comp->useful_bits++;
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
            for (int i = provider_comp-1; i >= 0; i--)   
                tagged_tables[i][table_indices[i]].useful_bits--;
        } 
        else if (eviction_candidates == 1) {
            // Allocate entry
            target_bank = canditates_table[0]; 
            component_t *target_component = &tagged_tables[target_bank][table_indices[target_bank]];            
            target_component->useful_bits = 0;
            target_component->tag = computed_tags[target_bank];
            target_component->cnt_bits = taken ? 4:3;
        }
        // More than 1 
        else {
            target_bank = canditates_table[0];
            int random_val = rand() % 100;

            if (eviction_candidates == 2) {
                if (random_val < 33) target_bank = canditates_table[1]; 
            } 
            else if (eviction_candidates == 3) {
                if (random_val < 14) target_bank = canditates_table[2];
                else if (random_val < 43) target_bank = canditates_table[1];
            } 
            else if (eviction_candidates == 4) {
                if (random_val < 6) target_bank = canditates_table[3];
                else if (random_val < 20) target_bank = canditates_table[2];
                else if (random_val < 47) target_bank = canditates_table[1];
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

bool TAGE::predict_branch(uint64_t ip) {
    bool prediction;
    bool bimodal_pred;

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
        alt_pred = prime_pred;
        prediction = comp_pred(tagged_tables[2][table_indices[2]].cnt_bits);
        prime_pred = prediction;
        provider_comp = 2;
    } 

    table_indices[1] = find_index(ip, 1);
    computed_tags[1] = TAGE::calc_tag(ip, 1);
    
    if (computed_tags[1] == tagged_tables[1][table_indices[1]].tag) {
        alt_pred = prime_pred;
        prediction = comp_pred(tagged_tables[1][table_indices[1]].cnt_bits);
        prime_pred = prediction;
        provider_comp = 1;
    } 

    table_indices[0] = find_index(ip, 0);
    computed_tags[0] = TAGE::calc_tag(ip, 0);
    
    if (computed_tags[0] == tagged_tables[0][table_indices[0]].tag) {
        alt_pred = prime_pred;
        prediction = comp_pred(tagged_tables[0][table_indices[0]].cnt_bits);
        prime_pred = prediction;
        provider_comp = 0;
    }

    // Newly allocated optimization
    which_pred = 1;
    if (provider_comp < NUM_TABLES) {
        component_t *prov_comp = &tagged_tables[provider_comp][table_indices[provider_comp]]; 
        if (prov_comp->useful_bits == 0) {
            // Alt_pred maybe better
            if(alt_better > 7) {
                prediction = alt_pred;
                which_pred = 0;
            }
        }
    }

    return (prediction);
}

void TAGE::last_branch_result(uint64_t ip, uint64_t branch_target, uint8_t taken, uint8_t branch_type) {
    if (branch_type == BRANCH_CONDITIONAL) {
        predict_branch(ip);
        if (provider_comp == 4) {
            bimodal_provider_count++;
        } else {
            tagged_provider_count++;
        }

        bimodal_update(ip, taken);
             
        uint8_t is_misspred = (taken != (which_pred ? prime_pred : alt_pred)) ? 1:0;

        update_policy(is_misspred, taken);
            
        ghr <<= 1;
        ghr.set(0, taken);
        phr = ((phr << 1) ^ (ip & 0xFFFF)) & 0xFFFF;
    }
}

TAGE::~TAGE() {
    cout << "\n======================================" << endl;
    cout << "        TAGE PREDICTOR STATS          " << endl;
    cout << "======================================" << endl;
    cout << "Bimodal Provider Count : " << bimodal_provider_count << endl;
    cout << "Tagged Provider Count  : " << tagged_provider_count << endl;
    
    // Optional: Calculate the percentage safely
    uint64_t total = bimodal_provider_count + tagged_provider_count;
    if (total > 0) {
        double tagged_percent = (double)tagged_provider_count / total * 100.0;
        cout << "Tagged Table Hit Rate  : " << tagged_percent << "%" << endl;
    }
    cout << "======================================\n" << endl;
}
