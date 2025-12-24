# Lrix
# Copyright (C) 2025 lrisguan <lrisguan@outlook.com>
# 
# This program is released under the terms of the GNU General Public License version 2(GPLv2).
# See https://opensource.org/licenses/GPL-2.0 for more information.
# 
# Project homepage: https://github.com/lrisguan/Lrix
# Description: A scratch implemention of OS based on RISC-V

#!/usr/bin/env python3
"""
Generate kernel/fs/README.c from top-level README.md.

This script reads README.md in the project root and emits a C source file
kernel/fs/README.c that contains the README contents as a byte array and size
constants:

    const unsigned int README_MD_SIZE;
    const uint8_t README_MD[];

Usage (from project root):

    python3 gen_readme.py

Then rebuild the kernel to pick up the updated README.c.
"""

import pathlib

ROOT = pathlib.Path(__file__).parent
README = ROOT / "README.md"
OUT = ROOT / "kernel" / "fs" / "README.c"


def main() -> None:
    text = README.read_text(encoding="utf-8")
    data = text.encode("utf-8")

    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w", encoding="utf-8") as f:
        f.write("/*\n"
                  " * Lrix\n"
                  " * Copyright (C) 2025 lrisguan <lrisguan@outlook.com>\n"
                  " * \n"
                  " * This program is released under the terms of the GNU General Public License version 2(GPLv2). \n"
                  " * See https://opensource.org/licenses/GPL-2.0 for more information. \n"
                  " * \n"
                  " * Project homepage: https://github.com/lrisguan/Lrix \n"
                  " * Description: A scratch implemention of OS based on RISC-V \n"
                  " */\n\n")
        f.write("/*\n"
                  " * Auto-generated from README.md, do not edit manually.\n"
                  " */\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"const unsigned int README_MD_SIZE = {len(data)};\n\n")
        f.write("const uint8_t README_MD[] = {\n")

        # pretty-print hex bytes, 16 per line
        for i, b in enumerate(data):
            if i % 16 == 0:
                f.write("    ")
            f.write(f"0x{b:02x}, ")
            if i % 16 == 15:
                f.write("\n")
        if len(data) % 16 != 0:
            f.write("\n")

        f.write("};\n")


if __name__ == "__main__":
    main()
