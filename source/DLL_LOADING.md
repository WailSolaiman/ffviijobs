# Supported 7th Heaven DLL loading

The package declares `<LoadLibrary>FFVIIJobs.dll</LoadLibrary>` in `mod.xml`.
7th Heaven detects this declaration and displays its normal code-consent
warning before activation. Users decide whether to allow the mod.

7th Heaven extracts the declared DLL from the IRO and the Windows loader
loads its normal x86 PE image. There is no HEXT entry, executable `.bin`,
custom relocation loader, WinMain startup patch or executable-padding payload.
The previous HEXT implementation remains in Git history for review.

`DllMain` initializes the documented defaults and installs the sixteen
signature-guarded gameplay/menu hooks. Class marker reads are deferred until
the first gameplay or menu query, after 7th Heaven's virtual file handlers
are active. The existing guarded ESUI font-permission repair runs at DLL
initialization, before game font setup. No other mod's code or art is bundled.

The native fixture compiles the same implementation with a test entry point.
The integration fixture additionally exercises the production DLL through
7th Heaven's actual IRO extraction, Windows LoadLibrary and virtual file APIs.
Run it against private copies of the installed 7H assemblies: the standard
library extraction API writes its LoadLibTemp folder beside AppWrapper.dll.
Neither fixture starts FF7, imports a mod or saves application profiles.
