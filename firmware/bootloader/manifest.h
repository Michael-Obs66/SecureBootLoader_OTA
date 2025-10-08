#ifndef MANIFEST_H
#define MANIFEST_H
#include <stdint.h>

typedef struct {
    uint32_t version;
    uint32_t size;
    uint32_t slot_address;
    uint8_t  signature[64];
} FirmwareManifest;

#endif
