# Microsoft Developer Studio Project File - Name="IParse" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=IParse - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "IParse.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "IParse.mak" CFG="IParse - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "IParse - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "IParse - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "IParse - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "IParse - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "IParse - Win32 Release"
# Name "IParse - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AbstractParser.cpp
# End Source File
# Begin Source File

SOURCE=.\AbstractParseTree.cpp
# End Source File
# Begin Source File

SOURCE=.\BTHeapParser.cpp
# End Source File
# Begin Source File

SOURCE=.\BTParser.cpp
# End Source File
# Begin Source File

SOURCE=.\CodePages.cpp
# End Source File
# Begin Source File

SOURCE=.\Ident.cpp
# End Source File
# Begin Source File

SOURCE=.\IParse.cpp
# End Source File
# Begin Source File

SOURCE=.\LL1HeapParser.cpp
# End Source File
# Begin Source File

SOURCE=.\LL1Parser.cpp
# End Source File
# Begin Source File

SOURCE=.\ParParser.cpp
# End Source File
# Begin Source File

SOURCE=.\ParserGrammar.cpp
# End Source File
# Begin Source File

SOURCE=.\ProtosScanner.cpp
# End Source File
# Begin Source File

SOURCE=.\RcScanner.cpp
# End Source File
# Begin Source File

SOURCE=.\Scanner.cpp
# End Source File
# Begin Source File

SOURCE=.\Streams.cpp
# End Source File
# Begin Source File

SOURCE=.\TextFileBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\TextReader.cpp
# End Source File
# Begin Source File

SOURCE=.\Unparser.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLParser.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AbstractParser.h
# End Source File
# Begin Source File

SOURCE=.\AbstractParseTree.h
# End Source File
# Begin Source File

SOURCE=.\BTHeapParser.h
# End Source File
# Begin Source File

SOURCE=.\BTParser.h
# End Source File
# Begin Source File

SOURCE=.\CodePages.h
# End Source File
# Begin Source File

SOURCE=.\Ident.h
# End Source File
# Begin Source File

SOURCE=.\LL1HeapParser.h
# End Source File
# Begin Source File

SOURCE=.\LL1Parser.h
# End Source File
# Begin Source File

SOURCE=.\ParParser.h
# End Source File
# Begin Source File

SOURCE=.\ParserGrammar.h
# End Source File
# Begin Source File

SOURCE=.\ParseSolution.h
# End Source File
# Begin Source File

SOURCE=.\ProtosScanner.h
# End Source File
# Begin Source File

SOURCE=.\RcScanner.h
# End Source File
# Begin Source File

SOURCE=.\Scanner.h
# End Source File
# Begin Source File

SOURCE=.\Streams.h
# End Source File
# Begin Source File

SOURCE=.\TextFileBuffer.h
# End Source File
# Begin Source File

SOURCE=.\TextFilePos.h
# End Source File
# Begin Source File

SOURCE=.\TextReader.h
# End Source File
# Begin Source File

SOURCE=.\Unparser.h
# End Source File
# Begin Source File

SOURCE=.\XMLParser.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
