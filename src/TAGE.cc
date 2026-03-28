#include <stdio.h>

#define MIN_LEN 5 // bits
#define MAX_LENGTH 130
#define NUM_TABLES 4
#define ALPHA ((MAX_LENGTH / MIN_LEN) ^ (1/(NUM_TABLES-1)))
#define GEO_LENGTH(i) (MIN_LEN * (ALPHA ^ (i-1))) // isws thelei kai +0.5 gia kalytero rounding
