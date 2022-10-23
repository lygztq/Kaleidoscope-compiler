import os
import argparse
from subprocess import Popen, PIPE, TimeoutExpired

_EXTS = set([".cc", ".h"])
_SRC_DIRS = ["./include", "./src", "./tests"]

def fmt_directory(dir: str) -> None:
    if not os.path.isdir(dir):
        raise ValueError("Expect a directory as input")
    for (dirpath, _, filenames) in os.walk(dir):
        for filename in filenames:
            ext = os.path.splitext(filename)[-1]
            if ext in _EXTS:
                path = os.path.join(dirpath, filename)
                cmd = f"clang-format -style=file -i {path}".split(" ")
                p = Popen(args=cmd, stdout=PIPE, stderr=PIPE)
                try:
                    _, err = p.communicate(timeout=15)
                    err = err.decode("utf-8")
                    if err != "":
                        raise RuntimeError(err)
                except TimeoutExpired:
                    p.kill()
                    print(f"Time expired when formating file {path}")
                    exit(-1)

parser = argparse.ArgumentParser(
    description="format c++ source files in a directory")
parser.add_argument("--dir", type=str, default=None,
                    help="the source file directory to be formated, "
                         "if None, format all source files")
args = parser.parse_args()

if __name__ == "__main__":
    if args.dir is None:
        for d in _SRC_DIRS:
            fmt_directory(d)
    else:
        fmt_directory(args.dir)
