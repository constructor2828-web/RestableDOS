#ifndef ROFS_H
#define ROFS_H
#include <stdint.h>
#include <stdint.h>
void rofs_init(uint64_t addr);
void rofs_ls(void);
int rofs_read_file(const char *name, uint8_t *buf, uint32_t max_len);
int rofs_write_file(const char *name, const uint8_t *data, uint32_t len);
#endif
