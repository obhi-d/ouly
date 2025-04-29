import os

def add_spdx_header_to_file(file_path, header_text):
    # 1. Read raw bytes
    with open(file_path, 'rb') as f:
        data = f.read()

    # 2. Keep only ASCII bytes (0x00–0x7F)
    ascii_data = bytes(b for b in data if b < 0x80)

    # 3. Write back in binary mode—no BOM, no surprises
    with open(file_path, 'wb') as f:
        f.write(ascii_data)

def add_spdx_headers(directory, extensions=('.hpp', '.cpp')):
    """
    Recursively walks through `directory` and adds SPDX headers to files
    with the specified extensions.
    """
    header_text = "// SPDX-License-Identifier: MIT"
    for root, _, files in os.walk(directory):
        for filename in files:
            if filename.lower().endswith(extensions):
                file_path = os.path.join(root, filename)
                add_spdx_header_to_file(file_path, header_text)

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description="Recursively add SPDX MIT license headers to .hpp and .cpp files."
    )
    parser.add_argument(
        "directory", nargs='?', default='.',
        help="Root directory to start scanning (default: current directory)."
    )
    args = parser.parse_args()
    add_spdx_headers(args.directory)
    print(f"SPDX headers added to .hpp and .cpp files under {args.directory}")
