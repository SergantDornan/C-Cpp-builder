#include <cstdint>

#define LITTLE_ENDIAN (1)
#define BIG_ENDIAN (2)

uint16_t swapBytes(uint16_t);
uint32_t swapBytes(uint32_t);
uint64_t swapBytes(uint64_t);
uint8_t checkSystemEndian();