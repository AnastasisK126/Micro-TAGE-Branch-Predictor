#!/bin/bash

# Checking if we have at least the first input (predictor)
if [ -z "$1" ]; then
    echo "Usage: ./run_eval.sh <predictor_name> [optional_benchmark_filter]"
    echo "Example 1 (Runs EVERYTHING) : ./run_eval.sh tage"
    echo "Example 2 (Runs only bzip2) : ./run_eval.sh tage bzip2"
    echo "Example 3 (Runs only gcc)   : ./run_eval.sh bimodal gcc"
    exit 1
fi

PREDICTOR=$1
FILTER=$2
TRACES_DIR="../traces"

# Simulation Parameters
WARMUP=20000000
SIMULATION=50000000

# Set up the search pattern and CSV filename based on whether a filter was provided
if [ -z "$FILTER" ]; then
    SEARCH_PATTERN="*.champsimtrace.xz"
    CSV_FILE="results_${PREDICTOR}_all.csv"
    DISPLAY_TEXT="ALL benchmarks"
else
    SEARCH_PATTERN="*${FILTER}*.champsimtrace.xz"
    CSV_FILE="results_${PREDICTOR}_${FILTER}.csv"
    DISPLAY_TEXT="ONLY benchmarks containing '${FILTER}'"
fi

# Initialisation of the CSV file 
echo "Benchmark,Accuracy(%),MPKI" > "$CSV_FILE"

echo "==================================================="
echo " Starting evaluation for predictor: $PREDICTOR"
echo " Target: $DISPLAY_TEXT"
echo " Results will be saved in: $CSV_FILE"
echo "==================================================="

# Loop through the .champsimtrace.xz files using the dynamic search pattern
for TRACE_FILE in "$TRACES_DIR"/$SEARCH_PATTERN; do
    
    # Safety check in case the directory is empty or the filter matched nothing
    if [ ! -f "$TRACE_FILE" ]; then
        echo "Error: No traces found matching '$SEARCH_PATTERN' in directory $TRACES_DIR/"
        exit 1
    fi

    # Extract the base name of the file for a cleaner CSV output
    BENCHMARK_NAME=$(basename "$TRACE_FILE")
    
    echo "Running benchmark: $BENCHMARK_NAME ..."

    # Create a temporary file to capture the ChampSim output
    TEMP_OUT=$(mktemp)

    # Run ChampSim and redirect output to the temporary file
    ./../ChampSim/bin/champsim_TAGE --warmup-instructions "$WARMUP" --simulation-instructions "$SIMULATION" "$TRACE_FILE" > "$TEMP_OUT"

    # Extract Accuracy (6th column of the matched line) and remove the '%' symbol
    ACCURACY=$(grep "Branch Prediction Accuracy:" "$TEMP_OUT" | awk '{print $6}' | tr -d '%')
    
    # Extract MPKI (8th column of the matched line)
    MPKI=$(grep "Branch Prediction Accuracy:" "$TEMP_OUT" | awk '{print $8}')

    # Fallback in case ChampSim crashes or doesn't output expected metrics
    if [ -z "$ACCURACY" ]; then
        ACCURACY="N/A"
        MPKI="N/A"
    fi

    # Write the extracted data to the CSV file
    echo "$BENCHMARK_NAME,$ACCURACY,$MPKI" >> "$CSV_FILE"

    # Clean up the temporary file
    rm "$TEMP_OUT"

    echo "  -> Completed! (Accuracy: $ACCURACY%, MPKI: $MPKI)"

done

echo "==================================================="
echo "Requested benchmarks completed successfully!"
echo "Open the file $CSV_FILE to see the results."