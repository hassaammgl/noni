#pragma once

#include <extensions/extension_manager.hpp>

#include <memory>

// Tiny built-in extension proving registration / events / cleanup.
std::unique_ptr<Extension> make_hello_extension();
