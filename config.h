#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>

// ════════════════════════════════════════════════════════════
//  superfetch v7 — Configuration
//  Edit anything here, then run  make  to apply.
//
//  Runtime overrides (no recompile needed):
//    --bar-width N, --no-labels, --color-* #RRGGBB
//    --logo DISTRO, --modules LIST, --watch N
//    ~/.config/superfetch/superfetch.conf
// ════════════════════════════════════════════════════════════

// ── Image Settings ───────────────────────────────────────────
#define IMAGE_PATH        NULL
#define IMAGE_WIDTH_CELLS 46    /* Terminal cell width of image column  */
#define LOGO_PADDING      2     /* Blank spaces between logo and info   */

// ── Compile-time Defaults ────────────────────────────────────
// Overridable at runtime via config file or CLI flags.
#define SHOW_LABELS   1     /* 0 = hide label column           */
#define BAR_WIDTH     10    /* Width of progress bars          */

// ── Dynamic Colors ────────────────────────────────────────────
// Set by parse_pywal() or parse_xresources() at startup.
// Also overridable via --color-* CLI flags.
extern char COLOR_USER[32];
extern char COLOR_AT[32];
extern char COLOR_HOST[32];
extern char COLOR_SEP[32];
extern char COLOR_LABEL[32];
extern char COLOR_ICON[32];
extern char COLOR_VALUE[32];
extern char COLOR_BAR_FILL[32];
extern char COLOR_BAR_EMPTY[32];
extern char COLOR_TEMP_OK[32];
extern char COLOR_TEMP_WARN[32];
extern char COLOR_TEMP_HOT[32];

#define COLOR_RESET "\033[0m"

// ── Module Types ──────────────────────────────────────────────
typedef enum {
    MODULE_TITLE,       /* user@hostname header                   */
    MODULE_SEPARATOR,   /* horizontal rule / section divider      */
    MODULE_BLANK,       /* empty line                             */
    MODULE_OS,          /* Operating system name                  */
    MODULE_HOST,        /* Machine vendor + model                 */
    MODULE_KERNEL,      /* Linux kernel version                   */
    MODULE_SHELL,       /* Shell + version (bash 5.2, zsh 5.9…)  */
    MODULE_TERMINAL,    /* Terminal emulator                      */
    MODULE_WM,          /* Desktop / Window Manager               */
    MODULE_THEME,       /* GTK Theme                              */
    MODULE_ICONS,       /* GTK Icons                              */
    MODULE_CURSOR,      /* GTK Cursor theme                       */
    MODULE_FONT,        /* GTK Font                               */
    MODULE_UPTIME,      /* System uptime                          */
    MODULE_PACKAGES,    /* Package manager counts                 */
    MODULE_LOCALE,      /* LANG / locale string                   */
    MODULE_CPU,         /* CPU model, core count, max freq        */
    MODULE_CPU_TEMP,    /* CPU package temperature (°C)           */
    MODULE_CPU_LOAD,    /* Load average (1m / 5m / 15m)           */
    MODULE_CPU_USAGE,   /* Live CPU usage % + bar (100ms sample)  */
    MODULE_GPU,         /* GPU model from PCI                     */
    MODULE_GPU_TEMP,    /* GPU temperature (°C)                   */
    MODULE_GPU_VRAM,    /* GPU dedicated VRAM (AMD DRM sysfs)     */
    MODULE_RESOLUTION,  /* Connected display resolution(s)        */
    MODULE_MEMORY,      /* RAM used / total + bar                 */
    MODULE_SWAP,        /* Swap used / total + bar                */
    MODULE_DISK,        /* Multi-disk partitions                  */
    MODULE_NETWORK,     /* Interface, IP, WiFi signal             */
    MODULE_BATTERY,     /* Battery % + status                     */
    MODULE_DATETIME,    /* Current date + time                    */
    MODULE_PROCESSES,   /* Total running process count            */
    MODULE_COLORS,      /* 8-color terminal palette block         */
    MODULE_CUSTOM,      /* Arbitrary static text                  */
    MODULE_CUSTOM_FUNC  /* Calls your custom C function below     */
} ModuleType;

