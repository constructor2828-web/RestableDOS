#ifndef DISK_H
#define DISK_H
#include <stdint.h>
void disk_write_sectors(uint32_t lba, uint8_t count, uint64_t buffer_addr);
void disk_read_sectors(uint32_t lba, uint8_t count, uint64_t buffer_addr);
#endif
