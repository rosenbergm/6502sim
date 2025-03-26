#include "gp_memory.h"
#include "address.h"

#include <filesystem>
#include <format>
#include <fstream>

/**
 * Get current size of the memory.
 */
size_t GP_Memory::size() const { return memory_.size(); }

/**
 * Read an address in the memory.
 *
 * @param address The address to read.
 * @return std::byte The byte at the address.
 * @throws std::out_of_range If the address is out of bounds.
 */
std::byte GP_Memory::read(address address) const {
  return read(static_cast<size_t>(address));
}

/**
 * Read a raw address from the memory.
 *
 * @param address The address to read.
 * @return std::byte The byte at the address.
 * @throws std::out_of_range If the address is out of bounds.
 */
std::byte GP_Memory::read(size_t address) const { return memory_[address]; }

/**
 * Write a value to an address in the memory.
 *
 * @param address The address to write to.
 * @param value The value to write.
 */
void GP_Memory::write(address address, std::byte value) {
  if (address == print_device_addr_) {
    std::cout << static_cast<char>(value);
  }

  memory_[static_cast<size_t>(address)] = value;
}

/**
 * Import a binary file into the memory.
 *
 * @param s The input stream to read from.
 */
void GP_Memory::import(std::istream &s) {
  std::byte byte;

  while (s.read(reinterpret_cast<char *>(&byte), sizeof(std::byte))) {
    memory_.push_back(byte);
  }
}

/**
 * Import a binary file into the memory.
 *
 * @param filename The path to the binary file.
 * @throws std::runtime_error If the file could not be opened.
 */
void GP_Memory::import(const std::string &filename) {
  std::filesystem::path path(filename);
  uintmax_t mem_size = std::filesystem::file_size(path);

  std::ifstream file(filename, std::ios::binary);

  if (!file.good()) {
    throw std::runtime_error(std::format(
        "Could not open file {}. Make sure the assembled binary is there.",
        filename));
  }

  memory_.reserve(mem_size);

  import(file);
}