// ── Module Structure ──────────────────────────────────────────
typedef struct {
    ModuleType  type;
    const char *icon;        /* Nerd Font glyph + space, or NULL  */
    const char *icon_color;  /* Override icon color, or NULL      */
    const char *label;       /* Key text e.g. "OS", "Kernel"      */
    void (*func)(void);      /* Used only by MODULE_CUSTOM_FUNC   */
} ConfigModule;

// ── Custom User Functions ─────────────────────────────────────
static void my_custom_hello(void) {
    printf("\033[38;5;213mHello from custom C function!\033[0m\n");
}

// ── Layout ────────────────────────────────────────────────────
// Add, remove, or reorder rows freely — then run  make .
// Uncomment any of the commented-out modules to enable them.
static const ConfigModule modules[] = {
    /* header */
    { MODULE_TITLE,      NULL,   NULL, NULL,         NULL },
    { MODULE_SEPARATOR,  NULL,   NULL, NULL,         NULL },
    /* system */
    { MODULE_OS,        " ",   NULL, "OS",         NULL },
    { MODULE_HOST,      "󰌢 ",   NULL, "Host",       NULL },
    { MODULE_KERNEL,    " ",   NULL, "Kernel",     NULL },
    { MODULE_UPTIME,    "󰅐 ",   NULL, "Uptime",     NULL },
    { MODULE_PACKAGES,  "󰏖 ",   NULL, "Packages",   NULL },
    /* environment */
    { MODULE_SEPARATOR,  NULL,   NULL, NULL,         NULL },
    { MODULE_SHELL,     " ",   NULL, "Shell",      NULL },
    { MODULE_TERMINAL,  " ",   NULL, "Terminal",   NULL },
    { MODULE_WM,        " ",   NULL, "DE/WM",      NULL },
    { MODULE_THEME,     "󰏘 ",   NULL, "Theme",      NULL },
    { MODULE_ICONS,     "󰀻 ",   NULL, "Icons",      NULL },
    { MODULE_CURSOR,    "󰆿 ",   NULL, "Cursor",     NULL },
    { MODULE_FONT,      " ",   NULL, "Font",       NULL },
    /* hardware */
    { MODULE_SEPARATOR,  NULL,   NULL, NULL,         NULL },
    { MODULE_CPU,       " ",   NULL, "CPU",        NULL },
    { MODULE_CPU_TEMP,  "󰔏 ",   NULL, "CPU Temp",   NULL },
    { MODULE_CPU_LOAD,  " ",   NULL, "Load Avg",   NULL },
 /* { MODULE_CPU_USAGE, "󰍛 ",   NULL, "CPU Usage",  NULL }, */
    { MODULE_GPU,       "󰢮 ",   NULL, "GPU",        NULL },
 /* { MODULE_GPU_TEMP,  "󰔏 ",   NULL, "GPU Temp",   NULL }, */
 /* { MODULE_GPU_VRAM,  "󰘚 ",   NULL, "GPU VRAM",   NULL }, */
    { MODULE_RESOLUTION,"󰍹 ",   NULL, "Resolution", NULL },
    { MODULE_MEMORY,    " ",   NULL, "Memory",     NULL },
    { MODULE_SWAP,      "󰓡 ",   NULL, "Swap",       NULL },
    { MODULE_DISK,      "󰋊 ",   NULL, "Disk",       NULL },
    { MODULE_NETWORK,   "󰤨 ",   NULL, "Network",    NULL },
    { MODULE_BATTERY,   " ",   NULL, "Battery",    NULL },
 /* { MODULE_PROCESSES, "󰊠 ",   NULL, "Processes",  NULL }, */
    /* footer */
    { MODULE_BLANK,      NULL,   NULL, NULL,         NULL },
    { MODULE_CUSTOM_FUNC,"󰈺 ",   NULL, "Custom",     my_custom_hello },
    { MODULE_COLORS,     NULL,   NULL, NULL,         NULL },
};

#endif /* CONFIG_H */
