#ifndef _H_INSTRUCTION_TYPES
#define _H_INSTRUCTION_TYPES

#include "address.h"
#include <functional>
#include <string>

class CPU6502;

enum class AddressingMode {
  Absolute,
  AbsoluteIndexedIndirect,
  AbsoluteIndexedX,
  AbsoluteIndexedY,
  AbsoluteIndirect,
  Accumulator,
  Immediate,
  Implied,
  PCRelative,
  Stack,
  ZeroPage,
  ZeroPageIndexedIndirect,
  ZeroPageIndexedX,
  ZeroPageIndexedY,
  ZeroPageIndirect,
  ZeroPageIndirectIndexedY
};

static size_t bytes_for_addressing_mode(AddressingMode mode) {
  switch (mode) {
  case AddressingMode::Absolute:
  case AddressingMode::AbsoluteIndexedX:
  case AddressingMode::AbsoluteIndexedY:
  case AddressingMode::AbsoluteIndirect:
    return 3;
  case AddressingMode::Immediate:
  case AddressingMode::PCRelative:
  case AddressingMode::ZeroPage:
  case AddressingMode::ZeroPageIndexedIndirect:
  case AddressingMode::ZeroPageIndexedX:
  case AddressingMode::ZeroPageIndexedY:
  case AddressingMode::ZeroPageIndirect:
  case AddressingMode::ZeroPageIndirectIndexedY:
    return 2;
  case AddressingMode::AbsoluteIndexedIndirect:
  case AddressingMode::Accumulator:
  case AddressingMode::Implied:
  case AddressingMode::Stack:
    return 1;
  default:
    return 0;
  }
};

enum class InstructionErr {
  OK,
  OKPCModified,
  SIRaised,
  UnknownInstruction,
  GoToDebugger,
  Stop
};

struct Instruction {
  std::string name;
  AddressingMode mode;
  size_t bytes;
  std::function<InstructionErr(CPU6502 &, address)> execute;

  Instruction(std::string name_, AddressingMode mode_,
              std::function<InstructionErr(CPU6502 &, address)> execute_) {
    name = name_;
    mode = mode_;
    bytes = bytes_for_addressing_mode(mode);
    execute = execute_;
  };
};

#endif
