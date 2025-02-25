#ifndef _H_6502ISA
#define _H_6502ISA

#include "instruction_types.h"
#include <unordered_map>

using CPU6502ISA = std::unordered_map<size_t, Instruction>;

extern CPU6502ISA isa;

#endif
