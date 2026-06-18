#include "logging/ConsoleSink.hpp"

#include <iostream>

void ConsoleSink::write(const std::string& line) { std::cout << line << "\n"; }
