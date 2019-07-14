@echo off

REM Microsoft VisualStudio 2013 Community Edition

REM StartMenu > Visual Studio 2013 > Visual Studio Tools > VS2013 x86 Native Tools Command Prompt

cl /GS /GL /analyze- /W3 /Gy /Zc:wchar_t /Zi /Gm- /O2 /sdl /fp:precise /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "JINTER_EXPORTS" /D "_WINDLL" /D "_MBCS" /errorReport:prompt /WX- /Zc:forScope /Gd /Oy- /Oi /MD /EHsc /nologo jinter.c /link /OUT:"jinter.dll" /LTCG /NXCOMPAT /DYNAMICBASE "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" /DEF:"jinter.def" /DLL /MACHINE:X86 /OPT:REF /SAFESEH /INCREMENTAL:NO /SUBSYSTEM:WINDOWS /OPT:ICF /ERRORREPORT:PROMPT /NOLOGO /TLBID:1 

del jinter.exp
del jinter.lib
del jinter.pdb
del jinter.obj
del vc120.pdb
