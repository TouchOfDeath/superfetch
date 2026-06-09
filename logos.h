#ifndef LOGOS_H
#define LOGOS_H

#include <string.h>
#include <stdio.h>

// ════════════════════════════════════════════════════════════
//  ANSI-AWARE VISIBLE LENGTH
//  Counts visible terminal columns, skipping escape sequences
//  and counting multi-byte UTF-8 chars as single columns.
// ════════════════════════════════════════════════════════════
static inline int ansi_visible_len(const char *s) {
    int len = 0;
    while (*s) {
        if ((unsigned char)*s == 0x1B) { /* ESC */
            s++;
            if (*s == '[') { /* CSI sequence */
                s++;
                while (*s && ((*s >= '0' && *s <= '9') || *s == ';')) s++;
                if (*s) s++; /* final byte */
            } else if (*s) {
                s++;
            }
        } else {
            unsigned char c = (unsigned char)*s;
            if      (c < 0x80) { len++; s++; }     /* ASCII */
            else if (c < 0xC0) { s++; }             /* UTF-8 continuation */
            else if (c < 0xE0) { len++; s += 2; }  /* 2-byte */
            else if (c < 0xF0) { len++; s += 3; }  /* 3-byte (e.g. Nerd Fonts) */
            else               { len++; s += 4; }  /* 4-byte */
        }
    }
    return len;
}

// ════════════════════════════════════════════════════════════
//  DISTRO LOGOS
// ════════════════════════════════════════════════════════════

/* Ubuntu — orange + white */
static const char *logo_ubuntu[] = {
    "\033[38;5;202m            .-/+oossssoo+/-.",
    "\033[38;5;202m        `:+ssssssssssssssssss+:`",
    "\033[38;5;202m      -+ssssssssssssssssssyyssss+-",
    "\033[38;5;202m    .ossssssssssssssssss\033[1;37mdMMMNy\033[38;5;202msssso.",
    "\033[38;5;202m   /sssssssssss\033[1;37mhdmmNNmmyNMMMMh\033[38;5;202mssssss/",
    "\033[38;5;202m  +sssssssss\033[1;37mhm\033[38;5;202myd\033[1;37mMMMMMMMNddddy\033[38;5;202mssssssss+",
    "\033[38;5;202m /ssssssss\033[1;37mhNMMM\033[38;5;202myh\033[1;37mhyyyyhmNMMMNh\033[38;5;202mssssssss/",
    "\033[38;5;202m.ssssssss\033[1;37mdMMMNh\033[38;5;202mssssssssss\033[1;37mhNMMMd\033[38;5;202mssssssss.",
    "\033[38;5;202m+ssss\033[1;37mhhhyNMMNy\033[38;5;202mssssssssssss\033[1;37myNMMMy\033[38;5;202msssssss+",
    "\033[38;5;202moss\033[1;37myNMMMNyMMh\033[38;5;202mssssssssssssss\033[1;37mhmmmh\033[38;5;202mssssssso",
    "\033[38;5;202moss\033[1;37myNMMMNyMMh\033[38;5;202mssssssssssssss\033[1;37mhmmmh\033[38;5;202mssssssso",
    "\033[38;5;202m+ssss\033[1;37mhhhyNMMNy\033[38;5;202mssssssssssss\033[1;37myNMMMy\033[38;5;202msssssss+",
    "\033[38;5;202m.ssssssss\033[1;37mdMMMNh\033[38;5;202mssssssssss\033[1;37mhNMMMd\033[38;5;202mssssssss.",
    "\033[38;5;202m /ssssssss\033[1;37mhNMMM\033[38;5;202myh\033[1;37mhyyyyhdNMMMNh\033[38;5;202mssssssss/",
    "\033[38;5;202m  +sssssssss\033[1;37mdm\033[38;5;202myd\033[1;37mMMMMMMMMddddy\033[38;5;202mssssssss+",
    "\033[38;5;202m   /sssssssssss\033[1;37mhdmNNNNmyNMMMMh\033[38;5;202mssssss/",
    "\033[38;5;202m    .ossssssssssssssssss\033[1;37mdMMMNy\033[38;5;202msssso.",
    "\033[38;5;202m      -+sssssssssssssssss\033[1;37myyy\033[38;5;202mssss+-",
    "\033[38;5;202m        `:+ssssssssssssssssss+:`",
    "\033[38;5;202m            .-/+oossssoo+/-.",
};

