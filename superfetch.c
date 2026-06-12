// superfetch v4 — blazing fast system information tool
// Dynamic Colors, Universal Packages, Advanced Hardware

#ifdef ENABLE_IMAGES
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <pthread.h>
#include <malloc.h>
#include <sys/ioctl.h>

// ── Define dynamic color variables BEFORE config.h ────────────
char COLOR_USER[32]      = "\033[1;97m";
char COLOR_AT[32]        = "\033[0;90m";
char COLOR_HOST[32]      = "\033[1;97m";
char COLOR_SEP[32]       = "\033[0;90m";
char COLOR_LABEL[32]     = "\033[1;96m";
char COLOR_ICON[32]      = "\033[1;96m";
char COLOR_VALUE[32]     = "\033[0;97m";
char COLOR_BAR_FILL[32]  = "\033[38;5;39m";
char COLOR_BAR_EMPTY[32] = "\033[0;90m";
char COLOR_TEMP_OK[32]   = "\033[1;32m";
char COLOR_TEMP_WARN[32] = "\033[1;33m";
char COLOR_TEMP_HOT[32]  = "\033[1;31m";

#include "config.h"
#include "logos.h"

// ── Runtime Configuration (overridable via config file / CLI flags) ──────────
static int   g_bar_width        = BAR_WIDTH;
static int   g_show_labels      = SHOW_LABELS;
static int   g_term_width       = 80;
static int   g_val_max          = 40;
static char *g_forced_logo      = NULL;
static char  g_module_filter[1024] = {0};
static int   g_watch_interval   = 0;
static int   g_colors_from_pywal = 0;

static long long get_time_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

static void ensure_cache_dir() {
    const char *home = getenv("HOME"); if (!home) return;
    char path[512];
    snprintf(path, sizeof(path), "%s/.cache", home);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/.cache/superfetch", home);
    mkdir(path, 0755);
}

static ssize_t read_sys_file(const char *path, char *buf, size_t sz) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t n = read(fd, buf, sz - 1);
    if (n >= 0) buf[n] = '\0';
    close(fd);
    return n;
}

// ════════════════════════════════════════════════════════════
//  UTILITIES
// ════════════════════════════════════════════════════════════

static void strip_quotes_nl(char *s) {
    size_t len = strlen(s);
    if (len && s[len-1] == '\n') s[--len] = '\0';
    if (len >= 2 && (s[0] == '"' || s[0] == '\'') && s[len-1] == s[0]) {
        memmove(s, s+1, len - 1);
        s[len-2] = '\0';
    }
}

static void str_remove_all(char *str, const char *needle) {
    size_t nlen = strlen(needle);
    char *p;
    while ((p = strstr(str, needle)) != NULL)
        memmove(p, p + nlen, strlen(p + nlen) + 1);
}

static void collapse_spaces(char *s) {
    char *r = s, *w = s;
    int sp = 0;
    while (*r) {
        if (*r == ' ') { if (!sp) { *w++ = ' '; sp = 1; } }
        else           { *w++ = *r; sp = 0; }
        r++;
    }
    *w = '\0';
}

static void strip_ansi(char *s) {
    char *r = s, *w = s;
    while (*r) {
        if (*r == '\033') {
            r++;
            if (*r == '[') {
                r++;
                while (*r && ((*r >= '0' && *r <= '9') || *r == ';')) r++;
                if (*r) r++;
            }
        } else {
            *w++ = *r++;
        }
    }
    *w = '\0';
}

static void json_escape(const char *src, char *dst, size_t dst_sz) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j < dst_sz - 2; i++) {
        switch (src[i]) {
            case '"':  if (j < dst_sz-3) { dst[j++] = '\\'; dst[j++] = '"'; } break;
            case '\\': if (j < dst_sz-3) { dst[j++] = '\\'; dst[j++] = '\\'; } break;
            case '\n': if (j < dst_sz-3) { dst[j++] = '\\'; dst[j++] = 'n'; } break;
            case '\t': if (j < dst_sz-3) { dst[j++] = '\\'; dst[j++] = 't'; } break;
            default: dst[j++] = src[i]; break;
        }
    }
    dst[j] = '\0';
}

#ifdef ENABLE_IMAGES
static const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *base64_encode(const unsigned char *src, size_t len, size_t *out_len) {
    *out_len = 4 * ((len + 2) / 3);
    char *out = (char *)malloc(*out_len + 1);
    if (!out) return NULL;
    size_t i = 0, j = 0;
    while (i < len) {
        unsigned int a = (i < len) ? src[i++] : 0u;
        unsigned int b = (i < len) ? src[i++] : 0u;
        unsigned int c = (i < len) ? src[i++] : 0u;
        unsigned int t = (a << 16) | (b << 8) | c;
        out[j++] = b64chars[(t >> 18) & 0x3F];
        out[j++] = b64chars[(t >> 12) & 0x3F];
        out[j++] = b64chars[(t >>  6) & 0x3F];
        out[j++] = b64chars[(t >>  0) & 0x3F];
    }
    int pad = (3 - (int)(len % 3)) % 3;
    for (int p = 0; p < pad; p++) out[*out_len - 1 - p] = '=';
    out[*out_len] = '\0';
    return out;
}
#endif

// ════════════════════════════════════════════════════════════
//  SYSTEM CATCHES & THEMES
// ════════════════════════════════════════════════════════════

static void parse_pywal(void) {
    const char *home = getenv("HOME");
    if (!home) return;
    char path[512];
    snprintf(path, sizeof(path), "%s/.cache/wal/colors", home);
    if (access(path, R_OK) != 0) return;
    FILE *f = fopen(path, "r");
    if (!f) return;
    
    char lines[16][16] = {0};
    for (int i = 0; i < 16; i++) {
        if (!fgets(lines[i], 16, f)) break;
        strip_quotes_nl(lines[i]);
    }
    fclose(f);

    if (lines[4][0] == '#') { // Color 4 (Blue/Accent)
        unsigned int r,g,b;
        if (sscanf(lines[4], "#%02x%02x%02x", &r, &g, &b) == 3) {
            snprintf(COLOR_USER, 32, "\033[38;2;%u;%u;%um", r, g, b);
            snprintf(COLOR_HOST, 32, "\033[38;2;%u;%u;%um", r, g, b);
            snprintf(COLOR_BAR_FILL, 32, "\033[38;2;%u;%u;%um", r, g, b);
        }
    }
    if (lines[8][0] == '#') { // Color 8 (Dark Gray)
        unsigned int r,g,b;
        if (sscanf(lines[8], "#%02x%02x%02x", &r, &g, &b) == 3) {
            snprintf(COLOR_AT, 32, "\033[38;2;%u;%u;%um", r, g, b);
            snprintf(COLOR_SEP, 32, "\033[38;2;%u;%u;%um", r, g, b);
            snprintf(COLOR_BAR_EMPTY, 32, "\033[38;2;%u;%u;%um", r, g, b);
        }
    }
    if (lines[6][0] == '#') { // Color 6 (Cyan/Accent)
        unsigned int r,g,b;
        if (sscanf(lines[6], "#%02x%02x%02x", &r, &g, &b) == 3) {
            snprintf(COLOR_LABEL, 32, "\033[38;2;%u;%u;%um", r, g, b);
            snprintf(COLOR_ICON, 32, "\033[38;2;%u;%u;%um", r, g, b);
        }
    }
    if (lines[7][0] == '#') { // Color 7 (White/Foreground)
        unsigned int r,g,b;
        if (sscanf(lines[7], "#%02x%02x%02x", &r, &g, &b) == 3) {
            snprintf(COLOR_VALUE, 32, "\033[38;2;%u;%u;%um", r, g, b);
        }
    }
    g_colors_from_pywal = 1; // Signal to parse_xresources to skip
}

typedef struct { char pretty[256]; char id[64]; char id_like[128]; } OsInfo;
static OsInfo g_os = {{0},{0},{0}};

static void parse_os_release(void) {
    char buf[4096];
    if (read_sys_file("/etc/os-release", buf, sizeof(buf)) <= 0) {
        strcpy(g_os.id, "linux"); strcpy(g_os.pretty, "Unknown Linux"); return;
    }
    char *p = buf;
    while (*p) {
        if (!g_os.pretty[0] && strncmp(p, "PRETTY_NAME=", 12) == 0) {
            p += 12; char *end = strchr(p, '\n'); size_t len = end ? (size_t)(end - p) : strlen(p);
            if (len >= sizeof(g_os.pretty)) len = sizeof(g_os.pretty) - 1;
            memcpy(g_os.pretty, p, len); g_os.pretty[len] = '\0'; strip_quotes_nl(g_os.pretty);
        } else if (!g_os.id_like[0] && strncmp(p, "ID_LIKE=", 8) == 0) {
            p += 8; char *end = strchr(p, '\n'); size_t len = end ? (size_t)(end - p) : strlen(p);
            if (len >= sizeof(g_os.id_like)) len = sizeof(g_os.id_like) - 1;
            memcpy(g_os.id_like, p, len); g_os.id_like[len] = '\0'; strip_quotes_nl(g_os.id_like);
        } else if (!g_os.id[0] && strncmp(p, "ID=", 3) == 0) {
            p += 3; char *end = strchr(p, '\n'); size_t len = end ? (size_t)(end - p) : strlen(p);
            if (len >= sizeof(g_os.id)) len = sizeof(g_os.id) - 1;
            memcpy(g_os.id, p, len); g_os.id[len] = '\0'; strip_quotes_nl(g_os.id);
        }
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }
    if (!g_os.id[0]) strcpy(g_os.id, "linux");
    if (!g_os.pretty[0]) strcpy(g_os.pretty, "Unknown Linux");
}

