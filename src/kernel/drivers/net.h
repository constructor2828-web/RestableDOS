#ifndef NET_H
#define NET_H

#include <stdint.h>

void net_init(void);
int net_get_status(void); // 1 = Connected, 0 = Offline

#endif
