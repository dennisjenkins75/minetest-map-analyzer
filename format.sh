#!/bin/bash

MDFORMAT="${HOME}/.local/bin/mdformat"
TYPOS="${HOME}/.cargo/bin/typos"

GREEN="\x1b[32m"
CLEAR="\x1b[0m"

echo -e "${GREEN}clang-format${CLEAR}"
clang-format -i $(
  find src -name "*.cc" -o -name "*.h" | sort
)

[[ -x ${MDFORMAT} ]] && {
  echo -e "${GREEN}mdformat${CLEAR}"
  ${MDFORMAT} README.md ./src ./doc
}

[[ -x ${TYPOS} ]] && {
  echo -e "${GREEN}typos${CLEAR}"
  ${TYPOS} ./doc ./src
}
