#!/bin/bash

# Default to 100 files if no argument is provided
NUM_FILES=${1:-100}
TEST_DIR="testfile"
ARCHIVE="testfile.sxz"

echo "[0/4] Compiling latest sxzip engine (Silent)..."
make -C ../build -j4 > /dev/null 2>&1
cp ../build/sxzip ./sxzip

# Clean up any previous runs
rm -rf "$TEST_DIR" "$ARCHIVE" extracted_dir
mkdir -p "$TEST_DIR"

echo "[1/4] Generating $NUM_FILES mixed-entropy files in $TEST_DIR/..."
for (( i=1; i<=NUM_FILES; i++ )); do
    echo "This is test file number $i." > "$TEST_DIR/file_$i.txt"
    echo "Sxzip is an incredibly advanced compressor." >> "$TEST_DIR/file_$i.txt"
    if (( i % 5 == 0 )); then
        head -c 200 /dev/urandom | base64 >> "$TEST_DIR/file_$i.txt"
    fi
    if (( i % 10 == 0 )); then
        head -c 200 /dev/urandom >> "$TEST_DIR/file_$i.txt"
    fi
done

ORIG_SIZE_BYTES=$(python3 -c "import os; print(sum(os.path.getsize(os.path.join('$TEST_DIR', f)) for f in os.listdir('$TEST_DIR') if os.path.isfile(os.path.join('$TEST_DIR', f))))")
ORIG_SIZE_KB=$(echo "scale=2; $ORIG_SIZE_BYTES / 1024" | bc)

echo "[2/4] Running Benchmarks (Suppressing raw output)..."

declare -a MODES=("Baseline(-e 4096)" "Default(-e 200)" "Auto-Tune(-E)" "BruteForce(-Ea)")
declare -a PARAMS=("-e 4096" "-e 200" "-E" "-Ea")

# Table Header
echo ""
printf "+-------------------+-----------+----------------+---------------+------------+\n"
printf "| Mode              | Orig Size | Compressed     | Space Saved   | Time (ms)  |\n"
printf "+-------------------+-----------+----------------+---------------+------------+\n"

for i in "${!MODES[@]}"; do
    mode_name="${MODES[$i]}"
    param="${PARAMS[$i]}"
    
    # Run compression silently, capture time
    T1=$(python3 -c "import time; print(int(time.time() * 1000))")
    ./sxzip -c "$TEST_DIR" "$ARCHIVE" $param > /dev/null 2>&1
    T2=$(python3 -c "import time; print(int(time.time() * 1000))")
    
    TIME_MS=$((T2 - T1))
    
    # Get compressed size
    COMP_SIZE_BYTES=$(stat -f%z "$ARCHIVE")
    COMP_SIZE_KB=$(echo "scale=2; $COMP_SIZE_BYTES / 1024" | bc)
    
    # Calculate ratio
    SAVED_PCT=$(echo "scale=2; 100 - ($COMP_SIZE_BYTES * 100 / $ORIG_SIZE_BYTES)" | bc)
    
    printf "| %-17s | %6s KB | %6s KB (%5s) | %6s %%      | %6s ms |\n" \
        "$mode_name" "$ORIG_SIZE_KB" "$COMP_SIZE_KB" "$COMP_SIZE_BYTES" "$SAVED_PCT" "$TIME_MS"
done
printf "+-------------------+-----------+----------------+---------------+------------+\n"

echo ""
echo "[3/4] Verifying extraction on last archive (-E)..."
mkdir -p extracted_dir
cd extracted_dir
../sxzip -d "../$ARCHIVE" testfile_recovered > /dev/null 2>&1
cd ..

if diff -r "$TEST_DIR" "extracted_dir/testfile" > /dev/null 2>&1; then
    echo "[4/4] ✅ SUCCESS: Extracted directory perfectly matches original!"
else
    echo "[4/4] ❌ ERROR: Differences found in extraction!"
fi

echo "======================================"
echo " Benchmark Complete! "
echo "======================================"
