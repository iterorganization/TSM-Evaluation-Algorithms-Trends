#!/bin/bash
set -e

COV_DIR="/home/ITER/iglesid/work/Projects/TSM/reconstruction_algorithms_model_based/evaluation_algorithms_trends/public/coverage"
SRC_DIR="/home/ITER/iglesid/work/Projects/TSM/reconstruction_algorithms_model_based/evaluation_algorithms_trends/public"

mkdir -p "$COV_DIR"
cd "$SRC_DIR"

echo "Capturing initial coverage..."
lcov --capture --directory build --include "/home/ITER/iglesid/work/Projects/TSM/reconstruction_algorithms_model_based/evaluation_algorithms_trends/public/src/c++/*" --initial -o $COV_DIR/initial.coverage.info -q

echo "Capturing main coverage..."
lcov --capture --directory build --include "/home/ITER/iglesid/work/Projects/TSM/reconstruction_algorithms_model_based/evaluation_algorithms_trends/public/src/c++/*" -o $COV_DIR/main.coverage.info -q

echo "Merging initial and main coverage files..."
lcov -a $COV_DIR/initial.coverage.info -a $COV_DIR/main.coverage.info -o $COV_DIR/coverage_cpp.info -q

if [ "" = "ON" ]; then
    echo "Capturing Python coverage..."
     -m coverage lcov \
        --data-file=$COV_DIR/.coverage \
        --omit=/tmp/*,/home/ITER/iglesid/work/Projects/TSM/reconstruction_algorithms_model_based/evaluation_algorithms_trends/public/tests/* \
        -o $COV_DIR/coverage_python.info -q
    # coverage.py LCOV output includes column numbers,
    # while C++ lcov does not. This causes conflicts when merging.
    # Remove the column numbers to ensure compatibility during merge.
    sed -i -E 's/,[0-9]+,/,/g' $COV_DIR/coverage_python.info
    echo "Merging Python and C++ coverage files..."
    lcov -a $COV_DIR/coverage_cpp.info -a $COV_DIR/coverage_python.info -o $COV_DIR/coverage.info -q
fi

echo "Generating HTML report..."
genhtml $COV_DIR/ -o $COV_DIR/html --function-coverage --demangle-cpp -p $SRC_DIR/src -q

echo "Coverage report generated: $COV_DIR/html/index.html"
