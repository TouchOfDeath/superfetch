# Superfetch

A blazing fast, sub-millisecond system information tool written in C.

Superfetch is an ultra-optimized alternative to tools like Neofetch and Fastfetch. By aggressively caching hardware metadata and bypassing `glibc` overhead with raw syscalls, it achieves execution times under **1 millisecond** (typically ~600µs on modern hardware).

## Features
- **Sub-Millisecond Execution:** Bypasses `fopen` and `fstat` using low-level POSIX `open/read` stack buffers.
- **Hardware Caching:** Caches massive files like `/usr/share/hwdata/pci.ids` and package directory reads for near-instant launches.
- **Dynamic Threading:** Threading overhead (futex) is completely removed for sequential execution, dropping syscalls to an absolute minimum.
- **Built-in Profiling:** Use the `--timing` flag to get a microsecond (`µs`) breakdown of exactly where the program spends its time.

## Installation

You can download the latest `.deb` package from the [Releases](https://github.com/) page and install it globally:

```bash
sudo apt install ./superfetch-1.0.deb
```

## Building from Source

To compile the latest version from source:

```bash
git clone https://github.com/YOUR_USERNAME/superfetch.git
cd superfetch
make
sudo make install
```

## Usage

Simply run:
```bash
superfetch
```

To see the internal performance profile:
```bash
superfetch --timing
```
