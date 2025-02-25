#ifndef _H_6502CPU
#define _H_6502CPU

#include "6502isa.h"
#include "address.h"
#include "byte_utils.h"
#include "gp_memory.h"
#include "instruction_types.h"
#include "psr.h"

#include <iostream>

constexpr size_t MAX_MEMORY = 0x10000;
constexpr unsigned short RESET_VECTOR_LOW = 0xFFFC;
constexpr unsigned short RESET_VECTOR_HIGH = 0xFFFD;

constexpr size_t STACK_START = 0x1FF;

constexpr const char *STP_MSG = "== ENCOUNTERED STP, terminating... ==";

class CPUException {
private:
  const char *message_;

public:
  CPUException(const char *message) : message_(message) {}

  const char *message() const { return message_; }
};

class CPU6502 {
private:
  // registers
  std::byte A, X, Y, S;
  PSR P;
  address PC;

  // general purpose memory
  GP_Memory *memory_;

  CPU6502ISA isa_ = isa;

  bool debug_ = false;
  bool verbose_ = false;

public:
  CPU6502(GP_Memory *memory, bool debug = false)
      : A(), X(), Y(), S(std::byte(STACK_START)), P(), PC(), memory_(memory),
        debug_(debug) {
    if (memory_ == nullptr) {
      throw CPUException("Memory cannot be null.");
    }

    // a warning is perfectly fine here. the simulator is just
    // going to ignore it because of the 16-bit address space.
    if (memory_->size() > MAX_MEMORY) {
      std::cout
          << "Warning: Memory size is over the addressable limit of the CPU."
          << std::endl;
    }

    // reset sequence:
    // 1. read the reset vector from FFFC and FFFD
    std::byte low = memory_->read(RESET_VECTOR_LOW);
    std::byte high = memory_->read(RESET_VECTOR_HIGH);

    if ((low == std::byte(0xFF) && high == std::byte(0xFF)) ||
        (low == std::byte(0x00) && high == std::byte(0x00))) {
      std::cout << "Warning: Reset vector appears not to be set." << std::endl;
    }

    // 2. set the program counter to the address in the reset vector
    PC = address(low, high);
  }

  std::byte get_A() const { return A; };
  void set_A(std::byte value) { A = value; };
  std::byte get_X() const { return X; };
  void set_X(std::byte value) { X = value; };
  std::byte get_Y() const { return Y; };
  void set_Y(std::byte value) { Y = value; };
  std::byte get_S() const { return S; };
  void set_S(std::byte value) { S = value; };
  address get_PC() const { return PC; };
  void set_PC(address value) { PC = value; };

  void set_PSR(PSR value) { P = value; };
  const PSR *get_PSR() const { return &P; };
  PSR *get_PSR() { return &P; };
  PSR copy_PSR() { return P; };

  const GP_Memory *get_memory() const { return memory_; };
  GP_Memory *get_memory() { return memory_; };

  void update_flags(std::byte value);

  std::byte pop_stack();
  void push_stack(std::byte value);

  void execute();
  InstructionErr step();

  void set_debug(bool value) { debug_ = value; };
  bool is_debug() { return debug_; };

  void set_verbose(bool value) { verbose_ = value; };
  bool is_verbose() { return verbose_; };
};

#endif