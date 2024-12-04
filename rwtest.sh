#!/bin/bash

# Default port number
PORT=8080

# Parse arguments for port number (optional)
while getopts "p:" opt; do
    case $opt in
        p) PORT=$OPTARG ;;
        *) echo "Usage: $0 [-p port]"; exit 1 ;;
    esac
done

# Initialize output directory
OUTPUT_DIR="./output"
if [[ -d $OUTPUT_DIR ]]; then
    rm -rf $OUTPUT_DIR  # Delete the directory if it exists
fi
mkdir -p $OUTPUT_DIR    # Create a new directory

# Define the list of requests, expected responses, and intervals between requests

# REQUESTS=(
#     "CREATE hello 1"
#     "CREATE hello 2"
#     "CREATE hello 3"
#     "CREATE hello 4"
# )
# RESPONSES=(
#     "CREATE OK"
#     "COLLISION"
#     "COLLISION"
#     "COLLISION"
# )
# INTERVALS=(5 5 5 5)

# REQUESTS=(
#     "CREATE hello world"
#     "READ hello"
#     "READ hello"
#     "READ hello"
# )
# RESPONSES=(
#     "CREATE OK"
#     "world"
#     "world"
#     "world"
# )
# INTERVALS=(5 5 0 0)  # Time intervals: (initial delay + subsequent intervals)

# REQUESTS=(
#     "CREATE hello world"
#     "READ hello"
#     "CREATE bye snu"
#     "READ bye"
# )
# RESPONSES=(
#     "CREATE OK"
#     "world"
#     "CREATE OK"
#     "snu"
# )
# INTERVALS=(5 5 -5 0)  # Time intervals: (initial delay + subsequent intervals)

REQUESTS=(
    "CREATE hello world"
    "DELETE bye"
    "READ hello"
    "UPDATE hello snu"
    "DELETE hello"
    "READ hello"
)
RESPONSES=(
    "CREATE OK"
    "NOT FOUND"
    "world"
    "UPDATE OK"
    "DELETE OK"
    "world"
)
# INTERVALS=(5 0 5 5 5 -10)  # Time intervals: (first request delay + subsequent)

echo "=== Starting Requests and Responses ==="

# Execute requests and record response times
START_TIME=$(date +%s)
RESPONSE_TIMES=()

for i in "${!REQUESTS[@]}"; do
    echo "Sending Request $i: '${REQUESTS[$i]}'"
    
    # Send request and capture the response time
    { 
        echo "${REQUESTS[$i]}" | ./client -p $PORT | ts '[%Y-%m-%d %H:%M:%S]' > \
        "$OUTPUT_DIR/response_$i.log"
        RESPONSE_TIMES[$i]=$(date +%s)  # Record the time when response is received
    } &
    
    # Small delay to ensure proper sequence
    sleep 0.1
done

# Wait for all background processes to finish
wait

echo "=== Verifying Responses ==="

# Function to check if the expected string exists in the log file
check_required_string() {
    local file=$1
    local string=$2
    if ! grep -q "$string" "$file"; then
        echo -e "\033[31mError: Expected '$string' not found in $file\033[0m"
        return 1
    fi
    return 0
}

# Verify the responses
for i in "${!REQUESTS[@]}"; do
    echo "Checking response for Request $i..."
    check_required_string "$OUTPUT_DIR/response_$i.log" "${RESPONSES[$i]}" || { 
        echo -e "\033[31mTest Failed: '${RESPONSES[$i]}' missing in response_$i.log\033[0m"
        exit 1
    }
    echo "Response for Request $i: '${RESPONSES[$i]}' verified successfully."
done

echo "=== Calculating Response Time Differences ==="

# Verify the response time intervals
for ((i = 1; i < ${#RESPONSE_TIMES[@]}; i++)); do
    diff=$((RESPONSE_TIMES[$i] - RESPONSE_TIMES[$((i - 1))]))
    expected_interval="${INTERVALS[$i]}"
    echo "Response $((i - 1)) to Response $i: $diff seconds (expected: \
    $expected_interval seconds)"
    
    # Allow small deviations (±1 second)
    if [[ $diff -lt $((expected_interval - 1)) || $diff -gt $((expected_interval + 1)) ]]; then
        echo -e "\033[31mTest Failed: Time difference between Response $((i - 1)) and \
        Response $i is not in range $((expected_interval - 1))-$((expected_interval + 1)) \
        seconds\033[0m"
        exit 1
    fi
done

echo -e "\033[32mTest Passed: All conditions satisfied.\033[0m"
exit 0
