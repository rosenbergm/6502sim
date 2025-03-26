#include "psr.h"

/**
 * Get the value of a bit in the PSR.
 *
 * @param bit The bit to get.
 * @return bool Is the bit set?
 */
bool PSR::get_bit(psr_bit bit) const {
  if (bit == psr_bit::unused) {
    return true;
  }

  return (psr_ & std::byte(1 << static_cast<size_t>(bit))) != std::byte(0);
}

/**
 * Set the value of a bit in the PSR.
 *
 * @param bit The bit to set.
 * @param value The value to set the bit to.
 */
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

/**
 * Update the value of a bit in the PSR through a mapper.
 *
 * @param bit The bit to update.
 * @param update The function to update the bit.
 */
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
