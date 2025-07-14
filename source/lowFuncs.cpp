#include "lowFuncs.h"

uint32_t swapBytes(uint32_t value) {
  return ((value >> 24) & 0xFF) |
         ((value >> 8)  & 0xFF00) |
         ((value << 8)  & 0xFF0000) |
         ((value << 24) & 0xFF000000);
}

uint16_t swapBytes(uint16_t value) {
    return (value >> 8) | (value << 8);
}

uint64_t swapBytes(uint64_t value) {
  uint32_t low = value & 0xFFFFFFFF;
  uint32_t high = (value >> 32) & 0xFFFFFFFF;

  uint32_t swappedLow = swapBytes(high);
  uint32_t swappedHigh = swapBytes(low);

  return ((uint64_t)swappedHigh << 32) | swappedLow;
}

uint8_t checkSystemEndian(){
  uint32_t number = 0x12345678;
  uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&number);
  return (bytePtr[0] == 0x78) ? LITTLE_ENDIAN : BIG_ENDIAN;
  //std::cout << "Число: 0x" << std::hex << number << std::endl;
  //std::cout << "Байты в памяти: 0x" << std::hex;
  //for (int i = 0; i < sizeof(number); ++i) {
  //  std::cout << static_cast<int>(bytePtr[i]) << " ";
  //}
  //if (bytePtr[0] == 0x78) {
  //  std::cout << "Система: Little-endian" << std::endl;
  //} else {
  //  std::cout << "Система: Big-endian" << std::endl;
  //}
}