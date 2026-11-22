# FFVII Job System

**By Willard82** · A free mod for original English Final Fantasy VII on PC, using 7th Heaven.

![The 25 classes in FFVII Job System](preview.png)

Choose one of **25 Final Fantasy classes** for each of the nine characters before launching the game. Each class has its own Materia permissions, and character weapon icons show who can equip each Materia. Characters can share a class.

Configure Mod includes class descriptions, complete Materia lists and a reference for all 32 accessories. **Accessory class suggestions are informational:** active accessory permissions still follow the existing character rules.

## Download and install

Download the `.iro` from [Releases](https://github.com/WailSolaiman/ffviijobs/releases) when a packaged release is available. Import it into 7th Heaven, activate **FFVII Job System**, and open **Configure Mod** to choose your classes. Changes apply on the next launch.

Materia with no eligible selected character remain visible, without weapon icons, and cannot be equipped. Previously equipped forbidden Materia return to inventory with their AP. If inventory is full, they are preserved but inactive during stat and command calculations until space is available.

See [the installation guide](README.txt) and [the complete class permissions](CLASS_PERMISSIONS.txt).

## Compatibility

Tested with the English Steam FF7 PC 1.02 executable, 7th Heaven 4.6.1, FFNx 1.25.2.0 and Enhanced Stock UI 2.968. Other languages and gameplay overhauls that change Materia IDs or the same executable hooks are unverified.

The current source release is **0.8.0**, displayed as **0.80** in 7th Heaven.
It uses HEXT patches and a freestanding runtime, with no DLL/plugin/program
declarations and no native-library activation prompt in 7th Heaven 4.6.1.
7th Heaven's security settings are unchanged. The new loading backend has
passed private native and installed-7H integration checks and **awaits an
in-game test before a public packaged release**. Class permissions and saved
configuration selections are unchanged from 0.7.2.

## Build from source

Build on Windows with Python 3, Pillow, pefile, Windows Segoe UI fonts and Zig 0.17.1:

```powershell
py -m pip install Pillow pefile
py source/build.py --zig C:/path/to/zig.exe
py source/package.py
```

The build creates the HEXT entry, flat runtime and configuration previews. Packaging creates the IRO and a source ZIP. Intermediate compiler DLLs and archives are excluded from Git; the generated HEXT and runtime payload are included for review. Packaged downloads belong in Releases. See [runtime format](source/HEXT_RUNTIME.md).

Validation details are in [VALIDATION.txt](VALIDATION.txt). The `source` folder also contains private native, loader, configuration and metadata verification tools, which do not require launching FF7.

## Publishing

[PUBLISHING.txt](PUBLISHING.txt) describes the prepared `v0.8.0` GitHub release and catalogue submission. The files under `catalog-submission` are staged metadata; their download links become usable after the corresponding release assets are uploaded. This repository alone does not add the mod to 7th Heaven's community catalogue.

## Feedback and support

[Report an issue](https://github.com/WailSolaiman/ffviijobs/issues). Please include your game language, mod version, 7th Heaven version and enabled mods.

If you enjoy the mod, you can [buy Willard82 a coffee](https://ko-fi.com/ffviijobs). The mod is free, and tips are always optional.

The preview is an illustrated roster generated using classic Final Fantasy job artwork as visual references. See [art sources](ART_SOURCES.txt) and the included generation/edit prompts. This is an unofficial fan-made mod.
