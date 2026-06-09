#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

int main() {
    DIR *dir = opendir("/sys/bus/pci/devices");
    if (!dir) return 1;

    struct dirent *entry;
    char vendor_str[16] = {0};
    char device_str[16] = {0};
    int found = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char path[512];
        snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/class", entry->d_name);
        FILE *f = fopen(path, "r");
        if (f) {
            char class_str[16];
            if (fscanf(f, "0x%s", class_str) == 1) {
                // VGA class is 030000
                if (strncmp(class_str, "0300", 4) == 0) {
                    fclose(f);
                    snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/vendor", entry->d_name);
                    f = fopen(path, "r");
                    if (f) { fscanf(f, "0x%s", vendor_str); fclose(f); }

                    snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/device", entry->d_name);
                    f = fopen(path, "r");
                    if (f) { fscanf(f, "0x%s", device_str); fclose(f); }

                    found = 1;
                    break;
                }
            }
            fclose(f);
        }
    }
    closedir(dir);

    if (!found) {
        printf("GPU: Unknown\n");
        return 0;
    }

    FILE *fpci = fopen("/usr/share/hwdata/pci.ids", "r");
    if (!fpci) fpci = fopen("/usr/share/misc/pci.ids", "r");
    if (!fpci) {
        printf("GPU: %s:%s\n", vendor_str, device_str);
        return 0;
    }

    char line[256];
    int in_vendor = 0;
    char gpu_name[256] = "Unknown GPU";
    char vendor_name[256] = "Unknown Vendor";
    
    while (fgets(line, sizeof(line), fpci)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        
        if (line[0] != '\t') {
            if (strncasecmp(line, vendor_str, 4) == 0 && line[4] == ' ') {
                in_vendor = 1;
                char *name = line + 4;
                while (*name == ' ') name++;
                char *end = name + strlen(name) - 1;
                if (*end == '\n') *end = '\0';
                strncpy(vendor_name, name, sizeof(vendor_name)-1);
            } else {
                if (in_vendor) break;
                in_vendor = 0;
            }
        } else if (in_vendor && line[0] == '\t' && line[1] != '\t') {
            if (strncasecmp(line + 1, device_str, 4) == 0 && line[5] == ' ') {
                char *name = line + 5;
                while (*name == ' ') name++;
                char *end = name + strlen(name) - 1;
                if (*end == '\n') *end = '\0';
                snprintf(gpu_name, sizeof(gpu_name), "%s %s", vendor_name, name);
                break;
            }
        }
    }
    fclose(fpci);
    printf("GPU: %s\n", gpu_name);
    return 0;
}
