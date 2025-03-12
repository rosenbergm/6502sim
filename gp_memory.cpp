#include "gp_memory.h"
#include "address.h"

#include <format>
#include <fstream>

size_t GP_Memory::size() const { return size_; }

std::byte GP_Memory::read(address address) const {
  return read(static_cast<size_t>(address));
}

std::byte GP_Memory::read(size_t address) const { return memory_[address]; }

void GP_Memory::write(address address, std::byte value) {
  if (address == DEFAULT_OUTPUT_ADDRESS) {
    std::cout << static_cast<char>(value);
  }

  memory_[static_cast<size_t>(address)] = value;
}

void GP_Memory::import(std::istream &s) {
  std::byte byte;

  while (s.read(reinterpret_cast<char *>(&byte), sizeof(std::byte))) {
    size_++;
    memory_.push_back(byte);
  }
}

void GP_Memory::import(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);

  if (!file.good()) {
    throw std::runtime_error(std::format(
        "Could not open file {}. Make sure the assembled binary is there.",
        filename));
  }

  import(file);
}