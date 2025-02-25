#include "psr.h"

bool PSR::get_bit(psr_bit bit) const {
  if (bit == psr_bit::unused) {
    return true;
  }

  return (psr_ & std::byte(1 << static_cast<size_t>(bit))) != std::byte(0);
}

void PSR::set_bit(psr_bit bit, bool value) {
  if (bit == psr_bit::unused) {
    return;
  }

  if (value) {
    psr_ |= std::byte(1 << static_cast<size_t>(bit));
  } else {
    psr_ &= std::byte(~(1 << static_cast<size_t>(bit)));
  }
}

void PSR::update_bit(psr_bit bit, std::function<bool(bool)> update) {
  if (bit == psr_bit::unused) {
    return;
  }

  bool current = get_bit(bit);
  set_bit(bit, update(current));
}

std::byte PSR::get() const { return psr_; }

void PSR::set(std::byte value) {
  psr_ = value | std::byte(1 << static_cast<size_t>(psr_bit::unused));
}