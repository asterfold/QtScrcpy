"""Compile and run the log pressure regression with the configured Qt build."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
flags = (root / "build/QtScrcpy/CMakeFiles/QtScrcpy.dir/flags.make").read_text()
includes = shlex.split(next(line.split(" = ", 1)[1] for line in flags.splitlines()
                           if line.startswith("CXX_INCLUDES = ")))
with tempfile.TemporaryDirectory(prefix="qtscrcpy-logs-") as directory:
    binary = str(Path(directory) / "logbuffer_tests")
    subprocess.run(["/usr/bin/c++", "-std=c++11", "-fPIC", *includes,
                    str(Path(__file__).parent / "logbuffer_tests.cpp"), "-o", binary,
                    "-F/opt/homebrew/opt/qt@5/lib", "-framework", "QtWidgets",
                    "-framework", "QtGui", "-framework", "QtCore"], check=True)
    subprocess.run([binary], env={**os.environ, "QT_QPA_PLATFORM": "offscreen"}, check=True)
