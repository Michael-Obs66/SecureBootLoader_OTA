#ifndef SHA256_H
#define SHA256_H
#include <stdint.h>
#include <stdbool.h>

bool sha256_compute(const uint8_t *data, size_t len, uint8_t *out32);

#endif
