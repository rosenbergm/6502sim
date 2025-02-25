#include "6502cpu.h"
#include "debugger.h"
#include <cstring>
#include <format>
#include <iostream>

constexpr const char *USAGE =
    "\n{} <path to binary file> [-d|--debug|-v|--verbose]\n"
    "  -d, --debug: enable debug mode\n"
    "  -v, --verbose: enable verbose mode\n\n";

int main(int argc, char **argv) {
  GP_Memory memory;

  if (argc <= 1) {
    std::cout << std::format(USAGE, argv[0]);

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