/* Arch Linux — cyan */
static const char *logo_arch[] = {
    "\033[1;36m                   -`",
    "\033[1;36m                  .o+`",
    "\033[1;36m                 `ooo/",
    "\033[1;36m                `+oooo:",
    "\033[1;36m               `+oooooo:",
    "\033[1;36m               -+oooooo+:",
    "\033[1;36m             `/:-:++oooo+:",
    "\033[1;36m            `/++++/+++++++:",
    "\033[1;36m           `/++++++++++++++:",
    "\033[1;36m          `/+++ooooooooooooo/`",
    "\033[1;36m         ./ooosssso++osssssso+`",
    "\033[1;36m        .oossssso-````/ossssss+`",
    "\033[1;36m       -osssssso.      :ssssssso.",
    "\033[1;36m      :osssssss/        osssso+++.",
    "\033[1;36m     /ossssssss/        +ssssooo/-",
    "\033[1;36m   `/ossssso+/:-        -:/+osssso+-",
    "\033[1;36m  `+sso+:-`                 `.-/+oso:",
    "\033[1;36m `++:.                           `-/+/",
    "\033[1;36m .`                                 `/",
};

/* Fedora — blue */
static const char *logo_fedora[] = {
    "\033[1;34m          /:-------------:\\",
    "\033[1;34m       :-------------------::",
    "\033[1;34m     :-----------/shhOHbmp---:\\",
    "\033[1;34m   /-----------omMMMNNNMMD  ---:",
    "\033[1;34m  :-----------sMMMMNMNMP.    ---:",
    "\033[1;34m :-----------:MMMdP-------    ---\\",
    "\033[1;34m,------------:MMMd--------    ---:",
    "\033[1;34m:------------:MMMd-------    .---:",
    "\033[1;34m:----    oNMMMMMMMMMNho     .----:",
    "\033[1;34m:--     .+shhhMMMmhhy++   .------/",
    "\033[1;34m:-    -------:MMMd--------------:",
    "\033[1;34m:-   --------/MMMd-------------;",
    "\033[1;34m:-    ------/hMMMy------------:",
    "\033[1;34m:-- :dMNdhhdNMMNo------------;",
    "\033[1;34m:---:sdNMMMMNds:------------:",
    "\033[1;34m:------:://:-------------::",
    "\033[1;34m:-----------  ----------::",
    "\033[1;34m:------------------------:",
};

/* Debian — red */
static const char *logo_debian[] = {
    "\033[1;31m       _,met$$$$$gg.",
    "\033[1;31m    ,g$$$$$$$$$$$$$$$P.",
    "\033[1;31m  ,g$$P\"     \"\"\"Y$$.\".\"",
    "\033[1;31m ,$$P'              `$$$.",
    "\033[1;31m',$$P       ,ggs.     `$$b:",
    "\033[1;31m`d$$'     ,$P\"'   .    $$$",
    "\033[1;31m $$P      d$'     ,    $$P",
    "\033[1;31m $$:      $$.   -    ,d$$'",
    "\033[1;31m $$;      Y$b._   _,d$P'",
    "\033[1;31m Y$$.    `.`\"Y$$$$P\"'",
    "\033[1;31m `$$b      \"-.__",
    "\033[1;31m  `Y$$",
    "\033[1;31m   `Y$$.",
    "\033[1;31m     `$$b.",
    "\033[1;31m       `Y$$b.",
    "\033[1;31m          `\"Y$b._",
    "\033[1;31m              `\"\"\"",
};