typedef struct { char theme[128]; char icons[128]; char cursor[128]; char font[128]; } GtkInfo;
static GtkInfo g_gtk = {{0},{0},{0},{0}};

static void parse_gtk(void) {
    const char *home = getenv("HOME");
    if (!home) return;
    char path[512];
    snprintf(path, sizeof(path), "%s/.config/gtk-3.0/settings.ini", home);
    char buf[4096];
    if (read_sys_file(path, buf, sizeof(buf)) <= 0) return;
    char *p = buf;
    while (*p) {
        if (!g_gtk.theme[0] && strncmp(p, "gtk-theme-name=", 15) == 0) {
            p += 15; char *end = strchr(p, '\n'); size_t len = end ? (size_t)(end - p) : strlen(p);
            if (len >= sizeof(g_gtk.theme)) len = sizeof(g_gtk.theme) - 1;
            memcpy(g_gtk.theme, p, len); g_gtk.theme[len] = '\0'; strip_quotes_nl(g_gtk.theme);
        } else if (!g_gtk.icons[0] && strncmp(p, "gtk-icon-theme-name=", 20) == 0) {
            p += 20; char *end = strchr(p, '\n'); size_t len = end ? (size_t)(end - p) : strlen(p);
            if (len >= sizeof(g_gtk.icons)) len = sizeof(g_gtk.icons) - 1;
            memcpy(g_gtk.icons, p, len); g_gtk.icons[len] = '\0'; strip_quotes_nl(g_gtk.icons);
        } else if (!g_gtk.cursor[0] && strncmp(p, "gtk-cursor-theme-name=", 22) == 0) {
            p += 22; char *end = strchr(p, '\n'); size_t len = end ? (size_t)(end - p) : strlen(p);
            if (len >= sizeof(g_gtk.cursor)) len = sizeof(g_gtk.cursor) - 1;
            memcpy(g_gtk.cursor, p, len); g_gtk.cursor[len] = '\0'; strip_quotes_nl(g_gtk.cursor);
        } else if (!g_gtk.font[0] && strncmp(p, "gtk-font-name=", 14) == 0) {
            p += 14; char *end = strchr(p, '\n'); size_t len = end ? (size_t)(end - p) : strlen(p);
            if (len >= sizeof(g_gtk.font)) len = sizeof(g_gtk.font) - 1;
            memcpy(g_gtk.font, p, len); g_gtk.font[len] = '\0'; strip_quotes_nl(g_gtk.font);
        }
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }
}

// ════════════════════════════════════════════════════════════
//  RUNTIME CONFIGURATION HELPERS
// ════════════════════════════════════════════════════════════

static void parse_hex_color(const char *hex, char *buf, size_t sz) {
    if (hex[0] != '#') return;
    unsigned int r, g, b;
    if (sscanf(hex + 1, "%02x%02x%02x", &r, &g, &b) == 3)
        snprintf(buf, sz, "\033[38;2;%u;%u;%um", r, g, b);
}

static void parse_xresources(void) {
    if (g_colors_from_pywal) return; // pywal takes priority
    const char *home = getenv("HOME");
    if (!home) return;
    char path[512];
    snprintf(path, sizeof(path), "%s/.Xresources", home);
    if (access(path, R_OK) != 0) {
        snprintf(path, sizeof(path), "%s/.Xdefaults", home);
        if (access(path, R_OK) != 0) return;
    }
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[512], colors[16][32] = {{0}};
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '!' || line[0] == '\n') continue;
        int idx = -1; char val[64];
        if      (sscanf(line, "*color%d: %63s",  &idx, val) == 2 && idx >= 0 && idx < 16)
            strncpy(colors[idx], val, sizeof(colors[idx])-1);
        else if (sscanf(line, "*.color%d: %63s", &idx, val) == 2 && idx >= 0 && idx < 16)
            strncpy(colors[idx], val, sizeof(colors[idx])-1);
    }
    fclose(f);
    if (colors[4][0] == '#') {
        parse_hex_color(colors[4], COLOR_USER, sizeof(COLOR_USER));
        memcpy(COLOR_HOST,     COLOR_USER, sizeof(COLOR_HOST));
        memcpy(COLOR_BAR_FILL, COLOR_USER, sizeof(COLOR_BAR_FILL));
    }
    if (colors[8][0] == '#') {
        parse_hex_color(colors[8], COLOR_AT, sizeof(COLOR_AT));
        memcpy(COLOR_SEP,       COLOR_AT, sizeof(COLOR_SEP));
        memcpy(COLOR_BAR_EMPTY, COLOR_AT, sizeof(COLOR_BAR_EMPTY));
    }
    if (colors[6][0] == '#') {
        parse_hex_color(colors[6], COLOR_LABEL, sizeof(COLOR_LABEL));
        memcpy(COLOR_ICON, COLOR_LABEL, sizeof(COLOR_ICON));
    }
    if (colors[7][0] == '#') parse_hex_color(colors[7], COLOR_VALUE, sizeof(COLOR_VALUE));
}

static void parse_config_file(void) {
    char path[512];
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0]) snprintf(path, sizeof(path), "%s/superfetch/superfetch.conf", xdg);
    else {
        const char *home = getenv("HOME");
        if (!home) return;
        snprintf(path, sizeof(path), "%s/.config/superfetch/superfetch.conf", home);
    }
    if (access(path, R_OK) != 0) return;
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;
        char *eq = strchr(line, '='); if (!eq) continue;
        *eq = '\0';
        char *key = line, *val = eq + 1;
        while (*key == ' ' || *key == '\t') key++;
        while (*val == ' ' || *val == '\t') val++;
        char *kend = key + strlen(key) - 1;
        while (kend > key && (*kend == ' ' || *kend == '\t')) *kend-- = '\0';
        if      (strcmp(key, "bar_width")   == 0) g_bar_width   = atoi(val);
        else if (strcmp(key, "show_labels") == 0) g_show_labels = (strcmp(val,"true")==0 || strcmp(val,"1")==0);
        else if (strcmp(key, "color_label") == 0) parse_hex_color(val, COLOR_LABEL, sizeof(COLOR_LABEL));
        else if (strcmp(key, "color_value") == 0) parse_hex_color(val, COLOR_VALUE, sizeof(COLOR_VALUE));
        else if (strcmp(key, "color_icon")  == 0) parse_hex_color(val, COLOR_ICON,  sizeof(COLOR_ICON));
        else if (strcmp(key, "logo")        == 0) { static char lb[64]; strncpy(lb, val, sizeof(lb)-1); g_forced_logo = lb; }
        else if (strcmp(key, "watch")       == 0) g_watch_interval = atoi(val);
    }
    fclose(f);
}

// ════════════════════════════════════════════════════════════
//  ASYNC BACKGROUND WORKERS (PTHREADS)
// ════════════════════════════════════════════════════════════

typedef struct { long mem_total; long mem_avail; long swap_total; long swap_free; } MemInfo;
static MemInfo g_mem = {0,0,0,0};

static void parse_meminfo(void) {
    char buf[4096];
    if (read_sys_file("/proc/meminfo", buf, sizeof(buf)) <= 0) return;
    char *p = buf;
    while (*p) {
        if (!g_mem.mem_total && strncmp(p, "MemTotal:", 9) == 0) {
            p += 9; while (*p == ' ' || *p == '\t') p++;
            g_mem.mem_total = strtol(p, NULL, 10);
        } else if (!g_mem.mem_avail && strncmp(p, "MemAvailable:", 13) == 0) {
            p += 13; while (*p == ' ' || *p == '\t') p++;
            g_mem.mem_avail = strtol(p, NULL, 10);
        } else if (!g_mem.swap_total && strncmp(p, "SwapTotal:", 10) == 0) {
            p += 10; while (*p == ' ' || *p == '\t') p++;
            g_mem.swap_total = strtol(p, NULL, 10);
        } else if (!g_mem.swap_free && strncmp(p, "SwapFree:", 9) == 0) {
            p += 9; while (*p == ' ' || *p == '\t') p++;
            g_mem.swap_free = strtol(p, NULL, 10);
        }
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }
}

// ── Live CPU Usage (two-sample /proc/stat) ───────────────────
typedef struct { long long idle, total; } CpuStat;
static CpuStat   g_cpustat[2]        = {{0,0},{0,0}};
static pthread_t t_cpu_usage_t;
static int       t_cpu_usage_started = 0;

static CpuStat read_cpu_stat(void) {
    CpuStat s = {0, 0};
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return s;
    long long u,n,sy,id,wa,hi,si,st;
    if (fscanf(f, "cpu %lld %lld %lld %lld %lld %lld %lld %lld", &u,&n,&sy,&id,&wa,&hi,&si,&st) == 8) {
        s.idle  = id + wa;
        s.total = u + n + sy + id + wa + hi + si + st;
    }
    fclose(f);
    return s;
}

static int pkg_pacman = 0, pkg_dpkg = 0, pkg_flatpak = 0, pkg_snap = 0, pkg_apk = 0, pkg_brew = 0;
static char async_cpu_str[256] = {0};
static char async_gpu_str[256] = {0};
static char async_disk_str[512] = {0};
static char async_net_str[256] = {0};
static char async_batt_str[256] = {0};

static pthread_t t_pkg, t_cpu, t_gpu, t_disk, t_net, t_batt;
static int t_pkg_started = 0, t_cpu_started = 0, t_gpu_started = 0, t_disk_started = 0, t_net_started = 0, t_batt_started = 0;

