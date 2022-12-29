#!/bin/bash

MDFORMAT="${HOME}/.local/bin/mdformat"
TYPOS="${HOME}/.cargo/bin/typos"

clang-format -i $(
  find src -name "*.cc" -o -name "*.h" | sort
)

[[ -x ${MDFORMAT} ]] && ${MDFORMAT} README.md ./src ./doc

[[ -x ${TYPOS} ]] && ${TYPOS} ./doc ./src
