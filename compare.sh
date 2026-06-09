#!/bin/bash

REPORT="comparison_report.txt"
echo "=== Superfetch vs Fastfetch Aggressive Comparison ===" > "$REPORT"
echo "" >> "$REPORT"

echo "1. Binary Size" >> "$REPORT"
echo "------------------------------" >> "$REPORT"
ls -lh ./superfetch | awk '{print "Superfetch: " $5}' >> "$REPORT"
ls -lh $(which fastfetch) | awk '{print "Fastfetch: " $5}' >> "$REPORT"
echo "" >> "$REPORT"

echo "2. Execution Time (hyperfine)" >> "$REPORT"
echo "------------------------------" >> "$REPORT"
hyperfine --warmup 3 './superfetch' 'fastfetch' >> "$REPORT" 2>&1
echo "" >> "$REPORT"

echo "3. Syscalls (strace -c)" >> "$REPORT"
echo "------------------------------" >> "$REPORT"
echo "Superfetch:" >> "$REPORT"
strace -c ./superfetch >/dev/null 2>> "$REPORT"
echo "" >> "$REPORT"
echo "Fastfetch:" >> "$REPORT"
strace -c fastfetch >/dev/null 2>> "$REPORT"
echo "" >> "$REPORT"

echo "4. Memory Usage (Maximum Resident Set Size)" >> "$REPORT"
echo "------------------------------" >> "$REPORT"
echo "Superfetch:" >> "$REPORT"
/usr/bin/time -v ./superfetch >/dev/null 2>&1 | grep "Maximum resident set size" >> "$REPORT"
/usr/bin/time -v ./superfetch >/dev/null 2>> "$REPORT"
echo "Fastfetch:" >> "$REPORT"
/usr/bin/time -v fastfetch >/dev/null 2>&1 | grep "Maximum resident set size" >> "$REPORT"
/usr/bin/time -v fastfetch >/dev/null 2>> "$REPORT"
echo "" >> "$REPORT"

echo "5. Output Structure" >> "$REPORT"
echo "------------------------------" >> "$REPORT"
echo "Superfetch: $(./superfetch | wc -l) lines, $(./superfetch | wc -c) characters" >> "$REPORT"
echo "Fastfetch:  $(fastfetch | wc -l) lines, $(fastfetch | wc -c) characters" >> "$REPORT"

echo "Report generated at $REPORT"
