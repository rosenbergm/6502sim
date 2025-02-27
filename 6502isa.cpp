#include "6502isa.h"
#include "6502cpu.h"
#include "address.h"
#include "byte_utils.h"
#include "instruction_types.h"
#include "psr.h"
#include <cstddef>
#include <cstdint>

namespace {
inline InstructionErr ADC(CPU6502 &cpu, address address) {
  std::byte operand = cpu.get_memory()->read(address);
  std::byte accumulator = cpu.get_A();

  auto result = accumulator + operand + cpu.get_PSR()->get_bit(psr_bit::carry);

  cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);

  bool operand_negative = is_negative(operand);
  bool accum_negative = is_negative(accumulator);
  bool result_negative = is_negative(result.value);

  bool overflow = (!operand_negative && !accum_negative && result_negative) ||
                  (operand_negative && accum_negative && !result_negative);

  cpu.get_PSR()->set_bit(psr_bit::overflow, overflow);

  cpu.set_A(result.value);

  cpu.update_flags(cpu.get_A());

  return InstructionErr::OK;
}

inline InstructionErr SBC(CPU6502 &cpu, address address) {
  std::byte value = cpu.get_memory()->read(address);
  bool borrow = !cpu.get_PSR()->get_bit(psr_bit::carry);

  uint8_t a = static_cast<uint8_t>(cpu.get_A());
  uint8_t m = static_cast<uint8_t>(value);

  uint16_t result = a - m - borrow;

  cpu.get_PSR()->set_bit(psr_bit::carry, a >= (m + borrow));
  std::byte result_byte = std::byte(result);

  bool overflow = ((cpu.get_A() ^ result_byte) & (cpu.get_A() ^ value) &
                   std::byte(0x80)) != std::byte(0);

  cpu.get_PSR()->set_bit(psr_bit::overflow, overflow);

  cpu.set_A(result_byte);
  cpu.update_flags(cpu.get_A());

  return InstructionErr::OK;
}

inline InstructionErr BBRx(CPU6502 &cpu, address address, int bit) {
  std::byte value = cpu.get_memory()->read(address);

  if (!is_bit_set(value, bit)) {
    cpu.set_PC(address);

    return InstructionErr::OKPCModified;
  }

  return InstructionErr::OK;
}

inline InstructionErr BBSx(CPU6502 &cpu, address address, int bit) {
  std::byte value = cpu.get_memory()->read(address);

  if (is_bit_set(value, 0)) {
    cpu.set_PC(address);

    return InstructionErr::OKPCModified;
  }

  return InstructionErr::OK;
}

inline InstructionErr RMBx(CPU6502 &cpu, address address, int bit) {
  std::byte value = cpu.get_memory()->read(address);

  cpu.get_memory()->write(address, value & ~std::byte(1 << bit));

  return InstructionErr::OK;
}

inline InstructionErr SMBx(CPU6502 &cpu, address address, int bit) {
  std::byte value = cpu.get_memory()->read(address);

  cpu.get_memory()->write(address, value | std::byte(1 << bit));

  return InstructionErr::OK;
}
} // namespace

