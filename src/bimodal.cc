#include <stdio.h>
#include <math.h>

#define PC_LENGTH 12
#define BIMODAL_ENTRIES 16384

#define STRONGLY_NT 0
#define WEAKLY_NT 1
#define WEAKLY_T 2
#define STRONGLY_T 3

int bimodal_table[BIMODAL_ENTRIES];

void bimodal_init() {

    for(int i = 0; i < BIMODAL_ENTRIES; i++) {
        bimodal_table[i] = WEAKLY_NT;
    }
}

void bimodal_predict(int pc) {
    
}

void bimodal_update() {
    
}