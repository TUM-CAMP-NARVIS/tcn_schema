"""Post-process fastddsgen 3.0.0 output: clamp `uint32_t <type>_max_cdr_typesize` constants that overflow uint32_t.

For very large ROS 2 types (visualization_msgs InteractiveMarker*, some moveit_msgs types, whose unbounded sequences
fastddsgen sizes at 100 elements each) fastddsgen 3.0.0 writes a maximum serialized size above 2^32 into a
`constexpr uint32_t`, which is a narrowing error in C++. Such constants are set to CLAMP: the largest 4-aligned
value for which the generated PubSubType's `size += alignment(size, 4); m_typeSize = size + 4` does not wrap (with
UINT32_MAX it wraps to 4). The constants only size buffers; with Fast DDS (WITH_DDS) use a payload pool policy that
reallocates (the default, PREALLOCATED_WITH_REALLOC) for these types.

    python3 -I tools/fastddsgen_fixup.py <output dir>...     # every *CdrAux.hpp directly in each directory
"""
import re
import sys
from pathlib import Path

_CONST = re.compile(r"(constexpr uint32_t \w+_max_(?:key_)?cdr_typesize \{)(\d+)(UL\};)")
UINT32_MAX = 2 ** 32 - 1
CLAMP = 2 ** 32 - 8  # 4294967288: 4-aligned, + 4 still fits


def fix_file(path: Path) -> bool:
    text = path.read_text(encoding="utf-8")
    new = _CONST.sub(lambda m: m.group(1) + (str(CLAMP) if int(m.group(2)) > UINT32_MAX else m.group(2))
                     + m.group(3), text)
    if new != text:
        path.write_text(new, encoding="utf-8")
        return True
    return False


def fix_dir(directory: Path) -> int:
    """Fix every *CdrAux.hpp in `directory`; return how many files changed."""
    return sum(fix_file(p) for p in sorted(directory.glob("*CdrAux.hpp")))


if __name__ == "__main__":
    for arg in sys.argv[1:]:
        p = Path(arg)
        if p.is_dir():
            fix_dir(p)
        elif p.is_file():
            fix_file(p)
    sys.exit(0)
