#!/bin/bash

clang-format -i $(
  find src -name "*.cc" -o -name "*.h" | sort
)

~/.local/bin/mdformat README.md src/ doc/
