#!/bin/sh
# Auto-formats all C/C++ code with Artistic Style (http://astyle.sourceforge.net/)
# to adhere to a consistent coding style.  Should be run before committing.
astyle --options=none --suffix=".orig~" -Q -A2 -tSHU  -R "*.hpp" "*.cpp" "*.h" "*.c"

