# TAGE Branch Predictor

1. **Clone the repository:**
clone the champsim repo

```bash
git clone https://github.com/ChampSim/ChampSim.git

```


2. **Download dependencies:**
Navigate in the champsim folder
Download depedencies

```bash
git submodule update --init
vcpkg/bootstrap-vcpkg.sh
vcpkg/vcpkg install

```


3. **Configure the branch predictor:**
Move the TAGE folder with the source files in the branch directory
Open the `champsim_config.json`
and make sure that the selected branch predictor is TAGE
in line 31:

```json
"branch_predictor": "TAGE"

```


4. **Build:** This may take some time.

```bash
./config.sh champsim_config.json
make

```

5. **Download traces:**
Download traces from [https://dpc3.compas.cs.stonybrook.edu/champsim-traces/speccpu/](https://dpc3.compas.cs.stonybrook.edu/champsim-traces/speccpu/)


6. **Run:**

```bash
bin/champsim --warmup-instructions 20000000 --simulation-instructions 50000000 ~/path/to/traces/600.perlbench_s-210B.champsimtrace.xz

```
