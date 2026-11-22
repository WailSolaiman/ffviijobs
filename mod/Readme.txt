FFVII JOB SYSTEM 0.7.2 (shown as 0.72 in 7th Heaven)
Author: Willard82
Project: https://github.com/WailSolaiman/ffviijobs
Optional tips: https://ko-fi.com/ffviijobs

Choose one of 25 Final Fantasy jobs for each character. Your choices decide
which Materia they can equip, with weapon icons showing eligible characters.
Configure Mod includes class and accessory guides. For original English FF7 PC.

INSTALL / CONFIGURE
1. Import FFVIIJobSystem-0.7.2.iro as the update to FFVII Job System.
2. Open Configure Mod. Choose one class for each of the nine characters.
3. The selected class shows its role, full Materia list and accessory suggestions.
4. Select Accessory reference to browse all 32 items, their effects, suggested
   classes and the reason for each assignment. Resize the window for larger text.
5. Save the configuration, then launch the game yourself.

Every character can choose any of the same 25 classes. Characters can share
a class. Upgrading from 0.7.0 preserves class choices because all nine setting
IDs and their values are unchanged. Earlier fixed-class versions use the new
documented defaults.

ACCESSORY INFORMATION
The class-based accessory assignments are INFORMATION ONLY in this release.
Actual accessory access and icons still follow the previous character rules.
The reference dropdown only chooses which guide to display; it does not change
accessory permissions. This status is also stated on every information preview.
See CLASS_PERMISSIONS.txt for active Materia pools, current character accessory
permissions, all class descriptions and the complete accessory proposal.

MATERIA RULES
Materia permissions and weapon icons follow the selected classes. Icons
identify characters using the active Items menu weapon artwork. When five
to nine characters can equip a Materia, smaller icons keep them all visible.
Materia with no eligible selected character remain visible, without icons,
and cannot be equipped. This is expected with the defaults because nine
selected classes do not cover every Materia in the 25-class catalogue.

CHANGING CLASSES
Changes apply on the next launch. Previously equipped forbidden Materia
return to inventory with their AP. When inventory is full they are preserved
in their slots but inactive during stat/command calculations until space is
available. Empty slots and the flashback guest retain their normal behavior.

COMPATIBILITY
Gameplay core is unchanged from 0.7.0. The 0.7.2 update adds public release metadata, support links and the mod preview. Original English FF7 PC 1.02 with 7th
Heaven native LoadLibrary support. Configuration previews were verified with
the installed 7th Heaven 4.6.1 UI code. Prior stock/Enhanced Stock UI behavior
is retained. The gameplay core reads selections after virtual file hooks are
ready; it does not rewrite profiles or copy another mod's artwork.
Other gameplay overhauls that change Materia IDs or the same executable hook
sites need their own compatibility review. All sixteen hook signatures must
match before installation; conflicting hooks prevent partial patching.
This release retains 7th Heaven's existing native-code trust prompt.

No import, activation, mod-order change or game launch was performed.

SOURCE / REBUILD
source/rules.json contains active shared Materia classes and character defaults.
source/class_descriptions.json contains the 25 class explanations.
ACCESSORY_PROPOSAL.json contains informational class assignments and effects.
CLASS_REFERENCE.json records the exact approved research Materia pools.
REFERENCE.md retains the original character accessory rules.

Build requires Python 3, Pillow, Windows Segoe UI fonts and Zig 0.17.1:
  py source/build.py --zig C:/path/to/zig.exe
An information-only rebuild can reuse an already built native DLL:
  py source/build.py --skip-native
Package:
  py source/package.py

source/test_configure.cs exercises the actual installed Configure Mod view
model in a private STA host, without opening or controlling the application.
source/test_loader.cs checks the installed 7H archive/configuration/native path.
Their runtimeconfig.json examples are included. Tests must use a private work
folder; no test or build should import the mod, save profiles, or launch FF7.
