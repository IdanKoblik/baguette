#!/usr/bin/env python3
"""List TODO-style comments across the project, grouped by file with line numbers.

Usage:
    dev/todos.py                      # scan the repo root for the default markers
    dev/todos.py src tests            # scan only these paths
    dev/todos.py -m TODO -m FIXME     # custom markers
    dev/todos.py --no-color           # plain output (e.g. when piping)
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

DEFAULT_MARKERS = ["TODO", "FIXME", "HACK", "XXX", "BUG"]

SKIP_DIRS = {".git", "build", ".cache", "__pycache__"}

TEXT_SUFFIXES = {
    ".c", ".h", ".cpp", ".cc", ".hpp", ".py", ".sh", ".rs", ".go",
    ".js", ".ts", ".lua", ".kdl", ".toml", ".yml", ".yaml", ".json",
    ".jsonc", ".md", ".css", ".mk", "",  # "" -> extensionless (Makefile)
}


class Color:
    def __init__(self, enabled: bool):
        self.enabled = enabled

    def _wrap(self, code: str, text: str) -> str:
        return f"\033[{code}m{text}\033[0m" if self.enabled else text

    def bold(self, t: str) -> str:
        return self._wrap("1", t)

    def cyan(self, t: str) -> str:
        return self._wrap("36", t)

    def yellow(self, t: str) -> str:
        return self._wrap("33", t)

    def dim(self, t: str) -> str:
        return self._wrap("2", t)


def repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


def git_tracked_files(root: Path) -> list[Path] | None:
    try:
        out = subprocess.run(
            ["git", "-C", str(root), "ls-files", "--cached", "--others",
             "--exclude-standard"],
            capture_output=True, text=True, check=True,
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None
    files = [root / line for line in out.stdout.splitlines() if line]
    return [f for f in files if f.is_file()]


def walk_files(root: Path) -> list[Path]:
    """Filesystem fallback: walk the tree skipping noise directories."""
    found: list[Path] = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for name in filenames:
            found.append(Path(dirpath) / name)
    return found


def candidate_files(paths: list[Path], root: Path) -> list[Path]:
    tracked = git_tracked_files(root)
    files: list[Path] = []
    seen: set[Path] = set()

    for p in paths:
        p = p.resolve()
        if p.is_file():
            candidates = [p]
        elif p.is_dir():
            if tracked is not None:
                candidates = [f for f in tracked if _is_within(f, p)]
            else:
                candidates = walk_files(p)
        else:
            print(f"warning: {p} does not exist, skipping", file=sys.stderr)
            continue

        for f in candidates:
            rf = f.resolve()
            if rf not in seen and rf.suffix.lower() in TEXT_SUFFIXES:
                seen.add(rf)
                files.append(rf)

    files.sort()
    return files


def _is_within(child: Path, parent: Path) -> bool:
    try:
        child.resolve().relative_to(parent.resolve())
        return True
    except ValueError:
        return False


def build_pattern(markers: list[str]) -> re.Pattern:
    alt = "|".join(re.escape(m) for m in markers)
    return re.compile(rf"\b({alt})\b(?:[:( ]|$)")


def scan(files: list[Path], pattern: re.Pattern) -> dict[Path, list[tuple[int, str, str]]]:
    """Return {file: [(lineno, marker, text), ...]}."""
    results: dict[Path, list[tuple[int, str, str]]] = {}
    for f in files:
        try:
            with f.open("r", encoding="utf-8", errors="replace") as fh:
                for lineno, line in enumerate(fh, start=1):
                    m = pattern.search(line)
                    if m:
                        results.setdefault(f, []).append(
                            (lineno, m.group(1), line.strip())
                        )
        except OSError as e:
            print(f"warning: cannot read {f}: {e}", file=sys.stderr)
    return results


def main() -> int:
    parser = argparse.ArgumentParser(
        description="List TODO-style comments with file paths and line numbers.",
    )
    parser.add_argument("paths", nargs="*", help="files or dirs to scan (default: repo root)")
    parser.add_argument("-m", "--marker", action="append", dest="markers",
                        help="marker to search for (repeatable; default: "
                             + ", ".join(DEFAULT_MARKERS) + ")")
    parser.add_argument("--no-color", action="store_true", help="disable colored output")
    args = parser.parse_args()

    root = repo_root()
    markers = args.markers or DEFAULT_MARKERS
    paths = [Path(p) for p in args.paths] if args.paths else [root]

    color = Color(enabled=not args.no_color and sys.stdout.isatty())
    pattern = build_pattern(markers)

    files = candidate_files(paths, root)
    results = scan(files, pattern)

    if not results:
        print(f"No {'/'.join(markers)} comments found.")
        return 0

    total = 0
    for f in sorted(results):
        rel = os.path.relpath(f, root)
        hits = results[f]
        total += len(hits)
        print(color.bold(color.cyan(rel)) + color.dim(f"  ({len(hits)})"))
        for lineno, marker, text in hits:
            loc = color.dim(f"{rel}:{lineno}")
            print(f"  {loc}  {color.yellow(marker)}  {text}")
        print()

    plural = "s" if total != 1 else ""
    print(color.bold(f"{total} marker{plural} across {len(results)} file(s)."))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