/* Linux Mint — green */
static const char *logo_linuxmint[] = {
    "\033[1;32m             ...-:::::-...",
    "\033[1;32m          .-MMMMMMMMMMMMMMM-.",
    "\033[1;32m      .-MMMM`..-:::::::-..`MMMM-.",
    "\033[1;32m    .:MMMM.:MMMMMMMMMMMMMMM:.MMMM:.",
    "\033[1;32m   -MMM-M---MMMMMMMMMMMMMMMMMMM.MMM-",
    "\033[1;32m `:MMM:MM`  :MMMM:....::-...-MMMM:MMM:`",
    "\033[1;32m :MMM:MMM`  :MM:`  ``    ``  `:MMM:MMM:",
    "\033[1;32m.MMM.MMMM`  :MM.  -MM.  .MM-  `MMMM.MMM.",
    "\033[1;32m:MMM:MMMM`  :MM.  -MM-  .MM:  `MMMM-MMM:",
    "\033[1;32m:MMM:MMMM`  :MM.  -MM-  .MM:  `MMMM:MMM:",
    "\033[1;32m:MMM:MMMM`  :MM.  -MM-  .MM:  `MMMM-MMM:",
    "\033[1;32m.MMM.MMMM`  :MM:--:MM:--:MM:  `MMMM.MMM.",
    "\033[1;32m :MMM:MMM-  `-MMMMMMMMMMMM-`  -MMM-MMM:",
    "\033[1;32m  :MMM:MMM:`                `:MMM:MMM:",
    "\033[1;32m    .MMM.MMMM-            -MMMM.MMM.",
    "\033[1;32m      `-MMMM`.`-:::::::-`.`MMMM-`",
    "\033[1;32m          `-MMMMMMMMMMMMMMM-`",
    "\033[1;32m              `--:::::--`",
};

/* Manjaro — bright green blocks */
static const char *logo_manjaro[] = {
    "\033[38;5;46m ██████████████████  ████████",
    "\033[38;5;46m ████████████████████████████",
    "\033[38;5;46m ████████            ████████",
    "\033[38;5;46m ████████  ████████  ████████",
    "\033[38;5;46m ████████  ████████  ████████",
    "\033[38;5;46m ████████  ████████  ████████",
    "\033[38;5;46m ████████  ████████  ████████",
    "\033[38;5;46m ████████  ████████  ████████",
    "\033[38;5;46m ████████  ████████  ████████",
    "\033[38;5;46m ████████  ████████  ████████",
    "\033[38;5;46m ████████  ████████  ████████",
};

/* Pop!_OS — teal */
static const char *logo_pop[] = {
    "\033[38;5;45m          ..',;:ccccccc:;,..`",
    "\033[38;5;45m       .;lxxxxxxxxxxxxxxxxxxxxl;.",
    "\033[38;5;45m     ;oxxxxxxxxxxxxxxxxxxxxxxxxxxxxo;",
    "\033[38;5;45m   ,oxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx,",
    "\033[38;5;45m  ;xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;",
    "\033[38;5;45m .xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
    "\033[38;5;45m .xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
    "\033[38;5;45m .xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
    "\033[38;5;45m  ;xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;",
    "\033[38;5;45m   `oxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxo`",
    "\033[38;5;45m     `:lxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx:l:`",
    "\033[38;5;45m         `.:;oxxxxxxxxxxxxxxxxxxxxxxo;:.`",
    "\033[38;5;45m                `.,;:ccccccc:;,..`",
    "\033[38;5;45m",
    "\033[38;5;45m        System76 Pop!_OS",
};

/* openSUSE — green */
static const char *logo_opensuse[] = {
    "\033[38;5;34m           .;ldkO0000Okdl;.",
    "\033[38;5;34m         .OK0o:';d0Oo:.:.OM.",
    "\033[38;5;34m        .OO     `0O     `O:",
    "\033[38;5;34m       :0K.       0K       0l",
    "\033[38;5;34m      ;O0          0O       0;",
    "\033[38;5;34m     'OO    .oOOOo. 0O       0o",
    "\033[38;5;34m     `00    O0OOOO0O0O       Ok",
    "\033[38;5;34m      O0.   0O     00        O0",
    "\033[38;5;34m      `0O    `0OOO0O'       .0O",
    "\033[38;5;34m       .KO.    `000'       .0K",
    "\033[38;5;34m         l0O:           .;d00'",
    "\033[38;5;34m           .OK0OdolllodO0Ko.",
    "\033[38;5;34m              `0OOOOOO00K'",
    "\033[38;5;34m                 `K0000'",
    "\033[38;5;34m                   `0Ko",
    "\033[38;5;34m                    `0",
    "\033[38;5;34m                     .",
};