static void *worker_packages(void *arg) {
    (void)arg;
    char cache_path[512];
    snprintf(cache_path, sizeof(cache_path), "%s/.cache/superfetch/packages", getenv("HOME") ? getenv("HOME") : "");
    
    struct stat st;
    long long max_mtime = 0;
    const char *paths[] = {
        "/var/lib/dpkg/status",
        "/var/lib/pacman/local",
        "/var/lib/flatpak/app",
        "/snap",
        "/usr/local/Cellar",
        "/home/linuxbrew/.linuxbrew/Cellar",
        "/opt/homebrew/Cellar",
        "/lib/apk/db/installed"
    };
    for (int i=0; i<8; i++) {
        if (stat(paths[i], &st) == 0 && (long long)st.st_mtime > max_mtime) max_mtime = (long long)st.st_mtime;
    }
    
    FILE *fc = fopen(cache_path, "r");
    if (fc) {
        long long cache_mtime = 0;
        if (fscanf(fc, "%lld\n%d %d %d %d %d %d", &cache_mtime, &pkg_pacman, &pkg_dpkg, &pkg_apk, &pkg_flatpak, &pkg_snap, &pkg_brew) == 7) {
            if (cache_mtime >= max_mtime) {
                fclose(fc);
                return NULL;
            }
        }
        fclose(fc);
    }
    pkg_pacman = pkg_dpkg = pkg_flatpak = pkg_snap = pkg_apk = pkg_brew = 0;

    DIR *d; struct dirent *e;

    // Pacman
    d = opendir("/var/lib/pacman/local");
    if (d) { while ((e = readdir(d)) != NULL) if (e->d_name[0] != '.') pkg_pacman++; closedir(d); }

    // Dpkg
    int fd = open("/var/lib/dpkg/status", O_RDONLY);
    if (fd >= 0) {
        struct stat st;
        if (fstat(fd, &st) == 0 && st.st_size > 0) {
            char *map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
            if (map != MAP_FAILED) {
                const char *tgt = "Status: install ok installed"; size_t tlen = 28;
                char *p = map, *end = map + st.st_size - tlen;
                while (p <= end) {
                    char *m = memchr(p, 'S', end - p + 1);
                    if (!m) break;
                    if (memcmp(m, tgt, tlen) == 0) { pkg_dpkg++; p = m + tlen; } else { p = m + 1; }
                }
                munmap(map, st.st_size);
            }
        }
        close(fd);
    }

    // APK (Alpine)
    fd = open("/lib/apk/db/installed", O_RDONLY);
    if (fd >= 0) {
        struct stat st;
        if (fstat(fd, &st) == 0 && st.st_size > 0) {
            char *map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
            if (map != MAP_FAILED) {
                char *p = map, *end = map + st.st_size;
                while (p < end) {
                    if (p[0] == 'P' && p[1] == ':') pkg_apk++;
                    char *nl = memchr(p, '\n', end - p);
                    if (!nl) break;
                    p = nl + 1;
                }
                munmap(map, st.st_size);
            }
        }
        close(fd);
    }

    // Flatpak
    d = opendir("/var/lib/flatpak/app");
    if (d) { while ((e = readdir(d)) != NULL) if (e->d_name[0] != '.') pkg_flatpak++; closedir(d); }
    d = opendir("/home");
    if (d) {
        while ((e = readdir(d)) != NULL) {
            if (e->d_name[0] == '.') continue;
            char p[512]; snprintf(p, sizeof(p), "/home/%s/.local/share/flatpak/app", e->d_name);
            DIR *fd_dir = opendir(p);
            if (fd_dir) { struct dirent *fe; while ((fe = readdir(fd_dir)) != NULL) if (fe->d_name[0] != '.') pkg_flatpak++; closedir(fd_dir); }
        }
        closedir(d);
    }

    // Snap
    d = opendir("/snap");
    if (d) {
        const char *skip[] = {"bin","core","core18","core20","core22","snapd",NULL};
        while ((e = readdir(d)) != NULL) {
            if (e->d_name[0] == '.') continue;
            int skip_it = 0; for (int i = 0; skip[i]; i++) if (strcmp(e->d_name, skip[i]) == 0) { skip_it = 1; break; }
            if (!skip_it) pkg_snap++;
        }
        closedir(d);
    }

    // Brew
    const char *brew_paths[] = { "/home/linuxbrew/.linuxbrew/Cellar", "/opt/homebrew/Cellar", "/usr/local/Cellar", NULL };
    for (int i = 0; brew_paths[i]; i++) {
        d = opendir(brew_paths[i]);
        if (d) { while ((e = readdir(d)) != NULL) if (e->d_name[0] != '.') pkg_brew++; closedir(d); }
    }

    ensure_cache_dir();
    FILE *fw = fopen(cache_path, "w");
    if (fw) {
        fprintf(fw, "%lld\n%d %d %d %d %d %d\n", max_mtime, pkg_pacman, pkg_dpkg, pkg_apk, pkg_flatpak, pkg_snap, pkg_brew);
        fclose(fw);
    }

    return NULL;
}

static void *worker_cpu(void *arg) {
    (void)arg;
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f) { strcpy(async_cpu_str, "Unknown CPU"); return NULL; }
    char line[256], name[256] = {0}; int threads = 0;
    while (fgets(line, sizeof(line), f)) {
        if (!name[0] && strncmp(line, "model name", 10) == 0) {
            char *p = strchr(line, ':');
            if (p) { p++; while (*p == ' ') p++; char *e = p + strlen(p) - 1; if (*e == '\n') *e = '\0'; strncpy(name, p, sizeof(name)-1); }
        } else if (strncmp(line, "processor", 9) == 0) { threads++; }
    }
    fclose(f);
    str_remove_all(name, "(R)"); str_remove_all(name, "(TM)");
    char *at = strstr(name, " @ "); if (at) *at = '\0';
    collapse_spaces(name);
    char *start = name; while (*start == ' ') start++;
    char *end2 = start + strlen(start) - 1; while (end2 > start && *end2 == ' ') *end2-- = '\0';

    float max_ghz = 0; char tmp[512];
    if (read_sys_file("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", tmp, sizeof(tmp)) > 0) {
        long khz = atol(tmp);
        if (khz > 0) max_ghz = khz / 1000000.0f;
    }
    if (max_ghz > 0) snprintf(async_cpu_str, sizeof(async_cpu_str), "%s (%d) @ %.2f GHz", start, threads, max_ghz);
    else snprintf(async_cpu_str, sizeof(async_cpu_str), "%s (%d)", start, threads);
    return NULL;
}

static void *worker_gpu(void *arg) {
    (void)arg;
    DIR *dir = opendir("/sys/class/drm");
    if (!dir) { strcpy(async_gpu_str, "Unknown"); return NULL; }
    char vendor_str[16]={0}, device_str[16]={0}; int found = 0; struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "card", 4) == 0 && strchr(entry->d_name + 4, '-') == NULL) {
            char path[512]; char tmp[512];
            snprintf(path, sizeof(path), "/sys/class/drm/%s/device/vendor", entry->d_name);
            if (read_sys_file(path, tmp, sizeof(tmp)) > 0) {
                sscanf(tmp, "0x%15s", vendor_str);
                snprintf(path, sizeof(path), "/sys/class/drm/%s/device/device", entry->d_name);
                if (read_sys_file(path, tmp, sizeof(tmp)) > 0) {
                    sscanf(tmp, "0x%15s", device_str);
                    found = 1;
                    break;
                }
            }
        }
    }
    closedir(dir);
    if (!found) { strcpy(async_gpu_str, "Unknown"); return NULL; }

    char cache_path[512];
    snprintf(cache_path, sizeof(cache_path), "%s/.cache/superfetch/gpu", getenv("HOME") ? getenv("HOME") : "");
    FILE *fc = fopen(cache_path, "r");
    if (fc) {
        char cached_id[64] = {0};
        char cached_name[256] = {0};
        if (fscanf(fc, "%63[^\n]\n%255[^\n]", cached_id, cached_name) == 2) {
            char curr_id[64]; snprintf(curr_id, sizeof(curr_id), "%s:%s", vendor_str, device_str);
            if (strcmp(curr_id, cached_id) == 0) {
                snprintf(async_gpu_str, sizeof(async_gpu_str), "%s", cached_name);
                fclose(fc);
                return NULL;
            }
        }
        fclose(fc);
    }

    int fds = open("/usr/share/hwdata/pci.ids", O_RDONLY);
    if (fds < 0) fds = open("/usr/share/misc/pci.ids", O_RDONLY);
    if (fds >= 0) {
        struct stat st;
        if (fstat(fds, &st) == 0 && st.st_size > 0) {
            char *map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fds, 0);
            if (map != MAP_FAILED) {
                char *p = map, *end = map + st.st_size; int in_vendor = 0; char vendor_name[128] = {0};
                while (p < end) {
                    char *nl = memchr(p, '\n', end - p); if (!nl) nl = end;
                    size_t len = nl - p;
                    if (len > 0 && p[0] != '#' && p[0] != '\n') {
                        if (p[0] != '\t') {
                            if (len >= 5 && strncasecmp(p, vendor_str, 4) == 0 && p[4] == ' ') {
                                in_vendor = 1; char *n = p + 4; while (*n == ' ' && n < nl) n++;
                                snprintf(vendor_name, sizeof(vendor_name), "%.*s", (int)(nl - n), n);
                            } else { if (in_vendor) break; in_vendor = 0; }
                        } else if (in_vendor && len >= 6 && p[1] != '\t') {
                            if (strncasecmp(p + 1, device_str, 4) == 0 && p[5] == ' ') {
                                char *n = p + 5; while (*n == ' ' && n < nl) n++;
                                snprintf(async_gpu_str, sizeof(async_gpu_str), "%s %.*s", vendor_name, (int)(nl - n), n); break;
                            }
                        }
                    }
                    p = nl + 1;
                }
                munmap(map, st.st_size);
            }
        }
        close(fds);
    }
    if (!async_gpu_str[0]) snprintf(async_gpu_str, sizeof(async_gpu_str), "%s:%s", vendor_str, device_str);
    
    ensure_cache_dir();
    FILE *fw = fopen(cache_path, "w");
    if (fw) {
        fprintf(fw, "%s:%s\n%s\n", vendor_str, device_str, async_gpu_str);
        fclose(fw);
    }
    
    return NULL;
}

