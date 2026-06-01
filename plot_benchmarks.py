import pandas as pd
import matplotlib.pyplot as plt
import glob

def main():
    print("Searching for original trace CSV files...")
    all_csv_files = glob.glob("*.csv")
    
    # Keep ONLY the original traces (ignore "_s" traces)
    target_files = [f for f in all_csv_files if "traces_s" not in f]
    
    if not target_files:
        print("Error: No CSV files from the original traces were found.")
        return

    df_list = []
    for file in target_files:
        # Extract Predictor name from filename
        pred_name = file.replace("results_", "").replace("_all_s.csv", "").replace(".csv", "")
        
        # Read the CSV normally
        df = pd.read_csv(file)
        
        # BULLETPROOF LOGIC: Check if a data row accidentally became the header
        # (e.g., if a column name contains '.xz')
        has_header = True
        for col in df.columns:
            if ".xz" in str(col) or ".champsimtrace" in str(col):
                has_header = False
                break
                
        # If no header was found, read again without headers
        if not has_header:
            df = pd.read_csv(file, header=None)
            
        # Force rename the columns based on their position (ignoring old names)
        # This prevents any KeyError from typos in the CSV headers
        num_cols = len(df.columns)
        if num_cols == 3:
            df.columns = ['Trace_Name', 'Accuracy(%)', 'MPKI']
        elif num_cols >= 4:
            df = df.iloc[:, :4] # Keep only the first 4 columns
            df.columns = ['Trace_Name', 'Predictor', 'Accuracy(%)', 'MPKI']
            
        # Overwrite/Add the Predictor column with the clean name from the filename
        df['Predictor'] = pred_name
        
        # Extract the Benchmark name safely
        df['Benchmark'] = df['Trace_Name'].apply(lambda x: str(x).split('-')[0].replace('.champsimtrace.xz', ''))
        
        df_list.append(df)
        
    full_df = pd.concat(df_list, ignore_index=True)

    # Clean numeric data (converts any weird strings to NaN)
    full_df['Accuracy(%)'] = pd.to_numeric(full_df['Accuracy(%)'], errors='coerce')
    full_df['MPKI'] = pd.to_numeric(full_df['MPKI'], errors='coerce')

    # Calculate Average per Benchmark AND Predictor
    avg_df = full_df.groupby(['Benchmark', 'Predictor'])[['Accuracy(%)', 'MPKI']].mean().reset_index()

    # Pivot data to create Grouped Bar Charts
    acc_pivot = avg_df.pivot(index='Benchmark', columns='Predictor', values='Accuracy(%)')
    mpki_pivot = avg_df.pivot(index='Benchmark', columns='Predictor', values='MPKI')

    # Define Grayscale colors for the predictors (for academic/print style)
    greyscale_colors = ['#2D2D2D', '#5E5E5E', '#8C8C8C', '#B8B8B8', '#E3E3E3']

    # --- PLOT 1: ACCURACY ---
    ax1 = acc_pivot.plot(kind='bar', figsize=(14, 7), color=greyscale_colors, edgecolor='black', width=0.8)
    plt.title('Average Branch Prediction Accuracy per Benchmark', fontsize=16, fontweight='bold')
    plt.ylabel('Accuracy (%)', fontsize=14)
    plt.xlabel('SPEC CPU2017', fontsize=14)
    plt.ylim(0, 100)
    plt.xticks(rotation=0, fontsize=12) 
    
    # Move legend outside the plot to avoid overlapping bars
    plt.legend(title='Predictors', bbox_to_anchor=(1.02, 1), loc='upper left', fontsize=11, title_fontsize=12)
    plt.grid(axis='y', linestyle='--', alpha=0.5)
    
    plt.tight_layout()
    plt.savefig('benchmark_accuracy_grey.png', dpi=300)
    plt.close()
    print("Successfully created: benchmark_accuracy_grey.png")

    # --- PLOT 2: MPKI ---
    ax2 = mpki_pivot.plot(kind='bar', figsize=(14, 7), color=greyscale_colors, edgecolor='black', width=0.8)
    plt.title('Average MPKI per Benchmark', fontsize=16, fontweight='bold')
    plt.ylabel('MPKI', fontsize=14)
    plt.xlabel('SPEC CPU2017', fontsize=14)
    plt.xticks(rotation=0, fontsize=12)
    
    # Move legend outside the plot
    plt.legend(title='Predictors', bbox_to_anchor=(1.02, 1), loc='upper left', fontsize=11, title_fontsize=12)
    plt.grid(axis='y', linestyle='--', alpha=0.5)

    plt.tight_layout()
    plt.savefig('benchmark_mpki_grey.png', dpi=300)
    plt.close()
    print("Successfully created: benchmark_mpki_grey.png")

if __name__ == "__main__":
    main()
