#!/bin/bash

MDFORMAT="${HOME}/.local/bin/mdformat"
TYPOS="${HOME}/.cargo/bin/typos"
PYBLACK="${HOME}/.local/bin/black"

GREEN="\x1b[32m"
CLEAR="\x1b[0m"

echo -e "${GREEN}clang-format${CLEAR}"
clang-format -i $(
  find src -name "*.cc" -o -name "*.h" | sort
)

[[ -x ${MDFORMAT} ]] && {
  echo -e "${GREEN}mdformat${CLEAR}"
  ${MDFORMAT} *.md ./src ./doc
}

[[ -x ${TYPOS} ]] && {
  echo -e "${GREEN}typos${CLEAR}"
  ${TYPOS} ./doc ./src
}

[[ -x ${PYBLACK} ]] && {
  echo -e "${GREEN}black${CLEAR}"
  LC_ALL=en_US.utf8 LANG=en_US.utf8 ${PYBLACK} \
    --line-length=80 $(find . -name "*.py" | sort | egrep -v vendor)
}
