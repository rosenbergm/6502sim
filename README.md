# 6502sim

A minimal 6502 processor simulator written in C++ as part of Programming in C++
course I took on MFF CUNI.

## About the 6502

The 6502 processor is an 8-bit processor used in many home computers in the
second half of the last century (including the Commodore 64, Apple II, and many
more). It features an 8 bit data line and 16 bits of address space (64K of
adressable memory).

The CPU is based on the accumulator-based architecture with support of twos-complement
negative number representation and binary coded decimals. It has one 8-bit accumulator
(A), two 8-bit index registers (X, Y) used for indexing into memory which is supported
by the addressing modes, stack pointer (SP), a program counter (PC) and a processor
status register (PSR).

The stack pointer is implemented as a 9-bit register with the most significant bit
always set, which makes the stack placed at a fixed memory address (from `0x100` to `0x1FF`).

The processor status register is an 8-bit register with the following flags:

- (N) Negative: set when a result of an operation is a negative number (MSb is set)
- (V) Overflow: set when a result of an operation overflows (eg. addition of two positive
  numbers yields a negative number )
- (1) constant one
- (B) BRK: set when the processor encountered a BRK instruction
- (D) Decimal: enables binary coded decimal for arithmetic operations
- (I) IRQB disable: when set high, no hardware interrupts are processed (not used
  in this project)
- (Z) Zero: set when a result of an operation is zero
- (C) Carry: set when a result of an arithmetic operation yields a carry bit on MSb

## Features

This simulator simulates the W65C02S, a backwards-compatible modernized version
of the 6502. The benefits of using this processor are mostly seen in hardware
applications, not in the emulation. The W65C02S is CMOS-based, which results
in better performance. It also adds a few new instructions, addressing modes
and fixes bugs.

Hereafter, "6502" refers to the W65C02S unless stated otherwise.

- Full instruction set support
- Full addressing mode support
- Debugger with basic inspection (registers, memory, stepping)
- Memory-mapped simple unbuffered print device

### Debugger

When the emulator is run with the `-d` (`--debug`) flag, debugging mode is enabled.
With the imported [`debuc.inc`](examples/includes/debug.inc) file in an assembly
file, the `DBGBREAK` macro can be used to enter the debugger.

In the debugger, these commands can be used:

- `d/dump` prints the contents of the registers.
- `s/step` advances the program counter and executes the following instruction.
- `c/continue` breaks out of the debugger.
- `g/get` can be used to inspect memory.
- `e/exit` quits the program.

### Print device

The emulator is capable of printing out ASCII characters. When a byte is stored
at `FFFB`, it is both stored in the and printed out to standard output.

Use the `PRINT` macro in [`print.inc`](examples/includes/print.inc).

## Usage

The recommended assembler is [VASM](http://sun.hasenbraten.de/vasm/) in "oldstyle 6502" mode.
You can also compile an assembly program from the root of the project by running `make <program>.bin`
if you have the source `<program>.s` file in project root as well.

```
6502sim <path to binary file> [-d|-v|--debug|--verbose]
  -d, --debug: enable debug mode
  -v, --verbose: enable verbose mode
```

## Useful links

- [W65C02S manual](manuals/w65c02s.pdf) (referred to as "the manual")
- [6502 user's manual](manuals/6502UsersManual.pdf)
- [6502 Assembly reference](http://www.6502.org/users/obelisk/6502/reference.html)

## Formal assignment (in Czech)

Rád bych vytvořil emulátor procesoru 6502 (konkrétně W65C02S). Podporoval by kompletní
instrukční sadu včetně všech režimů adresace. K dispozici by byla konfigurace (nejspíše
na velikost operační paměti a MMIO - pro začátek nejspíše jen výstupní, tedy tisk do terminálu).

Také bych rád implementoval jakési breakpoints - nejspíše vlastní instrukce, která uživatele
vhodí do REPL prostředí, aby mohl prozkoumat, co je momentálně v paměti, v registrech, apod.
