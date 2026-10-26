#!/usr/bin/env python3
"""Build the patched BootloadHID uploader used by the RSI-05 board.

The RBoard identifies itself with VID/PID 16c0:05df but uses USB strings that
do not match the original BootloadHID defaults. This script clears those
string filters, builds the upstream command-line utility, and installs it next
to this script for PlatformIO's custom upload command.
"""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile


TOOLS_DIR = Path(__file__).resolve().parent
ARCHIVE = TOOLS_DIR / "bootloadHID-2012-12-08.tar.gz"
SOURCE_ROOT = "bootloadHID.2012-12-08"
SOURCE_RELATIVE_PATH = Path(SOURCE_ROOT) / "commandline" / "main.c"
BUILD_RELATIVE_PATH = Path(SOURCE_ROOT) / "commandline" / "bootloadHID"

IDENTIFIER_REPLACEMENTS = {
    '#define IDENT_VENDOR_STRING     "obdev.at"':
        "#define IDENT_VENDOR_STRING     NULL",
    '#define IDENT_PRODUCT_STRING    "HIDBoot"':
        "#define IDENT_PRODUCT_STRING    NULL",
}


def require_tools() -> None:
    """Fail early with a useful message if the native build tools are absent."""
    missing = [name for name in ("make", "gcc") if shutil.which(name) is None]
    if missing:
        raise RuntimeError("Required build command(s) not found: " + ", ".join(missing))


def extract_source(destination: Path) -> None:
    """Extract the bundled source archive without allowing path traversal."""
    if not ARCHIVE.is_file():
        raise FileNotFoundError(f"BootloadHID source archive is missing: {ARCHIVE}")

    with tarfile.open(ARCHIVE, mode="r:gz") as archive:
        root = destination.resolve()
        for member in archive.getmembers():
            target = (destination / member.name).resolve()
            if target != root and root not in target.parents:
                raise RuntimeError(f"Unsafe path in source archive: {member.name}")
        archive.extractall(destination)


def patch_usb_identifiers(source_file: Path) -> None:
    """Disable matching on manufacturer/product strings, retaining VID/PID."""
    source = source_file.read_text(encoding="utf-8")
    for expected, replacement in IDENTIFIER_REPLACEMENTS.items():
        occurrences = source.count(expected)
        if occurrences != 1:
            raise RuntimeError(
                f"Expected exactly one occurrence of {expected!r} in {source_file}; "
                f"found {occurrences}. The bundled source may have changed."
            )
        source = source.replace(expected, replacement, 1)
    source_file.write_text(source, encoding="utf-8")


def build_and_install(work_dir: Path) -> Path:
    """Compile the CLI utility and atomically place it in the tools folder."""
    source_file = work_dir / SOURCE_RELATIVE_PATH
    build_dir = source_file.parent
    if not source_file.is_file():
        raise FileNotFoundError(f"BootloadHID source file not found: {source_file}")

    patch_usb_identifiers(source_file)
    subprocess.run(["make", "-C", str(build_dir)], check=True)

    built_binary = work_dir / BUILD_RELATIVE_PATH
    if not built_binary.is_file():
        raise FileNotFoundError(f"Build completed without producing {built_binary}")

    installed_binary = TOOLS_DIR / "bootloadHID"
    temporary_binary = TOOLS_DIR / ".bootloadHID.tmp"
    shutil.copy2(built_binary, temporary_binary)
    temporary_binary.chmod(temporary_binary.stat().st_mode | 0o111)
    os.replace(temporary_binary, installed_binary)
    return installed_binary


def main() -> int:
    require_tools()
    with tempfile.TemporaryDirectory(prefix="bootloadhid-build-") as temporary_dir:
        work_dir = Path(temporary_dir)
        extract_source(work_dir)
        installed_binary = build_and_install(work_dir)

    subprocess.run([str(installed_binary), "--help"], check=True)
    print(f"Installed BootloadHID uploader: {installed_binary}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (FileNotFoundError, RuntimeError, subprocess.CalledProcessError, tarfile.TarError) as error:
        raise SystemExit(f"BootloadHID build failed: {error}") from error
