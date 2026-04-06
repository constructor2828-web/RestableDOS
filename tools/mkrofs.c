#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>

#define ROFS_MAGIC "ROFS"

#pragma pack(push, 1)
typedef struct {
    char name[32];
    uint32_t size;
    uint32_t offset;
    uint32_t type;
} rofs_entry_t;
#pragma pack(pop)

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <dir> <output.bin>\n", argv[0]);
        return 1;
    }
    DIR *d = opendir(argv[1]);
    if (!d) return 1;

    FILE *out = fopen(argv[2], "wb");
    if (!out) return 1;

    fwrite(ROFS_MAGIC, 1, 4, out);
    uint32_t count = 0;
    fwrite(&count, 1, 4, out);

    struct dirent *dir;
    rofs_entry_t entries[256];
    uint32_t num_entries = 0;
    
    uint32_t current_offset = 8 + 256 * sizeof(rofs_entry_t);

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;
        char path[512];
        int n = snprintf(path, sizeof(path), "%s/%s", argv[1], dir->d_name);
        if (n >= sizeof(path)) continue;
        FILE *f = fopen(path, "rb");
        if (!f) continue;
        fseek(f, 0, SEEK_END);
        uint32_t size = ftell(f);
        fclose(f);

        strncpy(entries[num_entries].name, dir->d_name, 31);
        entries[num_entries].name[31] = '\0';
        entries[num_entries].size = size;
        entries[num_entries].offset = current_offset;
        entries[num_entries].type = 0;
        current_offset += size;
        num_entries++;
        if (num_entries >= 256) break;
    }
    
    fseek(out, 4, SEEK_SET);
    fwrite(&num_entries, 1, 4, out);
    fwrite(entries, sizeof(rofs_entry_t), num_entries, out);

    for (uint32_t i = 0; i < num_entries; i++) {
        char path[512];
        int n = snprintf(path, sizeof(path), "%s/%s", argv[1], entries[i].name);
        if (n >= sizeof(path)) continue;
        FILE *f = fopen(path, "rb");
        if (!f) continue;
        if (entries[i].size > 0) {
            char *buf = malloc(entries[i].size);
            if (fread(buf, 1, entries[i].size, f) != entries[i].size) {}
            fseek(out, entries[i].offset, SEEK_SET);
            fwrite(buf, 1, entries[i].size, out);
            free(buf);
        }
        fclose(f);
    }

    fseek(out, 32767, SEEK_SET);
    char zero = 0;
    fwrite(&zero, 1, 1, out);

    fclose(out);
    closedir(d);
    return 0;
}
