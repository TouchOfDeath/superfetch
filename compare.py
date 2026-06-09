import subprocess
import os

def run_cmd(cmd):
    try:
        return subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, text=True)
    except subprocess.CalledProcessError as e:
        return e.output

report_path = "/home/lifelonglearner/.gemini/antigravity/scratch/superfetch/comparison_report.txt"

with open(report_path, "w") as f:
    f.write("=== Superfetch vs Fastfetch Aggressive Comparison ===\n\n")

    # 1. Binary Size
    f.write("1. Binary Size\n")
    f.write("-" * 30 + "\n")
    superfetch_bin = "./superfetch"
    fastfetch_bin = subprocess.getoutput("which fastfetch")
    if os.path.exists(superfetch_bin):
        f.write(f"Superfetch: {os.path.getsize(superfetch_bin) / 1024:.2f} KB\n")
    else:
        f.write("Superfetch binary not found.\n")
    
    if fastfetch_bin and os.path.exists(fastfetch_bin):
        f.write(f"Fastfetch:  {os.path.getsize(fastfetch_bin) / 1024:.2f} KB\n")
    else:
        f.write("Fastfetch binary not found.\n")
    f.write("\n")

    # 2. Performance (Hyperfine)
    f.write("2. Execution Time (hyperfine)\n")
    f.write("-" * 30 + "\n")
    hyperfine_out = run_cmd("hyperfine --warmup 5 './superfetch' 'fastfetch'")
    f.write(hyperfine_out + "\n\n")

    # 3. Syscalls (strace)
    f.write("3. Syscalls (strace -c)\n")
    f.write("-" * 30 + "\n")
    f.write("Superfetch:\n")
    # strace outputs to stderr, so redirect to stdout
    f.write(run_cmd("strace -c ./superfetch >/dev/null 2>&1") + "\n")
    f.write("Fastfetch:\n")
    f.write(run_cmd("strace -c fastfetch >/dev/null 2>&1") + "\n\n")

    # 4. Memory Usage (/usr/bin/time -v)
    f.write("4. Memory Usage (Maximum Resident Set Size)\n")
    f.write("-" * 30 + "\n")
    sf_time = run_cmd("/usr/bin/time -v ./superfetch >/dev/null 2>&1")
    for line in sf_time.splitlines():
        if "Maximum resident set size" in line:
            f.write(f"Superfetch: {line.split(':')[-1].strip()} KB\n")
            break
            
    ff_time = run_cmd("/usr/bin/time -v fastfetch >/dev/null 2>&1")
    for line in ff_time.splitlines():
        if "Maximum resident set size" in line:
            f.write(f"Fastfetch:  {line.split(':')[-1].strip()} KB\n")
            break
    f.write("\n")

    # 5. Output comparison (Lines, Characters)
    f.write("5. Output Structure\n")
    f.write("-" * 30 + "\n")
    sf_out = run_cmd("./superfetch")
    ff_out = run_cmd("fastfetch")
    f.write(f"Superfetch: {len(sf_out.splitlines())} lines, {len(sf_out)} characters\n")
    f.write(f"Fastfetch:  {len(ff_out.splitlines())} lines, {len(ff_out)} characters\n")

print(f"Report generated at {report_path}")
