FFVII JOB SYSTEM 1.0.0 (shown as 1.00 in 7th Heaven)
Author: Willard82
Project: https://github.com/WailSolaiman/ffviijobs
Optional tips: https://ko-fi.com/ffviijobs

Choose one of 25 Final Fantasy jobs for each character. Your choices decide
which Materia they can equip, with weapon icons showing eligible characters.
Configure Mod includes class and accessory guides. For original English FF7 PC.

INSTALL / CONFIGURE
1. Import FFVIIJobSystem-1.0.0.iro as the update to FFVII Job System.
2. Open Configure Mod. Choose one class for each of the nine characters.
3. The selected class shows its role, full Materia list and accessory suggestions.
4. Select Accessory reference to browse all 32 items, their effects, suggested
   classes and the reason for each assignment. Resize the window for larger text.
5. Save the configuration, then launch the game yourself.

Every character can choose any of the same 25 classes. Characters can share
a class. Upgrading from 0.7.0 preserves class choices because all nine setting
IDs and their values are unchanged. Earlier fixed-class versions use the new
documented defaults. Barret defaults to Paladin and Cait Sith to Gambler.
Existing saved class choices are retained; use Configure Mod to change them.

ACCESSORY INFORMATION
The class-based accessory assignments are INFORMATION ONLY in this release.
Actual accessory access and icons still follow the previous character rules.
The reference dropdown only chooses which guide to display; it does not change
accessory permissions. This status is also stated on every information preview.
See CLASS_PERMISSIONS.txt for active Materia pools, current character accessory
permissions, all class descriptions and the complete accessory proposal.

MATERIA RULES
Materia permissions and weapon icons follow the selected classes. Icons
identify characters using the active Items menu weapon artwork. One to five
icons use normal size; six to nine use compact size. Both sizes have tighter
spacing. Complete long names condense horizontally only when needed to fit.
Materia with no eligible selected character remain visible, without icons,
and cannot be equipped. This is expected with the defaults because nine
selected classes do not cover every Materia in the 25-class catalogue.

CHANGING CLASSES
Changes apply on the next launch. Previously equipped forbidden Materia
return to inventory with their AP. When inventory is full they are preserved
in their slots but inactive during stat/command calculations until space is
available. Empty slots and the flashback guest retain their normal behavior.

COMPATIBILITY
This version declares FFVIIJobs.dll through 7th Heaven's supported
LoadLibrary mechanism. 7th Heaven displays its normal executable-code warning
when the mod is activated. Only approve the mod if you trust its author.
There is no HEXT bootstrap or separately loaded executable binary payload.
7th Heaven's security settings and consent mechanism are unchanged.

The supported target is the original English Steam FF7 PC 1.02 executable.
Private checks use installed 7th Heaven 4.6.1 and Enhanced Stock UI 2.968.
All class pools, defaults, weapon icons, accessory rules and saved class
setting IDs are retained. Current permissions are in CLASS_PERMISSIONS.txt.
Class selections load after virtual file handlers are ready; the guarded ESUI
font-permission repair runs during DLL initialization before font setup.
Other gameplay overhauls that change Materia IDs or the same executable hooks
need their own compatibility review. All sixteen hook signatures must match
before installation. This corrected build still needs an in-game test.

No import, activation, mod-order change or game launch was performed.

SOURCE / REBUILD
source/rules.json contains active shared Materia classes and character defaults.
source/class_descriptions.json contains the 25 class explanations.
ACCESSORY_PROPOSAL.json contains informational class assignments and effects.
CLASS_REFERENCE.json records the approved Materia pools, including the
playability adaptations in source/approved_changes.json.
REFERENCE.md retains the original character accessory rules.

Build requires Python 3, Pillow, pefile, Windows Segoe UI fonts and Zig 0.17.1:
  py -m pip install Pillow pefile
  py source/build.py --zig C:/path/to/zig.exe
  py source/package.py
To reuse the existing DLL while regenerating configuration guides:
  py source/build.py --skip-native
To build and run the private native fixture:
  py source/build.py --zig C:/path/to/zig.exe --test-output work/jobs-tests

source/ffvii_jobs.c builds a standard x86 Windows DLL. The compiled DLL is
distributed inside the IRO; intermediate PDB/LIB files are not packaged.
source/test_loader.cs exercises the installed 7H library extraction and
virtual files with the actual production DLL. Run against private copies of
installed 7H assemblies to keep LoadLibTemp inside the test work directory.
source/test_configure.cs exercises the installed Configure Mod view model.
Both run in private x86 .NET 10 hosts. No test imports a mod, saves a profile
or launches FF7. source/audit_package.py checks the standard loading contract.

See VALIDATION.txt for results and source/DLL_LOADING.md for the load path.
See PUBLISHING.txt for the separate public catalogue submission process.
