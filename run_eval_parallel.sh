#!/bin/bash

# Checking if we have at least the first input
if [ -z "$1" ]; then
    echo "Usage: ./run_eval.sh <predictor1,predictor2,...> [optional_benchmark_filter]"
    echo "Example 1 (Multiple) : ./run_eval.sh tage,bimodal,perceptron"
    echo "Example 2 (Filtered) : ./run_eval.sh tage,bimodal bzip2"
    exit 1
fi

# Convert the comma-separated first argument into an array
IFS=',' read -r -a PREDICTORS <<< "$1"
FILTER=$2
TRACES_DIR="../traces_s"

# Simulation Parameters
WARMUP=20000000
SIMULATION=50000000

# Function to evaluate a single predictor
evaluate_predictor() {
    local PREDICTOR=$1
    
    # Set up the search pattern and CSV filename based on whether a filter was provided
    if [ -z "$FILTER" ]; then
        local SEARCH_PATTERN="*.champsimtrace.xz"
        local CSV_FILE="results_${PREDICTOR}_all_s.csv"
        local DISPLAY_TEXT="ALL benchmarks"
    else
        local SEARCH_PATTERN="*${FILTER}*.champsimtrace.xz"
        local CSV_FILE="results_${PREDICTOR}_${FILTER}.csv"
        local DISPLAY_TEXT="ONLY benchmarks containing '${FILTER}'"
    fi

    #Ensure binaries are uniquely named per predictor
    local BINARY="./../ChampSim/bin/champsim_${PREDICTOR}"

    if [ ! -f "$BINARY" ]; then
        echo "[ERROR] Binary $BINARY not found! Skipping $PREDICTOR..."
        return
    fi

    echo "==================================================="
    echo " Starting evaluation for predictor: $PREDICTOR"
    echo " Target: $DISPLAY_TEXT"
    echo " Results will be saved in: $CSV_FILE"
    echo "==================================================="

    # Initialisation of the CSV file
    echo "Benchmark,Accuracy(%),MPKI" > "$CSV_FILE"

    # Loop through the .champsimtrace.xz files using the dynamic search pattern
    for TRACE_FILE in "$TRACES_DIR"/$SEARCH_PATTERN; do
        
        # Safety check in case the directory is empty or the filter matched nothing
        if [ ! -f "$TRACE_FILE" ]; then
            echo "Error: No traces found matching '$SEARCH_PATTERN' for $PREDICTOR"
            return
        fi

        # Extract the base name of the file for a cleaner CSV output
        local BENCHMARK_NAME=$(basename "$TRACE_FILE")
        
        echo "[$PREDICTOR] Running benchmark: $BENCHMARK_NAME ..."

        # Create a temporary file to capture the ChampSim output
        local TEMP_OUT=$(mktemp)

        # Run ChampSim and redirect output to the temporary file
        $BINARY --warmup-instructions "$WARMUP" --simulation-instructions "$SIMULATION" "$TRACE_FILE" > "$TEMP_OUT"

        # Extract Accuracy (6th column of the matched line) and remove the '%' symbol
        local ACCURACY=$(grep "Branch Prediction Accuracy:" "$TEMP_OUT" | awk '{print $6}' | tr -d '%')
        
        # Extract MPKI (8th column of the matched line)
        local MPKI=$(grep "Branch Prediction Accuracy:" "$TEMP_OUT" | awk '{print $8}')

        # Fallback in case ChampSim crashes or doesn't output expected metrics
        if [ -z "$ACCURACY" ]; then
            ACCURACY="N/A"
            MPKI="N/A"
        fi

        # Write the extracted data to the CSV file
        echo "$BENCHMARK_NAME,$ACCURACY,$MPKI" >> "$CSV_FILE"

        # Clean up the temporary file
        rm "$TEMP_OUT"

        echo "  -> [$PREDICTOR] Completed! (Accuracy: $ACCURACY%, MPKI: $MPKI)"

    done

    echo "[$PREDICTOR] Evaluation finished!"
}

echo "CONFIGURE AND BUILD FIRST!!!!"

echo "Launching parallel evaluations..."

# Loop through the array of predictors and launch them in the background
for PRED in "${PREDICTORS[@]}"; do
    evaluate_predictor "$PRED" &
done

# 'wait' pauses the script until all background jobs (&) have finished executing
wait

echo "==================================================="
echo "All requested predictors completed successfully!"
echo "Check the respective CSV files in your directory for results."