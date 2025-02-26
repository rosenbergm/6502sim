#ifndef _H_BYTE_UTILS
#define _H_BYTE_UTILS

#include <cstdint>
#include <iostream>
#include <sstream>

// inline std::ostream &operator<<(std::ostream &os, std::byte b) {
//   return os << "0x" << std::hex << std::uppercase
//             << static_cast<unsigned int>(b) << std::dec;

// return os << std::bitset<8>(std::to_integer<unsigned int>(b));
// }

constexpr std::byte ZERO_BYTE = std::byte(0x00);
constexpr std::byte FULL_BYTE = std::byte(0xFF);

static size_t hex_to_number(const std::string &hex) {
  size_t result = 0;

  std::stringstream ss;
  ss << std::hex << hex;
  ss >> result;

  return result;
}

static size_t hex_to_number(std::string &&hex) {
  size_t result = 0;

  std::stringstream ss;
  ss << std::hex << hex;
  ss >> result;

  return result;
}

struct byte_result {
  std::byte value{};
  bool carry;
  bool negative;

  constexpr byte_result() = default;
  constexpr byte_result(std::byte value_, bool carry_, bool negative_)
      : value(value_), carry(carry_), negative(negative_) {}
};

constexpr byte_result operator+(std::byte a, std::byte b) {
  uint16_t result = std::to_integer<uint16_t>(a) + std::to_integer<uint16_t>(b);
  return {std::byte(result & 0xFF), result > 0xFF, result > 0x7F};
}

constexpr byte_result operator-(std::byte a, std::byte b) {
  uint16_t result = std::to_integer<uint16_t>(a) - std::to_integer<uint16_t>(b);
  return {std::byte(result & 0xFF), result > 0xFF, result > 0x7F};
}

constexpr byte_result operator+(std::byte a, int b) {
  uint16_t result = std::to_integer<uint16_t>(a) + b;
  return {std::byte(result & 0xFF), result > 0xFF, result > 0x7F};
}

constexpr byte_result operator-(std::byte a, int b) {
  uint16_t result = std::to_integer<uint16_t>(a) - b;
  return {std::byte(result & 0xFF), result > 0xFF, result > 0x7F};
}

constexpr byte_result operator+(const byte_result &a, int b) {
  uint16_t result = std::to_integer<uint16_t>(a.value) + b;
  return {std::byte(result & 0xFF), result > 0xFF || a.carry, result > 0x7F};
}

constexpr byte_result operator+(int b, const byte_result &a) {
  uint16_t result = b + std::to_integer<uint16_t>(a.value);
  return {std::byte(result & 0xFF), result > 0xFF || a.carry, result > 0x7F};
}

constexpr byte_result operator+(std::byte a, const byte_result &b) {
  uint16_t result =
      std::to_integer<uint16_t>(a) + std::to_integer<uint16_t>(b.value);
  return {std::byte(result & 0xFF), result > 0xFF || b.carry, result > 0x7F};
}

constexpr byte_result operator+(const byte_result &a, std::byte b) {
  uint16_t result =
      std::to_integer<uint16_t>(a.value) + std::to_integer<uint16_t>(b);
  return {std::byte(result & 0xFF), result > 0xFF || a.carry, result > 0x7F};
}

constexpr byte_result operator-(const byte_result &a, std::byte b) {
  uint16_t result =
      std::to_integer<uint16_t>(a.value) - std::to_integer<uint16_t>(b);
  return {std::byte(result & 0xFF), result > 0xFF || a.carry, result > 0x7F};
}

constexpr byte_result operator-(std::byte a, const byte_result &b) {
  uint16_t result =
      std::to_integer<uint16_t>(a) - std::to_integer<uint16_t>(b.value);
  return {std::byte(result & 0xFF), result > 0xFF || b.carry, result > 0x7F};
}

constexpr bool is_negative(std::byte b) {
  return (std::to_integer<int>(b) & 0x80);
}

constexpr bool is_zero(std::byte b) { return std::to_integer<int>(b) == 0; }

constexpr bool is_bit_set(std::byte b, int bit) {
  return (b & std::byte(1 << bit)) != ZERO_BYTE;
}

#endif