#!/bin/bash

# OULY Documentation Build Script
# This script builds the complete documentation using Doxygen, Breathe, and Sphinx

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DOCS_DIR="$PROJECT_ROOT/docs"
BUILD_DIR="$DOCS_DIR/build"
XML_DIR="$DOCS_DIR/xml"
HTML_DIR="$BUILD_DIR/html"

echo -e "${BLUE}OULY Documentation Build Script${NC}"
echo "=================================="
echo "Project root: $PROJECT_ROOT"
echo "Documentation: $DOCS_DIR"
echo ""

# Function to print status messages
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    # Check for Doxygen
    if ! command -v doxygen &> /dev/null; then
        print_error "Doxygen is not installed. Please install it first."
        echo "  Ubuntu/Debian: sudo apt-get install doxygen"
        echo "  macOS: brew install doxygen"
        exit 1
    fi
    
    # Check for Python
    if ! command -v python3 &> /dev/null; then
        print_error "Python 3 is not installed. Please install it first."
        exit 1
    fi
    
    print_success "All dependencies found"
}

# Setup Python virtual environment
setup_python_env() {
    print_status "Setting up Python environment..."
    
    cd "$DOCS_DIR"
    
    # Create virtual environment if it doesn't exist
    if [ ! -d "venv" ]; then
        print_status "Creating Python virtual environment..."
        python3 -m venv venv
    fi
    
    # Activate virtual environment
    source venv/bin/activate
    
    # Install/upgrade pip
    pip install --upgrade pip
    
    # Install requirements
    print_status "Installing Python dependencies..."
    pip install -r requirements.txt
    
    print_success "Python environment ready"
}

# Generate Doxygen XML
generate_doxygen() {
    print_status "Generating Doxygen XML documentation..."
    
    cd "$PROJECT_ROOT"
    
    # Clean previous XML output
    if [ -d "$XML_DIR" ]; then
        rm -rf "$XML_DIR"
    fi
    
    # Run Doxygen
    if doxygen Doxyfile; then
        print_success "Doxygen XML generation completed"
    else
        print_error "Doxygen generation failed"
        exit 1
    fi
    
    # Check if XML was generated
    if [ ! -d "$XML_DIR" ]; then
        print_error "Doxygen XML output directory not found"
        exit 1
    fi
}

# Build Sphinx documentation
build_sphinx() {
    print_status "Building Sphinx HTML documentation..."
    
    cd "$DOCS_DIR"
    
    # Activate virtual environment
    source venv/bin/activate
    
    # Clean previous build
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    
    # Build documentation
    if sphinx-build -b html source "$HTML_DIR" -E -a; then
        print_success "Sphinx documentation build completed"
    else
        print_error "Sphinx build failed"
        exit 1
    fi
}

# Validate build output
validate_build() {
    print_status "Validating build output..."
    
    # Check if HTML was generated
    if [ ! -f "$HTML_DIR/index.html" ]; then
        print_error "Main HTML file not found"
        exit 1
    fi
    
    # Check for API reference
    if [ ! -f "$HTML_DIR/api_reference/index.html" ]; then
        print_warning "API reference HTML not found - this might be expected if no Doxygen comments exist"
    fi
    
    # Calculate size
    local size=$(du -sh "$HTML_DIR" 2>/dev/null | cut -f1)
    print_success "Documentation build validated (Size: $size)"
}

# Main build function
main() {
    echo "Starting documentation build process..."
    echo ""
    
    # Parse command line arguments
    CLEAN_BUILD=false
    SERVE_DOCS=false
    OPEN_BROWSER=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --clean)
                CLEAN_BUILD=true
                shift
                ;;
            --serve)
                SERVE_DOCS=true
                shift
                ;;
            --open)
                OPEN_BROWSER=true
                shift
                ;;
            --help|-h)
                echo "Usage: $0 [OPTIONS]"
                echo ""
                echo "Options:"
                echo "  --clean     Clean all build artifacts before building"
                echo "  --serve     Start a local HTTP server after building"
                echo "  --open      Open the documentation in the default browser"
                echo "  --help, -h  Show this help message"
                echo ""
                echo "Examples:"
                echo "  $0                    # Build documentation"
                echo "  $0 --clean           # Clean build"
                echo "  $0 --serve --open    # Build, serve, and open in browser"
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
    
    # Clean build if requested
    if [ "$CLEAN_BUILD" = true ]; then
        print_status "Cleaning previous build artifacts..."
        rm -rf "$BUILD_DIR" "$XML_DIR" "$DOCS_DIR/venv"
        print_success "Clean completed"
    fi
    
    # Build process
    check_dependencies
    setup_python_env
    generate_doxygen
    build_sphinx
    validate_build
    
    echo ""
    echo "=================================="
    print_success "Documentation build completed successfully!"
    echo ""
    echo "Output location: $HTML_DIR"
    echo "Main page: file://$HTML_DIR/index.html"
    
    # Serve documentation if requested
    if [ "$SERVE_DOCS" = true ]; then
        echo ""
        print_status "Starting local web server..."
        cd "$HTML_DIR"
        
        # Find available port
        PORT=8000
        while lsof -Pi :$PORT -sTCP:LISTEN -t >/dev/null; do
            PORT=$((PORT + 1))
        done
        
        echo "Server will be available at: http://localhost:$PORT"
        
        # Open browser if requested
        if [ "$OPEN_BROWSER" = true ]; then
            if command -v open &> /dev/null; then
                open "http://localhost:$PORT"  # macOS
            elif command -v xdg-open &> /dev/null; then
                xdg-open "http://localhost:$PORT"  # Linux
            elif command -v start &> /dev/null; then
                start "http://localhost:$PORT"  # Windows (if running in WSL)
            fi
        fi
        
        # Start server (this will block)
        python3 -m http.server $PORT
    else
        echo ""
        echo "To view the documentation:"
        echo "  Open file://$HTML_DIR/index.html in your browser"
        echo "  Or run: $0 --serve --open"
    fi
}

# Run main function with all arguments
main "$@"