/* NixOS — blue + purple snowflake */
static const char *logo_nixos[] = {
    "\033[1;34m          ::::.    \033[1;35m:::::",
    "\033[1;34m        :::::::  \033[1;35m:::::::",
    "\033[1;34m       ::::::::  \033[1;35m::::::::",
    "\033[1;34m  .::::::::      \033[1;35m::::::::",
    "\033[1;34m ::::::::::   \033[1;35m::::::::::",
    "\033[1;34m ::::::::::::\033[1;35m:::::::::::",
    "\033[1;35m  :::::::::::::::::::::",
    "\033[1;35m   ::::::::::::::::::::",
    "\033[1;35m    ::::::::::::::::::",
    "\033[1;35m      ::::::::::::::::",
    "\033[1;34m       :::::::: \033[1;35m::::::::",
    "\033[1;34m        :::::::  \033[1;35m:::::::",
    "\033[1;34m          ::::::   \033[1;35m:::::",
    "\033[1;34m            `::::   \033[1;35m:::",
    "\033[1;34m              `:::   \033[1;35m::",
    "\033[1;34m                `:   \033[1;35m:",
    "\033[1;34m                  `.",
};

/* Kali Linux — blue + dark */
static const char *logo_kali[] = {
    "\033[1;34m..............                 ",
    "\033[1;34m          ..,;:ccc,.",
    "\033[1;34m        ......''';lxO.",
    "\033[1;34m.....''''..........,:ld;",
    "\033[1;34m           .';;;:::;,,.x,",
    "\033[1;34m      ..'''.            0Xxoc:,",
    "\033[1;34m  ....                ,ONkc;,;cokOdc',",
    "\033[1;34m .                   OMo           ':ddo.",
    "\033[1;34m                    dMc               :OO;",
    "\033[1;34m                    0M.                 .:o.",
    "\033[1;34m                    ;Wd",
    "\033[1;34m                     ;XO,",
    "\033[1;34m                       ,d0Odlc;,..",
    "\033[1;34m                           ..',;:cdo",
    "\033[1;34m                                    .l;",
    "\033[1;34m                                      `.",
};

/* Void Linux — green */
static const char *logo_void[] = {
    "\033[1;32m                __.;=====;.__",
    "\033[1;32m            _=`'          '`=_",
    "\033[1;32m          _-'    .=.  .=.    '-_",
    "\033[1;32m         =      (    )(    )      =",
    "\033[1;32m         |     /.    .  .    .\\     |",
    "\033[1;32m         |    / /   .    .   \\ \\    |",
    "\033[1;32m         |   = /.=.    .=.\\   \\ |   |",
    "\033[1;32m         |  =  ;=|     |=;  =  |   |",
    "\033[1;32m          \\ =  =|#|;, ,;|#|=  = /",
    "\033[1;32m          |\\    =|=  =  =|=    /|",
    "\033[1;32m          | \\  .'-;=====;-'.  / |",
    "\033[1;32m          |  '=.            .='  |",
    "\033[1;32m           \\    '-.______.-'    /",
    "\033[1;32m            \\                  /",
    "\033[1;32m             '-.____________.-'",
};

/* Gentoo — purple */
static const char *logo_gentoo[] = {
    "\033[1;35m         -/oyddmdhs+:",
    "\033[1;35m      -odNMMMMMMMMNNmhy+-`",
    "\033[1;35m    .+dmMMMMMMMMMMMMMMMMMMy",
    "\033[1;35m    ddMMMMMMMMMMMMMMMMMMMMM.",
    "\033[1;35m    NMMMMMMMMMMMMMMMMMMMMM/",
    "\033[1;35m    dMMMMMMMMMMMMMMMMMMMMy",
    "\033[1;35m    `NMMMMMMMMMMMMMMMMMMm.",
    "\033[1;35m      -/oydNMMMMMMMMMMMm.",
    "\033[1;35m   `hNMMMMMMMMMMMMMMMMMMm",
    "\033[1;35m   dMMMMMMMMMMMMMMMMMMMMh",
    "\033[1;35m   /MMMMMMMMMMMMMMMMMMMMm.",
    "\033[1;35m   yMMMMMMMMMMMMMMMMMMMMM.",
    "\033[1;35m    :NMMMMMMMMMMMMMMMMMMm:",
    "\033[1;35m    odMMMMMMMMMMMMMMMMMMM.",
    "\033[1;35m     .dmMMMMMMMMMMMMMMMMN.",
    "\033[1;35m      `.:+shhdNMMMMMMMMMy:",
    "\033[1;35m              .+mMMMMMMNs.",
};