// ════════════════════════════════════════════════════════════
//  MODULE FETCHERS
// ════════════════════════════════════════════════════════════

int g_title_len = 0;

void fetch_title(char *buf, size_t sz) {
    char host[256] = "unknown"; gethostname(host, sizeof(host));
    const char *user = getenv("USER"); if (!user || !user[0]) user = "user";
    g_title_len = (int)(strlen(user) + 1 + strlen(host));
    snprintf(buf, sz, "%s%s%s@%s%s", COLOR_USER, user, COLOR_AT, COLOR_HOST, host);
}

void fetch_os(char *buf, size_t sz) { snprintf(buf, sz, "%s", g_os.pretty); }

void fetch_packages(char *buf, size_t sz) {
    int first = 1; size_t pos = 0;
    if (pkg_pacman)  { snprintf(buf+pos, sz-pos, "%d (pacman)", pkg_pacman); pos = strlen(buf); first = 0; }
    if (pkg_dpkg)    { snprintf(buf+pos, sz-pos, "%s%d (dpkg)", first?"":", ", pkg_dpkg); pos = strlen(buf); first = 0; }
    if (pkg_apk)     { snprintf(buf+pos, sz-pos, "%s%d (apk)", first?"":", ", pkg_apk); pos = strlen(buf); first = 0; }
    if (pkg_brew)    { snprintf(buf+pos, sz-pos, "%s%d (brew)", first?"":", ", pkg_brew); pos = strlen(buf); first = 0; }
    if (pkg_flatpak) { snprintf(buf+pos, sz-pos, "%s%d (flatpak)", first?"":", ", pkg_flatpak); pos = strlen(buf); first = 0; }
    if (pkg_snap)    { snprintf(buf+pos, sz-pos, "%s%d (snap)", first?"":", ", pkg_snap); pos = strlen(buf); first = 0; }
    if (first) snprintf(buf+pos, sz-pos, "None");
}

void fetch_cpu(char *buf, size_t sz) {
    snprintf(buf, sz, "%s", async_cpu_str);
}

void fetch_gpu(char *buf, size_t sz) {
    snprintf(buf, sz, "%s", async_gpu_str);
}

void fetch_host(char *buf, size_t sz) {
    char vendor[128] = {0}, product[128] = {0}; char tmp[512];
    if (read_sys_file("/sys/class/dmi/id/board_vendor", tmp, sizeof(tmp)) > 0) {
        strncpy(vendor, tmp, sizeof(vendor)-1); strip_quotes_nl(vendor);
    }
    if (read_sys_file("/sys/class/dmi/id/product_name", tmp, sizeof(tmp)) > 0) {
        strncpy(product, tmp, sizeof(product)-1); strip_quotes_nl(product);
    }
    if (!product[0]) { snprintf(buf, sz, "Unknown"); return; }
    if (vendor[0] && strstr(product, vendor) == NULL) snprintf(buf, sz, "%s %s", vendor, product);
    else snprintf(buf, sz, "%s", product);
}

void fetch_kernel(char *buf, size_t sz) {
    struct utsname u;
    if (uname(&u) == 0) snprintf(buf, sz, "%s %s", u.sysname, u.release);
    else snprintf(buf, sz, "Unknown");
}

void fetch_shell(char *buf, size_t sz) {
    const char *sh = getenv("SHELL");
    if (!sh) { snprintf(buf, sz, "Unknown"); return; }
    const char *b = strrchr(sh, '/');
    const char *name = b ? b+1 : sh;
    const char *ver = NULL;
    if      (strcmp(name, "bash") == 0) ver = getenv("BASH_VERSION");
    else if (strcmp(name, "zsh")  == 0) ver = getenv("ZSH_VERSION");
    else if (strcmp(name, "fish") == 0) ver = getenv("FISH_VERSION");
    else if (strcmp(name, "nu")   == 0) ver = getenv("NU_VERSION");
    if (ver && ver[0]) {
        char v[64]; strncpy(v, ver, sizeof(v)-1); v[sizeof(v)-1] = '\0';
        /* Trim at first '(' or '-': "5.2.26(1)-release" -> "5.2.26" */
        char *p = v; while (*p && *p != '(' && *p != '-') p++; *p = '\0';
        snprintf(buf, sz, "%s %s", name, v);
    } else {
        snprintf(buf, sz, "%s", name);
    }
}

void fetch_terminal(char *buf, size_t sz) {
    const char *t = getenv("TERM_PROGRAM"); if (!t) t = getenv("COLORTERM"); if (!t) t = getenv("TERM");
    snprintf(buf, sz, "%s", t ? t : "Unknown");
}

void fetch_wm(char *buf, size_t sz) {
    const char *wm = getenv("XDG_CURRENT_DESKTOP"); if (!wm) wm = getenv("DESKTOP_SESSION"); if (!wm) wm = getenv("WAYLAND_DISPLAY") ? "Wayland" : "Unknown";
    snprintf(buf, sz, "%s", wm ? wm : "Unknown");
}

void fetch_theme(char *buf, size_t sz) { snprintf(buf, sz, "%s", g_gtk.theme[0] ? g_gtk.theme : "Unknown"); }
void fetch_icons(char *buf, size_t sz) { snprintf(buf, sz, "%s", g_gtk.icons[0] ? g_gtk.icons : "Unknown"); }
void fetch_cursor(char *buf, size_t sz){ snprintf(buf, sz, "%s", g_gtk.cursor[0]? g_gtk.cursor: "Unknown"); }
void fetch_font(char *buf, size_t sz)  { snprintf(buf, sz, "%s", g_gtk.font[0]  ? g_gtk.font  : "Unknown"); }

void fetch_uptime(char *buf, size_t sz) {
    struct sysinfo si;
    if (sysinfo(&si) != 0) { snprintf(buf, sz, "Unknown"); return; }
    long d = si.uptime / 86400, h = (si.uptime / 3600) % 24, m = (si.uptime / 60) % 60;
    size_t pos = 0;
    if (d) { snprintf(buf+pos, sz-pos, "%ld day%s, ", d, d != 1 ? "s" : ""); pos = strlen(buf); }
    if (h) { snprintf(buf+pos, sz-pos, "%ld hr%s, ",  h, h != 1 ? "s" : ""); pos = strlen(buf); }
    snprintf(buf+pos, sz-pos, "%ld min%s", m, m != 1 ? "s" : "");
}

void fetch_locale(char *buf, size_t sz) {
    const char *l = getenv("LANG"); if (!l) l = getenv("LC_ALL");
    snprintf(buf, sz, "%s", l ? l : "Unknown");
}

void fetch_cpu_temp(char *buf, size_t sz) {
    int temp_mc = -1;
    char cache_path[512];
    snprintf(cache_path, sizeof(cache_path), "%s/.cache/superfetch/hwmon_cpu", getenv("HOME") ? getenv("HOME") : "");
    char tmp[512];
    
    if (read_sys_file(cache_path, tmp, sizeof(tmp)) > 0) {
        char cached_path[512];
        if (sscanf(tmp, "%511s", cached_path) == 1) {
            if (read_sys_file(cached_path, tmp, sizeof(tmp)) > 0) {
                if (sscanf(tmp, "%d", &temp_mc) == 1) goto done_cpu;
            }
        }
    }
    
    DIR *dir = opendir("/sys/class/hwmon");
    if (!dir) { snprintf(buf, sz, "N/A"); return; }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char path[512]; snprintf(path, sizeof(path), "/sys/class/hwmon/%s/name", entry->d_name);
        if (read_sys_file(path, tmp, sizeof(tmp)) <= 0) continue;
        char name[64]; if (sscanf(tmp, "%63s", name) != 1) continue;
        if (strcmp(name,"coretemp")==0 || strcmp(name,"k10temp")==0 || strcmp(name,"zenpower")==0) {
            snprintf(path, sizeof(path), "/sys/class/hwmon/%s/temp1_input", entry->d_name);
            if (read_sys_file(path, tmp, sizeof(tmp)) > 0) {
                if (sscanf(tmp, "%d", &temp_mc) == 1) {
                    ensure_cache_dir();
                    int fw = open(cache_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (fw >= 0) {
                        if (write(fw, path, strlen(path)) < 0) { /* ignore */ }
                        close(fw);
                    }
                }
                break;
            }
        }
    }
    closedir(dir);
    
done_cpu:
    if (temp_mc < 0) { snprintf(buf, sz, "N/A"); return; }
    int temp = temp_mc / 1000;
    const char *col = (temp < 60) ? COLOR_TEMP_OK : (temp < 80) ? COLOR_TEMP_WARN : COLOR_TEMP_HOT;
    snprintf(buf, sz, "%s%d°C%s", col, temp, COLOR_RESET);
}

void fetch_cpu_load(char *buf, size_t sz) {
    FILE *f = fopen("/proc/loadavg", "r");
    if (!f) { snprintf(buf, sz, "Unknown"); return; }
    float l1, l5, l15;
    if (fscanf(f, "%f %f %f", &l1, &l5, &l15) == 3) snprintf(buf, sz, "%.2f, %.2f, %.2f", l1, l5, l15);
    else snprintf(buf, sz, "Unknown");
    fclose(f);
}

