"""Post-process fastddsgen 3.0.0 output: clamp `uint32_t <type>_max_cdr_typesize` constants to UINT32_MAX.

For very large ROS 2 types (visualization_msgs InteractiveMarker*, whose unbounded sequences fastddsgen sizes at 100
elements each) fastddsgen 3.0.0 writes a maximum serialized size above 2^32 into a `constexpr uint32_t`, which is a
narrowing error in C++. The constant only bounds buffer sizes; UINT32_MAX means "as large as can be stated".

    python3 -I tools/fastddsgen_fixup.py <file>...
"""
import re
import sys
from pathlib import Path

_CONST = re.compile(r"(constexpr uint32_t \w+_max_(?:key_)?cdr_typesize \{)(\d+)(UL\};)")
UINT32_MAX = 2 ** 32 - 1


def fix_file(path: Path) -> bool:
    text = path.read_text(encoding="utf-8")
    new = _CONST.sub(lambda m: m.group(1) + (str(UINT32_MAX) if int(m.group(2)) > UINT32_MAX else m.group(2))
                     + m.group(3), text)
    if new != text:
        path.write_text(new, encoding="utf-8")
        return True
    return False


if __name__ == "__main__":
    for arg in sys.argv[1:]:
        p = Path(arg)
        if p.is_file():
            fix_file(p)
    sys.exit(0)
