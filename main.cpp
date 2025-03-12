#include "6502cpu.h"
#include "debugger.h"
#include "gp_memory.h"
#include <cstring>
#include <format>
#include <iostream>

constexpr const char *USAGE =
    "\n{} <path to binary file> [-d|--debug|-v|--verbose|--print-device "
    "ADDR|--memory-size SIZE]\n"
    "  -d, --debug: enable debug mode\n"
    "  -v, --verbose: enable verbose mode\n"
    "  --print-device ADDR: set address of print device to ADDR, default {:X}\n"
    "  --memory-size SIZE: set size of memory (hex number of bytes), default "
    "{:X}\n\n";

int main(int argc, char **argv) {
  GP_Memory memory;

  if (argc <= 1) {
    std::cout << std::format(USAGE, argv[0], DEFAULT_OUTPUT_ADDRESS,
                             DEFAULT_MEMORY_SIZE);

    return 1;
  }

  // load the binary file into memory
  try {
    memory.import(argv[1]);
  } catch (std::runtime_error &e) {
    std::cerr << e.what() << std::endl;

    return 1;
  }

  CPU6502 cpu(&memory);

  for (int i = 0; i < argc; ++i) {
    // skip program name
    if (i == 0) {
      continue;
    }

    char *arg = argv[i];

    if (arg[0] == '-') {
      // this is a flag

      if (arg[1] == '-') {
        // long flag
        if (strcmp(arg, "--debug") == 0) {
          // debug
          cpu.set_debug(true);
        } else if (strcmp(arg, "--verbose") == 0) {
          // verbose
          cpu.set_verbose(true);
        } else if (strcmp(arg, "--print-device") == 0) {
          // set print device address
          char *addr_str = argv[++i];

          try {
            address print_addr = address(std::stoul(addr_str, nullptr, 16));

            memory.set_print_device(print_addr);
          } catch (std::invalid_argument &e) {
            std::cerr << "Invalid address: " << addr_str << std::endl;

            return 1;
          }
        } else {
          // short flag
          if (arg[1] == 'd') {
            // debug
            cpu.set_debug(true);
          } else if (arg[1] == 'v') {
            // verbose
            cpu.set_verbose(true);
          }
        }
      }
    }

    Debugger debugger(&cpu);

    try {
      debugger.run();
    } catch (CPUException &e) {
      std::cerr << e.message() << std::endl;

      return 1;
    }

    return 0;
  }
}