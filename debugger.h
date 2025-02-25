#ifndef _H_DEBUGGER
#define _H_DEBUGGER

#include "6502cpu.h"
#include "address.h"
#include "byte_utils.h"
#include "gp_memory.h"
#include "instruction_types.h"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <ostream>
#include <regex>
#include <string>
#include <vector>

constexpr const char *PROMPT = "> ";
constexpr const char *HELP_MSG =
    "Available commands:\n"
    "  d/dump - dump registers\n"
    "  g/get <address> - get value at address\n"
    "  g/get <start> <count> - get <count> values starting at <start>\n"
    "  s/step - step one instruction\n"
    "  c/continue - continue execution\n"
    "  h/help - show this help message";
constexpr const char *INVALID_COMMAND_MSG =
    "Unknown command (type help for more info).";

constexpr const char *BREAKPOINT_MSG = "== BREAKPOINT REACHED ==";

struct DebuggerOptions {
  bool enabled = false;

  DebuggerOptions() = default;
  DebuggerOptions(bool is_enabled) : enabled(is_enabled) {}
};

namespace Command {
enum class Name { DUMP, GET, EXIT, STEP, CONTINUE, HELP };

static std::optional<Name> parse_name(const std::string &name) {
  if (name == "dump" || name == "d") {
    return Name::DUMP;
  } else if (name == "get" || name == "g") {
    return Name::GET;
  } else if (name == "exit" || name == "e" || name == "q") {
    return Name::EXIT;
  } else if (name == "step" || name == "s") {
    return Name::STEP;
  } else if (name == "continue" || name == "c") {
    return Name::CONTINUE;
  } else if (name == "help" || name == "h") {
    return Name::HELP;
  }

  return std::nullopt;
}

static std::string name_to_string(Name name) {
  switch (name) {
  case Name::DUMP:
    return "dump";
  case Name::GET:
    return "get";
  case Name::EXIT:
    return "exit";
  case Name::STEP:
    return "step";
  case Name::CONTINUE:
    return "continue";
  case Name::HELP:
    return "help";
  default:
    return "unknown";
  }
}

class Command {
public:
  Name name;
  std::vector<std::string> args;

  static std::optional<Command> parse(const std::string &command) {
    std::istringstream iss(command);
    std::string part;

    Command cmd;

    if (iss >> part) {
      auto name = parse_name(part);

      if (!name) {
        return std::nullopt;
      }

      cmd.name = *name;

      while (iss >> part) {
        cmd.args.push_back(part);
      }

      return cmd;
    }

    return std::nullopt;
  }
};
} // namespace Command

class Debugger {
private:
  CPU6502 *cpu_;
  DebuggerOptions options_;

  int16_t twos_complement(uint8_t byte);

public:
  Debugger(CPU6502 *cpu) : cpu_(cpu), options_(DebuggerOptions(true)) {}

  Debugger(CPU6502 *cpu, DebuggerOptions &&options)
      : cpu_(cpu), options_(std::move(options)) {}

  void print_memory(std::ostream &stream, GP_Memory *memory, size_t start,
                    size_t count);

  bool go_to_debugger();

  void run() {
    while (cpu_->get_PC().inner() < cpu_->get_memory()->size()) {
      InstructionErr err = cpu_->step();

      if (err == InstructionErr::GoToDebugger) {
        std::cout << std::endl << BREAKPOINT_MSG << std::endl;

        if (go_to_debugger()) {
          return;
        }
      } else if (err == InstructionErr::Stop) {
        std::cout << std::endl << STP_MSG << std::endl;

        return;
      }
    }
  }
};

#endif