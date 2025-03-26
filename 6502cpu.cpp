#include "6502cpu.h"
#include "address.h"
#include "byte_utils.h"
#include "instruction_types.h"
#include "psr.h"

/**
 * Sets the ZERO (Z) and NEGATIVE (N) flags according to the value passed.
 *
 * @param value The value to update the flags with.
 */
void CPU6502::update_flags(std::byte value) {
  P.set_bit(psr_bit::zero, value == ZERO_BYTE);
  P.set_bit(psr_bit::negative, (value & MS_BIT_MASK) != ZERO_BYTE);
}

/**
 * Pop a value from the CPU stack and increment the stack pointer.
 *
 * There are no guarantees the stack pointer will not overflow.
 *
 * @return Value popped from the stack.
 */
std::byte CPU6502::pop_stack() {
  std::byte value = memory_->read(address(S));

  S = (S + 1).value;

  return value;
}

/**
 * Pushes a value to the CPU stack and decrements the stack pointer.
 *
 * There are not guarantees the stack pointer will not underflow.
 *
 * @param value The value to push to the stack.
 */
void CPU6502::push_stack(std::byte value) {
  S = (S - 1).value;

  memory_->write(address(S), value);
}

/**
 * @brief Executes the provided code in memory.
 *
 * The CPU will execute instructions until it reaches the end of the memory.
 *
 * @return When the CPU reaches the end of the memory.
 */
void CPU6502::execute() {
  while (PC.inner() < memory_->size()) {
    InstructionErr err = step();

    if (err == InstructionErr::Stop) {
      std::cout << std::endl << STP_MSG << std::endl;

      return;
    }
  }
}

/**
 * Makes one step of the CPU, equivallent to executing one instruction and
 * advances the program counter.
 *
 * @return InstructionErr The result of the executed instruction.
 */
InstructionErr CPU6502::step() {
  std::byte opcode = memory_->read(PC);

  auto instruction = isa_.find(static_cast<size_t>(opcode));

  if (instruction == isa_.end()) {
    std::cout << "Unknown opcode: " << std::hex << static_cast<int>(opcode)
              << std::endl;
    std::cout << "Exiting..." << std::endl;

    return InstructionErr::UnknownInstruction;
  }

  if (verbose_) {
    std::cout << "INSTRUCTION: " << instruction->second.name << " ("
              << instruction->first << ")" << std::endl
              << "  PC: " << std::hex << static_cast<int>(PC.inner())
              << std::endl;
  }

  address op_address;

  switch (instruction->second.mode) {
  case AddressingMode::Absolute: {
    std::byte low = memory_->read((PC + 1).value);
    std::byte high = memory_->read((PC + 2).value);

    op_address = address(low, high);

    break;
  }

  case AddressingMode::AbsoluteIndexedIndirect: {
    std::byte low = memory_->read((PC + 1).value);
    std::byte high = memory_->read((PC + 2).value);

    address indirect_address = address(low, high);

    std::byte low_indirect = memory_->read((indirect_address + X).value);
    std::byte high_indirect =
        memory_->read(((indirect_address + X).value + 1).value);

    op_address = address(low_indirect, high_indirect);

    break;
  }

  case AddressingMode::AbsoluteIndexedX: {
    std::byte low = memory_->read((PC + 1).value);
    std::byte high = memory_->read((PC + 2).value);

    op_address = (address(low, high) + X).value;

    break;
  }

  case AddressingMode::AbsoluteIndexedY: {
    std::byte low = memory_->read((PC + 1).value);
    std::byte high = memory_->read((PC + 2).value);

    op_address = (address(low, high) + Y).value;

    break;
  }

  case AddressingMode::AbsoluteIndirect: {
    std::byte low = memory_->read((PC + 1).value);
    std::byte high = memory_->read((PC + 2).value);

    size_t indirect_address =
        (static_cast<size_t>(high) << 8) | static_cast<size_t>(low);

    std::byte low_indirect = memory_->read(indirect_address);
    std::byte high_indirect = memory_->read(indirect_address + 1);

    op_address = address(low_indirect, high_indirect);

    break;
  }

  case AddressingMode::Accumulator: {
    op_address = address(A);

    break;
  }

  case AddressingMode::Immediate: {
    op_address = address((PC + 1).value);

    break;
  }

  case AddressingMode::Implied: {
    // used for instructions that manipulate PSR or instructions that have their
    // operand implied
    break;
  }

  case AddressingMode::PCRelative: {
    std::byte offset = memory_->read((PC + 1).value);

    int8_t signed_offset = static_cast<int8_t>(offset);
    op_address = address(((PC + 2).value + signed_offset).value);

    break;
  }

  case AddressingMode::Stack: {
    op_address = address(static_cast<size_t>(S));

    break;
  }

  case AddressingMode::ZeroPage: {
    std::byte low = memory_->read((PC + 1).value);
    op_address = address(low);

    break;
  }

  case AddressingMode::ZeroPageIndexedIndirect: {
    std::byte low = memory_->read((PC + 1).value);
    op_address = address((low + X).value);

    std::byte low_indirect = memory_->read(op_address);
    std::byte high_indirect = memory_->read((op_address + 1).value);

    op_address = address(low_indirect, high_indirect);

    break;
  }

  case AddressingMode::ZeroPageIndexedX: {
    std::byte low = memory_->read((PC + 1).value);
    op_address = address((low + X).value);

    break;
  }

  case AddressingMode::ZeroPageIndexedY: {
    std::byte low = memory_->read((PC + 1).value);
    op_address = address((low + Y).value);

    break;
  }

  case AddressingMode::ZeroPageIndirect: {
    std::byte low = memory_->read((PC + 1).value);

    std::byte low_indirect = memory_->read(address(low));
    std::byte high_indirect = memory_->read((address(low) + 1).value);

    op_address = address(low_indirect, high_indirect);

    break;
  }

  case AddressingMode::ZeroPageIndirectIndexedY: {
    std::byte low = memory_->read((PC + 1).value);

    std::byte low_indirect = memory_->read(address(low));
    std::byte high_indirect = memory_->read((address(low) + 1).value);

    op_address = (address(low_indirect, high_indirect) + Y).value;

    break;
  }

  default:
    // This should theoretically never happen. The assembler should produce the
    // correct machine code.
    throw CPUException("Addressing mode not implemented.");
  }

  InstructionErr ret_code = instruction->second.execute(*this, op_address);

  if (ret_code == InstructionErr::OKPCModified) {
    return ret_code;
  }

  PC = (PC + instruction->second.bytes).value;

  return ret_code;
}
