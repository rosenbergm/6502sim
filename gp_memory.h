#ifndef _H_GP_MEMORY
#define _H_GP_MEMORY

#include "address.h"
#include <iostream>
#include <stddef.h>
#include <vector>

/**
 * Default address of the memory mapped output print device.
 */
constexpr size_t DEFAULT_OUTPUT_ADDRESS = 0xFFFB;

/**
 * Class that represents general purpose random access memory that could be used
 * by the emulated CPU.
 */
class GP_Memory {
private:
  std::vector<std::byte> memory_;

  address print_device_addr_;

public:
  GP_Memory() : memory_(), print_device_addr_(DEFAULT_OUTPUT_ADDRESS) {}

  size_t size() const;

  std::byte read(address address) const;
  std::byte read(size_t address) const;
  void write(address address, std::byte value);

  void import(std::istream &s);
  void import(const std::string &filename);

  void set_print_device(address addr) { print_device_addr_ = addr; }
  address print_device_addr() const { return print_device_addr_; }
};

#endif