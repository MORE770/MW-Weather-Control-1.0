#!/usr/bin/env bash
# Builds a Windows x86 ASI from Linux with GCC, binutils and Python 3.
set -euo pipefail
cd "$(dirname "$0")"
tmp="$(mktemp -d)"
trap 'rm -rf -- "$tmp"' EXIT
mkdir -p scripts
gcc -m32 -std=c99 -O2 -ffreestanding -fno-ident -fno-pic -fno-pie \
  -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables \
  -fleading-underscore -mpreferred-stack-boundary=2 -Wall -Wextra -Werror \
  -c src/MW_WeatherControl.c -o "$tmp/mod.o"
objcopy -R .note.GNU-stack -O pe-i386 "$tmp/mod.o" "$tmp/mod.obj"
python3 build_relocs.py "$tmp/mod.o" "$tmp/mod.obj"
for dll in KERNEL32 USER32 ADVAPI32 GDI32; do
  { printf 'LIBRARY %s.dll\nEXPORTS\n' "$dll"
    case "$dll" in
      KERNEL32) printf '%s\n' CloseHandle CreateFileA WriteFile DisableThreadLibraryCalls FlushInstructionCache GetCurrentProcess GetCurrentProcessId GetModuleFileNameA GetModuleHandleA GetModuleHandleExA GetProcAddress GetTickCount VirtualProtect ;;
      USER32) printf '%s\n' GetAsyncKeyState GetForegroundWindow GetWindowThreadProcessId CreateWindowExA DestroyWindow GetCursorPos ScreenToClient ShowCursor LoadCursorA SetCursor GetCursorInfo ;;
      GDI32) printf '%s\n' CreateCompatibleDC DeleteDC CreateDIBSection SelectObject DeleteObject CreateFontW SetTextColor SetBkMode TextOutW GetTextExtentPoint32W ;;
      ADVAPI32) printf '%s\n' RegOpenKeyExA RegCreateKeyExA RegQueryValueExA RegSetValueExA RegCloseKey ;;
    esac
  } > "$tmp/$dll.def"
  awk 'NR>2 {print ".globl _"$1"\n_"$1": ret"}' "$tmp/$dll.def" > "$tmp/$dll.s"
  as --32 -o "$tmp/$dll.elf" "$tmp/$dll.s"
  objcopy -O pe-i386 "$tmp/$dll.elf" "$tmp/$dll.obj"
  ld -m i386pe --dll -e 0 --out-implib "$tmp/$dll.a" -o "$tmp/$dll.dll" "$tmp/$dll.obj" "$tmp/$dll.def"
done
ld -m i386pe --dll --entry _DllMain --subsystem windows --no-insert-timestamp \
  --no-undefined --exclude-all-symbols -o scripts/MW_WeatherControl.asi \
  "$tmp/mod.obj" "$tmp/KERNEL32.a" "$tmp/USER32.a" "$tmp/ADVAPI32.a" "$tmp/GDI32.a"
