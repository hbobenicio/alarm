#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <errno.h>

static int getenv_int(const char* env_var_name, int default_value, bool required);
static const char* getenv_str(const char* env_var_name, const char* default_value, bool required);

Config config_from_env(void)
{
    const char* msg = getenv_str("MSG", "!!! ALARM !!!", false);
    return (Config) {
        .hour = getenv_int("HOUR", -1, true),
        .min = getenv_int("MIN", -1, true),
        .msg = msg,
        .msg_len = strlen(msg),
    };
}

static int getenv_int(const char* env_var_name, int default_value, bool required)
{
    const char* value = getenv(env_var_name);
    if (value == NULL) {
        if (required) {
            fprintf(stderr, "[error] required environment variable \"%s\" not specified.\n", env_var_name);
            exit(EXIT_FAILURE);
        }
#ifdef DEBUG
        fprintf(stderr, "[DEBUG] environment variable \"%s\" is not defined. assuming default value: %d\n", env_var_name, default_value);
#endif
        return default_value;
    }

    char* endptr = NULL;
    errno = 0;
    unsigned long int ul_value = strtoul(value, &endptr, 10);
    if (errno != 0) {
        fprintf(stderr, "[error] could not parse environment variable %s to int: [%d] %s\n", env_var_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (ul_value >= INT_MAX) {
        fprintf(stderr, "[error] could not parse environment variable %s to int: integer overflow\n", env_var_name);
        exit(EXIT_FAILURE);
    }
    return (int) ul_value;
}

static const char* getenv_str(const char* env_var_name, const char* default_value, bool required)
{
    const char* value = getenv(env_var_name);
    if (value == NULL) {
        if (required) {
            fprintf(stderr, "[error] required environment variable \"%s\" not specified.\n", env_var_name);
            exit(EXIT_FAILURE);
        }
#ifdef DEBUG
        fprintf(stderr, "[DEBUG] environment variable \"%s\" is not defined. assuming default value: \"%s\"\n", env_var_name, default_value);
#endif
        return default_value;
    }
    return value;
}
