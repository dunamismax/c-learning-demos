#!/bin/bash

# Test runner script for C Learning Demos
# This script runs all tests and provides detailed reporting

set -e

echo "=== C Learning Demos Test Runner ==="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_test() {
    echo -e "${BLUE}[TEST]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "Makefile" ]; then
    print_error "No Makefile found. Please run this script from the project root."
    exit 1
fi

# Parse command line arguments
BUILD_MODE="release"
VERBOSE=false
UNIT_TESTS=true
INTEGRATION_TESTS=false
PERFORMANCE_TESTS=false
STOP_ON_FAILURE=false
COVERAGE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -m|--mode)
            BUILD_MODE="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -u|--unit-only)
            UNIT_TESTS=true
            INTEGRATION_TESTS=false
            PERFORMANCE_TESTS=false
            shift
            ;;
        -i|--integration)
            INTEGRATION_TESTS=true
            shift
            ;;
        -p|--performance)
            PERFORMANCE_TESTS=true
            shift
            ;;
        -a|--all)
            UNIT_TESTS=true
            INTEGRATION_TESTS=true
            PERFORMANCE_TESTS=true
            shift
            ;;
        -s|--stop-on-failure)
            STOP_ON_FAILURE=true
            shift
            ;;
        -c|--coverage)
            COVERAGE=true
            BUILD_MODE="profile"
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -m, --mode MODE      Build mode (debug, release, profile)"
            echo "  -v, --verbose        Verbose output"
            echo "  -u, --unit-only      Run only unit tests (default)"
            echo "  -i, --integration    Run integration tests"
            echo "  -p, --performance    Run performance tests"
            echo "  -a, --all            Run all tests"
            echo "  -s, --stop-on-failure Stop on first failure"
            echo "  -c, --coverage       Run with coverage analysis"
            echo "  -h, --help           Show this help"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

print_status "Test mode: $BUILD_MODE"
print_status "Running tests..."

# Build tests first
print_status "Building tests..."
if ! make MODE=$BUILD_MODE build-tests; then
    print_error "Failed to build tests"
    exit 1
fi

# Initialize test results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Function to run a test
run_test() {
    local test_name=$1
    local test_path=$2
    
    print_test "Running $test_name"
    
    if [ "$VERBOSE" = true ]; then
        echo "  Command: $test_path"
    fi
    
    # Run the test and capture output
    if output=$($test_path 2>&1); then
        echo -e "  ${GREEN}PASSED${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        if [ "$VERBOSE" = true ]; then
            echo "$output" | sed 's/^/    /'
        fi
    else
        echo -e "  ${RED}FAILED${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        echo "$output" | sed 's/^/    /'
        
        if [ "$STOP_ON_FAILURE" = true ]; then
            print_error "Stopping on first failure"
            exit 1
        fi
    fi
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo ""
}

# Run unit tests
if [ "$UNIT_TESTS" = true ]; then
    print_status "Running unit tests..."
    echo ""
    
    # Find all unit test executables
    for test_file in build/$BUILD_MODE/bin/test_*; do
        if [ -f "$test_file" ] && [ -x "$test_file" ]; then
            test_name=$(basename "$test_file")
            run_test "$test_name" "$test_file"
        fi
    done
fi

# Run integration tests
if [ "$INTEGRATION_TESTS" = true ]; then
    print_status "Running integration tests..."
    echo ""
    
    # Find all integration test executables
    for test_file in build/$BUILD_MODE/bin/integration_*; do
        if [ -f "$test_file" ] && [ -x "$test_file" ]; then
            test_name=$(basename "$test_file")
            run_test "$test_name" "$test_file"
        fi
    done
fi

# Run performance tests
if [ "$PERFORMANCE_TESTS" = true ]; then
    print_status "Running performance tests..."
    echo ""
    
    # Find all performance test executables
    for test_file in build/$BUILD_MODE/bin/perf_*; do
        if [ -f "$test_file" ] && [ -x "$test_file" ]; then
            test_name=$(basename "$test_file")
            run_test "$test_name" "$test_file"
        fi
    done
fi

# Generate coverage report if requested
if [ "$COVERAGE" = true ]; then
    print_status "Generating coverage report..."
    
    # Check if gcov is available
    if command -v gcov > /dev/null 2>&1; then
        mkdir -p coverage
        
        # Find all .gcda files and generate coverage
        find build/$BUILD_MODE -name "*.gcda" -exec gcov {} \; > coverage/coverage.log 2>&1
        
        # Generate HTML report if lcov is available
        if command -v lcov > /dev/null 2>&1 && command -v genhtml > /dev/null 2>&1; then
            lcov --capture --directory build/$BUILD_MODE --output-file coverage/coverage.info
            genhtml coverage/coverage.info --output-directory coverage/html
            print_status "Coverage report generated in coverage/html/index.html"
        else
            print_warning "lcov/genhtml not found, basic coverage info in coverage/coverage.log"
        fi
    else
        print_warning "gcov not found, skipping coverage report"
    fi
fi

# Print final results
echo "=== Test Results ==="
echo "Total tests: $TOTAL_TESTS"
echo "Passed: $PASSED_TESTS"
echo "Failed: $FAILED_TESTS"
echo "Skipped: $SKIPPED_TESTS"

if [ $TOTAL_TESTS -eq 0 ]; then
    print_warning "No tests were run!"
    exit 1
elif [ $FAILED_TESTS -eq 0 ]; then
    print_status "All tests passed! ✓"
    exit 0
else
    print_error "Some tests failed! ✗"
    exit 1
fi