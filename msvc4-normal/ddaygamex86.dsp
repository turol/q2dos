# Microsoft Developer Studio Project File - Name="ddaygamex86" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ddaygamex86 - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ddaygamex86.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ddaygamex86.mak" CFG="ddaygamex86 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ddaygamex86 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ddaygamex86 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ddaygamex86 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\dday\Release"
# PROP Intermediate_Dir ".\dday\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "id386=1" /D "NDEBUG" /D "_WINDOWS" /D "WITH_ACEBOT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x1009 /d "NDEBUG"
# ADD RSC /l 0x1009 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:".\dday\Release/gamex86.dll"

!ELSEIF  "$(CFG)" == "ddaygamex86 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\dday\Debug"
# PROP Intermediate_Dir ".\dday\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x1009 /d "_DEBUG"
# ADD RSC /l 0x1009 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\dday\Debug/gamex86.dll"

!ENDIF 

# Begin Target

# Name "ddaygamex86 - Win32 Release"
# Name "ddaygamex86 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Group "ai"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\dday\ai\ai_class_dmbot.c
# End Source File
# Begin Source File

SOURCE=..\dday\ai\ai_class_monster_default.c
# End Source File
# Begin Source File

SOURCE=..\dday\ai\ai_dropnodes.c
# End Source File
# Begin Source File

SOURCE=..\dday\ai\ai_items.c
# End Source File
# Begin Source File

SOURCE=..\dday\ai\ai_links.c
# End Source File
# Begin Source File

SOURCE=..\dday\ai\ai_main.c
# End Source File
# Begin Source File

SOURCE=..\dday\ai\ai_movement.c
# End Source File
# Begin Source File

SOURCE=..\dday\ai\ai_navigation.c
# End Source File
# Begin Source File

SOURCE=..\dday\ai\ai_nodes.c
# End Source File
# Begin Source File

SOURCE=..\dday\ai\ai_tools.c
# End Source File
# Begin Source File

SOURCE=..\dday\ai\ai_weapons.c
# End Source File
# Begin Source File

SOURCE=..\dday\ai\AStar.c
# End Source File
# Begin Source File

SOURCE=..\dday\ai\bot_common.c
# End Source File
# Begin Source File

SOURCE=..\dday\ai\bot_spawn.c
# End Source File
# End Group
# Begin Group "gbr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\dday\gbr\gbr_guninfo.c
# End Source File
# Begin Source File

SOURCE=..\dday\gbr\gbr_item.c
# End Source File
# Begin Source File

SOURCE=..\dday\gbr\gbr_main.c
# End Source File
# Begin Source File

SOURCE=..\dday\gbr\gbr_spawn.c
# End Source File
# Begin Source File

SOURCE=..\dday\gbr\gbr_weapon.c
# End Source File
# End Group
# Begin Group "grm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\dday\grm\grm_guninfo.c
# End Source File
# Begin Source File

SOURCE=..\dday\grm\grm_item.c
# End Source File
# Begin Source File

SOURCE=..\dday\grm\grm_main.c
# End Source File
# Begin Source File

SOURCE=..\dday\grm\grm_spawn.c
# End Source File
# Begin Source File

SOURCE=..\dday\grm\grm_weapon.c
# End Source File
# End Group
# Begin Group "ita"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\dday\ita\ita_guninfo.c
# End Source File
# Begin Source File

SOURCE=..\dday\ita\ita_item.c
# End Source File
# Begin Source File

SOURCE=..\dday\ita\ita_main.c
# End Source File
# Begin Source File

SOURCE=..\dday\ita\ita_spawn.c
# End Source File
# Begin Source File

SOURCE=..\dday\ita\ita_weapon.c
# End Source File
# End Group
# Begin Group "jpn"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\dday\jpn\jpn_guninfo.c
# End Source File
# Begin Source File

SOURCE=..\dday\jpn\jpn_item.c
# End Source File
# Begin Source File

