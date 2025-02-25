#ifndef _H_GP_MEMORY
#define _H_GP_MEMORY

#include "address.h"
#include <iostream>
#include <stddef.h>
#include <vector>

/**
 * Address of the memory mapped output print device.
 */
constexpr size_t OUTPUT_ADDRESS = 0xFFFB;

/**
 * Class that represents general purpose random access memory that could be used
 * by the emulated CPU.
 */
class GP_Memory {
private:
  size_t size_;
  std::vector<std::byte> memory_;

public:
  GP_Memory() : size_(0), memory_() {}
  GP_Memory(size_t size) : size_(size), memory_(size) {}

  size_t size() const;

  std::byte read(address address) const;
  std::byte read(size_t address) const;
  void write(address address, std::byte value);

  void import(std::istream &s);
  void import(const std::string &filename);
};

#endif