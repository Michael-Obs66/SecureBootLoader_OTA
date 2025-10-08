#ifndef LOG_H
#define LOG_H
#include <stdio.h>
#include <stdbool.h>

#define log_info(...) printf("[INFO] "__VA_ARGS__)
#define log_error(...) printf("[ERROR] "__VA_ARGS__)

#endif
