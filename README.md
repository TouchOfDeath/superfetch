# Superfetch

A blazing fast, sub-millisecond system information tool written in raw C.

Superfetch is an ultra-optimized alternative to tools like Neofetch and Fastfetch. By aggressively caching hardware metadata and bypassing `glibc` overhead with raw syscalls, it achieves execution times under **1 millisecond** (typically ~600µs on modern hardware).

## Features
- **Sub-Millisecond Execution:** Bypasses `fopen` and `fstat` using low-level POSIX `open/read` stack buffers.
- **Hardware Caching:** Caches massive files like `/usr/share/hwdata/pci.ids` and package directory reads for near-instant launches.
- **Dynamic Color Themes:** Automatically reads `pywal` and `~/.Xresources` for perfect terminal matching.
- **Dynamic Logos:** Reads `/etc/os-release` to dynamically switch ASCII logos for Ubuntu, Arch, Fedora, Pop!_OS, and many more.
- **Built-in Profiling:** Use the `--timing` flag to get a microsecond (`µs`) breakdown of exactly where the program spends its time.

## Installation

### Ubuntu / Debian / Pop!_OS
Available via Launchpad PPA (supports `amd64`, `arm64`, and `armhf`):
```bash
sudo add-apt-repository ppa:lifelonglearner/superfetch
sudo apt update
sudo apt install superfetch
```

### Fedora / CentOS / RHEL / AlmaLinux
Available via Fedora Copr (supports `x86_64` and `aarch64`):
```bash
sudo dnf copr enable touchofdeath/superfetch
sudo dnf install superfetch
```

### NixOS
Nix Flakes are fully supported. Run it instantly without installing:
```bash
nix run github:TouchOfDeath/superfetch
```
Or install it permanently to your Nix profile:
```bash
nix profile install github:TouchOfDeath/superfetch
```

### Arch Linux / Manjaro
The repository includes an official `PKGBUILD`. To compile and install natively via `pacman`:
```bash
git clone https://github.com/TouchOfDeath/superfetch.git
cd superfetch
makepkg -si
```

## Building from Source

To compile manually on any other Linux distribution:
```bash
git clone https://github.com/TouchOfDeath/superfetch.git
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
