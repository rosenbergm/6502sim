#ifndef _H_ADDRESS
#define _H_ADDRESS

#include <concepts>
#include <cstddef>
#include <cstdint>

class address {
private:
  uint16_t value_;

public:
  constexpr address() noexcept = default;
  constexpr address(const address &) noexcept = default;

  constexpr address(std::byte low, std::byte high) noexcept
      : value_(static_cast<uint16_t>(high) << 8 | static_cast<uint16_t>(low)) {}

  template <typename IntegerType>
    requires std::integral<IntegerType>
  constexpr explicit address(IntegerType value) noexcept
      : value_(static_cast<uint16_t>(value)) {}

  constexpr explicit address(std::byte value) noexcept
      : value_(static_cast<uint16_t>(value)) {}

  constexpr explicit operator std::size_t() const noexcept {
    return static_cast<std::size_t>(value_);
  }

  /** Implicit conversion to std::byte. This is UNSAFE, because it discards
   * the high byte of the address.
   */
  constexpr explicit operator std::byte() const noexcept {
    return static_cast<std::byte>(value_);
  }

  constexpr uint16_t inner() const noexcept { return value_; }

  constexpr std::byte low() const noexcept { return std::byte(value_ & 0xFF); }

  constexpr std::byte high() const noexcept {
    return std::byte((value_ >> 8) & 0xFF);
  }

  static constexpr bool is_carry(const uint32_t a) noexcept {
    return a > 0xFFFF;
  }
};

constexpr bool operator==(const address &a, const address &b) noexcept {
  return a.inner() == b.inner();
}

constexpr bool operator==(const address &a, const size_t &b) noexcept {
  return a.inner() == b;
}

struct address_result {
  address value;
  bool carry;

  constexpr address_result() = default;
  constexpr address_result(address value_, bool carry_)
      : value(value_), carry(carry_) {}
};

// address + address
constexpr address_result operator+(const address &a,
                                   const address &b) noexcept {
  uint32_t result = a.inner() + b.inner();

  return {address(result), address::is_carry(result)};
}

// address + byte
constexpr address_result operator+(const address &a,
                                   const std::byte &b) noexcept {
  uint32_t result = a.inner() + static_cast<uint16_t>(b);

  return {address(result), address::is_carry(result)};
}

// address + int
constexpr address_result operator+(const address &a,
                                   const uint16_t &b) noexcept {
  uint32_t result = a.inner() + b;

  return {address(result), address::is_carry(result)};
}

// address - int
constexpr address_result operator-(const address &a,
                                   const uint16_t &b) noexcept {
  bool borrow = a.inner() < b;
  uint32_t result = a.inner() - b;

  return {address(result), borrow};
}

#endif