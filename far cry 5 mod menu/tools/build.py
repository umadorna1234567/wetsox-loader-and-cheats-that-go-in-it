"""Build and test with the installed Visual Studio C++ toolchain."""
import json
import argparse
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--variant', default='vs', choices=['vs', 'runtime'])
parser.add_argument('--dll-name', help='Unique basename for a new DLL while an older version remains resident')
args = parser.parse_args()
dll_name = 'FC5MenuRuntime' if args.variant == 'runtime' else 'FC5Menu'
if args.dll_name:
    if not args.dll_name.isalnum():
        parser.error('--dll-name must be alphanumeric')
    dll_name = args.dll_name
vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
# Normalize case to avoid duplicate Path/PATH entries breaking MSBuild.
env = {key.upper(): value for key, value in os.environ.items()}
installations = json.loads(subprocess.check_output([
    str(vswhere), "-latest", "-products", "*", "-requires",
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-format", "json"
], env=env, text=True))
if not installations:
    raise SystemExit("Install Visual Studio with Desktop development with C++.")
installation = installations[0]
vs = Path(installation["installationPath"])
major = int(installation["installationVersion"].split(".")[0])
generators = {17: "Visual Studio 17 2022", 18: "Visual Studio 18 2026"}
if major not in generators:
    raise SystemExit(f"Unsupported Visual Studio major version: {major}")
cmake = vs / "Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
ctest = cmake.with_name("ctest.exe")
build = root / 'build' / args.variant
commands = [
    [str(cmake), "-S", str(root), "-B", str(build), "-G", generators[major], "-A", "x64", f'-DFC5_DLL_NAME={dll_name}'],
    [str(cmake), "--build", str(build), "--config", "Release"],
    [str(ctest), "--test-dir", str(build), "-C", "Release", "--output-on-failure"],
]
for command in commands:
    subprocess.run(command, env=env, cwd=root, check=True)
print(f"Menu preview: {build / 'Release/FC5MenuPreview.exe'}")
print(f"Internal DLL: {build / 'Release' / (dll_name + '.dll')}")
print(f"Loading utility: {build / 'Release/FC5MenuLoader.exe'}")
