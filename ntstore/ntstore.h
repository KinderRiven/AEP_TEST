#ifndef NVMEM_NVMEM_H
#define NVMEM_NVMEM_H

#include <stdint.h>
#include <stdio.h>

void nvmem_memcpy(char *dest, const char *src, size_t len);

#endif