void fetch_resolution(char *buf, size_t sz) {
    DIR *dir = opendir("/sys/class/drm");
    if (!dir) { snprintf(buf, sz, "Unknown"); return; }
    char res[256] = {0}; struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name,"card",4) != 0) continue;
        if (!strchr(entry->d_name+4,'-')) continue;
        char path[512]; snprintf(path, sizeof(path), "/sys/class/drm/%s/status", entry->d_name);
        FILE *f = fopen(path,"r"); if (!f) continue;
        char st[32]; int ok = fscanf(f,"%31s",st)==1; fclose(f);
        if (!ok || strcmp(st,"connected")!=0) continue;
        snprintf(path, sizeof(path), "/sys/class/drm/%s/modes", entry->d_name);
        f = fopen(path,"r"); if (!f) continue;
        char mode[64];
        if (fgets(mode, sizeof(mode), f)) {
            char *e = mode+strlen(mode)-1; if (*e=='\n') *e='\0';
            size_t rlen = strlen(res);
            if (res[0] && rlen < sizeof(res) - 1) { strncat(res, ", ", sizeof(res) - rlen - 1); rlen = strlen(res); }
            if (rlen < sizeof(res) - 1) strncat(res, mode, sizeof(res) - rlen - 1);
        }
        fclose(f);
    }
    closedir(dir);
    snprintf(buf, sz, "%s", res[0] ? res : "Unknown");
}

void fetch_memory(char *buf, size_t sz) {
    if (!g_mem.mem_total) { snprintf(buf, sz, "Unknown"); return; }
    long total = g_mem.mem_total, avail = g_mem.mem_avail;
    long used_mib = (total - avail) / 1024, total_mib = total / 1024;
    int pct = (int)(100.0 * (total - avail) / total), filled = (pct * g_bar_width) / 100;
    
    snprintf(buf, sz, "%ld MiB / %ld MiB  [", used_mib, total_mib);
    size_t p = strlen(buf);
    for (int i = 0; i < g_bar_width && p < sz; i++) {
        if (i < filled) snprintf(buf+p, sz-p, "%s█%s", COLOR_BAR_FILL, COLOR_RESET);
        else snprintf(buf+p, sz-p, "%s░%s", COLOR_BAR_EMPTY, COLOR_RESET);
        p = strlen(buf);
    }
    snprintf(buf+p, sz-p, "]  %d%%", pct);
}

void fetch_swap(char *buf, size_t sz) {
    if (!g_mem.swap_total) { buf[0] = '\0'; return; }
    long total = g_mem.swap_total, free = g_mem.swap_free;
    long used_mib = (total - free) / 1024, total_mib = total / 1024;
    int pct = (int)(100.0 * (total - free) / total), filled = (pct * g_bar_width) / 100;
    
    snprintf(buf, sz, "%ld MiB / %ld MiB  [", used_mib, total_mib);
    size_t p = strlen(buf);
    for (int i = 0; i < g_bar_width && p < sz; i++) {
        if (i < filled) snprintf(buf+p, sz-p, "%s█%s", COLOR_BAR_FILL, COLOR_RESET);
        else snprintf(buf+p, sz-p, "%s░%s", COLOR_BAR_EMPTY, COLOR_RESET);
        p = strlen(buf);
    }
    snprintf(buf+p, sz-p, "]  %d%%", pct);
}

static void *worker_disk(void *arg) {
    (void)arg; char *buf = async_disk_str; size_t sz = sizeof(async_disk_str);
    FILE *f = fopen("/proc/mounts", "r");
    if (!f) { snprintf(buf, sz, "Unknown"); return NULL; }
    char line[512]; size_t pos = 0; int count = 0;
    while (fgets(line, sizeof(line), f)) {
        char dev[256], mnt[256], fs[256];
        if (sscanf(line, "%255s %255s %255s", dev, mnt, fs) == 3) {
            if (strncmp(dev, "/dev/", 5) == 0 && strstr(dev, "loop") == NULL) {
                struct statvfs s;
                if (statvfs(mnt, &s) == 0 && s.f_blocks > 0) {
                    double total_gb = (double)s.f_blocks * s.f_frsize / 1073741824.0;
                    double free_gb = (double)s.f_bfree * s.f_frsize / 1073741824.0;
                    double used_gb = total_gb - free_gb;
                    int pct = (int)(100.0 * used_gb / total_gb);
                    
                    if (count > 0 && pos < sz) { snprintf(buf+pos, sz-pos, "   "); pos = strlen(buf); }
                    snprintf(buf+pos, sz-pos, "%s%s%s %.1f/%.1f GiB (%d%%)", 
                                    COLOR_ICON, mnt, COLOR_RESET, used_gb, total_gb, pct);
                    pos = strlen(buf);
                    count++;
                }
            }
        }
    }
    fclose(f);
    if (count == 0) snprintf(buf, sz, "Unknown");
    return NULL;
}

void fetch_disk(char *buf, size_t sz) {
#if ENABLE_THREADS
    if (t_disk_started) { pthread_join(t_disk, NULL); t_disk_started = 0; }
#endif
    snprintf(buf, sz, "%s", async_disk_str);
}

static void *worker_network(void *arg) {
    (void)arg; char *buf = async_net_str; size_t sz = sizeof(async_net_str);
    struct ifaddrs *ifaddr, *ifa;
    char ip[INET_ADDRSTRLEN]={0}, iface[IFNAMSIZ]={0};
    if (getifaddrs(&ifaddr)==0) {
        for (ifa=ifaddr; ifa; ifa=ifa->ifa_next) {
            if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET || strcmp(ifa->ifa_name,"lo")==0) continue;
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip));
            strncpy(iface, ifa->ifa_name, sizeof(iface)-1); break;
        }
        freeifaddrs(ifaddr);
    }
    int signal = -1;
    FILE *f = fopen("/proc/net/wireless","r");
    if (f) {
        char line[256]; 
        if (fgets(line,sizeof(line),f)) { /* ignore */ }
        if (fgets(line,sizeof(line),f)) { /* ignore */ }
        while (fgets(line,sizeof(line),f)) {
            char wif[IFNAMSIZ]; int st; float lnk, lvl, nse;
            if (sscanf(line," %15[^:]: %d %f %f %f",wif,&st,&lnk,&lvl,&nse)>=4) { signal=(int)lnk; break; }
        }
        fclose(f);
    }
    if (!ip[0]) { snprintf(buf, sz, "Disconnected"); return NULL; }
    if (signal >= 0) {
        const char *bars = (signal>=60) ? "▂▄▆█" : (signal>=40) ? "▂▄▆_" : (signal>=20) ? "▂▄__" : "▂___";
        snprintf(buf, sz, "%s%s%s  %s  %s%s%s", COLOR_ICON, iface, COLOR_RESET, ip, COLOR_BAR_FILL, bars, COLOR_RESET);
    } else {
        snprintf(buf, sz, "%s%s%s  %s", COLOR_ICON, iface, COLOR_RESET, ip);
    }
    return NULL;
}

void fetch_network(char *buf, size_t sz) {
#if ENABLE_THREADS
    if (t_net_started) { pthread_join(t_net, NULL); t_net_started = 0; }
#endif
    snprintf(buf, sz, "%s", async_net_str);
}

static void *worker_battery(void *arg) {
    (void)arg; char *buf = async_batt_str; size_t sz = sizeof(async_batt_str);
    const char *paths[] = { "/sys/class/power_supply/BAT0", "/sys/class/power_supply/BAT1", "/sys/class/power_supply/battery", NULL };
    for (int i = 0; paths[i]; i++) {
        char p[256]; snprintf(p, sizeof(p), "%s/capacity", paths[i]);
        char tmp[512];
        if (read_sys_file(p, tmp, sizeof(tmp)) <= 0) continue;
        int cap = atoi(tmp);
        snprintf(p, sizeof(p), "%s/status", paths[i]);
        char status[32]="Unknown";
        if (read_sys_file(p, tmp, sizeof(tmp)) > 0) {
            sscanf(tmp, "%31s", status);
        }
        const char *bicon = (strcmp(status,"Charging")==0) ? "⚡" : (cap>=90) ? "󰁹" : (cap>=70) ? "󰂀" : (cap>=50) ? "󰁾" : (cap>=30) ? "󰁼" : "󰁺";
        snprintf(buf, sz, "%s %d%%  [%s]", bicon, cap, status);
        return NULL;
    }
    snprintf(buf, sz, "N/A");
    return NULL;
}

void fetch_battery(char *buf, size_t sz) {
#if ENABLE_THREADS
    if (t_batt_started) { pthread_join(t_batt, NULL); t_batt_started = 0; }
#endif
    snprintf(buf, sz, "%s", async_batt_str);
}

void fetch_datetime(char *buf, size_t sz) {
    time_t t = time(NULL); struct tm *tm = localtime(&t);
    strftime(buf, sz, "%a %Y-%m-%d  %H:%M:%S", tm);
}

