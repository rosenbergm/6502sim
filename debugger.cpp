#include "debugger.h"
#include <cstdint>

int16_t Debugger::twos_complement(uint8_t byte) {
  return static_cast<int16_t>(static_cast<int8_t>(byte));
}

void Debugger::print_memory(std::ostream &stream, GP_Memory *memory,
                            size_t start, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (i % 16 == 0) {
      stream << std::endl
             << std::hex << std::setw(4) << std::setfill('0') << start + i
             << ": " << std::hex << std::setw(2) << std::setfill('0')
             << static_cast<int>(memory->read(start + i));

      continue;
    }

    if (i % 8 == 0) {
      stream << " | ";
    } else {
      stream << " ";
    }

    stream << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(memory->read(start + i));
  }

  stream << std::endl << std::endl;
}

bool Debugger::go_to_debugger() {
  std::string command;

  while (true) {
    std::cout << PROMPT;
    std::getline(std::cin, command);

    if (command.empty()) {
      continue;
    }

    auto cmd = Command::Command::parse(command);

    if (!cmd) {
      std::cout << INVALID_COMMAND_MSG << std::endl;

      continue;
    }

    switch (cmd->name) {
    case Command::Name::DUMP:
      std::cout << "format: HEX (UNSIGNED, SIGNED)" << std::endl;
      std::cout << "A:  " << std::hex << static_cast<int>(cpu_->get_A())
                << std::dec << " (" << std::to_integer<size_t>(cpu_->get_A())
                << ", "
                << twos_complement(std::to_integer<uint8_t>(cpu_->get_A()))
                << ")" << std::endl;
      std::cout << "X:  " << static_cast<int>(cpu_->get_X()) << std::endl;
      std::cout << "Y:  " << static_cast<int>(cpu_->get_Y()) << std::endl;
      std::cout << "S:  " << static_cast<int>(cpu_->get_S()) << std::endl;
      std::cout << "PC: " << std::hex << static_cast<size_t>(cpu_->get_PC())
                << std::dec << std::endl;
      std::cout << "P:  "
                << std::bitset<8>(
                       static_cast<unsigned char>(cpu_->get_PSR()->get()))
                << std::endl;
      std::cout << "    NV BDIZC" << std::endl;
      break;
    case Command::Name::GET: {
      switch (cmd->args.size()) {
      case 0:
        std::cout << "Missing address." << std::endl;
        continue;
      case 1: {
        // get one byte
        size_t address = hex_to_number(cmd->args[0]);

        print_memory(std::cout, cpu_->get_memory(), address, 1);

        continue;
      }
      case 2: {
        // get multiple bytes
        size_t start = hex_to_number(cmd->args[0]);
        size_t count = hex_to_number(cmd->args[1]);

        print_memory(std::cout, cpu_->get_memory(), start, count);

        continue;
      }
      default:
        std::cout << "Too many arguments." << std::endl;
        continue;
      }
    }
    case Command::Name::STEP: {
      InstructionErr err = cpu_->step();

      if (err == InstructionErr::GoToDebugger) {
        std::cout << std::endl << BREAKPOINT_MSG << std::endl;

        continue;
      } else if (err == InstructionErr::Stop) {
        std::cout << std::endl << STP_MSG << std::endl;

        return true;
      }

      continue;
    }
    case Command::Name::CONTINUE: {
      return false;
    }
    case Command::Name::HELP: {
      std::cout << HELP_MSG << std::endl;

      continue;
    }
    case Command::Name::EXIT: {
      exit(0);
    }
    default:
      continue;
    }
  }
}
