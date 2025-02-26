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

constexpr bool is_negative(std::byte b) {
  return (b & std::byte(1 << 7)) != ZERO_BYTE;
}

constexpr bool is_zero(std::byte b) { return b == ZERO_BYTE; }

constexpr bool is_bit_set(std::byte b, int bit) {
  return (b & std::byte(1 << bit)) != ZERO_BYTE;
}

struct byte_result {
  std::byte value{};
  bool carry;
  bool negative;

  constexpr byte_result() = default;
  constexpr byte_result(std::byte value_, bool carry_, bool negative_)
      : value(value_), carry(carry_), negative(negative_) {}
};

// byte + byte
constexpr byte_result operator+(std::byte a, std::byte b) {
  uint16_t result = std::to_integer<uint16_t>(a) + std::to_integer<uint16_t>(b);
  std::byte byte_result = std::byte(result);

  return {byte_result, result > 0xFF, is_negative(byte_result)};
}

// byte - byte
constexpr byte_result operator-(std::byte a, std::byte b) {
  bool borrow = a < b;

  uint16_t result = std::to_integer<uint16_t>(a) - std::to_integer<uint16_t>(b);
  std::byte byte_result = std::byte(result);

  return {byte_result, borrow, is_negative(byte_result)};
}

// byte + int
constexpr byte_result operator+(std::byte a, uint8_t b) {
  uint16_t result = std::to_integer<uint16_t>(a) + b;
  std::byte byte_result = std::byte(result);

  return {byte_result, result > 0xFF, is_negative(byte_result)};
}

// byte - int
constexpr byte_result operator-(std::byte a, uint8_t b) {
  bool borrow = a < std::byte(b);

  uint16_t result = std::to_integer<uint16_t>(a) - b;
  std::byte byte_result = std::byte(result);

  return {byte_result, borrow, is_negative(byte_result)};
}

// byte_result + int
constexpr byte_result operator+(const byte_result &a, uint8_t b) {
  uint16_t result = std::to_integer<uint16_t>(a.value) + b;
  std::byte byte_result = std::byte(result);

  return {byte_result, result > 0xFF || a.carry, is_negative(byte_result)};
}

// int + byte_result
constexpr byte_result operator+(uint8_t b, const byte_result &a) {
  uint16_t result = std::to_integer<uint16_t>(a.value) + b;
  std::byte byte_result = std::byte(result);

  return {byte_result, result > 0xFF || a.carry, is_negative(byte_result)};
}

// byte + byte_result
constexpr byte_result operator+(std::byte a, const byte_result &b) {
  uint16_t result =
      std::to_integer<uint16_t>(a) + std::to_integer<uint16_t>(b.value);
  std::byte byte_result = std::byte(result);

  return {byte_result, result > 0xFF || b.carry, is_negative(byte_result)};
}

// byte_result + byte
constexpr byte_result operator+(const byte_result &a, std::byte b) {
  uint16_t result =
      std::to_integer<uint16_t>(a.value) + std::to_integer<uint16_t>(b);
  std::byte byte_result = std::byte(result);

  return {byte_result, result > 0xFF || a.carry, is_negative(byte_result)};
}

// byte_result - byte
constexpr byte_result operator-(const byte_result &a, std::byte b) {
  bool borrow = a.value < b;
  uint16_t result =
      std::to_integer<uint16_t>(a.value) - std::to_integer<uint16_t>(b);
  std::byte byte_result = std::byte(result);

  return {byte_result, borrow || a.carry, is_negative(byte_result)};
}

// byte - byte_result
constexpr byte_result operator-(std::byte a, const byte_result &b) {
  bool borrow = a < b.value;
  uint16_t result =
      std::to_integer<uint16_t>(a) - std::to_integer<uint16_t>(b.value);
  std::byte byte_result = std::byte(result);

  return {byte_result, borrow || b.carry, is_negative(byte_result)};
}

#endif