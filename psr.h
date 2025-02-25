#ifndef PSR_H
#define PSR_H

#include <functional>
#include <stddef.h>

/**
 * The initial value of the processor status register (P).
 *
 * On the real chip, no exact boot state is guaranteed. We set the (I) bit high
 * because we do not support hardware interrupts.
 */
constexpr unsigned char PSR_INITIAL_VALUE = 0b00100100;

/**
 * The bits of the processor status register (P) of the 6502.
 *
 * More information can be found in the manual, section 3.11.
 */
enum class psr_bit {
  carry, // lsb
  zero,
  interrupt_disable,
  decimal_mode,
  break_command,
  unused,
  overflow,
  negative // msb
};

/**
 * Class that represents the processor status register (P) of the CPU.
 */
class PSR {
private:
  std::byte psr_;

public:
  PSR() : psr_(std::byte(PSR_INITIAL_VALUE)) {}
  PSR(std::byte psr) : psr_(psr) {}

  bool get_bit(psr_bit bit) const;
  void set_bit(psr_bit bit, bool value);
  void update_bit(psr_bit bit, std::function<bool(bool)> update);

  std::byte get() const;
  void set(std::byte value);
};

#endif