void fetch_gpu_temp(char *buf, size_t sz) {
    const char *gpu_hw[] = {"nvidia", "amdgpu", "radeon", "nouveau", NULL};
    DIR *dir = opendir("/sys/class/hwmon");
    if (!dir) { snprintf(buf, sz, "N/A"); return; }
    int temp_mc = -1; struct dirent *e;
    while ((e = readdir(dir)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char path[512]; snprintf(path, sizeof(path), "/sys/class/hwmon/%s/name", e->d_name);
        char tmp[512];
        if (read_sys_file(path, tmp, sizeof(tmp)) <= 0) continue;
        char nm[64]; if (sscanf(tmp, "%63s", nm) != 1) continue;
        int is_gpu = 0;
        for (int i = 0; gpu_hw[i]; i++) if (strcmp(nm, gpu_hw[i]) == 0) { is_gpu = 1; break; }
        if (!is_gpu) continue;
        snprintf(path, sizeof(path), "/sys/class/hwmon/%s/temp1_input", e->d_name);
        if (read_sys_file(path, tmp, sizeof(tmp)) > 0) {
            if (sscanf(tmp, "%d", &temp_mc) != 1) temp_mc = -1;
            break;
        }
    }
    closedir(dir);
    if (temp_mc < 0) { snprintf(buf, sz, "N/A"); return; }
    int temp = temp_mc / 1000;
    const char *col = (temp < 60) ? COLOR_TEMP_OK : (temp < 80) ? COLOR_TEMP_WARN : COLOR_TEMP_HOT;
    snprintf(buf, sz, "%s%d\xc2\xb0""C%s", col, temp, COLOR_RESET);
}

void fetch_gpu_vram(char *buf, size_t sz) {
    DIR *dir = opendir("/sys/class/drm");
    if (!dir) { snprintf(buf, sz, "N/A"); return; }
    long long total = -1; struct dirent *e;
    while ((e = readdir(dir)) != NULL) {
        if (strncmp(e->d_name, "card", 4) != 0 || strchr(e->d_name + 4, '-') != NULL) continue;
        char path[512]; snprintf(path, sizeof(path), "/sys/class/drm/%s/device/mem_info_vram_total", e->d_name);
        FILE *f = fopen(path, "r"); if (!f) continue;
        if (fscanf(f, "%lld", &total) != 1) total = -1;
        fclose(f); break;
    }
    closedir(dir);
    if (total <= 0) { snprintf(buf, sz, "N/A"); return; }
    snprintf(buf, sz, "%lld MiB", total / (1024LL * 1024LL));
}

void fetch_processes(char *buf, size_t sz) {
    DIR *dir = opendir("/proc");
    if (!dir) { snprintf(buf, sz, "Unknown"); return; }
    int count = 0; struct dirent *e;
    while ((e = readdir(dir)) != NULL)
        if (e->d_name[0] >= '1' && e->d_name[0] <= '9') count++;
    closedir(dir);
    snprintf(buf, sz, "%d", count);
}

void fetch_cpu_usage(char *buf, size_t sz) {
    g_cpustat[1] = read_cpu_stat(); // Read second sample instantly
    long long idle_d  = g_cpustat[1].idle  - g_cpustat[0].idle;
    long long total_d = g_cpustat[1].total - g_cpustat[0].total;
    if (total_d <= 0) { snprintf(buf, sz, "N/A (Exec < 1 jiffy)"); return; }
    int pct = (int)(100LL * (total_d - idle_d) / total_d);
    pct = pct < 0 ? 0 : (pct > 100 ? 100 : pct);
    int filled = (pct * g_bar_width) / 100;
    snprintf(buf, sz, "%d%%  [", pct);
    size_t p = strlen(buf);
    for (int i = 0; i < g_bar_width && p < sz; i++) {
        if (i < filled) snprintf(buf+p, sz-p, "%s\xe2\x96\x88%s", COLOR_BAR_FILL, COLOR_RESET);
        else             snprintf(buf+p, sz-p, "%s\xe2\x96\x91%s", COLOR_BAR_EMPTY, COLOR_RESET);
        p = strlen(buf);
    }
    snprintf(buf+p, sz-p, "]");
}

// ════════════════════════════════════════════════════════════
//  RENDERING UTILITIES
// ════════════════════════════════════════════════════════════

/* Count visible (non-ANSI) characters in a string. */
static int str_visible_len(const char *s) {
    int len = 0;
    while (*s) {
        if (*s == '\033') {
            s++;
            if (*s == '[') { s++; while (*s && ((*s >= '0' && *s <= '9') || *s == ';')) s++; if (*s) s++; }
        } else if ((unsigned char)*s >= 0xF0) { len++; s += 4; }
        else if ((unsigned char)*s >= 0xE0)   { len++; s += 3; }
        else if ((unsigned char)*s >= 0xC0)   { len++; s += 2; }
        else { len++; s++; }
    }
    return len;
}

/* Truncate buf so that its visible length is <= max_vis. Appends UTF-8 "…". */
static void truncate_to(char *buf, int max_vis) {
    if (max_vis <= 3 || str_visible_len(buf) <= max_vis) return;
    int visible = 0;
    char *p = buf, *cut = buf;
    while (*p) {
        if (*p == '\033') {
            p++;
            if (*p == '[') { p++; while (*p && ((*p >= '0' && *p <= '9') || *p == ';')) p++; if (*p) p++; }
            continue;
        }
        int bytes = 1;
        if ((unsigned char)*p >= 0xF0) bytes = 4;
        else if ((unsigned char)*p >= 0xE0) bytes = 3;
        else if ((unsigned char)*p >= 0xC0) bytes = 2;
        visible++;
        p += bytes;
        if (visible == max_vis - 1) cut = p;
        if (visible >= max_vis) {
            /* Write UTF-8 "…" (U+2026) */
            cut[0] = (char)0xE2; cut[1] = (char)0x80; cut[2] = (char)0xA6; cut[3] = '\0';
            return;
        }
    }
}

/* Module name → type table for --modules / --list-modules */
static const struct { const char *name; ModuleType type; } g_module_names[] = {
    {"os", MODULE_OS}, {"host", MODULE_HOST}, {"kernel", MODULE_KERNEL},
    {"shell", MODULE_SHELL}, {"terminal", MODULE_TERMINAL}, {"wm", MODULE_WM},
    {"theme", MODULE_THEME}, {"icons", MODULE_ICONS}, {"cursor", MODULE_CURSOR},
    {"font", MODULE_FONT}, {"uptime", MODULE_UPTIME}, {"packages", MODULE_PACKAGES},
    {"locale", MODULE_LOCALE}, {"cpu", MODULE_CPU}, {"cpu_temp", MODULE_CPU_TEMP},
    {"cpu_load", MODULE_CPU_LOAD}, {"cpu_usage", MODULE_CPU_USAGE},
    {"gpu", MODULE_GPU}, {"gpu_temp", MODULE_GPU_TEMP}, {"gpu_vram", MODULE_GPU_VRAM},
    {"resolution", MODULE_RESOLUTION}, {"memory", MODULE_MEMORY}, {"swap", MODULE_SWAP},
    {"disk", MODULE_DISK}, {"network", MODULE_NETWORK}, {"battery", MODULE_BATTERY},
    {"datetime", MODULE_DATETIME}, {"processes", MODULE_PROCESSES},
    {NULL, MODULE_TITLE}
};

/* Word-boundary aware filter: "cpu" must not match "cpu_temp". */
static int is_module_enabled(ModuleType type) {
    if (!g_module_filter[0]) return 1;
    for (int i = 0; g_module_names[i].name; i++) {
        if (g_module_names[i].type != type) continue;
        const char *name = g_module_names[i].name;
        size_t nlen = strlen(name);
        const char *p = g_module_filter;
        while ((p = strstr(p, name)) != NULL) {
            int ok_before = (p == g_module_filter || *(p-1) == ',');
            int ok_after  = (p[nlen] == '\0' || p[nlen] == ',');
            if (ok_before && ok_after) return 1;
            p++;
        }
        return 0;
    }
    return 1;
}

// ════════════════════════════════════════════════════════════
//  MAIN ENGINE
// ════════════════════════════════════════════════════════════

int main(int argc, char *argv[]) {
    mallopt(M_ARENA_MAX, 1);
    long start_time = get_time_us();

    /* Launch async workers immediately so they overlap with sequential parsing. */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 16384);
    // Read initial CPU stat as early as possible for delta measurement
    g_cpustat[0] = read_cpu_stat();

#if ENABLE_THREADS
    if (pthread_create(&t_pkg, &attr, worker_packages, NULL) == 0) t_pkg_started = 1;
    if (pthread_create(&t_cpu, &attr, worker_cpu, NULL) == 0) t_cpu_started = 1;
    if (pthread_create(&t_gpu, &attr, worker_gpu, NULL) == 0) t_gpu_started = 1;
    if (pthread_create(&t_disk, &attr, worker_disk, NULL) == 0) t_disk_started = 1;
    if (pthread_create(&t_net, &attr, worker_network, NULL) == 0) t_net_started = 1;
    if (pthread_create(&t_batt, &attr, worker_battery, NULL) == 0) t_batt_started = 1;
#else
    long long t0, t1;
    t0 = get_time_us(); worker_packages(NULL); t1 = get_time_us();
    long long time_pkg = t1 - t0;
    t0 = get_time_us(); worker_cpu(NULL); t1 = get_time_us();
    long long time_cpu = t1 - t0;
    t0 = get_time_us(); worker_gpu(NULL); t1 = get_time_us();
    long long time_gpu = t1 - t0;
    worker_disk(NULL); worker_network(NULL); worker_battery(NULL);
#endif
    pthread_attr_destroy(&attr);

    /* Config file first — CLI args override it. */
    parse_config_file();
    parse_xresources(); // no-op if pywal already applied colors

    int g_json_output = 0;
    int g_timing = 0;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "--json") == 0) { g_json_output = 1; }
        else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-V") == 0) {
            printf("superfetch v7\n"); return 0;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            puts(
                "superfetch v7 \xe2\x80\x94 blazing fast system info\n"
                "Usage: superfetch [OPTIONS]\n\n"
                "Output:\n"
                "  --json              Export as JSON\n"
                "  --no-labels         Hide the label column\n"
                "  --logo DISTRO       Force logo (arch, ubuntu, none, \xe2\x80\xa6)\n"
                "  --modules LIST      Comma-separated subset (e.g. cpu,memory,gpu)\n"
                "  --list-modules      Print all module names and exit\n"
                "  --timing            Print microsecond timing breakdown\n\n"
                "Appearance:\n"
                "  --bar-width N       Progress bar width (default: 10)\n"
                "  --color-label HEX   Label color  (#RRGGBB)\n"
                "  --color-value HEX   Value color  (#RRGGBB)\n"
                "  --color-icon  HEX   Icon color   (#RRGGBB)\n\n"
                "Live Mode:\n"
                "  --watch [N]         Smooth refresh every N seconds (default: 2)\n\n"
                "Config file: ~/.config/superfetch/superfetch.conf\n"
                "  bar_width=15  show_labels=true  color_label=#89b4fa\n"
                "  color_value=#cdd6f4  logo=arch  watch=0\n"
            );
            return 0;
        }
        else if (strcmp(argv[i], "--no-labels") == 0)  { g_show_labels = 0; }
        else if (strcmp(argv[i], "--bar-width") == 0 && i+1 < argc) {
            g_bar_width = atoi(argv[++i]);
            if (g_bar_width < 1) g_bar_width = 1;
            if (g_bar_width > 50) g_bar_width = 50;
        }
        else if (strcmp(argv[i], "--color-label") == 0 && i+1 < argc) parse_hex_color(argv[++i], COLOR_LABEL, sizeof(COLOR_LABEL));
        else if (strcmp(argv[i], "--color-value") == 0 && i+1 < argc) parse_hex_color(argv[++i], COLOR_VALUE, sizeof(COLOR_VALUE));
        else if (strcmp(argv[i], "--color-icon")  == 0 && i+1 < argc) parse_hex_color(argv[++i], COLOR_ICON,  sizeof(COLOR_ICON));
        else if (strcmp(argv[i], "--logo") == 0 && i+1 < argc)    { g_forced_logo = argv[++i]; }
        else if (strcmp(argv[i], "--modules") == 0 && i+1 < argc) {
            strncpy(g_module_filter, argv[++i], sizeof(g_module_filter)-1);
        } else if (strcmp(argv[i], "--timing") == 0) {
            g_timing = 1;
        } else if (strcmp(argv[i], "--list-modules") == 0) {
            puts("Available modules (use with --modules, comma-separated):\n"
                 "  os, host, kernel, shell, terminal, wm\n"
                 "  theme, icons, cursor, font, locale\n"
                 "  uptime, packages, datetime, processes\n"
                 "  cpu, cpu_temp, cpu_load, cpu_usage\n"
                 "  gpu, gpu_temp, gpu_vram, resolution\n"
                 "  memory, swap, disk, network, battery");
            return 0;
        }
        else if (strcmp(argv[i], "--watch") == 0) {
            g_watch_interval = 2; // default 2s
            if (i+1 < argc && argv[i+1][0] >= '0' && argv[i+1][0] <= '9')
                g_watch_interval = atoi(argv[++i]);
            if (g_watch_interval < 1) g_watch_interval = 1;
        }
    }

    parse_pywal();
    parse_xresources(); // no-op if pywal already applied colors
    parse_meminfo();
    parse_os_release();
    parse_gtk();

    int num_modules = (int)(sizeof(modules) / sizeof(modules[0]));
    long long time_modules[64] = {0};

    /* JSON mode */
    if (g_json_output) {
        printf("{\n");
        int first_json = 1;
        for (int i = 0; i < num_modules; i++) {
            const ConfigModule *m = &modules[i];
            if (!m->label || m->type == MODULE_SEPARATOR || m->type == MODULE_CUSTOM_FUNC ||
                m->type == MODULE_TITLE || m->type == MODULE_COLORS || m->type == MODULE_BLANK)
                continue;
            char buf[512] = {0};
            long long t_m0 = get_time_us();
            switch (m->type) {
                case MODULE_OS:         fetch_os(buf, sizeof(buf)); break;
                case MODULE_HOST:       fetch_host(buf, sizeof(buf)); break;
                case MODULE_KERNEL:     fetch_kernel(buf, sizeof(buf)); break;
                case MODULE_SHELL:      fetch_shell(buf, sizeof(buf)); break;
                case MODULE_TERMINAL:   fetch_terminal(buf, sizeof(buf)); break;
                case MODULE_WM:         fetch_wm(buf, sizeof(buf)); break;
                case MODULE_THEME:      fetch_theme(buf, sizeof(buf)); break;
                case MODULE_ICONS:      fetch_icons(buf, sizeof(buf)); break;
                case MODULE_CURSOR:     fetch_cursor(buf, sizeof(buf)); break;
                case MODULE_FONT:       fetch_font(buf, sizeof(buf)); break;
                case MODULE_UPTIME:     fetch_uptime(buf, sizeof(buf)); break;
                case MODULE_PACKAGES:   
#if ENABLE_THREADS
                    if (t_pkg_started) { pthread_join(t_pkg, NULL); t_pkg_started = 0; }
#endif
                    fetch_packages(buf, sizeof(buf)); break;
                case MODULE_LOCALE:     fetch_locale(buf, sizeof(buf)); break;
                case MODULE_CPU:
#if ENABLE_THREADS
                    if (t_cpu_started) { pthread_join(t_cpu, NULL); t_cpu_started = 0; }
#endif
                    snprintf(buf, sizeof(buf), "%s", async_cpu_str); break;
                case MODULE_CPU_TEMP:   fetch_cpu_temp(buf, sizeof(buf)); break;
                case MODULE_CPU_LOAD:   fetch_cpu_load(buf, sizeof(buf)); break;
                case MODULE_CPU_USAGE:  fetch_cpu_usage(buf, sizeof(buf)); break;
                case MODULE_GPU:
#if ENABLE_THREADS
                    if (t_gpu_started) { pthread_join(t_gpu, NULL); t_gpu_started = 0; }
#endif
                    snprintf(buf, sizeof(buf), "%s", async_gpu_str); break;
                case MODULE_GPU_TEMP:   fetch_gpu_temp(buf, sizeof(buf)); break;
                case MODULE_GPU_VRAM:   fetch_gpu_vram(buf, sizeof(buf)); break;
                case MODULE_RESOLUTION: fetch_resolution(buf, sizeof(buf)); break;
                case MODULE_MEMORY:     fetch_memory(buf, sizeof(buf)); break;
                case MODULE_SWAP:       fetch_swap(buf, sizeof(buf)); break;
                case MODULE_DISK:       fetch_disk(buf, sizeof(buf)); break;
                case MODULE_NETWORK:    fetch_network(buf, sizeof(buf)); break;
                case MODULE_BATTERY:    fetch_battery(buf, sizeof(buf)); break;
                case MODULE_DATETIME:   fetch_datetime(buf, sizeof(buf)); break;
                case MODULE_PROCESSES:  fetch_processes(buf, sizeof(buf)); break;
                case MODULE_CUSTOM:     snprintf(buf, sizeof(buf), "%s", m->label); break;
                default: break;
            }
            time_modules[i] = get_time_us() - t_m0;
            if (buf[0]) {
                strip_ansi(buf);
                char escaped[1024]; json_escape(buf, escaped, sizeof(escaped));
                if (!first_json) printf(",\n");
                printf("  \"%s\": \"%s\"", m->label, escaped);
                first_json = 0;
            }
        }
        printf("\n}\n");
        if (t_pkg_started)       pthread_join(t_pkg, NULL);
        if (t_cpu_started)       pthread_join(t_cpu, NULL);
        if (t_gpu_started)       pthread_join(t_gpu, NULL);
        if (t_disk_started)      pthread_join(t_disk, NULL);
        if (t_net_started)       pthread_join(t_net, NULL);
        if (t_batt_started)      pthread_join(t_batt, NULL);
        if (t_cpu_usage_started) pthread_join(t_cpu_usage_t, NULL);
        return 0;
    }

    /* Standard UI mode */
    int label_width = 0;
    if (g_show_labels) {
        for (int i = 0; i < num_modules; i++) {
            const ConfigModule *m = &modules[i];
            if (!m->label || m->type == MODULE_SEPARATOR || m->type == MODULE_CUSTOM ||
                m->type == MODULE_BLANK || m->type == MODULE_TITLE) continue;
            int l = (int)strlen(m->label);
            if (l > label_width) label_width = l;
        }
        label_width += 1;
    }

    /* Logo selection */
    const char **logo_lines = NULL; int num_logo_lines = 0;
    if (!g_forced_logo || strcmp(g_forced_logo, "none") != 0) {
        const char *lid   = g_forced_logo ? g_forced_logo : g_os.id;
        const char *llike = g_forced_logo ? g_forced_logo : g_os.id_like;
        get_os_logo_by_id(lid, llike, &logo_lines, &num_logo_lines);
    }

    int logo_col = 0;
    if (logo_lines) {
        for (int i = 0; i < num_logo_lines; i++) {
            int w = ansi_visible_len(logo_lines[i]);
            if (w > logo_col) logo_col = w;
        }
    }

    /* Detect terminal width, compute max value column width */
    {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
            g_term_width = (int)ws.ws_col;
        int icon_w = 2; /* nerd font icons occupy 2 terminal columns */
        int lbl_w  = g_show_labels ? (label_width + 2) : 0; /* "Label: " */
        g_val_max  = g_term_width - logo_col - LOGO_PADDING - icon_w - lbl_w;
        if (g_val_max < 15) g_val_max = 15;
    }

    /* Precompute which modules pass the --modules filter */
    int enabled[128] = {0};
    int num_enabled  = 0;
    for (int i = 0; i < num_modules && i < 128; i++) {
        const ConfigModule *m = &modules[i];
        if (m->type == MODULE_TITLE     || m->type == MODULE_SEPARATOR ||
            m->type == MODULE_BLANK     || m->type == MODULE_COLORS    ||
            m->type == MODULE_CUSTOM    || m->type == MODULE_CUSTOM_FUNC) {
            enabled[i] = 1;
        } else {
            enabled[i] = is_module_enabled(m->type);
        }
        if (enabled[i]) num_enabled++;
    }

    int image_loaded = 0;
    int watch_pass   = 0;

    do {
        if (watch_pass > 0) {
            /* Smooth redraw: move cursor up and clear below */
            printf("\033[%dA\033[J", num_enabled);
            parse_meminfo(); /* refresh dynamic RAM / swap data */
        } else {
            /* First pass: print blank lines then move cursor up to make room */
            for (int i = 0; i < num_enabled; i++) putchar('\n');
            printf("\033[%dA", num_enabled);
        }

#ifdef ENABLE_IMAGES
        if (watch_pass == 0 && IMAGE_PATH) {
            int w, h, ch; unsigned char *img = stbi_load(IMAGE_PATH, &w, &h, &ch, 4);
            if (img) {
                image_loaded = 1; size_t isz = (size_t)w * h * 4, b64_len;
                char *b64 = base64_encode(img, isz, &b64_len);
                stbi_image_free(img);
                if (b64) {
                    size_t sent = 0, chunk = 4096; int first = 1;
                    while (sent < b64_len) {
                        size_t ts = b64_len - sent; if (ts > chunk) ts = chunk;
                        int more = (sent+ts < b64_len) ? 1 : 0;
                        if (first) { printf("\033_Ga=T,f=32,s=%d,v=%d,r=%d,c=%d,m=%d;", w, h, num_enabled, IMAGE_WIDTH_CELLS, more); first = 0; }
                        else { printf("\033_Gm=%d;", more); }
                        fwrite(b64+sent, 1, ts, stdout); printf("\033\\"); sent += ts;
                    }
                    free(b64);
                }
            }
        }
#endif

        int logo_row = 0;
        for (int i = 0; i < num_modules && i < 128; i++) {
            const ConfigModule *m = &modules[i];
            if (!enabled[i]) continue;

            /* Logo column */
            if (!image_loaded) {
                if (logo_row < num_logo_lines && logo_lines) {
                    printf("%s", logo_lines[logo_row]);
                    int vis = ansi_visible_len(logo_lines[logo_row]);
                    int pad = logo_col - vis + LOGO_PADDING;
                    for (int p = 0; p < pad; p++) putchar(' ');
                    printf("%s", COLOR_RESET);
                } else {
                    for (int p = 0; p < logo_col + LOGO_PADDING; p++) putchar(' ');
                }
                logo_row++;
            } else {
                printf("\033[%dC", IMAGE_WIDTH_CELLS + LOGO_PADDING);
            }

            /* Module content */
            char buf[512] = {0};

            switch (m->type) {
            case MODULE_TITLE:
                fetch_title(buf, sizeof(buf));
                printf("%s\n", buf);
                break;
            case MODULE_SEPARATOR: {
                int len = m->label ? (int)strlen(m->label) : (g_title_len > 0 ? g_title_len : 24);
                if (m->label) { printf("%s%s%s\n", COLOR_SEP, m->label, COLOR_RESET); }
                else { printf("%s", COLOR_SEP); for (int k = 0; k < len; k++) printf("\xe2\x94\x80"); printf("%s\n", COLOR_RESET); }
                break;
            }
            case MODULE_COLORS:
                for (int k = 0; k < 8; k++) printf("\033[4%dm   \033[0m", k);
                printf("\n");
                break;
            case MODULE_BLANK:
                putchar('\n');
                break;
            case MODULE_CUSTOM_FUNC:
                if (m->icon) printf("%s%s%s", m->icon_color ? m->icon_color : COLOR_ICON, m->icon, COLOR_RESET);
                if (g_show_labels && m->label) { char key[128]; snprintf(key, sizeof(key), "%s:", m->label); printf("%s%-*s%s ", COLOR_LABEL, label_width, key, COLOR_RESET); }
                long long t_f = get_time_us();
                if (m->func) m->func(); else printf("\n");
                time_modules[i] = get_time_us() - t_f;
                break;
            default: {
                long long t_m0 = get_time_us();
                switch (m->type) {
                    case MODULE_OS:         fetch_os(buf, sizeof(buf)); break;
                    case MODULE_HOST:       fetch_host(buf, sizeof(buf)); break;
                    case MODULE_KERNEL:     fetch_kernel(buf, sizeof(buf)); break;
                    case MODULE_SHELL:      fetch_shell(buf, sizeof(buf)); break;
                    case MODULE_TERMINAL:   fetch_terminal(buf, sizeof(buf)); break;
                    case MODULE_WM:         fetch_wm(buf, sizeof(buf)); break;
                    case MODULE_THEME:      fetch_theme(buf, sizeof(buf)); break;
                    case MODULE_ICONS:      fetch_icons(buf, sizeof(buf)); break;
                    case MODULE_CURSOR:     fetch_cursor(buf, sizeof(buf)); break;
                    case MODULE_FONT:       fetch_font(buf, sizeof(buf)); break;
                    case MODULE_UPTIME:     fetch_uptime(buf, sizeof(buf)); break;
                    case MODULE_PACKAGES:   fetch_packages(buf, sizeof(buf)); break;
                    case MODULE_LOCALE:     fetch_locale(buf, sizeof(buf)); break;
                    case MODULE_CPU:        fetch_cpu(buf, sizeof(buf)); break;
                    case MODULE_CPU_TEMP:   fetch_cpu_temp(buf, sizeof(buf)); break;
                    case MODULE_CPU_LOAD:   fetch_cpu_load(buf, sizeof(buf)); break;
                    case MODULE_CPU_USAGE:  fetch_cpu_usage(buf, sizeof(buf)); break;
                    case MODULE_GPU:        fetch_gpu(buf, sizeof(buf)); break;
                    case MODULE_GPU_TEMP:   fetch_gpu_temp(buf, sizeof(buf)); break;
                    case MODULE_GPU_VRAM:   fetch_gpu_vram(buf, sizeof(buf)); break;
                    case MODULE_RESOLUTION: fetch_resolution(buf, sizeof(buf)); break;
                    case MODULE_MEMORY:     fetch_memory(buf, sizeof(buf)); break;
                    case MODULE_SWAP:       fetch_swap(buf, sizeof(buf)); break;
                    case MODULE_DISK:       fetch_disk(buf, sizeof(buf)); break;
                    case MODULE_NETWORK:    fetch_network(buf, sizeof(buf)); break;
                    case MODULE_BATTERY:    fetch_battery(buf, sizeof(buf)); break;
                    case MODULE_DATETIME:   fetch_datetime(buf, sizeof(buf)); break;
                    case MODULE_PROCESSES:  fetch_processes(buf, sizeof(buf)); break;
                    case MODULE_CUSTOM:     snprintf(buf, sizeof(buf), "%s", m->label); break;
                    default: break;
                }
                time_modules[i] = get_time_us() - t_m0;
                if (buf[0]) {
                    truncate_to(buf, g_val_max); /* fit values to terminal width */
                    if (m->icon) printf("%s%s%s", m->icon_color ? m->icon_color : COLOR_ICON, m->icon, COLOR_RESET);
                    if (g_show_labels && m->label) { char key[128]; snprintf(key, sizeof(key), "%s:", m->label); printf("%s%-*s%s ", COLOR_LABEL, label_width, key, COLOR_RESET); }
                    printf("%s%s%s\n", COLOR_VALUE, buf, COLOR_RESET);
                } else {
                    putchar('\n');
                }
                break;
            }
            }
        }

        if (g_watch_interval > 0) {
            fflush(stdout);
            watch_pass++;
            sleep(g_watch_interval);
        }
    } while (g_watch_interval > 0);

    if (g_timing && watch_pass == 0) {
        long long end_time = get_time_us();
        printf("\n\033[1;33m=== Timing Summary ===\033[0m\n");
#if !ENABLE_THREADS
        printf("%-15s : %5lld µs\n", "worker_packages", time_pkg);
        printf("%-15s : %5lld µs\n", "worker_cpu", time_cpu);
        printf("%-15s : %5lld µs\n", "worker_gpu", time_gpu);
#endif
        for (int i = 0; i < num_modules; i++) {
            if (time_modules[i] > 0) {
                const ConfigModule *m = &modules[i];
                const char *label = m->label ? m->label : "Unknown";
                printf("%-15s : %5lld µs\n", label, time_modules[i]);
            }
        }
        printf("---------------------------\n");
        printf("%-15s : %5lld µs\n", "Total Execution", end_time - start_time);
    }

    if (t_pkg_started)       pthread_join(t_pkg, NULL);
    if (t_cpu_started)       pthread_join(t_cpu, NULL);
    if (t_gpu_started)       pthread_join(t_gpu, NULL);
    if (t_disk_started)      pthread_join(t_disk, NULL);
    if (t_net_started)       pthread_join(t_net, NULL);
    if (t_batt_started)      pthread_join(t_batt, NULL);
    if (t_cpu_usage_started) pthread_join(t_cpu_usage_t, NULL);

    return 0;
}


