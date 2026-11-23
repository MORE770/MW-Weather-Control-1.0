"""Translate the ELF i386 relocations that GNU objcopy leaves as raw IDs.

Use the ELF symbol tables as the source of truth. objcopy copies a symbol's
metadata but its cross-flavour mode may leave stale relocation symbol indices.
"""
import struct
import sys
from pathlib import Path

elf_path, coff_path = map(Path, sys.argv[1:3])
elf, coff = elf_path.read_bytes(), bytearray(coff_path.read_bytes())
assert elf[:7] == b"\x7fELF\x01\x01\x01"
shoff = struct.unpack_from("<I", elf, 32)[0]
shsize, shcount, shnames = struct.unpack_from("<HHH", elf, 46)
sh = [struct.unpack_from("<10I", elf, shoff + i * shsize) for i in range(shcount)]

def cstring(buf, at):
    return bytes(buf[at:buf.index(0, at)]).decode("ascii")

def shname(i):
    return cstring(elf, sh[shnames][4] + sh[i][0])

machine, count, _, symoff, symcount, optsize = struct.unpack_from("<HHIIIH", coff)
assert machine == 0x14C and optsize == 0
strings = symoff + 18 * symcount
sections = {}
for i in range(count):
    off = 20 + i * 40
    name = bytes(coff[off:off+8]).rstrip(b"\0").decode("ascii")
    if name.startswith("/"):
        name = cstring(coff, strings + int(name[1:]))
    sections[name] = (i + 1, off)

named_symbols, section_symbols = {}, {}
for i in range(symcount):
    off = symoff + 18 * i
    name = (cstring(coff, strings + struct.unpack_from("<I", coff, off+4)[0])
            if coff[off:off+4] == b"\0" * 4
            else bytes(coff[off:off+8]).rstrip(b"\0").decode("ascii", "replace"))
    _, sec, _, storage, _ = struct.unpack_from("<IhHBB", coff, off + 8)
    if storage == 2 or storage == 3:
        named_symbols[(name, sec)] = i
        if sec >= 1:
            for sec_name, (sec_no, _) in sections.items():
                if sec == sec_no and name == sec_name:
                    section_symbols[sec] = i

def section_symbol(sec):
    if sec in section_symbols:
        return section_symbols[sec]
    # Inject a private zero-offset symbol in this section. Append before the
    # COFF string table, preserving all existing symbol and string indices.
    global strings, symcount
    idx = symcount
    entry = struct.pack("<8sIhHBB", ("_sec%03d" % sec).encode(), 0, sec, 0, 3, 0)
    coff[strings:strings] = entry
    strings += 18
    symcount += 1
    struct.pack_into("<I", coff, 12, symcount)
    section_symbols[sec] = idx
    return idx

for rel in sh:
    if rel[1] != 9:  # SHT_REL
        continue
    sec_name = shname(rel[7])
    _, tgt = sections[sec_name]
    symtab = sh[rel[6]]
    names = sh[symtab[6]][4]
    raw, records = struct.unpack_from("<II", coff, tgt + 20)
    count = struct.unpack_from("<H", coff, tgt + 32)[0]
    assert count * 8 == rel[5]
    for i in range(count):
        offset, info = struct.unpack_from("<II", elf, rel[4] + i * 8)
        nameoff, _, _, sym_type, _, sym_sec = struct.unpack_from(
            "<IIIBBH", elf, symtab[4] + (info >> 8) * 16)
        kind = info & 255
        rec = records + i * 10
        assert struct.unpack_from("<I", coff, rec)[0] == offset
        coff_sec = sections[shname(sym_sec)][0] if sym_sec else 0
        if sym_type & 15 == 3:  # STT_SECTION
            symbol = section_symbol(coff_sec)
        else:
            symbol = named_symbols[(cstring(elf, names + nameoff), coff_sec)]
        if kind == 2:  # COFF REL32 is relative to the end of the operand.
            addend = struct.unpack_from("<i", elf, sh[rel[7]][4] + offset)[0]
            struct.pack_into("<i", coff, raw + offset, addend + 4)
        struct.pack_into("<IH", coff, rec + 4, symbol, {1: 6, 2: 20}[kind])
coff_path.write_bytes(coff)