CPU6502ISA isa = {
    {0x00, Instruction("BRK", AddressingMode::Stack,
                       [](CPU6502 &cpu, address _address) -> InstructionErr {
                         cpu.push_stack(cpu.get_PC().high());
                         cpu.push_stack(cpu.get_PC().low());
                         cpu.push_stack(cpu.get_PSR()->get());

                         cpu.set_PC(
                             address(cpu.get_memory()->read(address(0xFFFE)),
                                     cpu.get_memory()->read(address(0xFFFF))));

                         cpu.get_PSR()->set_bit(psr_bit::break_command, true);

                         return InstructionErr::OKPCModified;
                       })},
    {0x02, Instruction("DBG", AddressingMode::Implied,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         if (cpu.is_debug()) {
                           return InstructionErr::GoToDebugger;
                         }

                         return InstructionErr::OK;
                       })},
    {0x10, Instruction("BPL", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         if (!cpu.get_PSR()->get_bit(psr_bit::negative)) {
                           cpu.set_PC(address);

                           return InstructionErr::OKPCModified;
                         }

                         return InstructionErr::OK;
                       })},
    {0x20, Instruction("JSR", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         // save push the high part of PC, then low
                         cpu.push_stack(cpu.get_PC().high());
                         cpu.push_stack(cpu.get_PC().low());

                         cpu.set_PC(addr);

                         return InstructionErr::OKPCModified;
                       })},
    {0x30, Instruction("BMI", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         if (cpu.get_PSR()->get_bit(psr_bit::negative)) {
                           cpu.set_PC(address);

                           return InstructionErr::OKPCModified;
                         }

                         return InstructionErr::OK;
                       })},
    {0x40, Instruction("RTI", AddressingMode::Stack,
                       [](CPU6502 &cpu, address _address) -> InstructionErr {
                         cpu.set_PSR(cpu.pop_stack());

                         auto low = cpu.pop_stack();
                         auto high = cpu.pop_stack();
                         cpu.set_PC((address(low, high) + 1).value);

                         return InstructionErr::OKPCModified;
                       })},
    {0x50, Instruction("BVC", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         if (!cpu.get_PSR()->get_bit(psr_bit::overflow)) {
                           cpu.set_PC(address);

                           return InstructionErr::OKPCModified;
                         }

                         return InstructionErr::OK;
                       })},
    {0x60, Instruction("RTS", AddressingMode::Stack,
                       [](CPU6502 &cpu, address _address) -> InstructionErr {
                         auto low = cpu.pop_stack();
                         auto high = cpu.pop_stack();
                         cpu.set_PC((address(low, high) + 3).value);

                         return InstructionErr::OKPCModified;
                       })},
    {0x70, Instruction("BVS", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         if (cpu.get_PSR()->get_bit(psr_bit::overflow)) {
                           cpu.set_PC(address);

                           return InstructionErr::OKPCModified;
                         }

                         return InstructionErr::OK;
                       })},
    {0x80, Instruction("BRA", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_PC(address);

                         return InstructionErr::OKPCModified;
                       })},
    {0x90, Instruction("BCC", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         if (!cpu.get_PSR()->get_bit(psr_bit::carry)) {
                           cpu.set_PC(address);

                           return InstructionErr::OKPCModified;
                         }

                         return InstructionErr::OK;
                       })},
    {0xA0, Instruction("LDY", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_Y(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_Y());

                         return InstructionErr::OK;
                       })},
    {0xB0, Instruction("BCS", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         if (cpu.get_PSR()->get_bit(psr_bit::carry)) {
                           cpu.set_PC(address);

                           return InstructionErr::OKPCModified;
                         }

                         return InstructionErr::OK;
                       })},
    {0xC0, Instruction("CPY", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address addr_value) -> InstructionErr {
                         auto value = std::byte(addr_value);
                         auto result = cpu.get_Y() - value;

                         cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);
                         cpu.get_PSR()->set_bit(psr_bit::zero,
                                                result.value >= std::byte(0));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                result.negative);

                         return InstructionErr::OK;
                       })},
    {0xD0, Instruction("BNE", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         if (!cpu.get_PSR()->get_bit(psr_bit::zero)) {
                           cpu.set_PC(address);

                           return InstructionErr::OKPCModified;
                         }

                         return InstructionErr::OK;
                       })},
    {0xE0, Instruction("CPX", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address addr_value) -> InstructionErr {
                         auto value = std::byte(addr_value);
                         auto result = cpu.get_X() - value;

                         cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);
                         cpu.get_PSR()->set_bit(psr_bit::zero,
                                                result.value >= std::byte(0));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                result.negative);

                         return InstructionErr::OK;
                       })},
    {0xF0, Instruction("BEQ", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         if (cpu.get_PSR()->get_bit(psr_bit::zero)) {
                           cpu.set_PC(address);

                           return InstructionErr::OKPCModified;
                         }

                         return InstructionErr::OK;
                       })},
    {0x01, Instruction("ORA", AddressingMode::ZeroPageIndexedIndirect,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() |
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x11, Instruction("ORA", AddressingMode::ZeroPageIndirectIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() |
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x21, Instruction("AND", AddressingMode::ZeroPageIndexedIndirect,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() &
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x31, Instruction("AND", AddressingMode::ZeroPageIndirectIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() &
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x41, Instruction("EOR", AddressingMode::ZeroPageIndexedIndirect,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() ^
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x51, Instruction("EOR", AddressingMode::ZeroPageIndirectIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() ^
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x61, Instruction("ADC", AddressingMode::ZeroPageIndexedIndirect,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return ADC(cpu, address);
                       })},
    {0x71, Instruction("ADC", AddressingMode::ZeroPageIndirectIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return ADC(cpu, address);
                       })},
    {0x81, Instruction("STA", AddressingMode::ZeroPageIndexedIndirect,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto result =
                             cpu.get_A() + cpu.get_memory()->read(address);

                         // check for carry
                         if (result.carry) {
                           cpu.get_PSR()->set_bit(psr_bit::carry, true);
                         }

                         cpu.set_A(result.value);

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x91, Instruction("STA", AddressingMode::ZeroPageIndirectIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto result =
                             cpu.get_A() + cpu.get_memory()->read(address);

                         // check for carry
                         if (result.carry) {
                           cpu.get_PSR()->set_bit(psr_bit::carry, true);
                         }

                         cpu.set_A(result.value);

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xA0, Instruction("LDY", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_Y(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_Y());

                         return InstructionErr::OK;
                       })},
    {0xA1, Instruction("LDA", AddressingMode::ZeroPageIndexedIndirect,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xB1, Instruction("LDA", AddressingMode::ZeroPageIndirectIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xC1,
     Instruction("CMP", AddressingMode::ZeroPageIndexedIndirect,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto result = cpu.get_A() - cpu.get_memory()->read(address);

                   cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);
                   cpu.get_PSR()->set_bit(psr_bit::zero,
                                          result.value >= std::byte(0));
                   cpu.get_PSR()->set_bit(psr_bit::negative, result.negative);

                   return InstructionErr::OK;
                 })},
    {0xD1,
     Instruction("CMP", AddressingMode::ZeroPageIndirectIndexedY,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto result = cpu.get_A() - cpu.get_memory()->read(address);

                   cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);
                   cpu.get_PSR()->set_bit(psr_bit::zero,
                                          result.value >= std::byte(0));
                   cpu.get_PSR()->set_bit(psr_bit::negative, result.negative);

                   return InstructionErr::OK;
                 })},
    {0xE1, Instruction("SBC", AddressingMode::ZeroPageIndexedIndirect,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return SBC(cpu, address);
                       })},
    {0xF1, Instruction("SBC", AddressingMode::ZeroPageIndirectIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return SBC(cpu, address);
                       })},
    {0x12, Instruction("ORA", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() |
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x32, Instruction("AND", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() &
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x52, Instruction("EOR", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() ^
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x72, Instruction("ADC", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return ADC(cpu, address);
                       })},
    {0x92, Instruction("STA", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, cpu.get_A());
                         return InstructionErr::OK;
                       })},
    {0xA2, Instruction("LDX", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_X(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0xB2, Instruction("LDA", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xD2,
     Instruction("CMP", AddressingMode::ZeroPageIndirect,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto result = cpu.get_A() - cpu.get_memory()->read(address);

                   cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);
                   cpu.get_PSR()->set_bit(psr_bit::zero,
                                          result.value >= std::byte(0));
                   cpu.get_PSR()->set_bit(psr_bit::negative, result.negative);

                   return InstructionErr::OK;
                 })},
    {0xF2, Instruction("SBC", AddressingMode::ZeroPageIndirect,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return SBC(cpu, address);
                       })},
    {0x04,
     Instruction("TSB", AddressingMode::ZeroPage,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto mem_byte = cpu.get_memory()->read(address);
                   cpu.get_PSR()->set_bit(
                       psr_bit::zero, (mem_byte & cpu.get_A()) == std::byte(0));
                   cpu.get_memory()->write(address, mem_byte | cpu.get_A());

                   return InstructionErr::OK;
                 })},
    {0x14,
     Instruction("TRB", AddressingMode::ZeroPage,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto mem_byte = cpu.get_memory()->read(address);
                   cpu.get_PSR()->set_bit(
                       psr_bit::zero, (mem_byte & cpu.get_A()) == std::byte(0));
                   cpu.get_memory()->write(address, mem_byte & ~cpu.get_A());

                   return InstructionErr::OK;
                 })},
    {0x24, Instruction("BIT", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto mem_byte = cpu.get_memory()->read(address);

                         cpu.get_PSR()->set_bit(psr_bit::zero,
                                                is_zero(mem_byte));
                         cpu.get_PSR()->set_bit(psr_bit::overflow,
                                                is_bit_set(mem_byte, 6));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                is_negative(mem_byte));

                         return InstructionErr::OK;
                       })},
    {0x34, Instruction("BIT", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto mem_byte = cpu.get_memory()->read(address);

                         cpu.get_PSR()->set_bit(psr_bit::zero,
                                                is_zero(mem_byte));
                         cpu.get_PSR()->set_bit(psr_bit::overflow,
                                                is_bit_set(mem_byte, 6));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                is_negative(mem_byte));

                         return InstructionErr::OK;
                       })},
    {0x64, Instruction("STZ", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, std::byte(0));

                         return InstructionErr::OK;
                       })},
    {0x74, Instruction("STZ", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, std::byte(0));

                         return InstructionErr::OK;
                       })},
    {0x84, Instruction("STY", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, cpu.get_Y());

                         return InstructionErr::OK;
                       })},
    {0x94, Instruction("STY", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, cpu.get_Y());

                         return InstructionErr::OK;
                       })},
    {0xA4, Instruction("LDY", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_Y(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_Y());

                         return InstructionErr::OK;
                       })},
    {0xB4, Instruction("LDY", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_Y(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_Y());

                         return InstructionErr::OK;
                       })},
    {0xC4,
     Instruction("CPY", AddressingMode::ZeroPage,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto result = cpu.get_Y() - cpu.get_memory()->read(address);

                   cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);
                   cpu.get_PSR()->set_bit(psr_bit::zero,
                                          result.value >= std::byte(0));
                   cpu.get_PSR()->set_bit(psr_bit::negative, result.negative);

                   return InstructionErr::OK;
                 })},
    {0xE4,
     Instruction("CPX", AddressingMode::ZeroPage,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto result = cpu.get_X() - cpu.get_memory()->read(address);

                   cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);
                   cpu.get_PSR()->set_bit(psr_bit::zero,
                                          result.value >= std::byte(0));
                   cpu.get_PSR()->set_bit(psr_bit::negative, result.negative);

                   return InstructionErr::OK;
                 })},
    {0x05, Instruction("ORA", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() |
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x15, Instruction("ORA", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() |
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x25, Instruction("AND", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() &
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x35, Instruction("AND", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() &
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x45, Instruction("EOR", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() ^
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x55, Instruction("EOR", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() ^
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x65, Instruction("ADC", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return ADC(cpu, address);
                       })},
    {0x75, Instruction("ADC", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return ADC(cpu, address);
                       })},
    {0x85, Instruction("STA", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x95, Instruction("STA", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xA5, Instruction("LDA", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xB5, Instruction("LDA", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xC5,
     Instruction("CMP", AddressingMode::ZeroPage,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto result = cpu.get_A() - cpu.get_memory()->read(address);

                   cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);
                   cpu.get_PSR()->set_bit(psr_bit::zero,
                                          result.value >= std::byte(0));
                   cpu.get_PSR()->set_bit(psr_bit::negative, result.negative);

                   return InstructionErr::OK;
                 })},
    {0xD5,
     Instruction("CMP", AddressingMode::ZeroPageIndexedX,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto result = cpu.get_A() - cpu.get_memory()->read(address);

                   cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);
                   cpu.get_PSR()->set_bit(psr_bit::zero,
                                          result.value >= std::byte(0));
                   cpu.get_PSR()->set_bit(psr_bit::negative, result.negative);

                   return InstructionErr::OK;
                 })},
    {0xE5, Instruction("SBC", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return SBC(cpu, address);
                       })},
    {0xF5, Instruction("SBC", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return SBC(cpu, address);
                       })},
    {0x06, Instruction("ASL", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         cpu.get_PSR()->set_bit(psr_bit::carry, is_zero(value));

                         value = value << 1;

                         cpu.get_memory()->write(address, value);

                         return InstructionErr::OK;
                       })},
    {0x16, Instruction("ASL", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         cpu.get_PSR()->set_bit(psr_bit::carry, is_zero(value));

                         value = value << 1;

                         cpu.get_memory()->write(address, value);

                         return InstructionErr::OK;
                       })},
    {0x26, Instruction("ROL", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         auto carry_in =
                             cpu.get_PSR()->get_bit(psr_bit::carry) ? 1 : 0;
                         auto carry_out =
                             (value & std::byte(0x80)) != std::byte(0);

                         auto new_value = (value << 1) | std::byte(carry_in);
                         cpu.get_memory()->write(address, new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry, carry_out);

                         return InstructionErr::OK;
                       })},
    {0x36, Instruction("ROL", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         auto carry_in =
                             cpu.get_PSR()->get_bit(psr_bit::carry) ? 1 : 0;
                         auto carry_out =
                             (value & std::byte(0x80)) != std::byte(0);

                         auto new_value = (value << 1) | std::byte(carry_in);
                         cpu.get_memory()->write(address, new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry, carry_out);

                         return InstructionErr::OK;
                       })},
    {0x46, Instruction("LSR", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         auto carry_out = value & std::byte(0x01);

                         auto new_value = (value >> 1);
                         cpu.get_memory()->write(address, new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry,
                                                carry_out == std::byte(1));
                         cpu.get_PSR()->set_bit(psr_bit::negative, 0);

                         return InstructionErr::OK;
                       })},
    {0x56, Instruction("LSR", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         auto carry_out = value & std::byte(0x01);

                         auto new_value = (value >> 1);
                         cpu.get_memory()->write(address, new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry,
                                                carry_out == std::byte(1));
                         cpu.get_PSR()->set_bit(psr_bit::negative, 0);

                         return InstructionErr::OK;
                       })},
    {0x66, Instruction("ROR", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         auto carry_in =
                             cpu.get_PSR()->get_bit(psr_bit::carry) ? 1 : 0;
                         auto carry_out =
                             (value & std::byte(0x80)) != std::byte(0);

                         auto new_value = (value >> 1) | std::byte(carry_in);
                         cpu.get_memory()->write(address, new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry, carry_out);

                         return InstructionErr::OK;
                       })},
    {0x76, Instruction("ROR", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         auto carry_in =
                             cpu.get_PSR()->get_bit(psr_bit::carry) ? 1 : 0;
                         auto carry_out =
                             (value & std::byte(0x80)) != std::byte(0);

                         auto new_value = (value >> 1) | std::byte(carry_in);
                         cpu.get_memory()->write(address, new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry, carry_out);

                         return InstructionErr::OK;
                       })},
    {0x86, Instruction("STX", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0x96, Instruction("STX", AddressingMode::ZeroPageIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0xA6, Instruction("LDX", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_X(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0xB6, Instruction("LDX", AddressingMode::ZeroPageIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_X(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0xC6, Instruction("DEC", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         std::byte value = cpu.get_memory()->read(address);
                         value = (value - 1).value;

                         cpu.get_memory()->write(address, value);

                         cpu.get_PSR()->set_bit(psr_bit::zero, is_zero(value));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                is_negative(value));

                         return InstructionErr::OK;
                       })},
    {0xD6, Instruction("DEC", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         std::byte value = cpu.get_memory()->read(address);
                         value = (value - 1).value;

                         cpu.get_memory()->write(address, value);

                         cpu.get_PSR()->set_bit(psr_bit::zero, is_zero(value));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                is_negative(value));

                         return InstructionErr::OK;
                       })},
    {0xE6, Instruction("INC", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         std::byte value = cpu.get_memory()->read(address);
                         value = (value + 1).value;

                         cpu.get_memory()->write(address, value);

                         cpu.get_PSR()->set_bit(psr_bit::zero, is_zero(value));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                is_negative(value));

                         return InstructionErr::OK;
                       })},
    {0xF6, Instruction("INC", AddressingMode::ZeroPageIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         std::byte value = cpu.get_memory()->read(address);
                         value = (value + 1).value;

                         cpu.get_memory()->write(address, value);

                         cpu.get_PSR()->set_bit(psr_bit::zero, is_zero(value));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                is_negative(value));

                         return InstructionErr::OK;
                       })},
    {0x07, Instruction("RMB0", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return RMBx(cpu, addr, 0);
                       })},
    {0x17, Instruction("RMB1", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return RMBx(cpu, addr, 1);
                       })},
    {0x27, Instruction("RMB2", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return RMBx(cpu, addr, 2);
                       })},
    {0x37, Instruction("RMB3", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return RMBx(cpu, addr, 3);
                       })},
    {0x47, Instruction("RMB4", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return RMBx(cpu, addr, 4);
                       })},
    {0x57, Instruction("RMB5", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return RMBx(cpu, addr, 5);
                       })},
    {0x67, Instruction("RMB6", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return RMBx(cpu, addr, 6);
                       })},
    {0x77, Instruction("RMB7", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return RMBx(cpu, addr, 7);
                       })},
    {0x87, Instruction("SMB0", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return SMBx(cpu, addr, 0);
                       })},
    {0x97, Instruction("SMB1", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return SMBx(cpu, addr, 1);
                       })},
    {0xA7, Instruction("SMB2", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return SMBx(cpu, addr, 2);
                       })},
    {0xB7, Instruction("SMB3", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return SMBx(cpu, addr, 3);
                       })},
    {0xC7, Instruction("SMB4", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return SMBx(cpu, addr, 4);
                       })},
    {0xD7, Instruction("SMB5", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return SMBx(cpu, addr, 5);
                       })},
    {0xE7, Instruction("SMB6", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return SMBx(cpu, addr, 6);
                       })},
    {0xF7, Instruction("SMB7", AddressingMode::ZeroPage,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return SMBx(cpu, addr, 7);
                       })},
    {0x08, Instruction("PHP", AddressingMode::Stack,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         PSR psr = cpu.copy_PSR();
                         psr.set_bit(psr_bit::break_command, true);

                         cpu.push_stack(std::byte(psr.get()));

                         return InstructionErr::OK;
                       })},
    {0x18, Instruction("CLC", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.get_PSR()->set_bit(psr_bit::carry, false);

                         return InstructionErr::OK;
                       })},
    {0x28, Instruction("PLP", AddressingMode::Stack,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         std::byte value = cpu.pop_stack();
                         cpu.set_PSR(PSR(value));

                         return InstructionErr::OK;
                       })},
    {0x38, Instruction("SEC", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.get_PSR()->set_bit(psr_bit::carry, true);

                         return InstructionErr::OK;
                       })},
    {0x48, Instruction("PHA", AddressingMode::Stack,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.push_stack(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x58, Instruction("CLI", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.get_PSR()->set_bit(psr_bit::interrupt_disable,
                                                false);

                         return InstructionErr::OK;
                       })},
    {0x68, Instruction("PLA", AddressingMode::Stack,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.set_A(cpu.pop_stack());

                         return InstructionErr::OK;
                       })},
    {0x78, Instruction("CLI", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.get_PSR()->set_bit(psr_bit::interrupt_disable,
                                                true);

                         return InstructionErr::OK;
                       })},
    {0x88, Instruction("DEY", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         auto result = cpu.get_Y() - 1;

                         if (result.carry) {
                           cpu.get_PSR()->set_bit(psr_bit::carry, true);
                         }

                         cpu.set_Y(result.value);

                         cpu.update_flags(cpu.get_Y());

                         return InstructionErr::OK;
                       })},
    {0x98, Instruction("TYA", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.set_A(cpu.get_Y());

                         return InstructionErr::OK;
                       })},
    {0xA8, Instruction("TAY", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.set_Y(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xB8, Instruction("CLV", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.get_PSR()->set_bit(psr_bit::overflow, false);

                         return InstructionErr::OK;
                       })},
    {0xC8, Instruction("INY", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         auto result = cpu.get_Y() + 1;

                         if (result.carry) {
                           cpu.get_PSR()->set_bit(psr_bit::carry, true);
                         }

                         cpu.set_Y(result.value);

                         cpu.update_flags(cpu.get_Y());

                         return InstructionErr::OK;
                       })},
    {0xD8, Instruction("CLD", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.get_PSR()->set_bit(psr_bit::decimal_mode, false);

                         return InstructionErr::OK;
                       })},
    {0xE8, Instruction("INX", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         auto result = cpu.get_X() + 1;

                         if (result.carry) {
                           cpu.get_PSR()->set_bit(psr_bit::carry, true);
                         }

                         cpu.set_X(result.value);

                         cpu.update_flags(cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0xF8, Instruction("SED", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.get_PSR()->set_bit(psr_bit::decimal_mode, true);

                         return InstructionErr::OK;
                       })},
    {0x09, Instruction("ORA", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() |
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x19, Instruction("ORA", AddressingMode::AbsoluteIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() |
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x29, Instruction("AND", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() &
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x39, Instruction("AND", AddressingMode::AbsoluteIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() &
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x49, Instruction("EOR", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() ^
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x59, Instruction("EOR", AddressingMode::AbsoluteIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() ^
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x69, Instruction("ADC", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return ADC(cpu, address);
                       })},
    {0x79, Instruction("ADC", AddressingMode::AbsoluteIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return ADC(cpu, address);
                       })},
    {0x89, Instruction("BIT", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto result =
                             cpu.get_A() & cpu.get_memory()->read(address);

                         cpu.update_flags(result);

                         return InstructionErr::OK;
                       })},
    {0x99, Instruction("STA", AddressingMode::AbsoluteIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xA9, Instruction("LDA", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xB9, Instruction("LDA", AddressingMode::AbsoluteIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xB9, Instruction("LDA", AddressingMode::AbsoluteIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xC9, Instruction("CMP", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto result =
                             cpu.get_A() - cpu.get_memory()->read(address);

                         cpu.update_flags(result.value);
                         cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);

                         return InstructionErr::OK;
                       })},
    {0xD9, Instruction("CMP", AddressingMode::AbsoluteIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto result =
                             cpu.get_A() - cpu.get_memory()->read(address);

                         cpu.update_flags(result.value);
                         cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);

                         return InstructionErr::OK;
                       })},
    {0xE9, Instruction("SBC", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return SBC(cpu, address);
                       })},
    {0xF9, Instruction("SBC", AddressingMode::AbsoluteIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return SBC(cpu, address);
                       })},
    {0x0A, Instruction("ASL", AddressingMode::Accumulator,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_A();

                         cpu.get_PSR()->set_bit(psr_bit::carry, is_zero(value));

                         value = value << 1;

                         cpu.get_memory()->write(address, value);

                         return InstructionErr::OK;
                       })},
    {0x1A, Instruction("INC", AddressingMode::Accumulator,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         std::byte value = cpu.get_A();
                         value = (value + 1).value;

                         cpu.set_A(value);

                         cpu.get_PSR()->set_bit(psr_bit::zero, is_zero(value));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                is_negative(value));

                         return InstructionErr::OK;
                       })},
    {0x2A, Instruction("ROL", AddressingMode::Accumulator,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_A();

                         auto carry_in =
                             cpu.get_PSR()->get_bit(psr_bit::carry) ? 1 : 0;
                         auto carry_out =
                             (value & std::byte(0x80)) != std::byte(0);

                         auto new_value = (value << 1) | std::byte(carry_in);
                         cpu.set_A(new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry, carry_out);

                         return InstructionErr::OK;
                       })},
    {0x3A, Instruction("DEC", AddressingMode::Accumulator,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         std::byte value = cpu.get_A();
                         value = (value - 1).value;

                         cpu.set_A(value);

                         cpu.get_PSR()->set_bit(psr_bit::zero, is_zero(value));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                is_negative(value));

                         return InstructionErr::OK;
                       })},
    {0x4A, Instruction("LSR", AddressingMode::Accumulator,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_A();

                         auto carry_out = value & std::byte(0x01);

                         auto new_value = (value >> 1);
                         cpu.set_A(new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry,
                                                carry_out == std::byte(1));
                         cpu.get_PSR()->set_bit(psr_bit::negative, 0);

                         return InstructionErr::OK;
                       })},
    {0x5A, Instruction("PHY", AddressingMode::Stack,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.push_stack(cpu.get_Y());

                         return InstructionErr::OK;
                       })},
    {0x6A, Instruction("ROR", AddressingMode::Accumulator,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_A();

                         auto carry_in =
                             cpu.get_PSR()->get_bit(psr_bit::carry) ? 1 : 0;
                         auto carry_out =
                             (value & std::byte(0x80)) != std::byte(0);

                         auto new_value = (value >> 1) | std::byte(carry_in);
                         cpu.set_A(new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry, carry_out);

                         return InstructionErr::OK;
                       })},
    {0x7A, Instruction("PLY", AddressingMode::Stack,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.set_Y(cpu.pop_stack());

                         return InstructionErr::OK;
                       })},
    {0x8A, Instruction("TXA", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.set_A(cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0x9A, Instruction("TXS", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.set_S(cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0xAA, Instruction("TAX", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.set_X(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xBA, Instruction("TSX", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.set_X(cpu.get_S());

                         return InstructionErr::OK;
                       })},
    {0xCA, Instruction("DEX", AddressingMode::Implied,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         std::byte value = cpu.get_X();
                         value = (value - 1).value;

                         cpu.set_X(value);

                         cpu.get_PSR()->set_bit(psr_bit::zero, is_zero(value));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                is_negative(value));

                         return InstructionErr::OK;
                       })},
    {0xDA, Instruction("PHX", AddressingMode::Stack,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.push_stack(cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0xEA, Instruction("NOP", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return InstructionErr::OK;
                       })},
    {0xFA, Instruction("PLX", AddressingMode::Stack,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         cpu.set_X(cpu.pop_stack());

                         return InstructionErr::OK;
                       })},
    {0xDB, Instruction("STP", AddressingMode::Implied,
                       [](CPU6502 &cpu, address addr) -> InstructionErr {
                         return InstructionErr::Stop;
                       })},
    {0x0C,
     Instruction("TSB", AddressingMode::Absolute,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto mem_byte = cpu.get_memory()->read(address);
                   cpu.get_PSR()->set_bit(
                       psr_bit::zero, (mem_byte & cpu.get_A()) == std::byte(0));
                   cpu.get_memory()->write(address, mem_byte | cpu.get_A());

                   return InstructionErr::OK;
                 })},
    {0x1C,
     Instruction("TSB", AddressingMode::Absolute,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto mem_byte = cpu.get_memory()->read(address);
                   cpu.get_PSR()->set_bit(
                       psr_bit::zero, (mem_byte & cpu.get_A()) == std::byte(0));
                   cpu.get_memory()->write(address, mem_byte & ~cpu.get_A());

                   return InstructionErr::OK;
                 })},
    {0x2C, Instruction("BIT", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto result =
                             cpu.get_A() & cpu.get_memory()->read(address);

                         cpu.update_flags(result);

                         return InstructionErr::OK;
                       })},
    {0x3C, Instruction("BIT", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto result =
                             cpu.get_A() & cpu.get_memory()->read(address);

                         cpu.update_flags(result);

                         return InstructionErr::OK;
                       })},
    {0x4C, Instruction("JMP", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_PC(address);

                         return InstructionErr::OKPCModified;
                       })},
    {0x6C, Instruction("JMP", AddressingMode::AbsoluteIndirect,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_PC(address);

                         return InstructionErr::OKPCModified;
                       })},
    {0x7C, Instruction("JMP", AddressingMode::AbsoluteIndexedIndirect,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_PC(address);

                         return InstructionErr::OKPCModified;
                       })},
    {0x8C, Instruction("STY", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, cpu.get_Y());

                         return InstructionErr::OK;
                       })},
    {0x9C, Instruction("STZ", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, std::byte(0));

                         return InstructionErr::OK;
                       })},
    {0xAC, Instruction("LDY", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_Y(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_Y());

                         return InstructionErr::OK;
                       })},
    {0xBC, Instruction("LDY", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_Y(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_Y());

                         return InstructionErr::OK;
                       })},
    {0xCC,
     Instruction("CPY", AddressingMode::Absolute,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto result = cpu.get_Y() - cpu.get_memory()->read(address);

                   cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);
                   cpu.get_PSR()->set_bit(psr_bit::zero,
                                          result.value >= std::byte(0));
                   cpu.get_PSR()->set_bit(psr_bit::negative, result.negative);

                   return InstructionErr::OK;
                 })},
    {0xEC,
     Instruction("CPX", AddressingMode::Absolute,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto result = cpu.get_X() - cpu.get_memory()->read(address);

                   cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);
                   cpu.get_PSR()->set_bit(psr_bit::zero,
                                          result.value >= std::byte(0));
                   cpu.get_PSR()->set_bit(psr_bit::negative, result.negative);

                   return InstructionErr::OK;
                 })},
    {0x0D, Instruction("ORA", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() |
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x1D, Instruction("ORA", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() |
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x2D, Instruction("AND", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() &
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x3D, Instruction("AND", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() &
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x4D, Instruction("EOR", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() ^
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x5D, Instruction("EOR", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_A() ^
                                   cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x6D, Instruction("ADC", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return ADC(cpu, address);
                       })},
    {0x7D, Instruction("ADC", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return ADC(cpu, address);
                       })},
    {0x8D, Instruction("STA", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x9D, Instruction("STA", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xAD, Instruction("LDA", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_memory()->read(address));

                         return InstructionErr::OK;
                       })},
    {0xBD, Instruction("LDA", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_memory()->read(address));

                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xCD,
     Instruction("CMP", AddressingMode::Absolute,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto result = cpu.get_A() - cpu.get_memory()->read(address);

                   cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);
                   cpu.get_PSR()->set_bit(psr_bit::zero,
                                          result.value >= std::byte(0));
                   cpu.get_PSR()->set_bit(psr_bit::negative, result.negative);

                   return InstructionErr::OK;
                 })},
    {0xDD,
     Instruction("CMP", AddressingMode::AbsoluteIndexedX,
                 [](CPU6502 &cpu, address address) -> InstructionErr {
                   auto result = cpu.get_A() - cpu.get_memory()->read(address);

                   cpu.get_PSR()->set_bit(psr_bit::carry, result.carry);
                   cpu.get_PSR()->set_bit(psr_bit::zero,
                                          result.value >= std::byte(0));
                   cpu.get_PSR()->set_bit(psr_bit::negative, result.negative);

                   return InstructionErr::OK;
                 })},
    {0xED, Instruction("SBC", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return SBC(cpu, address);
                       })},
    {0xFD, Instruction("SBC", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return SBC(cpu, address);
                       })},
    {0x0E, Instruction("ASL", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         cpu.get_PSR()->set_bit(psr_bit::carry, is_zero(value));

                         value = value << 1;

                         cpu.get_memory()->write(address, value);

                         return InstructionErr::OK;
                       })},
    {0x1E, Instruction("ASL", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         cpu.get_PSR()->set_bit(psr_bit::carry, is_zero(value));

                         value = value << 1;

                         cpu.get_memory()->write(address, value);

                         return InstructionErr::OK;
                       })},
    {0x2E, Instruction("ROL", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         auto carry_in =
                             cpu.get_PSR()->get_bit(psr_bit::carry) ? 1 : 0;
                         auto carry_out =
                             (value & std::byte(0x80)) != std::byte(0);

                         auto new_value = (value << 1) | std::byte(carry_in);
                         cpu.get_memory()->write(address, new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry, carry_out);

                         return InstructionErr::OK;
                       })},
    {0x3E, Instruction("ROL", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         auto carry_in =
                             cpu.get_PSR()->get_bit(psr_bit::carry) ? 1 : 0;
                         auto carry_out =
                             (value & std::byte(0x80)) != std::byte(0);

                         auto new_value = (value << 1) | std::byte(carry_in);
                         cpu.get_memory()->write(address, new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry, carry_out);

                         return InstructionErr::OK;
                       })},
    {0x4E, Instruction("LSR", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         auto carry_out = value & std::byte(0x01);

                         auto new_value = (value >> 1);
                         cpu.get_memory()->write(address, new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry,
                                                carry_out == std::byte(1));
                         cpu.get_PSR()->set_bit(psr_bit::negative, 0);

                         return InstructionErr::OK;
                       })},
    {0x5E, Instruction("LSR", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         auto carry_out = value & std::byte(0x01);

                         auto new_value = (value >> 1);
                         cpu.get_memory()->write(address, new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry,
                                                carry_out == std::byte(1));
                         cpu.get_PSR()->set_bit(psr_bit::negative, 0);

                         return InstructionErr::OK;
                       })},
    {0x6E, Instruction("ROR", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         auto carry_in =
                             cpu.get_PSR()->get_bit(psr_bit::carry) ? 1 : 0;
                         auto carry_out =
                             (value & std::byte(0x80)) != std::byte(0);

                         auto new_value = (value >> 1) | std::byte(carry_in);
                         cpu.get_memory()->write(address, new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry, carry_out);

                         return InstructionErr::OK;
                       })},
    {0x7E, Instruction("ROR", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         auto value = cpu.get_memory()->read(address);

                         auto carry_in =
                             cpu.get_PSR()->get_bit(psr_bit::carry) ? 1 : 0;
                         auto carry_out =
                             (value & std::byte(0x80)) != std::byte(0);

                         auto new_value = (value >> 1) | std::byte(carry_in);
                         cpu.get_memory()->write(address, new_value);

                         cpu.get_PSR()->set_bit(psr_bit::carry, carry_out);

                         return InstructionErr::OK;
                       })},
    {0x8E, Instruction("STX", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0x9E, Instruction("STZ", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, std::byte(0));

                         return InstructionErr::OK;
                       })},
    {0xAE, Instruction("LDX", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_X(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0xBE, Instruction("LDX", AddressingMode::AbsoluteIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_X(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0xCE, Instruction("DEC", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         std::byte value = cpu.get_memory()->read(address);
                         value = (value - 1).value;

                         cpu.get_memory()->write(address, value);

                         cpu.get_PSR()->set_bit(psr_bit::zero, is_zero(value));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                is_negative(value));

                         return InstructionErr::OK;
                       })},
    {0xDE, Instruction("DEC", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         std::byte value = cpu.get_memory()->read(address);
                         value = (value - 1).value;

                         cpu.get_memory()->write(address, value);

                         cpu.get_PSR()->set_bit(psr_bit::zero, is_zero(value));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                is_negative(value));

                         return InstructionErr::OK;
                       })},
    {0xEE, Instruction("INC", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         std::byte value = cpu.get_memory()->read(address);
                         value = (value + 1).value;

                         cpu.get_memory()->write(address, value);

                         cpu.get_PSR()->set_bit(psr_bit::zero, is_zero(value));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                is_negative(value));

                         return InstructionErr::OK;
                       })},
    {0xFE, Instruction("INC", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         std::byte value = cpu.get_memory()->read(address);
                         value = (value + 1).value;

                         cpu.get_memory()->write(address, value);

                         cpu.get_PSR()->set_bit(psr_bit::zero, is_zero(value));
                         cpu.get_PSR()->set_bit(psr_bit::negative,
                                                is_negative(value));

                         return InstructionErr::OK;
                       })},
    {0x0F, Instruction("BBR0", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBRx(cpu, address, 0);
                       })},
    {0x1F, Instruction("BBR1", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBRx(cpu, address, 1);
                       })},
    {0x2F, Instruction("BBR2", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBRx(cpu, address, 2);
                       })},
    {0x3F, Instruction("BBR3", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBRx(cpu, address, 3);
                       })},
    {0x4F, Instruction("BBR4", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBRx(cpu, address, 4);
                       })},
    {0x5F, Instruction("BBR5", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBRx(cpu, address, 5);
                       })},
    {0x6F, Instruction("BBR6", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBRx(cpu, address, 6);
                       })},
    {0x7F, Instruction("BBR7", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBRx(cpu, address, 7);
                       })},
    {0x8F, Instruction("BBS0", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBSx(cpu, address, 0);
                       })},
    {0x9F, Instruction("BBS1", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBSx(cpu, address, 1);
                       })},
    {0xAF, Instruction("BBS2", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBSx(cpu, address, 2);
                       })},
    {0xBF, Instruction("BBS3", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBSx(cpu, address, 3);
                       })},
    {0xCF, Instruction("BBS4", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBSx(cpu, address, 4);
                       })},
    {0xDF, Instruction("BBS5", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBSx(cpu, address, 5);
                       })},
    {0xEF, Instruction("BBS6", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBSx(cpu, address, 6);
                       })},
    {0xFF, Instruction("BBS7", AddressingMode::PCRelative,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         return BBSx(cpu, address, 7);
                       })},
    {0xA9, Instruction("LDA", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xBD, Instruction("LDA", AddressingMode::AbsoluteIndexedX,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0xA2, Instruction("LDX", AddressingMode::Immediate,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_X(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0xB9, Instruction("LDA", AddressingMode::AbsoluteIndexedY,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_A(cpu.get_memory()->read(address));
                         cpu.update_flags(cpu.get_A());

                         return InstructionErr::OK;
                       })},
    {0x8D, Instruction("STA", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.get_memory()->write(address, cpu.get_A());
                         return InstructionErr::OK;
                       })},
    {0xE8, Instruction("INX", AddressingMode::Implied,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_X(
                             std::byte(static_cast<int>(cpu.get_X()) + 1));
                         cpu.update_flags(cpu.get_X());

                         return InstructionErr::OK;
                       })},
    {0x4C, Instruction("JMP", AddressingMode::Absolute,
                       [](CPU6502 &cpu, address address) -> InstructionErr {
                         cpu.set_PC(address);

                         return InstructionErr::OKPCModified;
                       })}};