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
int getenv_int(const char* env_var_name, int default_value, bool required);
const char* getenv_str(const char* env_var_name, const char* default_value, bool required);

#endif