SOURCE=..\dday\jpn\jpn_main.c
# End Source File
# Begin Source File

SOURCE=..\dday\jpn\jpn_spawn.c
# End Source File
# Begin Source File

SOURCE=..\dday\jpn\jpn_weapon.c
# End Source File
# End Group
# Begin Group "pol"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\dday\pol\pol_guninfo.c
# End Source File
# Begin Source File

SOURCE=..\dday\pol\pol_item.c
# End Source File
# Begin Source File

SOURCE=..\dday\pol\pol_main.c
# End Source File
# Begin Source File

SOURCE=..\dday\pol\pol_spawn.c
# End Source File
# Begin Source File

SOURCE=..\dday\pol\pol_weapon.c
# End Source File
# End Group
# Begin Group "rus"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\dday\rus\rus_guninfo.c
# End Source File
# Begin Source File

SOURCE=..\dday\rus\rus_item.c
# End Source File
# Begin Source File

SOURCE=..\dday\rus\rus_main.c
# End Source File
# Begin Source File

SOURCE=..\dday\rus\rus_spawn.c
# End Source File
# Begin Source File

SOURCE=..\dday\rus\rus_weapon.c
# End Source File
# End Group
# Begin Group "usa"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\dday\usa\usa_guninfo.c
# End Source File
# Begin Source File

SOURCE=..\dday\usa\usa_item.c
# End Source File
# Begin Source File

SOURCE=..\dday\usa\usa_main.c
# End Source File
# Begin Source File

SOURCE=..\dday\usa\usa_spawn.c
# End Source File
# Begin Source File

SOURCE=..\dday\usa\usa_weapon.c
# End Source File
# End Group
# Begin Group "usm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\dday\usm\usm_guninfo.c
# End Source File
# Begin Source File

SOURCE=..\dday\usm\usm_item.c
# End Source File
# Begin Source File

SOURCE=..\dday\usm\usm_main.c
# End Source File
# Begin Source File

SOURCE=..\dday\usm\usm_spawn.c
# End Source File
# Begin Source File

SOURCE=..\dday\usm\usm_weapon.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\dday\g_ai.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_arty.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_cmds.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_combat.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_ents.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_func.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_items.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_main.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_maps.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_misc.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_monster.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_objectives.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_phys.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_save.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_spawn.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_svcmds.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_target.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_trigger.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_turret.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_utils.c
# End Source File
# Begin Source File

SOURCE=..\dday\g_weapon.c
# End Source File
# Begin Source File

SOURCE=..\dday\game.def
# End Source File
# Begin Source File

SOURCE=..\dday\m_actor.c
# End Source File
# Begin Source File

SOURCE=..\dday\m_civilian.c
# End Source File
# Begin Source File

SOURCE=..\dday\m_flash.c
# End Source File
# Begin Source File

SOURCE=..\dday\m_insane.c
# End Source File
# Begin Source File

SOURCE=..\dday\m_move.c
# End Source File
# Begin Source File

SOURCE=..\dday\p_classes.c
# End Source File
# Begin Source File

SOURCE=..\dday\p_client.c
# End Source File
# Begin Source File

SOURCE=..\dday\p_generic_wep.c
# End Source File
# Begin Source File

SOURCE=..\dday\p_hud.c
# End Source File
# Begin Source File

SOURCE=..\dday\p_menu.c
# End Source File
# Begin Source File

SOURCE=..\dday\p_observer.c
# End Source File
# Begin Source File

SOURCE=..\dday\p_trail.c
# End Source File
# Begin Source File

SOURCE=..\dday\p_view.c
# End Source File
# Begin Source File

SOURCE=..\dday\p_weapon.c
# End Source File
# Begin Source File

SOURCE=..\dday\q_shared.c
# End Source File
# Begin Source File

SOURCE=..\dday\u_entmgr.c
# End Source File
# Begin Source File

SOURCE=..\dday\x_fbomb.c
# End Source File
# Begin Source File

SOURCE=..\dday\x_fire.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
