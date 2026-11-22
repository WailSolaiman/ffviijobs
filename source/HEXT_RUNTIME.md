# HEXT runtime

The build emits `mod/hext/FFVIIJobs.txt` and
`mod/runtime/ffvii-jobs-v080.bin`. The unconditional runtime ModFolder maps
the binary into the game directory through 7H's virtual file handlers;
the nine selected class-marker folders use their existing paths and IDs.

The English Steam executable has zero-filled executable padding from
0x7B5800 to 0x7B6000. The compact entry uses this space, with base/status
slots at 0x7B5FF0/0x7B5FF4. A five-byte jump at 0x67DB38, after AppLoader's
WinMain trampoline, runs after virtual files are ready and before font setup.
The bridge preserves general registers/flags, restores the original stack
setup call, loads the flat runtime, then resumes the original call.

HEXT ends with explicit execute-permission directives for both patched pages.
This matters because 7H's HexPatch write path can leave pages read/write.
The integration fixture uses the installed parser to check exact bytes and
final page permissions in a private allocation, without executing those bytes.
The native fixture separately executes the actual entry at game addresses.

The runtime begins with eight little-endian DWORDs: magic FJX1 (0x31584A46),
image size, preferred base, entry RVA, HIGHLOW relocation count, file size,
and two reserved zeros. A 32-byte header preserves 16-byte image alignment.
The section image follows, then relocation RVAs. There are no PE/MZ headers
or import directory. Compiler PE files are intermediate build artifacts.
Win32 APIs are resolved using the tested game's kernel32 IAT entries.

The entry verifies the expected file length and FNV-1a hash before relocation
and execution. This detects accidental corruption, not malicious modification.
Missing/corrupt files restore normal startup and leave gameplay hooks alone.
Core installation retains all sixteen original signature guards. The new
static entry/cave itself is specific to this executable and must be checked
against other HEXT mods. This mod still executes code; it uses 7H's HEXT
mechanism rather than its separate DLL/plugin/program loader.

`test_native_legacy.c` is the source fixture baseline transformed by
`build_hext.py` into a flat-runtime test host; it is not a DLL build target.
