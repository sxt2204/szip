#pragma once

#include <string>
#include <vector>

namespace sxzip {
namespace tui {

// Main entry point for the TUI
int run();

// Helper to construct and run the CLI programmatically
int execute_cli(const std::vector<std::string>& args);

} // namespace tui
} // namespace sxzip
