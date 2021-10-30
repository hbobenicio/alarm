#ifndef ALARM_CONFIG_H
#define ALARM_CONFIG_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    int hour, min;
    const char* msg;
    size_t msg_len;
} Config;

Config config_from_env(void);

#endif