/* Generic Linux (fallback — Tux) */
static const char *logo_linux[] = {
    "        #####        ",
    "       #######       ",
    "       ##O#O##       ",
    "       #######       ",
    "     ###########     ",
    "    #############    ",
    "   ###############   ",
    "   ####  ###  ####   ",
    "   ####  ###  ####   ",
    "    ###  ###  ###    ",
    "      ####  ####     ",
    "    ####  ####       ",
    "   ###########       ",
    "   ###########       ",
    "    #########        ",
    "      #####          ",
};

// ════════════════════════════════════════════════════════════
//  LOGO REGISTRY
// ════════════════════════════════════════════════════════════

typedef struct {
    const char  *id;
    const char **lines;
    int          num_lines;
} OsLogo;

#define LOGO_ENTRY(id, arr) { id, arr, (int)(sizeof(arr) / sizeof(arr[0])) }

static const OsLogo os_logos[] = {
    LOGO_ENTRY("ubuntu",              logo_ubuntu),
    LOGO_ENTRY("kubuntu",             logo_ubuntu),
    LOGO_ENTRY("xubuntu",             logo_ubuntu),
    LOGO_ENTRY("lubuntu",             logo_ubuntu),
    LOGO_ENTRY("arch",                logo_arch),
    LOGO_ENTRY("manjaro",             logo_manjaro),
    LOGO_ENTRY("fedora",              logo_fedora),
    LOGO_ENTRY("debian",              logo_debian),
    LOGO_ENTRY("linuxmint",           logo_linuxmint),
    LOGO_ENTRY("pop",                 logo_pop),
    LOGO_ENTRY("opensuse",            logo_opensuse),
    LOGO_ENTRY("opensuse-tumbleweed", logo_opensuse),
    LOGO_ENTRY("opensuse-leap",       logo_opensuse),
    LOGO_ENTRY("nixos",               logo_nixos),
    LOGO_ENTRY("kali",                logo_kali),
    LOGO_ENTRY("void",                logo_void),
    LOGO_ENTRY("gentoo",              logo_gentoo),
    /* fallback */
    LOGO_ENTRY("linux",               logo_linux),
};

#define NUM_OS_LOGOS ((int)(sizeof(os_logos) / sizeof(os_logos[0])))

// ════════════════════════════════════════════════════════════
//  LOGO LOOKUP
//  Accepts already-parsed id + id_like strings.
//  Tries exact ID match, then each token in ID_LIKE,
//  then substring match for compound IDs like opensuse-*.
// ════════════════════════════════════════════════════════════
static inline void get_os_logo_by_id(
        const char   *id,
        const char   *id_like,
        const char ***out_lines,
        int          *out_count)
{
    /* Default: generic Linux */
    *out_lines = (const char **)logo_linux;
    *out_count = (int)(sizeof(logo_linux) / sizeof(logo_linux[0]));

    /* 1. Exact ID match */
    for (int i = 0; i < NUM_OS_LOGOS; i++) {
        if (strcmp(id, os_logos[i].id) == 0) {
            *out_lines = os_logos[i].lines;
            *out_count = os_logos[i].num_lines;
            return;
        }
    }

    /* 2. Substring match for compound IDs (e.g. opensuse-tumbleweed) */
    for (int i = 0; i < NUM_OS_LOGOS; i++) {
        if (strstr(id, os_logos[i].id) != NULL) {
            *out_lines = os_logos[i].lines;
            *out_count = os_logos[i].num_lines;
            return;
        }
    }

    /* 3. ID_LIKE space-separated list */
    if (id_like && id_like[0]) {
        char buf[128];
        strncpy(buf, id_like, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        char *token = strtok(buf, " ");
        while (token) {
            for (int i = 0; i < NUM_OS_LOGOS; i++) {
                if (strcmp(token, os_logos[i].id) == 0) {
                    *out_lines = os_logos[i].lines;
                    *out_count = os_logos[i].num_lines;
                    return;
                }
            }
            token = strtok(NULL, " ");
        }
    }
}

#endif /* LOGOS_H */
