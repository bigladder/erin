#!/bin/bash
rm -f "test.toml"
modelkit template-compose --output="test.toml" "ex.pxt"
if diff "test.toml" "reference/expected_ex.toml"; then
  echo .
else
  echo test.toml not the same as reference/expected_ex.toml
  exit 1
fi

exit 0
