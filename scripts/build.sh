#!/bin/bash

# Build script for C Learning Demos
# This script builds all libraries and applications

set -e

echo "=== C Learning Demos Build Script ==="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
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

# Check if we're in the right directory
if [ ! -f "Makefile" ]; then
    print_error "No Makefile found. Please run this script from the project root."
    exit 1
fi

# Parse command line arguments
BUILD_MODE="release"
CLEAN_BUILD=false
VERBOSE=false
BUILD_TESTS=false
RUN_TESTS=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_MODE="debug"
            shift
            ;;
        -p|--profile)
            BUILD_MODE="profile"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -t|--tests)
            BUILD_TESTS=true
            shift
            ;;
        -r|--run-tests)
            BUILD_TESTS=true
            RUN_TESTS=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -d, --debug      Build in debug mode"
            echo "  -p, --profile    Build in profile mode"
            echo "  -c, --clean      Clean build (remove build directory first)"
            echo "  -v, --verbose    Verbose output"
            echo "  -t, --tests      Build tests"
            echo "  -r, --run-tests  Build and run tests"
            echo "  -h, --help       Show this help"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

print_status "Build mode: $BUILD_MODE"

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    print_status "Cleaning build directory..."
    make clean
fi

# Set verbose flag for make
MAKE_FLAGS=""
if [ "$VERBOSE" = true ]; then
    MAKE_FLAGS="V=1"
fi

# Build libraries
print_status "Building libraries..."
if ! make MODE=$BUILD_MODE libs $MAKE_FLAGS; then
    print_error "Failed to build libraries"
    exit 1
fi

# Build applications
print_status "Building applications..."
if ! make MODE=$BUILD_MODE apps $MAKE_FLAGS; then
    print_error "Failed to build applications"
    exit 1
fi

# Build tests if requested
if [ "$BUILD_TESTS" = true ]; then
    print_status "Building tests..."
    if ! make MODE=$BUILD_MODE build-tests $MAKE_FLAGS; then
        print_error "Failed to build tests"
        exit 1
    fi
    
    # Run tests if requested
    if [ "$RUN_TESTS" = true ]; then
        print_status "Running tests..."
        if ! make MODE=$BUILD_MODE test; then
            print_error "Some tests failed"
            exit 1
        fi
    fi
fi

print_status "Build completed successfully!"

# Show build results
echo ""
echo "=== Build Results ==="
echo "Build mode: $BUILD_MODE"
echo "Libraries built: $(find build/$BUILD_MODE/lib -name "*.a" 2>/dev/null | wc -l)"
echo "Applications built: $(find build/$BUILD_MODE/bin -type f -executable 2>/dev/null | wc -l)"

if [ "$BUILD_TESTS" = true ]; then
    echo "Tests built: $(find build/$BUILD_MODE/bin -name "test_*" 2>/dev/null | wc -l)"
fi

echo ""
echo "To run applications:"
echo "  ./build/$BUILD_MODE/bin/<application_name>"
echo ""
echo "To run tests:"
echo "  make MODE=$BUILD_MODE test"
echo ""
echo "To install (optional):"
echo "  make MODE=$BUILD_MODE install"