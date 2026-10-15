/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas

This program is free software; you can redistribute it and/or
modify it under the terms of the AROS Public License version 1.1.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
AROS Public License for more details.

The release of Directory Opus 5 under the GPL in NO WAY affects
the existing commercial status of Directory Opus for Windows.

For more information on Directory Opus for Windows please see:

				 http://www.gpsoft.com.au

*/

#ifndef STRING_DATA_H
#define STRING_DATA_H

/****************************************************************************/

/* This file was created automatically by CatComp.
 * Do NOT edit by hand!
 */

#ifndef EXEC_TYPES_H
	#include <exec/types.h>
#endif

#ifdef CATCOMP_ARRAY
	#undef CATCOMP_NUMBERS
	#undef CATCOMP_STRINGS
	#define CATCOMP_NUMBERS
	#define CATCOMP_STRINGS
#endif

#ifdef CATCOMP_BLOCK
	#undef CATCOMP_STRINGS
	#define CATCOMP_STRINGS
#endif

/****************************************************************************/

#ifdef CATCOMP_NUMBERS

	#define MSG_ABORTED 0
	#define MSG_TRY_AGAIN 1
	#define MSG_RETRY 2
	#define MSG_ABORT 3
	#define MSG_REPLACE 4
	#define MSG_SKIP 5
	#define MSG_SKIP_ALL 6
	#define MSG_ALL 7
	#define MSG_QUIT 8
	#define MSG_SAVE 9
	#define MSG_CLOSE 10
	#define MSG_YES 11
	#define MSG_NO 12
	#define MSG_CONTINUE 13
	#define MSG_OKAY 14
	#define MSG_CANCEL 15
	#define MSG_REPLACE_ALL 16
	#define MSG_PROCEED 17
	#define MSG_MOVE 18
	#define MSG_DISCARD 19
	#define MSG_BLOCKS 20
	#define MSG_FREE 21
	#define MSG_MAKELINK 22
	#define MSG_SELECT_FILE 23
	#define MSG_SELECT_DIR 24
	#define MSG_ENTER_FILE 25
	#define MSG_OPEN 26
	#define MSG_DEFAULTS 27
	#define MSG_LAST_SAVED 28
	#define MSG_RESTORE 29
	#define MSG_PROJECT 30
	#define MSG_EDIT 31
	#define MSG_UNKNOWN_TYPE 32
	#define MSG_SNIFF_CONFIRMATION 33
	#define MSG_SNIFF 34
	#define MSG_SCRIPTS_CHANGED 35
	#define MSG_LISTER_MENU_CHANGED 36
	#define MSG_USER_MENU_CHANGED 37
	#define MSG_HOTKEYS_CHANGED 38
	#define MSG_OPEN_NEW_WINDOW 39
	#define MSG_OPEN_WITH_MENU 40
	#define MSG_SELECT_APP 41
	#define MSG_OPEN_WITH_MENU_SUB 42
	#define MSG_OPEN_WITH_MENU_OTHER 43
	#define MSG_ENTER_ARGUMENTS_FOR 1000
	#define MSG_DIRECTORY 1001
	#define MSG_SELECT_UNPROTECT 1002
	#define MSG_FOUND_A_MATCH 1003
	#define MSG_COMMENTING 1004
	#define MSG_PROTECTING 1005
	#define MSG_DATESTAMPING 1006
	#define MSG_DELETING 1007
	#define MSG_RENAMING 1008
	#define MSG_ENTER_COMMENT 1009
	#define MSG_ENTER_PASSWORD 1010
	#define MSG_ENTER_DATE_AND_TIME 1011
	#define MSG_ENTER_ARGUMENTS 1012
	#define MSG_REALLY_QUIT 1013
	#define MSG_FILE 1014
	#define MSG_SELECT_PROTECTION_BITS 1015
	#define MSG_DIRECTORY_OPUS_REQUEST 1016
	#define MSG_MULTI_DRAG 1020
	#define MSG_PRESS_MOUSE_BUTTON 1021
	#define MSG_COMMENT 1022
	#define MSG_PROTECT 1023
	#define MSG_DATESTAMP 1024
	#define MSG_PROGRESS_OPERATION_MOVING 1025
	#define MSG_PROGRAM_LOADING 1026
	#define MSG_OPEN_MENU 1027
	#define MSG_SAVE_MENU 1028
	#define MSG_SAVEAS_MENU 1029
	#define MSG_ACT_RECURSIVELY 1030
	#define MSG_PROGRESS_OPERATION_COMMENT 1031
	#define MSG_PROGRESS_OPERATION_PROTECT 1032
	#define MSG_PROGRESS_OPERATION_DATESTAMP 1033
	#define MSG_SET_PROTECT_OLD 1034
	#define MSG_SET_PROTECT_SET 1035
	#define MSG_SET_PROTECT_CLEAR 1036
	#define MSG_OK_BUTTON 1037
	#define MSG_ALL_BUTTON 1038
	#define MSG_SKIP_BUTTON 1039
	#define MSG_ABORT_BUTTON 1040
	#define MSG_PROGRESS_OPERATION_ENCRYPTING 1041
	#define MSG_USER_MENU 1042
	#define MSG_LISTER_MENU_TITLE 1043
	#define MSG_USER_MENU_TITLE 1044
	#define MSG_EMPTY 1045
	#define MSG_CANCEL_BUTTON 1046
	#define MSG_OPEN_BUTTON 1047
	#define MSG_NEW_BUTTON 1048
	#define MSG_PARENT 1049
	#define MSG_ROOT 1050
	#define MSG_DEVICE_LIST 1051
	#define MSG_SCANNING_DIRECTORIES 1052
	#define MSG_READING_DIRECTORY 1053
	#define MSG_CHECK_FIT_RESULTS 1054
	#define MSG_FIT 1055
	#define MSG_BLOCKS_NEEDED 1056
	#define MSG_EXECUTE_ENTER_COMMAND 1057
	#define MSG_TOOL_MENU 1058
	#define MSG_BUFFER_LIST 1059
	#define MSG_REREAD_DIR 1060
	#define MSG_ASSIGN 1061
	#define MSG_FULL 1062
	#define MSG_USED 1063
	#define MSG_VALIDATING 1064
	#define MSG_ENTER_FILTER_STRING 1065
	#define MSG_VOLUME 1066
	#define MSG_SELECT_SOURCE 1067
	#define MSG_SELECT_DESTINATION 1068
	#define MSG_NO_OWNER 1069
	#define MSG_NO_GROUP 1070
	#define MSG_SCRIPTS_TITLE 1071
	#define MSG_PROGRESS_OPERATION_LINKING 1072
	#define MSG_LINKING 1073
	#define MSG_DECRYPT 1074
	#define MSG_EMPTY_TRASH 1075
	#define MSG_REALLY_EMPTY_TRASH 1076
	#define MSG_ENTER_ASSIGN_NAME 1077
	#define MSG_ADD 1078
	#define MSG_PATH 1079
	#define MSG_DEFER 1080
	#define MSG_ASSIGNING 1081
	#define MSG_CHANGE_NEEDS_RESET 1082
	#define MSG_OPENING 1083
	#define MSG_LIBRARY_ERROR 1084
	#define MSG_LIBRARY_ANY_VERSION 1085
	#define MSG_CLI 1086
	#define MSG_CX_DESC 1087
	#define MSG_ABORT_STARTUP 1088
	#define MSG_ABORT_STARTUP_BUTTONS 1089
	#define MSG_PROTECT_TITLE 1090
	#define MSG_DATESTAMP_TITLE 1091
	#define MSG_COMMENT_TITLE 1092
	#define MSG_PATTERN_OFF 1093
	#define MSG_THEMES_MENU 1094
	#define MSG_THEMES_LOAD 1095
	#define MSG_THEMES_SAVE 1096
	#define MSG_THEMES_BUILD 1097
	#define MSG_ADDICONING 1098
	#define MSG_REGISTERED_TO 2000
	#define MSG_SERIAL_NUMBER 2001
	#define MSG_UNREGISTERED 2002
	#define MSG_YOU_SHOULD_REGISTER 2003
	#define MSG_TRANSLATION_BY_YOUR_NAME 2004
	#define MSG_SEARCH_DIRECTORIES 2100
	#define MSG_ENTER_SEARCH_STRING 2101
	#define MSG_SEARCH_NO_CASE 2102
	#define MSG_SEARCH_WILD 2103
	#define MSG_SEARCH_ONLYWORD 2104
	#define MSG_SEARCH_LEAVE_SELECTED 2105
	#define MSG_SEARCH_PROMPT 2106
	#define MSG_SEARCH_OUTPUT 2107
	#define MSG_PROGRESS_SEARCHING 2108
	#define MSG_SEARCH_OUTPUT_HEADER 2109
	#define MSG_SEARCH_READ 2110
	#define MSG_ENTER_NEW_NAME 2200
	#define MSG_PROGRESS_RENAMING 2201
	#define MSG_RENAME 2202
	#define MSG_RENAME_DISK 2203
	#define MSG_ENTER_ANOTHER_NAME 2204
	#define MSG_ENTER_DIRECTORY_NAME 2300
	#define MSG_MAKEDIR_ICON 2301
	#define MSG_MAKEDIR_NOICON 2302
	#define MSG_DIRECTORY_CREATED 2303
	#define MSG_ENTER_FILE_NAME 2304
	#define MSG_FILE_CREATED 2305
	#define MSG_PROGRESS_ADDING_ICONS 2400
	#define MSG_ICON_ALREADY_EXISTS 2401
	#define MSG_ICON_REPLACE_IMAGE 2402
	#define MSG_COPYING 2600
	#define MSG_PROGRESS_OPERATION_COPYING 2601
	#define MSG_CANT_OVERCOPY_FILES 2602
	#define MSG_COPY 2603
	#define MSG_PROGRESS_OPERATION_CLONING 2604
	#define MSG_DUPLICATE 2605
	#define MSG_CANT_COPY_DIR_INTO_ITSELF 2606
	#define MSG_MOVING 2607
	#define MSG_CANT_OVERLINK_FILES 2608
	#define MSG_CANT_LINK_DIR_INTO_ITSELF 2609
	#define MSG_FROM_TO 2610
	#define MSG_FROM 2611
	#define MSG_PROJECT_MENU 2700
	#define MSG_BACKDROP_MENU 2701
	#define MSG_ABOUT_MENU 2702
	#define MSG_HIDE_MENU 2703
	#define MSG_QUIT_MENU 2704
	#define MSG_EXECUTE_MENU 2705
	#define MSG_HELP_MENU 2706
	#define MSG_KEYFINDER_MENU 2707
	#define MSG_NEWSHELL_MENU 2708
	#define MSG_WINDOW_MENU 2800
	#define MSG_NEW_LISTER_MENU 2801
	#define MSG_CLOSE_LISTER_MENU 2802
	#define MSG_LISTER_SOURCE 2803
	#define MSG_LISTER_DEST 2804
	#define MSG_LISTER_LOCK_SOURCE 2805
	#define MSG_LISTER_LOCK_DEST 2806
	#define MSG_LISTER_UNLOCK 2807
	#define MSG_LISTER_OFF 2808
	#define MSG_LISTER_UNLOCK_ALL 2809
	#define MSG_CLOSE_ALL_MENU 2810
	#define MSG_EDIT_MENU 2811
	#define MSG_LISTER_BUTTONS_MENU 2812
	#define MSG_LISTER_MENU_MENU 2813
	#define MSG_TILE_MENU 2814
	#define MSG_TILE_HORIZ_MENU 2815
	#define MSG_TILE_VERT_MENU 2816
	#define MSG_CASCADE_MENU 2817
	#define MSG_LISTER_LOCK_POS 2818
	#define MSG_LISTER_ICONIFY 2819
	#define MSG_LISTER_VIEW_ICONS 2820
	#define MSG_LISTER_VIEW 2821
	#define MSG_LISTER_VIEW_NAME 2822
	#define MSG_LISTER_VIEW_ICON 2823
	#define MSG_LISTER_SHOW_ALL 2824
	#define MSG_LISTER_ICON_ACTION 2825
	#define MSG_LISTER_ARRANGE_ICONS 2826
	#define MSG_LISTER_ARRANGE_NAME 2827
	#define MSG_LISTER_ARRANGE_TYPE 2828
	#define MSG_LISTER_ARRANGE_SIZE 2829
	#define MSG_LISTER_ARRANGE_DATE 2830
	#define MSG_LISTER_NEW_DRAWER 2831
	#define MSG_OPEN_PARENT_MENU 2832
	#define MSG_LISTER_RESTORE 2833
	#define MSG_LISTER_RESIZE_FIT 2834
	#define MSG_LISTER_DUAL 2835
	#define MSG_LISTER_NEW_FILE 2836
	#define MSG_ICONS_MENU 2900
	#define MSG_ICON_OPEN_MENU 2901
	#define MSG_ICON_INFO_MENU 2902
	#define MSG_ICON_SNAPSHOT_MENU 2903
	#define MSG_ICON_LEAVE_OUT_MENU 2904
	#define MSG_ICON_PUT_AWAY_MENU 2905
	#define MSG_ICON_SELECT_ALL_MENU 2906
	#define MSG_ICON_CLEANUP 2907
	#define MSG_ICON_RESET 2908
	#define MSG_ICON_RENAME 2909
	#define MSG_ICON_FORMAT 2910
	#define MSG_ICON_DISKINFO 2911
	#define MSG_ICON_UNSNAPSHOT_MENU 2912
	#define MSG_ICON_COPY 2914
	#define MSG_ICONS_NEW_MENU 2915
	#define MSG_ICONS_MAKEDIR_MENU 2916
	#define MSG_ICONS_NEWGROUP_MENU 2917
	#define MSG_ICON_COPY_RAM 2918
	#define MSG_ICON_COPY_DF0 2919
	#define MSG_ICON_COPY_OTHER 2920
	#define MSG_ICON_COPY2 2921
	#define MSG_ICON_COPY_TO 2922
	#define MSG_ICONS_NEWCOMMAND_MENU 2923
	#define MSG_ICONS_SHORTCUT_MENU 2924
	#define MSG_DESKTOP_POPUP_LEFTOUT 2925
	#define MSG_DESKTOP_POPUP_COPY 2926
	#define MSG_DESKTOP_POPUP_MOVE 2927
	#define MSG_ICON_COPY_DESKTOP 2928
	#define MSG_ICON_LINEUP 2929
	#define MSG_ICON_SNAPSHOT_LISTER 2930
	#define MSG_ICON_SNAPSHOT_ICONS 2931
	#define MSG_ICON_SNAPSHOT_ALL 2932
	#define MSG_ICON_SNAPSHOT_WINDOW 2933
	#define MSG_ICONS_MAKEFILE_MENU 2934
	#define MSG_BUTTONS_MENU 3000
	#define MSG_CLOSE_BUTTONS_MENU 3002
	#define MSG_BUTTONS_NEW_GFX_MENU 3006
	#define MSG_BUTTONS_NEW_TEXT_MENU 3007
	#define MSG_BUTTONS_NEW_STARTMENU_MENU 3008
	#define MSG_SETTINGS_MENU 3100
	#define MSG_CLOCK_MENU 3101
	#define MSG_CREATE_ICONS_MENU 3102
	#define MSG_ENVIRONMENT_MENU 3103
	#define MSG_FILETYPES_MENU 3104
	#define MSG_HOTKEYS_MENU 3105
	#define MSG_MENU_MENU 3106
	#define MSG_FILTER_MENU 3107
	#define MSG_OPTIONS_MENU 3108
	#define MSG_DEFPUBSCR_MENU 3110
	#define MSG_MENU_HOTKEYS 3112
	#define MSG_HOTKEYS_TITLE 3113
	#define MSG_MENU_SCRIPTS 3114
	#define MSG_MENU_SYSMENU 3115
	#define MSG_MENU_ICONPOS 3116
	#define MSG_ENVIRONMENT_SAVE_LAYOUT_NEW 3117
	#define MSG_ENVIRONMENT_EDIT 3118
	#define MSG_ENVIRONMENT_LOAD_NEW 3119
	#define MSG_ENVIRONMENT_SAVE_NEW 3120
	#define MSG_SELECT_TITLE 3200
	#define MSG_SELECT_IGNORE 3201
	#define MSG_SELECT_MATCH 3202
	#define MSG_SELECT_NOMATCH 3203
	#define MSG_SELECT_NEWER 3204
	#define MSG_SELECT_OLDER 3205
	#define MSG_SELECT_DIFFERENT 3206
	#define MSG_SELECT_NAME 3207
	#define MSG_SELECT_DATE_FROM 3208
	#define MSG_SELECT_DATE_TO 3209
	#define MSG_SELECT_PROT 3210
	#define MSG_SELECT_INCLUDE 3211
	#define MSG_SELECT_EXCLUDE 3212
	#define MSG_SELECT_COMPARE 3213
	#define MSG_SELECT_SIMPLE 3214
	#define MSG_SELECT_COMPLEX 3215
	#define MSG_SELECT_ENTER_PATTERN 3216
	#define MSG_SELECT_PROT_OFF 3217
	#define MSG_SELECT_ENTRY_BOTH 3218
	#define MSG_SELECT_ENTRY_FILES 3219
	#define MSG_SELECT_ENTRY_DIRS 3220
	#define MSG_SELECT_ENTRY_TYPE 3221
	#define MSG_DOS_ERROR 3300
	#define MSG_DOS_ERROR_CODE 3301
	#define MSG_ERROR_OCCURED 3302
	#define MSG_ERROR_DEST_IS_DIRECTORY 3303
	#define MSG_ERROR_DEST_IS_FILE 3304
	#define MSG_ERROR_UNKNOWN_CODE 3306
	#define MSG_BUTTONS_ERROR_SAVING 3307
	#define MSG_BUTTONS_ERROR_LOADING 3308
	#define MSG_ERROR_CREATING_DIRECTORY 3309
	#define MSG_CUSTPORT_NOT_FOUND 3310
	#define MSG_ALREADY_RUNNING 3311
	#define MSG_ALREADY_RUNNING_BUTTONS 3312
	#define MSG_UNABLE_TO_CLOSE_SCREEN 3313
	#define MSG_ERROR_INVALID_DATE 3314
	#define MSG_UNABLE_TO_OPEN_SCREEN 3315
	#define MSG_AN_ERROR_OCCURED 3316
	#define MSG_ERROR 3317
	#define MSG_SIZE 3318
	#define MSG_DATE 3319
	#define MSG_OLD 3320
	#define MSG_NEW 3321
	#define MSG_FILE_EXISTS 3322
	#define MSG_ERROR_CREATING_FILE 3323
	#define MSG_PROGRESS_OPERATION_DELETING 3400
	#define MSG_DELETE_CONFIRM 3401
	#define MSG_DELETE 3402
	#define MSG_DELETE_ALL 3403
	#define MSG_DELETE_FILE_CONFIRM 3404
	#define MSG_DELETE_DIR_CONFIRM 3405
	#define MSG_DELETE_PROTECTED 3406
	#define MSG_UNPROTECT 3407
	#define MSG_UNPROTECT_ALL 3408
	#define MSG_DELETE_LINK_CONFIRM_SINGLE 3409
	#define MSG_DELETE_LINK_CONFIRM_MULTI 3410
	#define MSG_BUTTONS_ENTER_NAME 3500
	#define MSG_BUTTONS_SELECT_FILE 3501
	#define MSG_BUTTONS_STATUS_TITLE 3502
	#define MSG_BUTTONS_LOADING 3503
	#define MSG_ENVIRONMENT_ENTER_NAME 3504
	#define MSG_ENVIRONMENT_SELECT_FILE 3505
	#define MSG_ENVIRONMENT_LOADING 3506
	#define MSG_ENVIRONMENT_SAVING 3507
	#define MSG_BUTTONS_CHANGE_WARNING 3508
	#define MSG_BUTTONS_CREATE_WHICH_TYPE 3509
	#define MSG_BUTTONS_TEXT 3510
	#define MSG_BUTTONS_ICONS 3511
	#define MSG_SETTINGS_SELECT_FILE 3512
	#define MSG_SETTINGS_LOADING 3513
	#define MSG_SETTINGS_ENTER_NAME 3514
	#define MSG_SETTINGS_SAVING 3515
	#define MSG_ERROR_SAVING_ENV 3516
	#define MSG_ERROR_SAVING_OPTS 3517
	#define MSG_LAUNCHING_PROGRAM 3602
	#define MSG_MEMORY_COUNTER 3603
	#define MSG_MEMORY_COUNTER_LOC 3604
	#define MSG_MEMORY_COUNTER_CLOCK 3605
	#define MSG_MEMORY_COUNTER_LOC_CLOCK 3606
	#define MSG_MEMORY_FREE_OS4 3607
	#define MSG_LISTER_STATUS_SOURCE 3700
	#define MSG_LISTER_STATUS_DEST 3701
	#define MSG_LISTER_STATUS_BUSY 3702
	#define MSG_LISTER_STATUS_OFF 3703
	#define MSG_HUNT_ENTER_FILENAME 3800
	#define MSG_HUNT_FOUND_FILE 3801
	#define MSG_NEW_LISTER 3802
	#define MSG_SEARCH_COMMENTS 3803
	#define MSG_FUNC_DESC_ADDICON 3900
	#define MSG_FUNC_DESC_ALL 3901
	#define MSG_FUNC_DESC_CLEARBUFFERS 3902
	#define MSG_FUNC_DESC_COMMENT 3903
	#define MSG_FUNC_DESC_COPY 3904
	#define MSG_FUNC_DESC_COPYAS 3905
	#define MSG_FUNC_DESC_DATESTAMP 3906
	#define MSG_FUNC_DESC_DELETE 3907
	#define MSG_FUNC_DESC_DISKCOPY 3908
	#define MSG_FUNC_DESC_ENCRYPT 3909
	#define MSG_FUNC_DESC_FINDFILE 3910
	#define MSG_FUNC_DESC_FINISHSECTION 3911
	#define MSG_FUNC_DESC_FORMAT 3912
	#define MSG_FUNC_DESC_GETSIZES 3913
	#define MSG_FUNC_DESC_ICONINFO 3914
	#define MSG_FUNC_DESC_LEAVEOUT 3915
	#define MSG_FUNC_DESC_LOADBUTTONS 3916
	#define MSG_FUNC_DESC_MAKEDIR 3917
	#define MSG_FUNC_DESC_MOVE 3918
	#define MSG_FUNC_DESC_MOVEAS 3919
	#define MSG_FUNC_DESC_NONE 3920
	#define MSG_FUNC_DESC_PLAY 3921
	#define MSG_FUNC_DESC_PRINT 3922
	#define MSG_FUNC_DESC_PROTECT 3923
	#define MSG_FUNC_DESC_READ 3924
	#define MSG_FUNC_DESC_RENAME 3925
	#define MSG_FUNC_DESC_RUN 3926
	#define MSG_FUNC_DESC_SCANDIR 3927
	#define MSG_FUNC_DESC_SEARCH 3928
	#define MSG_FUNC_DESC_SELECT 3929
	#define MSG_FUNC_DESC_SHOW 3930
	#define MSG_FUNC_DESC_SMARTREAD 3931
	#define MSG_FUNC_DESC_TOGGLE 3932
	#define MSG_FUNC_DESC_CHECKFIT 3933
	#define MSG_FUNC_DESC_CLEARSIZES 3934
	#define MSG_FUNC_DESC_ANSIREAD 3935
	#define MSG_FUNC_DESC_HEXREAD 3936
	#define MSG_FUNC_DESC_CLONE 3937
	#define MSG_FUNC_DESC_LOADENVIRONMENT 3938
	#define MSG_FUNC_DESC_LOADSETTINGS 3939
	#define MSG_FUNC_DESC_PARENT 3940
	#define MSG_FUNC_DESC_ROOT 3941
	#define MSG_FUNC_DESC_USER 3942
	#define MSG_FUNC_DESC_DEVICELIST 3943
	#define MSG_FUNC_DESC_BUFFERLIST 3944
	#define MSG_FUNC_DESC_VERIFY 3945
	#define MSG_FUNC_DESC_PRINTDIR 3946
	#define MSG_FUNC_DESC_SET 3947
	#define MSG_FUNC_DESC_CLOSEBUTTONS 3948
	#define MSG_FUNC_DESC_QUIT 3949
	#define MSG_FUNC_DESC_HIDE 3950
	#define MSG_FUNC_DESC_REVEAL 3951
	#define MSG_FUNC_DESC_RESELECT 3952
	#define MSG_FUNC_DESC_MAKELINK 3953
	#define MSG_FUNC_DESC_MAKELINKAS 3954
	#define MSG_FUNC_DESC_ASSIGN 3955
	#define MSG_FUNC_DESC_CONFIGURE 3956
	#define MSG_FUNC_DESC_CLI 3957
	#define MSG_FUNC_DESC_SETBACKGROUND 3958
	#define MSG_FUNC_DESC_RESET 3959
	#define MSG_FUNC_DESC_STOPSNIFFER 3960
	#define MSG_FUNC_DESC_MAKEFILE 3961
	#define MSG_ICON_GROUP 4000
	#define MSG_GROUP_ENTER_NAME 4003
	#define MSG_RXERR_1 4500
	#define MSG_RXERR_5 4501
	#define MSG_RXERR_6 4502
	#define MSG_RXERR_8 4503
	#define MSG_RXERR_10 4504
	#define MSG_RXERR_12 4505
	#define MSG_RXERR_13 4506
	#define MSG_RXERR_14 4507
	#define MSG_RXERR_15 4508
	#define MSG_RXERR_20 4509
	#define MSG_RXERR_UNKNOWN 4510
	#define MSG_DESKTOP_REALLY_DELETE 6000
	#define MSG_DESKTOP_DELETE_GROUPS 6001
	#define MSG_DESKTOP_DELETE_ASSIGNS 6005
	#define MSG_DESKTOP_DELETE_GROUP_OBJECTS 6006
	#define MSG_DESKTOP_DELETE_DESKTOP_FILES 6007
	#define MSG_DESKTOP_DELETE_DESKTOP_DIRS 6008
	#define MSG_REPLACE_SAME 7000
	#define MSG_REPLACE_VERSION_SAME 7001
	#define MSG_REPLACE_VERSION_NEWER 7002
	#define MSG_REPLACE_VERSION_OLDER 7003
	#define MSG_REPLACE_SIZE_BIGGER 7004
	#define MSG_REPLACE_SIZE_SMALLER 7005
	#define MSG_REPLACE_SIZE_SAME 7006
	#define MSG_REPLACE_DATE_NEWER 7007
	#define MSG_REPLACE_DATE_OLDER 7008
	#define MSG_REPLACE_DATE_SAME 7009
	#define MSG_FILE_ALREADY_EXISTS 7010
	#define MSG_BRIEF_FILE_EXISTS 7011
	#define MSG_REPLACE_VER 7012
	#define MSG_REPLACE_VER_UNKNOWN 7013
	#define MSG_GET_VERSION 7014
	#define MSG_REPLACE_VERSION_TITLE 7015
	#define MSG_LISTER_TITLE_NAME 8000
	#define MSG_LISTER_TITLE_SIZE 8001
	#define MSG_LISTER_TITLE_PROTECT 8002
	#define MSG_LISTER_TITLE_DATE 8003
	#define MSG_LISTER_TITLE_COMMENT 8004
	#define MSG_LISTER_TITLE_FILETYPE 8005
	#define MSG_LISTER_TITLE_OWNER 8006
	#define MSG_LISTER_TITLE_GROUP 8007
	#define MSG_LISTER_TITLE_NETPROT 8008
	#define MSG_LISTER_TITLE_VERSION 8009
	#define MSG_LISTER_TITLE_VOLUME 8100
	#define MSG_LISTER_TITLE_DEVICE 8101
	#define MSG_LISTER_TITLE_FULL 8102
	#define MSG_LISTER_TITLE_FREE 8103
	#define MSG_LISTER_TITLE_USED 8104
	#define MSG_CLI_HELP 9000
	#define MSG_CLI_NO_TEMPLATE 9001
	#define MSG_CLI_TYPE_HELP 9002
	#define MSG_CLI_BAD_CMD 9003
	#define MSG_ICONPOS_WINDOW 10000
	#define MSG_ICONPOS_INSTRUCTIONS 10001
	#define MSG_ICONPOS_APPICON 10002
	#define MSG_ICONPOS_DISKS 10003
	#define MSG_ICONPOS_LISTERS 10004
	#define MSG_ICONPOS_PRIORITY 10005
	#define MSG_ICONPOS_GROUPS 10006
	#define MSG_ICONPOS_LEFTOUT 10007
	#define MSG_COMMAND_SAVE 12000
	#define MSG_SAVING_COMMAND 12001
	#define MSG_EDIT_COMMAND 12002
	#define MSG_COMMAND_DESCRIPTION 12003
	#define MSG_COMMAND_LEAVEOUT 12004
	#define MSG_START_APPEARANCE 13000
	#define MSG_START_BORDERLESS 13001
	#define MSG_START_DRAGBAR_LEFT 13002
	#define MSG_START_DRAGBAR_RIGHT 13003
	#define MSG_START_IMAGE 13004
	#define MSG_START_LABEL 13005
	#define MSG_START_SELECT_IMAGE 13006
	#define MSG_START_SELECT_LABEL 13007
	#define MSG_OPEN_STARTMENU_MENU 13009
	#define MSG_START_SELECT_MENU 13010
	#define MSG_START_FONT 13011
	#define MSG_START_SELECT_FONT 13012
	#define MSG_START_SCALING 13013
	#define MSG_START_DRAGBAR 13014
	#define MSG_START_DRAGBAR_OFF 13015
	#define MSG_START_LABEL_FONT 13016
	#define MSG_START_PICTURE 13017
	#define MSG_START_SELECT_PICTURE 13018
	#define MSG_KEYFINDER_TITLE 13500
	#define MSG_KEYFINDER_KEY 13501
	#define MSG_KEYFINDER_FOUND 13502
	#define MSG_KEYFINDER_TYPE_INVALID 13600
	#define MSG_KEYFINDER_TYPE_NONE 13601
	#define MSG_KEYFINDER_TYPE_BUTTONS 13602
	#define MSG_KEYFINDER_TYPE_TOOLBAR 13603
	#define MSG_KEYFINDER_TYPE_LISTERMENU 13604
	#define MSG_KEYFINDER_TYPE_MENU 13605
	#define MSG_KEYFINDER_TYPE_START 13606
	#define MSG_KEYFINDER_TYPE_HOTKEYS 13607
	#define MSG_KEYFINDER_TYPE_SCRIPTS 13608

#endif /* CATCOMP_NUMBERS */

/****************************************************************************/

#ifdef CATCOMP_STRINGS

	#define MSG_ABORTED_STR "Aborted..."
	#define MSG_TRY_AGAIN_STR "Try Again"
	#define MSG_RETRY_STR "Retry"
	#define MSG_ABORT_STR "Abort"
	#define MSG_REPLACE_STR "Replace"
	#define MSG_SKIP_STR "Skip"
	#define MSG_SKIP_ALL_STR "Skip All"
	#define MSG_ALL_STR "All"
	#define MSG_QUIT_STR "Quit"
	#define MSG_SAVE_STR "Save"
	#define MSG_CLOSE_STR "Close"
	#define MSG_YES_STR "Yes"
	#define MSG_NO_STR "No"
	#define MSG_CONTINUE_STR "Continue"
	#define MSG_OKAY_STR "OK"
	#define MSG_CANCEL_STR "Cancel"
	#define MSG_REPLACE_ALL_STR "Replace All"
	#define MSG_PROCEED_STR "Proceed"
	#define MSG_MOVE_STR "Move"
	#define MSG_DISCARD_STR "Discard"
	#define MSG_BLOCKS_STR "blocks"
	#define MSG_FREE_STR "free"
	#define MSG_MAKELINK_STR "MakeLink"
	#define MSG_SELECT_FILE_STR "Select File"
	#define MSG_SELECT_DIR_STR "Select Directory"
	#define MSG_ENTER_FILE_STR "Enter Filename"
	#define MSG_OPEN_STR "Open..."
	#define MSG_DEFAULTS_STR "Reset To Defaults"
	#define MSG_LAST_SAVED_STR "Last Saved"
	#define MSG_RESTORE_STR "Restore"
	#define MSG_PROJECT_STR "Project"
	#define MSG_EDIT_STR "Edit"
	#define MSG_UNKNOWN_TYPE_STR "<unknown>"
	#define MSG_SNIFF_CONFIRMATION_STR "The file '%s' could not be identified.\nLaunch Filetype Sniffer to investigate?"
	#define MSG_SNIFF_STR "Sniff!"
	#define MSG_SCRIPTS_CHANGED_STR "You have modified your scripts and not\nsaved them. Would you like to?"
	#define MSG_LISTER_MENU_CHANGED_STR "You have modified your lister menu and not\nsaved it. Would you like to?"
	#define MSG_USER_MENU_CHANGED_STR "You have modified your user menus and not\nsaved them. Would you like to?"
	#define MSG_HOTKEYS_CHANGED_STR "You have modified your hotkeys and not\nsaved them. Would you like to?"
	#define MSG_OPEN_NEW_WINDOW_STR "Open in New Lister"
	#define MSG_OPEN_WITH_MENU_STR "Open With..."
	#define MSG_SELECT_APP_STR "Select Application To Open '%s'"
	#define MSG_OPEN_WITH_MENU_SUB_STR "Open With"
	#define MSG_OPEN_WITH_MENU_OTHER_STR "Other..."
	#define MSG_ENTER_ARGUMENTS_FOR_STR "Enter arguments for '%s'"
	#define MSG_DIRECTORY_STR "Directory "
	#define MSG_SELECT_UNPROTECT_STR "\nSelect Unprotect to set the deletion bit."
	#define MSG_FOUND_A_MATCH_STR "Found a match in the file\n'%s'"
	#define MSG_COMMENTING_STR "commenting"
	#define MSG_PROTECTING_STR "protecting"
	#define MSG_DATESTAMPING_STR "date stamping"
	#define MSG_DELETING_STR "deleting"
	#define MSG_RENAMING_STR "renaming"
	#define MSG_ENTER_COMMENT_STR "Enter a comment for '%s'."
	#define MSG_ENTER_PASSWORD_STR "Enter password to encrypt files."
	#define MSG_ENTER_DATE_AND_TIME_STR "Enter new date (and time) for '%s'."
	#define MSG_ENTER_ARGUMENTS_STR "Enter arguments"
	#define MSG_REALLY_QUIT_STR "This will end your Directory Opus session."
	#define MSG_FILE_STR "File"
	#define MSG_SELECT_PROTECTION_BITS_STR "Select protection bits"
	#define MSG_DIRECTORY_OPUS_REQUEST_STR "Directory Opus Request"
	#define MSG_MULTI_DRAG_STR "Multi-drag - %ld files, %ld dirs selected"
	#define MSG_PRESS_MOUSE_BUTTON_STR "Click close button to continue..."
	#define MSG_COMMENT_STR "comment"
	#define MSG_PROTECT_STR "protect"
	#define MSG_DATESTAMP_STR "date stamp"
	#define MSG_PROGRESS_OPERATION_MOVING_STR "Moving..."
	#define MSG_PROGRAM_LOADING_STR "Opus 5 loading... please wait."
	#define MSG_OPEN_MENU_STR "Load..."
	#define MSG_SAVE_MENU_STR "Save"
	#define MSG_SAVEAS_MENU_STR "Save As..."
	#define MSG_ACT_RECURSIVELY_STR "Act on files in sub-directories?"
	#define MSG_PROGRESS_OPERATION_COMMENT_STR "Setting comments..."
	#define MSG_PROGRESS_OPERATION_PROTECT_STR "Setting protections..."
	#define MSG_PROGRESS_OPERATION_DATESTAMP_STR "Setting datestamps..."
	#define MSG_SET_PROTECT_OLD_STR "Current"
	#define MSG_SET_PROTECT_SET_STR "Set"
	#define MSG_SET_PROTECT_CLEAR_STR "Clear"
	#define MSG_OK_BUTTON_STR "_OK"
	#define MSG_ALL_BUTTON_STR "A_ll"
	#define MSG_SKIP_BUTTON_STR "S_kip"
	#define MSG_ABORT_BUTTON_STR "A_bort"
	#define MSG_PROGRESS_OPERATION_ENCRYPTING_STR "Encrypting..."
	#define MSG_USER_MENU_STR "User"
	#define MSG_LISTER_MENU_TITLE_STR "Lister Pop-Up Menu"
	#define MSG_USER_MENU_TITLE_STR "User Menus"
	#define MSG_EMPTY_STR "empty"
	#define MSG_CANCEL_BUTTON_STR "_Cancel"
	#define MSG_OPEN_BUTTON_STR "_Open"
	#define MSG_NEW_BUTTON_STR "_New"
	#define MSG_PARENT_STR "Parent Directory"
	#define MSG_ROOT_STR "Root Directory"
	#define MSG_DEVICE_LIST_STR "Device List"
	#define MSG_SCANNING_DIRECTORIES_STR "Scanning directories..."
	#define MSG_READING_DIRECTORY_STR "Reading directory..."
	#define MSG_CHECK_FIT_RESULTS_STR "Results of fit test:\n"
	#define MSG_FIT_STR "fit"
	#define MSG_BLOCKS_NEEDED_STR "blocks needed."
	#define MSG_EXECUTE_ENTER_COMMAND_STR "Enter command and arguments."
	#define MSG_TOOL_MENU_STR "Tools"
	#define MSG_BUFFER_LIST_STR "Cache List"
	#define MSG_REREAD_DIR_STR "Re-read Directory"
	#define MSG_ASSIGN_STR "Assign"
	#define MSG_FULL_STR "full"
	#define MSG_USED_STR "in use"
	#define MSG_VALIDATING_STR "disk validating"
	#define MSG_ENTER_FILTER_STRING_STR "Directories selected - enter filter pattern."
	#define MSG_VOLUME_STR "Volume"
	#define MSG_SELECT_SOURCE_STR "Select Source"
	#define MSG_SELECT_DESTINATION_STR "Select Destination"
	#define MSG_NO_OWNER_STR "<No Owner>"
	#define MSG_NO_GROUP_STR "<No Group>"
	#define MSG_SCRIPTS_TITLE_STR "Scripts"
	#define MSG_PROGRESS_OPERATION_LINKING_STR "Making Links..."
	#define MSG_LINKING_STR "making a link to"
	#define MSG_DECRYPT_STR "_Decrypt files"
	#define MSG_EMPTY_TRASH_STR "Empty Trash"
	#define MSG_REALLY_EMPTY_TRASH_STR "The contents of the trashcan will be lost!\nDo you really want to empty it?"
	#define MSG_ENTER_ASSIGN_NAME_STR "Enter name to assign to '%s'"
	#define MSG_ADD_STR "Add"
	#define MSG_PATH_STR "Path"
	#define MSG_DEFER_STR "Defer"
	#define MSG_ASSIGNING_STR "assigning"
	#define MSG_CHANGE_NEEDS_RESET_STR "Some changes you made will not be\neffective until you next reboot."
	#define MSG_OPENING_STR "opening"
	#define MSG_LIBRARY_ERROR_STR "Unable to open %s (%s)"
	#define MSG_LIBRARY_ANY_VERSION_STR "any version"
	#define MSG_CLI_STR "CLI"
	#define MSG_CX_DESC_STR "The Complete Workbench Replacement!"
	#define MSG_ABORT_STARTUP_STR "Do you want to abort startup?"
	#define MSG_ABORT_STARTUP_BUTTONS_STR "Continue|Abort"
	#define MSG_PROTECT_TITLE_STR "Protect"
	#define MSG_DATESTAMP_TITLE_STR "Datestamp"
	#define MSG_COMMENT_TITLE_STR "Comment"
	#define MSG_PATTERN_OFF_STR "The backdrop picture is disabled in the Environment.\nWould you like it enabled?"
	#define MSG_THEMES_MENU_STR "Themes"
	#define MSG_THEMES_LOAD_STR "Load Theme..."
	#define MSG_THEMES_SAVE_STR "Save Theme..."
	#define MSG_THEMES_BUILD_STR "Build Theme..."
	#define MSG_ADDICONING_STR "adding icons to"
	#define MSG_REGISTERED_TO_STR "This copy is registered to:"
	#define MSG_SERIAL_NUMBER_STR "Serial # "
	#define MSG_UNREGISTERED_STR "This copy is unregistered!"
	#define MSG_YOU_SHOULD_REGISTER_STR "Register to prevent this delay!"
	#define MSG_TRANSLATION_BY_YOUR_NAME_STR ""
	#define MSG_SEARCH_DIRECTORIES_STR "Search files in selected directories?"
	#define MSG_ENTER_SEARCH_STRING_STR "Enter text to search for..."
	#define MSG_SEARCH_NO_CASE_STR "_Case insensitive"
	#define MSG_SEARCH_WILD_STR "_Wildcard search"
	#define MSG_SEARCH_ONLYWORD_STR "_Match whole words"
	#define MSG_SEARCH_LEAVE_SELECTED_STR "Leave selected"
	#define MSG_SEARCH_PROMPT_STR "Ask"
	#define MSG_SEARCH_OUTPUT_STR "Output results"
	#define MSG_PROGRESS_SEARCHING_STR "Searching..."
	#define MSG_SEARCH_OUTPUT_HEADER_STR "Files containing '%s'\n\n"
	#define MSG_SEARCH_READ_STR "Read"
	#define MSG_ENTER_NEW_NAME_STR "Enter new filename or rename pattern."
	#define MSG_PROGRESS_RENAMING_STR "Renaming..."
	#define MSG_RENAME_STR "Rename"
	#define MSG_RENAME_DISK_STR "Enter a new name for '%s'."
	#define MSG_ENTER_ANOTHER_NAME_STR "Enter another filename."
	#define MSG_ENTER_DIRECTORY_NAME_STR "Enter directory name"
	#define MSG_MAKEDIR_ICON_STR "With icon"
	#define MSG_MAKEDIR_NOICON_STR "Without icon"
	#define MSG_DIRECTORY_CREATED_STR "Directory created."
	#define MSG_ENTER_FILE_NAME_STR "Enter file name"
	#define MSG_FILE_CREATED_STR "File created."
	#define MSG_PROGRESS_ADDING_ICONS_STR "Adding icons..."
	#define MSG_ICON_ALREADY_EXISTS_STR "An icon already exists for '%s'.\nDo you wish to replace it?"
	#define MSG_ICON_REPLACE_IMAGE_STR "Image Only"
	#define MSG_COPYING_STR "copying"
	#define MSG_PROGRESS_OPERATION_COPYING_STR "Copying..."
	#define MSG_CANT_OVERCOPY_FILES_STR "Source and destination directories\nmust be different to copy files."
	#define MSG_COPY_STR "Copy"
	#define MSG_PROGRESS_OPERATION_CLONING_STR "Duplicating..."
	#define MSG_DUPLICATE_STR "Duplicate"
	#define MSG_CANT_COPY_DIR_INTO_ITSELF_STR "You can't copy a directory into itself!"
	#define MSG_MOVING_STR "moving"
	#define MSG_CANT_OVERLINK_FILES_STR "Source and destination directories\nmust be different to make links."
	#define MSG_CANT_LINK_DIR_INTO_ITSELF_STR "You can't link a directory into itself!"
	#define MSG_FROM_TO_STR "From '%s' to '%s'"
	#define MSG_FROM_STR "From '%s'"
	#define MSG_PROJECT_MENU_STR "Opus"
	#define MSG_BACKDROP_MENU_STR "Backdrop"
	#define MSG_ABOUT_MENU_STR "About..."
	#define MSG_HIDE_MENU_STR "Hide"
	#define MSG_QUIT_MENU_STR "Quit..."
	#define MSG_EXECUTE_MENU_STR "Execute Command..."
	#define MSG_HELP_MENU_STR "Help!"
	#define MSG_KEYFINDER_MENU_STR "Key Finder..."
	#define MSG_NEWSHELL_MENU_STR "NewShell..."
	#define MSG_WINDOW_MENU_STR "Listers"
	#define MSG_NEW_LISTER_MENU_STR "New"
	#define MSG_CLOSE_LISTER_MENU_STR "Close"
	#define MSG_LISTER_SOURCE_STR "Make Source"
	#define MSG_LISTER_DEST_STR "Make Dest"
	#define MSG_LISTER_LOCK_SOURCE_STR "Lock As Source"
	#define MSG_LISTER_LOCK_DEST_STR "Lock As Dest"
	#define MSG_LISTER_UNLOCK_STR "Unlock"
	#define MSG_LISTER_OFF_STR "Turn Off"
	#define MSG_LISTER_UNLOCK_ALL_STR "Unlock All"
	#define MSG_CLOSE_ALL_MENU_STR "Close All"
	#define MSG_EDIT_MENU_STR "Edit..."
	#define MSG_LISTER_BUTTONS_MENU_STR "Edit Lister Toolbar..."
	#define MSG_LISTER_MENU_MENU_STR "Edit Lister Menu..."
	#define MSG_TILE_MENU_STR "Tile"
	#define MSG_TILE_HORIZ_MENU_STR "Horizontally"
	#define MSG_TILE_VERT_MENU_STR "Vertically"
	#define MSG_CASCADE_MENU_STR "Cascade"
	#define MSG_LISTER_LOCK_POS_STR "Lock Position?"
	#define MSG_LISTER_ICONIFY_STR "Iconify"
	#define MSG_LISTER_VIEW_ICONS_STR "View Icons"
	#define MSG_LISTER_VIEW_STR "View As"
	#define MSG_LISTER_VIEW_NAME_STR "Name"
	#define MSG_LISTER_VIEW_ICON_STR "Icon"
	#define MSG_LISTER_SHOW_ALL_STR "Show All"
	#define MSG_LISTER_ICON_ACTION_STR "Icon Action"
	#define MSG_LISTER_ARRANGE_ICONS_STR "Arrange Icons"
	#define MSG_LISTER_ARRANGE_NAME_STR "by Name"
	#define MSG_LISTER_ARRANGE_TYPE_STR "by Type"
	#define MSG_LISTER_ARRANGE_SIZE_STR "by Size"
	#define MSG_LISTER_ARRANGE_DATE_STR "by Date"
	#define MSG_LISTER_NEW_DRAWER_STR "New Drawer..."
	#define MSG_OPEN_PARENT_MENU_STR "Open Parent"
	#define MSG_LISTER_RESTORE_STR "Restore"
	#define MSG_LISTER_RESIZE_FIT_STR "Resize to Fit"
	#define MSG_LISTER_DUAL_STR "Dual Lister"
	#define MSG_LISTER_NEW_FILE_STR "New File..."
	#define MSG_ICONS_MENU_STR "Icons"
	#define MSG_ICON_OPEN_MENU_STR "Open"
	#define MSG_ICON_INFO_MENU_STR "Information..."
	#define MSG_ICON_SNAPSHOT_MENU_STR "Snapshot"
	#define MSG_ICON_LEAVE_OUT_MENU_STR "Leave Out"
	#define MSG_ICON_PUT_AWAY_MENU_STR "Put Away"
	#define MSG_ICON_SELECT_ALL_MENU_STR "Select All"
	#define MSG_ICON_CLEANUP_STR "Clean Up"
	#define MSG_ICON_RESET_STR "Reset"
	#define MSG_ICON_RENAME_STR "Rename..."
	#define MSG_ICON_FORMAT_STR "Format Disk..."
	#define MSG_ICON_DISKINFO_STR "Disk Information..."
	#define MSG_ICON_UNSNAPSHOT_MENU_STR "Un-Snapshot"
	#define MSG_ICON_COPY_STR "Copy"
	#define MSG_ICONS_NEW_MENU_STR "New"
	#define MSG_ICONS_MAKEDIR_MENU_STR "Drawer..."
	#define MSG_ICONS_NEWGROUP_MENU_STR "Group..."
	#define MSG_ICON_COPY_RAM_STR "to RAM:"
	#define MSG_ICON_COPY_DF0_STR "to DF0:"
	#define MSG_ICON_COPY_OTHER_STR "to other..."
	#define MSG_ICON_COPY2_STR "Copy..."
	#define MSG_ICON_COPY_TO_STR "to"
	#define MSG_ICONS_NEWCOMMAND_MENU_STR "Command..."
	#define MSG_ICONS_SHORTCUT_MENU_STR "Create Shortcut"
	#define MSG_DESKTOP_POPUP_LEFTOUT_STR "Create Left-out"
	#define MSG_DESKTOP_POPUP_COPY_STR "Copy to Desktop"
	#define MSG_DESKTOP_POPUP_MOVE_STR "Move to Desktop"
	#define MSG_ICON_COPY_DESKTOP_STR "to Desktop"
	#define MSG_ICON_LINEUP_STR "Line Up"
	#define MSG_ICON_SNAPSHOT_LISTER_STR "Snapshot Lister"
	#define MSG_ICON_SNAPSHOT_ICONS_STR "Snapshot Icons"
	#define MSG_ICON_SNAPSHOT_ALL_STR "Snapshot All"
	#define MSG_ICON_SNAPSHOT_WINDOW_STR "Snapshot Window"
	#define MSG_ICONS_MAKEFILE_MENU_STR "File..."
	#define MSG_BUTTONS_MENU_STR "Buttons"
	#define MSG_CLOSE_BUTTONS_MENU_STR "Close"
	#define MSG_BUTTONS_NEW_GFX_MENU_STR "New Graphic Buttons..."
	#define MSG_BUTTONS_NEW_TEXT_MENU_STR "New Text Buttons..."
	#define MSG_BUTTONS_NEW_STARTMENU_MENU_STR "New Start Menu..."
	#define MSG_SETTINGS_MENU_STR "Settings"
	#define MSG_CLOCK_MENU_STR "Clock"
	#define MSG_CREATE_ICONS_MENU_STR "Create Icons?"
	#define MSG_ENVIRONMENT_MENU_STR "Environment"
	#define MSG_FILETYPES_MENU_STR "File Types..."
	#define MSG_HOTKEYS_MENU_STR "Hotkeys..."
	#define MSG_MENU_MENU_STR "User Menus..."
	#define MSG_FILTER_MENU_STR "Recursive Filter"
	#define MSG_OPTIONS_MENU_STR "Options..."
	#define MSG_DEFPUBSCR_MENU_STR "Default PubScreen"
	#define MSG_MENU_HOTKEYS_STR "Hotkeys..."
	#define MSG_HOTKEYS_TITLE_STR "Hotkeys"
	#define MSG_MENU_SCRIPTS_STR "Scripts..."
	#define MSG_MENU_SYSMENU_STR "System Menus..."
	#define MSG_MENU_ICONPOS_STR "Icon Positioning..."
	#define MSG_ENVIRONMENT_SAVE_LAYOUT_NEW_STR "Save Layout"
	#define MSG_ENVIRONMENT_EDIT_STR "Environment..."
	#define MSG_ENVIRONMENT_LOAD_NEW_STR "Load Environment..."
	#define MSG_ENVIRONMENT_SAVE_NEW_STR "Save Environment"
	#define MSG_SELECT_TITLE_STR "File selection"
	#define MSG_SELECT_IGNORE_STR "Ignore"
	#define MSG_SELECT_MATCH_STR "Match"
	#define MSG_SELECT_NOMATCH_STR "No Match"
	#define MSG_SELECT_NEWER_STR "Newer"
	#define MSG_SELECT_OLDER_STR "Older"
	#define MSG_SELECT_DIFFERENT_STR "Different"
	#define MSG_SELECT_NAME_STR "_Name"
	#define MSG_SELECT_DATE_FROM_STR "Da_te"
	#define MSG_SELECT_DATE_TO_STR "to"
	#define MSG_SELECT_PROT_STR "Bits on"
	#define MSG_SELECT_INCLUDE_STR "Include"
	#define MSG_SELECT_EXCLUDE_STR "Exclude"
	#define MSG_SELECT_COMPARE_STR "Destination _Compare"
	#define MSG_SELECT_SIMPLE_STR "Simple"
	#define MSG_SELECT_COMPLEX_STR "Complex"
	#define MSG_SELECT_ENTER_PATTERN_STR "Enter pattern to match filenames."
	#define MSG_SELECT_PROT_OFF_STR "off"
	#define MSG_SELECT_ENTRY_BOTH_STR "Files & Dirs"
	#define MSG_SELECT_ENTRY_FILES_STR "Files Only"
	#define MSG_SELECT_ENTRY_DIRS_STR "Dirs Only"
	#define MSG_SELECT_ENTRY_TYPE_STR "_Operate on"
	#define MSG_DOS_ERROR_STR "DOS error"
	#define MSG_DOS_ERROR_CODE_STR "Error %ld"
	#define MSG_ERROR_OCCURED_STR "An error occured %s '%s'\n%s"
	#define MSG_ERROR_DEST_IS_DIRECTORY_STR "Destination '%s' is a directory."
	#define MSG_ERROR_DEST_IS_FILE_STR "Destination '%s' is a file."
	#define MSG_ERROR_UNKNOWN_CODE_STR "Unknown error code"
	#define MSG_BUTTONS_ERROR_SAVING_STR "Error saving button bank!"
	#define MSG_BUTTONS_ERROR_LOADING_STR "Error loading button bank!"
	#define MSG_ERROR_CREATING_DIRECTORY_STR "creating the directory"
	#define MSG_CUSTPORT_NOT_FOUND_STR "User-specified custom handler port\n'%s'\ncould not be found!"
	#define MSG_ALREADY_RUNNING_STR                                                                                   \
		"Directory Opus appears to be already running!\nRunning a second copy has the potential to cause\nproblems, " \
		"and it is recommended that you do not."
	#define MSG_ALREADY_RUNNING_BUTTONS_STR "Cancel|Run Again"
	#define MSG_UNABLE_TO_CLOSE_SCREEN_STR \
		"Directory Opus is unable to close its screen.\nPlease close all windows and try again."
	#define MSG_ERROR_INVALID_DATE_STR "Invalid date string entered!"
	#define MSG_UNABLE_TO_OPEN_SCREEN_STR "Can't open screen!"
	#define MSG_AN_ERROR_OCCURED_STR "An error occured "
	#define MSG_ERROR_STR "Error "
	#define MSG_SIZE_STR "Size"
	#define MSG_DATE_STR "Date"
	#define MSG_OLD_STR "Old"
	#define MSG_NEW_STR "New"
	#define MSG_FILE_EXISTS_STR "File '%s' already exists!"
	#define MSG_ERROR_CREATING_FILE_STR "creating the file"
	#define MSG_PROGRESS_OPERATION_DELETING_STR "Deleting..."
	#define MSG_DELETE_CONFIRM_STR                                                                                   \
		"Warning: you cannot get back\nwhat you delete! OK to delete:\n\n%ld file(s) and\n%ld drawer(s) (and their " \
		"contents)?"
	#define MSG_DELETE_STR "Delete"
	#define MSG_DELETE_ALL_STR "Delete All"
	#define MSG_DELETE_FILE_CONFIRM_STR "Delete '%s'?"
	#define MSG_DELETE_DIR_CONFIRM_STR "'%s' is a directory.\nProceed with delete operation?"
	#define MSG_DELETE_PROTECTED_STR "'%s' is protected against deletion.\nUnprotect and try again?"
	#define MSG_UNPROTECT_STR "Unprotect"
	#define MSG_UNPROTECT_ALL_STR "Unprotect All"
	#define MSG_DELETE_LINK_CONFIRM_SINGLE_STR "Really delete this shortcut?"
	#define MSG_DELETE_LINK_CONFIRM_MULTI_STR "Really delete these shortcuts?"
	#define MSG_BUTTONS_ENTER_NAME_STR "Save Buttons"
	#define MSG_BUTTONS_SELECT_FILE_STR "Load Buttons"
	#define MSG_BUTTONS_STATUS_TITLE_STR "Buttons Status"
	#define MSG_BUTTONS_LOADING_STR "Loading button bank..."
	#define MSG_ENVIRONMENT_ENTER_NAME_STR "Save Environment"
	#define MSG_ENVIRONMENT_SELECT_FILE_STR "Load Environment"
	#define MSG_ENVIRONMENT_LOADING_STR "Loading environment file..."
	#define MSG_ENVIRONMENT_SAVING_STR "Saving environment file..."
	#define MSG_BUTTONS_CHANGE_WARNING_STR "Button bank '%s' has been modified.\nDo you wish to save it?"
	#define MSG_BUTTONS_CREATE_WHICH_TYPE_STR "Which button bank type do you wish to create?"
	#define MSG_BUTTONS_TEXT_STR "Text"
	#define MSG_BUTTONS_ICONS_STR "Graphics"
	#define MSG_SETTINGS_SELECT_FILE_STR "Load Options"
	#define MSG_SETTINGS_LOADING_STR "Loading Options file..."
	#define MSG_SETTINGS_ENTER_NAME_STR "Save Options"
	#define MSG_SETTINGS_SAVING_STR "Saving Options file..."
	#define MSG_ERROR_SAVING_ENV_STR "Error saving environment file!"
	#define MSG_ERROR_SAVING_OPTS_STR "Error saving options file!"
	#define MSG_LAUNCHING_PROGRAM_STR "Attempting to launch '%s'..."
	#define MSG_MEMORY_COUNTER_STR "%s  %s graphics mem  %s other mem"
	#define MSG_MEMORY_COUNTER_LOC_STR "%s  %s graphics mem  %s other mem"
	#define MSG_MEMORY_COUNTER_CLOCK_STR "%s  %s graphics  %s other"
	#define MSG_MEMORY_COUNTER_LOC_CLOCK_STR "%s  %s graphics  %s other"
#define MSG_MEMORY_FREE_OS4_STR "%s  %s free"
	#define MSG_LISTER_STATUS_SOURCE_STR "SRCE"
	#define MSG_LISTER_STATUS_DEST_STR "DEST"
	#define MSG_LISTER_STATUS_BUSY_STR "BUSY"
	#define MSG_LISTER_STATUS_OFF_STR "OFF"
	#define MSG_HUNT_ENTER_FILENAME_STR "Enter file pattern to search for."
	#define MSG_HUNT_FOUND_FILE_STR "Found the file '%s' in the directory\n'%s'\nShall I go there?"
	#define MSG_NEW_LISTER_STR "New Lister"
	#define MSG_SEARCH_COMMENTS_STR "Search file comments"
	#define MSG_FUNC_DESC_ADDICON_STR "Add icons to files"
	#define MSG_FUNC_DESC_ALL_STR "Select all files"
	#define MSG_FUNC_DESC_CLEARBUFFERS_STR "Free directory caches"
	#define MSG_FUNC_DESC_COMMENT_STR "Give comments to files"
	#define MSG_FUNC_DESC_COPY_STR "Copy selected files"
	#define MSG_FUNC_DESC_COPYAS_STR "Copy with new names"
	#define MSG_FUNC_DESC_DATESTAMP_STR "Change datestamp of files"
	#define MSG_FUNC_DESC_DELETE_STR "Delete selected files"
	#define MSG_FUNC_DESC_DISKCOPY_STR "Make copies of disks"
	#define MSG_FUNC_DESC_ENCRYPT_STR "Encrypt files"
	#define MSG_FUNC_DESC_FINDFILE_STR "Search for filenames"
	#define MSG_FUNC_DESC_FINISHSECTION_STR "Finish above commands"
	#define MSG_FUNC_DESC_FORMAT_STR "Format disks"
	#define MSG_FUNC_DESC_GETSIZES_STR "Calculate directory sizes"
	#define MSG_FUNC_DESC_ICONINFO_STR "Show/edit icon information"
	#define MSG_FUNC_DESC_LEAVEOUT_STR "Leave objects on main window"
	#define MSG_FUNC_DESC_LOADBUTTONS_STR "Load a button file"
	#define MSG_FUNC_DESC_MAKEDIR_STR "Make a new directory"
	#define MSG_FUNC_DESC_MOVE_STR "Move selected files"
	#define MSG_FUNC_DESC_MOVEAS_STR "Move with new names"
	#define MSG_FUNC_DESC_NONE_STR "Deselect all files"
	#define MSG_FUNC_DESC_PLAY_STR "Play selected sounds"
	#define MSG_FUNC_DESC_PRINT_STR "Print selected files"
	#define MSG_FUNC_DESC_PROTECT_STR "Change file protections"
	#define MSG_FUNC_DESC_READ_STR "Read text files"
	#define MSG_FUNC_DESC_RENAME_STR "Rename selected files"
	#define MSG_FUNC_DESC_RUN_STR "Run selected programs"
	#define MSG_FUNC_DESC_SCANDIR_STR "Read a directory"
	#define MSG_FUNC_DESC_SEARCH_STR "Search for text strings"
	#define MSG_FUNC_DESC_SELECT_STR "Wildcard-select files"
	#define MSG_FUNC_DESC_SHOW_STR "Show selected pictures"
	#define MSG_FUNC_DESC_SMARTREAD_STR "Auto hex/text viewer"
	#define MSG_FUNC_DESC_TOGGLE_STR "Toggle selection state"
	#define MSG_FUNC_DESC_CHECKFIT_STR "See if files will fit"
	#define MSG_FUNC_DESC_CLEARSIZES_STR "Clear directory sizes"
	#define MSG_FUNC_DESC_ANSIREAD_STR "Text viewer (supports ANSI)"
	#define MSG_FUNC_DESC_HEXREAD_STR "Hex (binary) viewer"
	#define MSG_FUNC_DESC_CLONE_STR "Duplicate selected files"
	#define MSG_FUNC_DESC_LOADENVIRONMENT_STR "Load an Environment file"
	#define MSG_FUNC_DESC_LOADSETTINGS_STR "Load an Options file"
	#define MSG_FUNC_DESC_PARENT_STR "Read parent directory"
	#define MSG_FUNC_DESC_ROOT_STR "Read root directory"
	#define MSG_FUNC_DESC_USER_STR "Execute File Type function"
	#define MSG_FUNC_DESC_DEVICELIST_STR "Show devices and assigns"
	#define MSG_FUNC_DESC_BUFFERLIST_STR "Show cached directories"
	#define MSG_FUNC_DESC_VERIFY_STR "Ask for confirmation"
	#define MSG_FUNC_DESC_PRINTDIR_STR "Print directory listings"
	#define MSG_FUNC_DESC_SET_STR "Set something"
	#define MSG_FUNC_DESC_CLOSEBUTTONS_STR "Close a button bank"
	#define MSG_FUNC_DESC_QUIT_STR "Quit Directory Opus 5"
	#define MSG_FUNC_DESC_HIDE_STR "Hide Directory Opus"
	#define MSG_FUNC_DESC_REVEAL_STR "Reveal Directory Opus (when hidden)"
	#define MSG_FUNC_DESC_RESELECT_STR "Reselect last-used files"
	#define MSG_FUNC_DESC_MAKELINK_STR "Make a link to a file"
	#define MSG_FUNC_DESC_MAKELINKAS_STR "Make a link with a new name"
	#define MSG_FUNC_DESC_ASSIGN_STR "Make an assign"
	#define MSG_FUNC_DESC_CONFIGURE_STR "Configure lister display"
	#define MSG_FUNC_DESC_CLI_STR "Internal Command Line Interpreter"
	#define MSG_FUNC_DESC_SETBACKGROUND_STR "Set as background picture"
	#define MSG_FUNC_DESC_RESET_STR "Reset Things"
	#define MSG_FUNC_DESC_STOPSNIFFER_STR "Stop Filetype Sniffer"
	#define MSG_FUNC_DESC_MAKEFILE_STR "Make a new empty file"
	#define MSG_ICON_GROUP_STR "Program Groups"
	#define MSG_GROUP_ENTER_NAME_STR "Enter Program Group Name"
	#define MSG_RXERR_1_STR "File rejected by filters"
	#define MSG_RXERR_5_STR "Invalid set/query item"
	#define MSG_RXERR_6_STR "Invalid name or keyword"
	#define MSG_RXERR_8_STR "Invalid trap"
	#define MSG_RXERR_10_STR "Invalid lister handle"
	#define MSG_RXERR_12_STR "Invalid toolbar"
	#define MSG_RXERR_13_STR "Invalid path"
	#define MSG_RXERR_14_STR "Object not found"
	#define MSG_RXERR_15_STR "No memory"
	#define MSG_RXERR_20_STR "Unable to open lister"
	#define MSG_RXERR_UNKNOWN_STR "Unknown error code"
	#define MSG_DESKTOP_REALLY_DELETE_STR "Warning: you cannot get back\nwhat you delete! OK to delete:\n\n"
	#define MSG_DESKTOP_DELETE_GROUPS_STR "%ld program group(s)"
	#define MSG_DESKTOP_DELETE_ASSIGNS_STR "%ld assign(s)"
	#define MSG_DESKTOP_DELETE_GROUP_OBJECTS_STR "%ld group object(s)"
	#define MSG_DESKTOP_DELETE_DESKTOP_FILES_STR "%ld file(s)"
	#define MSG_DESKTOP_DELETE_DESKTOP_DIRS_STR "%ld dir(s)"
	#define MSG_REPLACE_SAME_STR "The two files appear to be the same."
	#define MSG_REPLACE_VERSION_SAME_STR "Both files are the same version (%ld.%ld.%ld)."
	#define MSG_REPLACE_VERSION_NEWER_STR "The new file is a later version (%ld.%ld.%ld)."
	#define MSG_REPLACE_VERSION_OLDER_STR "The new file is an earlier version (%ld.%ld.%ld)."
	#define MSG_REPLACE_SIZE_BIGGER_STR "The new file is larger than the old one (by %ld bytes).\n"
	#define MSG_REPLACE_SIZE_SMALLER_STR "The new file is smaller than the old one (by %ld bytes).\n"
	#define MSG_REPLACE_SIZE_SAME_STR "The two files are the same size (%ld bytes).\n"
	#define MSG_REPLACE_DATE_NEWER_STR "The new file has a more recent date (%s).\n"
	#define MSG_REPLACE_DATE_OLDER_STR "The new file has an earlier date (%s).\n"
	#define MSG_REPLACE_DATE_SAME_STR "The files have the same creation date.\n"
	#define MSG_FILE_ALREADY_EXISTS_STR "File '%s' exists and would be replaced.\n"
	#define MSG_BRIEF_FILE_EXISTS_STR \
		"File '%s' already exists. Replace it?\n\nNew - Size : %7ld  Date : %19s%s\nOld - Size : %7ld  Date : %19s%s"
	#define MSG_REPLACE_VER_STR "  Ver : %ld.%ld.%ld"
	#define MSG_REPLACE_VER_UNKNOWN_STR "  Ver : ?????"
	#define MSG_GET_VERSION_STR "Version"
	#define MSG_REPLACE_VERSION_TITLE_STR "Looking for version info..."
	#define MSG_LISTER_TITLE_NAME_STR "Name"
	#define MSG_LISTER_TITLE_SIZE_STR "Size"
	#define MSG_LISTER_TITLE_PROTECT_STR "Access"
	#define MSG_LISTER_TITLE_DATE_STR "Date"
	#define MSG_LISTER_TITLE_COMMENT_STR "Comment"
	#define MSG_LISTER_TITLE_FILETYPE_STR "Type"
	#define MSG_LISTER_TITLE_OWNER_STR "Owner"
	#define MSG_LISTER_TITLE_GROUP_STR "Group"
	#define MSG_LISTER_TITLE_NETPROT_STR "Net"
	#define MSG_LISTER_TITLE_VERSION_STR "Version"
	#define MSG_LISTER_TITLE_VOLUME_STR "Volume"
	#define MSG_LISTER_TITLE_DEVICE_STR "Device"
	#define MSG_LISTER_TITLE_FULL_STR "Full"
	#define MSG_LISTER_TITLE_FREE_STR "Free"
	#define MSG_LISTER_TITLE_USED_STR "In Use"
	#define MSG_CLI_HELP_STR                                                                                          \
		"Type 'help list' for a list of commands,\nor 'help <command>' for a command template.\nType 'quit' to exit " \
		"the CLI.\n\nEnter an internal command with arguments to\nexecute it. To execute a DOpus ARexx "              \
		"command,\nenter a + symbol before the command. Append a &\nif you want the command to be asynchronous.\n"
	#define MSG_CLI_NO_TEMPLATE_STR "Command has no template."
	#define MSG_CLI_TYPE_HELP_STR "Type 'help' for help.\n"
	#define MSG_CLI_BAD_CMD_STR "Unknown command."
	#define MSG_ICONPOS_WINDOW_STR "Icon positioning area"
	#define MSG_ICONPOS_INSTRUCTIONS_STR \
		"Click and drag to mark areas where icons can be positioned.\nClick OK or Cancel when finished."
	#define MSG_ICONPOS_APPICON_STR "AppIcons"
	#define MSG_ICONPOS_DISKS_STR "Disks"
	#define MSG_ICONPOS_LISTERS_STR "Listers/Buttons"
	#define MSG_ICONPOS_PRIORITY_STR "Priority"
	#define MSG_ICONPOS_GROUPS_STR "Groups"
	#define MSG_ICONPOS_LEFTOUT_STR "Left-outs"
	#define MSG_COMMAND_SAVE_STR "Enter Filename To Save Command"
	#define MSG_SAVING_COMMAND_STR "saving command"
	#define MSG_EDIT_COMMAND_STR "Edit Command..."
	#define MSG_COMMAND_DESCRIPTION_STR "Enter a description for this command."
	#define MSG_COMMAND_LEAVEOUT_STR "Leave out on desktop"
	#define MSG_START_APPEARANCE_STR "Appearance"
	#define MSG_START_BORDERLESS_STR "Border"
	#define MSG_START_DRAGBAR_LEFT_STR "Left"
	#define MSG_START_DRAGBAR_RIGHT_STR "Right"
	#define MSG_START_IMAGE_STR "Change Image..."
	#define MSG_START_LABEL_STR "Change Label..."
	#define MSG_START_SELECT_IMAGE_STR "Select Image For Start Menu"
	#define MSG_START_SELECT_LABEL_STR "Enter label for Start Menu."
	#define MSG_OPEN_STARTMENU_MENU_STR "Load Start Menu..."
	#define MSG_START_SELECT_MENU_STR "Select Start Menu To Load"
	#define MSG_START_FONT_STR "Change Font..."
	#define MSG_START_SELECT_FONT_STR "Select Font For Start Menu"
	#define MSG_START_SCALING_STR "Scale Images"
	#define MSG_START_DRAGBAR_STR "Drag Bar"
	#define MSG_START_DRAGBAR_OFF_STR "Off"
	#define MSG_START_LABEL_FONT_STR "Change Label Font..."
	#define MSG_START_PICTURE_STR "Change Background..."
	#define MSG_START_SELECT_PICTURE_STR "Select Background Picture For Start Menu"
	#define MSG_KEYFINDER_TITLE_STR "Key Finder"
	#define MSG_KEYFINDER_KEY_STR "_Key:"
	#define MSG_KEYFINDER_FOUND_STR "Found:"
	#define MSG_KEYFINDER_TYPE_INVALID_STR "Invalid key!"
	#define MSG_KEYFINDER_TYPE_NONE_STR "No match found!"
	#define MSG_KEYFINDER_TYPE_BUTTONS_STR "Match found in a Button Bank."
	#define MSG_KEYFINDER_TYPE_TOOLBAR_STR "Match found in Lister Toolbar."
	#define MSG_KEYFINDER_TYPE_LISTERMENU_STR "Match found in Lister Menu."
	#define MSG_KEYFINDER_TYPE_MENU_STR "Match found in User Menu."
	#define MSG_KEYFINDER_TYPE_START_STR "Match found in a Start Menu."
	#define MSG_KEYFINDER_TYPE_HOTKEYS_STR "Match found in Hotkeys."
	#define MSG_KEYFINDER_TYPE_SCRIPTS_STR "Match found in Scripts."

#endif /* CATCOMP_STRINGS */

/****************************************************************************/

#ifdef CATCOMP_ARRAY

struct CatCompArrayType
{
	LONG cca_ID;
	STRPTR cca_Str;
};

static const struct CatCompArrayType CatCompArray[] = {
	{MSG_ABORTED, (STRPTR)MSG_ABORTED_STR},
	{MSG_TRY_AGAIN, (STRPTR)MSG_TRY_AGAIN_STR},
	{MSG_RETRY, (STRPTR)MSG_RETRY_STR},
	{MSG_ABORT, (STRPTR)MSG_ABORT_STR},
	{MSG_REPLACE, (STRPTR)MSG_REPLACE_STR},
	{MSG_SKIP, (STRPTR)MSG_SKIP_STR},
	{MSG_SKIP_ALL, (STRPTR)MSG_SKIP_ALL_STR},
	{MSG_ALL, (STRPTR)MSG_ALL_STR},
	{MSG_QUIT, (STRPTR)MSG_QUIT_STR},
	{MSG_SAVE, (STRPTR)MSG_SAVE_STR},
	{MSG_CLOSE, (STRPTR)MSG_CLOSE_STR},
	{MSG_YES, (STRPTR)MSG_YES_STR},
	{MSG_NO, (STRPTR)MSG_NO_STR},
	{MSG_CONTINUE, (STRPTR)MSG_CONTINUE_STR},
	{MSG_OKAY, (STRPTR)MSG_OKAY_STR},
	{MSG_CANCEL, (STRPTR)MSG_CANCEL_STR},
	{MSG_REPLACE_ALL, (STRPTR)MSG_REPLACE_ALL_STR},
	{MSG_PROCEED, (STRPTR)MSG_PROCEED_STR},
	{MSG_MOVE, (STRPTR)MSG_MOVE_STR},
	{MSG_DISCARD, (STRPTR)MSG_DISCARD_STR},
	{MSG_BLOCKS, (STRPTR)MSG_BLOCKS_STR},
	{MSG_FREE, (STRPTR)MSG_FREE_STR},
	{MSG_MAKELINK, (STRPTR)MSG_MAKELINK_STR},
	{MSG_SELECT_FILE, (STRPTR)MSG_SELECT_FILE_STR},
	{MSG_SELECT_DIR, (STRPTR)MSG_SELECT_DIR_STR},
	{MSG_ENTER_FILE, (STRPTR)MSG_ENTER_FILE_STR},
	{MSG_OPEN, (STRPTR)MSG_OPEN_STR},
	{MSG_DEFAULTS, (STRPTR)MSG_DEFAULTS_STR},
	{MSG_LAST_SAVED, (STRPTR)MSG_LAST_SAVED_STR},
	{MSG_RESTORE, (STRPTR)MSG_RESTORE_STR},
	{MSG_PROJECT, (STRPTR)MSG_PROJECT_STR},
	{MSG_EDIT, (STRPTR)MSG_EDIT_STR},
	{MSG_UNKNOWN_TYPE, (STRPTR)MSG_UNKNOWN_TYPE_STR},
	{MSG_SNIFF_CONFIRMATION, (STRPTR)MSG_SNIFF_CONFIRMATION_STR},
	{MSG_SNIFF, (STRPTR)MSG_SNIFF_STR},
	{MSG_SCRIPTS_CHANGED, (STRPTR)MSG_SCRIPTS_CHANGED_STR},
	{MSG_LISTER_MENU_CHANGED, (STRPTR)MSG_LISTER_MENU_CHANGED_STR},
	{MSG_USER_MENU_CHANGED, (STRPTR)MSG_USER_MENU_CHANGED_STR},
	{MSG_HOTKEYS_CHANGED, (STRPTR)MSG_HOTKEYS_CHANGED_STR},
	{MSG_OPEN_NEW_WINDOW, (STRPTR)MSG_OPEN_NEW_WINDOW_STR},
	{MSG_OPEN_WITH_MENU, (STRPTR)MSG_OPEN_WITH_MENU_STR},
	{MSG_SELECT_APP, (STRPTR)MSG_SELECT_APP_STR},
	{MSG_OPEN_WITH_MENU_SUB, (STRPTR)MSG_OPEN_WITH_MENU_SUB_STR},
	{MSG_OPEN_WITH_MENU_OTHER, (STRPTR)MSG_OPEN_WITH_MENU_OTHER_STR},
	{MSG_ENTER_ARGUMENTS_FOR, (STRPTR)MSG_ENTER_ARGUMENTS_FOR_STR},
	{MSG_DIRECTORY, (STRPTR)MSG_DIRECTORY_STR},
	{MSG_SELECT_UNPROTECT, (STRPTR)MSG_SELECT_UNPROTECT_STR},
	{MSG_FOUND_A_MATCH, (STRPTR)MSG_FOUND_A_MATCH_STR},
	{MSG_COMMENTING, (STRPTR)MSG_COMMENTING_STR},
	{MSG_PROTECTING, (STRPTR)MSG_PROTECTING_STR},
	{MSG_DATESTAMPING, (STRPTR)MSG_DATESTAMPING_STR},
	{MSG_DELETING, (STRPTR)MSG_DELETING_STR},
	{MSG_RENAMING, (STRPTR)MSG_RENAMING_STR},
	{MSG_ENTER_COMMENT, (STRPTR)MSG_ENTER_COMMENT_STR},
	{MSG_ENTER_PASSWORD, (STRPTR)MSG_ENTER_PASSWORD_STR},
	{MSG_ENTER_DATE_AND_TIME, (STRPTR)MSG_ENTER_DATE_AND_TIME_STR},
	{MSG_ENTER_ARGUMENTS, (STRPTR)MSG_ENTER_ARGUMENTS_STR},
	{MSG_REALLY_QUIT, (STRPTR)MSG_REALLY_QUIT_STR},
	{MSG_FILE, (STRPTR)MSG_FILE_STR},
	{MSG_SELECT_PROTECTION_BITS, (STRPTR)MSG_SELECT_PROTECTION_BITS_STR},
	{MSG_DIRECTORY_OPUS_REQUEST, (STRPTR)MSG_DIRECTORY_OPUS_REQUEST_STR},
	{MSG_MULTI_DRAG, (STRPTR)MSG_MULTI_DRAG_STR},
	{MSG_PRESS_MOUSE_BUTTON, (STRPTR)MSG_PRESS_MOUSE_BUTTON_STR},
	{MSG_COMMENT, (STRPTR)MSG_COMMENT_STR},
	{MSG_PROTECT, (STRPTR)MSG_PROTECT_STR},
	{MSG_DATESTAMP, (STRPTR)MSG_DATESTAMP_STR},
	{MSG_PROGRESS_OPERATION_MOVING, (STRPTR)MSG_PROGRESS_OPERATION_MOVING_STR},
	{MSG_PROGRAM_LOADING, (STRPTR)MSG_PROGRAM_LOADING_STR},
	{MSG_OPEN_MENU, (STRPTR)MSG_OPEN_MENU_STR},
	{MSG_SAVE_MENU, (STRPTR)MSG_SAVE_MENU_STR},
	{MSG_SAVEAS_MENU, (STRPTR)MSG_SAVEAS_MENU_STR},
	{MSG_ACT_RECURSIVELY, (STRPTR)MSG_ACT_RECURSIVELY_STR},
	{MSG_PROGRESS_OPERATION_COMMENT, (STRPTR)MSG_PROGRESS_OPERATION_COMMENT_STR},
	{MSG_PROGRESS_OPERATION_PROTECT, (STRPTR)MSG_PROGRESS_OPERATION_PROTECT_STR},
	{MSG_PROGRESS_OPERATION_DATESTAMP, (STRPTR)MSG_PROGRESS_OPERATION_DATESTAMP_STR},
	{MSG_SET_PROTECT_OLD, (STRPTR)MSG_SET_PROTECT_OLD_STR},
	{MSG_SET_PROTECT_SET, (STRPTR)MSG_SET_PROTECT_SET_STR},
	{MSG_SET_PROTECT_CLEAR, (STRPTR)MSG_SET_PROTECT_CLEAR_STR},
	{MSG_OK_BUTTON, (STRPTR)MSG_OK_BUTTON_STR},
	{MSG_ALL_BUTTON, (STRPTR)MSG_ALL_BUTTON_STR},
	{MSG_SKIP_BUTTON, (STRPTR)MSG_SKIP_BUTTON_STR},
	{MSG_ABORT_BUTTON, (STRPTR)MSG_ABORT_BUTTON_STR},
	{MSG_PROGRESS_OPERATION_ENCRYPTING, (STRPTR)MSG_PROGRESS_OPERATION_ENCRYPTING_STR},
	{MSG_USER_MENU, (STRPTR)MSG_USER_MENU_STR},
	{MSG_LISTER_MENU_TITLE, (STRPTR)MSG_LISTER_MENU_TITLE_STR},
	{MSG_USER_MENU_TITLE, (STRPTR)MSG_USER_MENU_TITLE_STR},
	{MSG_EMPTY, (STRPTR)MSG_EMPTY_STR},
	{MSG_CANCEL_BUTTON, (STRPTR)MSG_CANCEL_BUTTON_STR},
	{MSG_OPEN_BUTTON, (STRPTR)MSG_OPEN_BUTTON_STR},
	{MSG_NEW_BUTTON, (STRPTR)MSG_NEW_BUTTON_STR},
	{MSG_PARENT, (STRPTR)MSG_PARENT_STR},
	{MSG_ROOT, (STRPTR)MSG_ROOT_STR},
	{MSG_DEVICE_LIST, (STRPTR)MSG_DEVICE_LIST_STR},
	{MSG_SCANNING_DIRECTORIES, (STRPTR)MSG_SCANNING_DIRECTORIES_STR},
	{MSG_READING_DIRECTORY, (STRPTR)MSG_READING_DIRECTORY_STR},
	{MSG_CHECK_FIT_RESULTS, (STRPTR)MSG_CHECK_FIT_RESULTS_STR},
	{MSG_FIT, (STRPTR)MSG_FIT_STR},
	{MSG_BLOCKS_NEEDED, (STRPTR)MSG_BLOCKS_NEEDED_STR},
	{MSG_EXECUTE_ENTER_COMMAND, (STRPTR)MSG_EXECUTE_ENTER_COMMAND_STR},
	{MSG_TOOL_MENU, (STRPTR)MSG_TOOL_MENU_STR},
	{MSG_BUFFER_LIST, (STRPTR)MSG_BUFFER_LIST_STR},
	{MSG_REREAD_DIR, (STRPTR)MSG_REREAD_DIR_STR},
	{MSG_ASSIGN, (STRPTR)MSG_ASSIGN_STR},
	{MSG_FULL, (STRPTR)MSG_FULL_STR},
	{MSG_USED, (STRPTR)MSG_USED_STR},
	{MSG_VALIDATING, (STRPTR)MSG_VALIDATING_STR},
	{MSG_ENTER_FILTER_STRING, (STRPTR)MSG_ENTER_FILTER_STRING_STR},
	{MSG_VOLUME, (STRPTR)MSG_VOLUME_STR},
	{MSG_SELECT_SOURCE, (STRPTR)MSG_SELECT_SOURCE_STR},
	{MSG_SELECT_DESTINATION, (STRPTR)MSG_SELECT_DESTINATION_STR},
	{MSG_NO_OWNER, (STRPTR)MSG_NO_OWNER_STR},
	{MSG_NO_GROUP, (STRPTR)MSG_NO_GROUP_STR},
	{MSG_SCRIPTS_TITLE, (STRPTR)MSG_SCRIPTS_TITLE_STR},
	{MSG_PROGRESS_OPERATION_LINKING, (STRPTR)MSG_PROGRESS_OPERATION_LINKING_STR},
	{MSG_LINKING, (STRPTR)MSG_LINKING_STR},
	{MSG_DECRYPT, (STRPTR)MSG_DECRYPT_STR},
	{MSG_EMPTY_TRASH, (STRPTR)MSG_EMPTY_TRASH_STR},
	{MSG_REALLY_EMPTY_TRASH, (STRPTR)MSG_REALLY_EMPTY_TRASH_STR},
	{MSG_ENTER_ASSIGN_NAME, (STRPTR)MSG_ENTER_ASSIGN_NAME_STR},
	{MSG_ADD, (STRPTR)MSG_ADD_STR},
	{MSG_PATH, (STRPTR)MSG_PATH_STR},
	{MSG_DEFER, (STRPTR)MSG_DEFER_STR},
	{MSG_ASSIGNING, (STRPTR)MSG_ASSIGNING_STR},
	{MSG_CHANGE_NEEDS_RESET, (STRPTR)MSG_CHANGE_NEEDS_RESET_STR},
	{MSG_OPENING, (STRPTR)MSG_OPENING_STR},
	{MSG_LIBRARY_ERROR, (STRPTR)MSG_LIBRARY_ERROR_STR},
	{MSG_LIBRARY_ANY_VERSION, (STRPTR)MSG_LIBRARY_ANY_VERSION_STR},
	{MSG_CLI, (STRPTR)MSG_CLI_STR},
	{MSG_CX_DESC, (STRPTR)MSG_CX_DESC_STR},
	{MSG_ABORT_STARTUP, (STRPTR)MSG_ABORT_STARTUP_STR},
	{MSG_ABORT_STARTUP_BUTTONS, (STRPTR)MSG_ABORT_STARTUP_BUTTONS_STR},
	{MSG_PROTECT_TITLE, (STRPTR)MSG_PROTECT_TITLE_STR},
	{MSG_DATESTAMP_TITLE, (STRPTR)MSG_DATESTAMP_TITLE_STR},
	{MSG_COMMENT_TITLE, (STRPTR)MSG_COMMENT_TITLE_STR},
	{MSG_PATTERN_OFF, (STRPTR)MSG_PATTERN_OFF_STR},
	{MSG_THEMES_MENU, (STRPTR)MSG_THEMES_MENU_STR},
	{MSG_THEMES_LOAD, (STRPTR)MSG_THEMES_LOAD_STR},
	{MSG_THEMES_SAVE, (STRPTR)MSG_THEMES_SAVE_STR},
	{MSG_THEMES_BUILD, (STRPTR)MSG_THEMES_BUILD_STR},
	{MSG_ADDICONING, (STRPTR)MSG_ADDICONING_STR},
	{MSG_REGISTERED_TO, (STRPTR)MSG_REGISTERED_TO_STR},
	{MSG_SERIAL_NUMBER, (STRPTR)MSG_SERIAL_NUMBER_STR},
	{MSG_UNREGISTERED, (STRPTR)MSG_UNREGISTERED_STR},
	{MSG_YOU_SHOULD_REGISTER, (STRPTR)MSG_YOU_SHOULD_REGISTER_STR},
	{MSG_TRANSLATION_BY_YOUR_NAME, (STRPTR)MSG_TRANSLATION_BY_YOUR_NAME_STR},
	{MSG_SEARCH_DIRECTORIES, (STRPTR)MSG_SEARCH_DIRECTORIES_STR},
	{MSG_ENTER_SEARCH_STRING, (STRPTR)MSG_ENTER_SEARCH_STRING_STR},
	{MSG_SEARCH_NO_CASE, (STRPTR)MSG_SEARCH_NO_CASE_STR},
	{MSG_SEARCH_WILD, (STRPTR)MSG_SEARCH_WILD_STR},
	{MSG_SEARCH_ONLYWORD, (STRPTR)MSG_SEARCH_ONLYWORD_STR},
	{MSG_SEARCH_LEAVE_SELECTED, (STRPTR)MSG_SEARCH_LEAVE_SELECTED_STR},
	{MSG_SEARCH_PROMPT, (STRPTR)MSG_SEARCH_PROMPT_STR},
	{MSG_SEARCH_OUTPUT, (STRPTR)MSG_SEARCH_OUTPUT_STR},
	{MSG_PROGRESS_SEARCHING, (STRPTR)MSG_PROGRESS_SEARCHING_STR},
	{MSG_SEARCH_OUTPUT_HEADER, (STRPTR)MSG_SEARCH_OUTPUT_HEADER_STR},
	{MSG_SEARCH_READ, (STRPTR)MSG_SEARCH_READ_STR},
	{MSG_ENTER_NEW_NAME, (STRPTR)MSG_ENTER_NEW_NAME_STR},
	{MSG_PROGRESS_RENAMING, (STRPTR)MSG_PROGRESS_RENAMING_STR},
	{MSG_RENAME, (STRPTR)MSG_RENAME_STR},
	{MSG_RENAME_DISK, (STRPTR)MSG_RENAME_DISK_STR},
	{MSG_ENTER_ANOTHER_NAME, (STRPTR)MSG_ENTER_ANOTHER_NAME_STR},
	{MSG_ENTER_DIRECTORY_NAME, (STRPTR)MSG_ENTER_DIRECTORY_NAME_STR},
	{MSG_MAKEDIR_ICON, (STRPTR)MSG_MAKEDIR_ICON_STR},
	{MSG_MAKEDIR_NOICON, (STRPTR)MSG_MAKEDIR_NOICON_STR},
	{MSG_DIRECTORY_CREATED, (STRPTR)MSG_DIRECTORY_CREATED_STR},
	{MSG_ENTER_FILE_NAME, (STRPTR)MSG_ENTER_FILE_NAME_STR},
	{MSG_FILE_CREATED, (STRPTR)MSG_FILE_CREATED_STR},
	{MSG_PROGRESS_ADDING_ICONS, (STRPTR)MSG_PROGRESS_ADDING_ICONS_STR},
	{MSG_ICON_ALREADY_EXISTS, (STRPTR)MSG_ICON_ALREADY_EXISTS_STR},
	{MSG_ICON_REPLACE_IMAGE, (STRPTR)MSG_ICON_REPLACE_IMAGE_STR},
	{MSG_COPYING, (STRPTR)MSG_COPYING_STR},
	{MSG_PROGRESS_OPERATION_COPYING, (STRPTR)MSG_PROGRESS_OPERATION_COPYING_STR},
	{MSG_CANT_OVERCOPY_FILES, (STRPTR)MSG_CANT_OVERCOPY_FILES_STR},
	{MSG_COPY, (STRPTR)MSG_COPY_STR},
	{MSG_PROGRESS_OPERATION_CLONING, (STRPTR)MSG_PROGRESS_OPERATION_CLONING_STR},
	{MSG_DUPLICATE, (STRPTR)MSG_DUPLICATE_STR},
	{MSG_CANT_COPY_DIR_INTO_ITSELF, (STRPTR)MSG_CANT_COPY_DIR_INTO_ITSELF_STR},
	{MSG_MOVING, (STRPTR)MSG_MOVING_STR},
	{MSG_CANT_OVERLINK_FILES, (STRPTR)MSG_CANT_OVERLINK_FILES_STR},
	{MSG_CANT_LINK_DIR_INTO_ITSELF, (STRPTR)MSG_CANT_LINK_DIR_INTO_ITSELF_STR},
	{MSG_FROM_TO, (STRPTR)MSG_FROM_TO_STR},
	{MSG_FROM, (STRPTR)MSG_FROM_STR},
	{MSG_PROJECT_MENU, (STRPTR)MSG_PROJECT_MENU_STR},
	{MSG_BACKDROP_MENU, (STRPTR)MSG_BACKDROP_MENU_STR},
	{MSG_ABOUT_MENU, (STRPTR)MSG_ABOUT_MENU_STR},
	{MSG_HIDE_MENU, (STRPTR)MSG_HIDE_MENU_STR},
	{MSG_QUIT_MENU, (STRPTR)MSG_QUIT_MENU_STR},
	{MSG_EXECUTE_MENU, (STRPTR)MSG_EXECUTE_MENU_STR},
	{MSG_HELP_MENU, (STRPTR)MSG_HELP_MENU_STR},
	{MSG_KEYFINDER_MENU, (STRPTR)MSG_KEYFINDER_MENU_STR},
	{MSG_NEWSHELL_MENU, (STRPTR)MSG_NEWSHELL_MENU_STR},
	{MSG_WINDOW_MENU, (STRPTR)MSG_WINDOW_MENU_STR},
	{MSG_NEW_LISTER_MENU, (STRPTR)MSG_NEW_LISTER_MENU_STR},
	{MSG_CLOSE_LISTER_MENU, (STRPTR)MSG_CLOSE_LISTER_MENU_STR},
	{MSG_LISTER_SOURCE, (STRPTR)MSG_LISTER_SOURCE_STR},
	{MSG_LISTER_DEST, (STRPTR)MSG_LISTER_DEST_STR},
	{MSG_LISTER_LOCK_SOURCE, (STRPTR)MSG_LISTER_LOCK_SOURCE_STR},
	{MSG_LISTER_LOCK_DEST, (STRPTR)MSG_LISTER_LOCK_DEST_STR},
	{MSG_LISTER_UNLOCK, (STRPTR)MSG_LISTER_UNLOCK_STR},
	{MSG_LISTER_OFF, (STRPTR)MSG_LISTER_OFF_STR},
	{MSG_LISTER_UNLOCK_ALL, (STRPTR)MSG_LISTER_UNLOCK_ALL_STR},
	{MSG_CLOSE_ALL_MENU, (STRPTR)MSG_CLOSE_ALL_MENU_STR},
	{MSG_EDIT_MENU, (STRPTR)MSG_EDIT_MENU_STR},
	{MSG_LISTER_BUTTONS_MENU, (STRPTR)MSG_LISTER_BUTTONS_MENU_STR},
	{MSG_LISTER_MENU_MENU, (STRPTR)MSG_LISTER_MENU_MENU_STR},
	{MSG_TILE_MENU, (STRPTR)MSG_TILE_MENU_STR},
	{MSG_TILE_HORIZ_MENU, (STRPTR)MSG_TILE_HORIZ_MENU_STR},
	{MSG_TILE_VERT_MENU, (STRPTR)MSG_TILE_VERT_MENU_STR},
	{MSG_CASCADE_MENU, (STRPTR)MSG_CASCADE_MENU_STR},
	{MSG_LISTER_LOCK_POS, (STRPTR)MSG_LISTER_LOCK_POS_STR},
	{MSG_LISTER_ICONIFY, (STRPTR)MSG_LISTER_ICONIFY_STR},
	{MSG_LISTER_VIEW_ICONS, (STRPTR)MSG_LISTER_VIEW_ICONS_STR},
	{MSG_LISTER_VIEW, (STRPTR)MSG_LISTER_VIEW_STR},
	{MSG_LISTER_VIEW_NAME, (STRPTR)MSG_LISTER_VIEW_NAME_STR},
	{MSG_LISTER_VIEW_ICON, (STRPTR)MSG_LISTER_VIEW_ICON_STR},
	{MSG_LISTER_SHOW_ALL, (STRPTR)MSG_LISTER_SHOW_ALL_STR},
	{MSG_LISTER_ICON_ACTION, (STRPTR)MSG_LISTER_ICON_ACTION_STR},
	{MSG_LISTER_ARRANGE_ICONS, (STRPTR)MSG_LISTER_ARRANGE_ICONS_STR},
	{MSG_LISTER_ARRANGE_NAME, (STRPTR)MSG_LISTER_ARRANGE_NAME_STR},
	{MSG_LISTER_ARRANGE_TYPE, (STRPTR)MSG_LISTER_ARRANGE_TYPE_STR},
	{MSG_LISTER_ARRANGE_SIZE, (STRPTR)MSG_LISTER_ARRANGE_SIZE_STR},
	{MSG_LISTER_ARRANGE_DATE, (STRPTR)MSG_LISTER_ARRANGE_DATE_STR},
	{MSG_LISTER_NEW_DRAWER, (STRPTR)MSG_LISTER_NEW_DRAWER_STR},
	{MSG_OPEN_PARENT_MENU, (STRPTR)MSG_OPEN_PARENT_MENU_STR},
	{MSG_LISTER_RESTORE, (STRPTR)MSG_LISTER_RESTORE_STR},
	{MSG_LISTER_RESIZE_FIT, (STRPTR)MSG_LISTER_RESIZE_FIT_STR},
	{MSG_LISTER_DUAL, (STRPTR)MSG_LISTER_DUAL_STR},
	{MSG_LISTER_NEW_FILE, (STRPTR)MSG_LISTER_NEW_FILE_STR},
	{MSG_ICONS_MENU, (STRPTR)MSG_ICONS_MENU_STR},
	{MSG_ICON_OPEN_MENU, (STRPTR)MSG_ICON_OPEN_MENU_STR},
	{MSG_ICON_INFO_MENU, (STRPTR)MSG_ICON_INFO_MENU_STR},
	{MSG_ICON_SNAPSHOT_MENU, (STRPTR)MSG_ICON_SNAPSHOT_MENU_STR},
	{MSG_ICON_LEAVE_OUT_MENU, (STRPTR)MSG_ICON_LEAVE_OUT_MENU_STR},
	{MSG_ICON_PUT_AWAY_MENU, (STRPTR)MSG_ICON_PUT_AWAY_MENU_STR},
	{MSG_ICON_SELECT_ALL_MENU, (STRPTR)MSG_ICON_SELECT_ALL_MENU_STR},
	{MSG_ICON_CLEANUP, (STRPTR)MSG_ICON_CLEANUP_STR},
	{MSG_ICON_RESET, (STRPTR)MSG_ICON_RESET_STR},
	{MSG_ICON_RENAME, (STRPTR)MSG_ICON_RENAME_STR},
	{MSG_ICON_FORMAT, (STRPTR)MSG_ICON_FORMAT_STR},
	{MSG_ICON_DISKINFO, (STRPTR)MSG_ICON_DISKINFO_STR},
	{MSG_ICON_UNSNAPSHOT_MENU, (STRPTR)MSG_ICON_UNSNAPSHOT_MENU_STR},
	{MSG_ICON_COPY, (STRPTR)MSG_ICON_COPY_STR},
	{MSG_ICONS_NEW_MENU, (STRPTR)MSG_ICONS_NEW_MENU_STR},
	{MSG_ICONS_MAKEDIR_MENU, (STRPTR)MSG_ICONS_MAKEDIR_MENU_STR},
	{MSG_ICONS_NEWGROUP_MENU, (STRPTR)MSG_ICONS_NEWGROUP_MENU_STR},
	{MSG_ICON_COPY_RAM, (STRPTR)MSG_ICON_COPY_RAM_STR},
	{MSG_ICON_COPY_DF0, (STRPTR)MSG_ICON_COPY_DF0_STR},
	{MSG_ICON_COPY_OTHER, (STRPTR)MSG_ICON_COPY_OTHER_STR},
	{MSG_ICON_COPY2, (STRPTR)MSG_ICON_COPY2_STR},
	{MSG_ICON_COPY_TO, (STRPTR)MSG_ICON_COPY_TO_STR},
	{MSG_ICONS_NEWCOMMAND_MENU, (STRPTR)MSG_ICONS_NEWCOMMAND_MENU_STR},
	{MSG_ICONS_SHORTCUT_MENU, (STRPTR)MSG_ICONS_SHORTCUT_MENU_STR},
	{MSG_DESKTOP_POPUP_LEFTOUT, (STRPTR)MSG_DESKTOP_POPUP_LEFTOUT_STR},
	{MSG_DESKTOP_POPUP_COPY, (STRPTR)MSG_DESKTOP_POPUP_COPY_STR},
	{MSG_DESKTOP_POPUP_MOVE, (STRPTR)MSG_DESKTOP_POPUP_MOVE_STR},
	{MSG_ICON_COPY_DESKTOP, (STRPTR)MSG_ICON_COPY_DESKTOP_STR},
	{MSG_ICON_LINEUP, (STRPTR)MSG_ICON_LINEUP_STR},
	{MSG_ICON_SNAPSHOT_LISTER, (STRPTR)MSG_ICON_SNAPSHOT_LISTER_STR},
	{MSG_ICON_SNAPSHOT_ICONS, (STRPTR)MSG_ICON_SNAPSHOT_ICONS_STR},
	{MSG_ICON_SNAPSHOT_ALL, (STRPTR)MSG_ICON_SNAPSHOT_ALL_STR},
	{MSG_ICON_SNAPSHOT_WINDOW, (STRPTR)MSG_ICON_SNAPSHOT_WINDOW_STR},
	{MSG_ICONS_MAKEFILE_MENU, (STRPTR)MSG_ICONS_MAKEFILE_MENU_STR},
	{MSG_BUTTONS_MENU, (STRPTR)MSG_BUTTONS_MENU_STR},
	{MSG_CLOSE_BUTTONS_MENU, (STRPTR)MSG_CLOSE_BUTTONS_MENU_STR},
	{MSG_BUTTONS_NEW_GFX_MENU, (STRPTR)MSG_BUTTONS_NEW_GFX_MENU_STR},
	{MSG_BUTTONS_NEW_TEXT_MENU, (STRPTR)MSG_BUTTONS_NEW_TEXT_MENU_STR},
	{MSG_BUTTONS_NEW_STARTMENU_MENU, (STRPTR)MSG_BUTTONS_NEW_STARTMENU_MENU_STR},
	{MSG_SETTINGS_MENU, (STRPTR)MSG_SETTINGS_MENU_STR},
	{MSG_CLOCK_MENU, (STRPTR)MSG_CLOCK_MENU_STR},
	{MSG_CREATE_ICONS_MENU, (STRPTR)MSG_CREATE_ICONS_MENU_STR},
	{MSG_ENVIRONMENT_MENU, (STRPTR)MSG_ENVIRONMENT_MENU_STR},
	{MSG_FILETYPES_MENU, (STRPTR)MSG_FILETYPES_MENU_STR},
	{MSG_HOTKEYS_MENU, (STRPTR)MSG_HOTKEYS_MENU_STR},
	{MSG_MENU_MENU, (STRPTR)MSG_MENU_MENU_STR},
	{MSG_FILTER_MENU, (STRPTR)MSG_FILTER_MENU_STR},
	{MSG_OPTIONS_MENU, (STRPTR)MSG_OPTIONS_MENU_STR},
	{MSG_DEFPUBSCR_MENU, (STRPTR)MSG_DEFPUBSCR_MENU_STR},
	{MSG_MENU_HOTKEYS, (STRPTR)MSG_MENU_HOTKEYS_STR},
	{MSG_HOTKEYS_TITLE, (STRPTR)MSG_HOTKEYS_TITLE_STR},
	{MSG_MENU_SCRIPTS, (STRPTR)MSG_MENU_SCRIPTS_STR},
	{MSG_MENU_SYSMENU, (STRPTR)MSG_MENU_SYSMENU_STR},
	{MSG_MENU_ICONPOS, (STRPTR)MSG_MENU_ICONPOS_STR},
	{MSG_ENVIRONMENT_SAVE_LAYOUT_NEW, (STRPTR)MSG_ENVIRONMENT_SAVE_LAYOUT_NEW_STR},
	{MSG_ENVIRONMENT_EDIT, (STRPTR)MSG_ENVIRONMENT_EDIT_STR},
	{MSG_ENVIRONMENT_LOAD_NEW, (STRPTR)MSG_ENVIRONMENT_LOAD_NEW_STR},
	{MSG_ENVIRONMENT_SAVE_NEW, (STRPTR)MSG_ENVIRONMENT_SAVE_NEW_STR},
	{MSG_SELECT_TITLE, (STRPTR)MSG_SELECT_TITLE_STR},
	{MSG_SELECT_IGNORE, (STRPTR)MSG_SELECT_IGNORE_STR},
	{MSG_SELECT_MATCH, (STRPTR)MSG_SELECT_MATCH_STR},
	{MSG_SELECT_NOMATCH, (STRPTR)MSG_SELECT_NOMATCH_STR},
	{MSG_SELECT_NEWER, (STRPTR)MSG_SELECT_NEWER_STR},
	{MSG_SELECT_OLDER, (STRPTR)MSG_SELECT_OLDER_STR},
	{MSG_SELECT_DIFFERENT, (STRPTR)MSG_SELECT_DIFFERENT_STR},
	{MSG_SELECT_NAME, (STRPTR)MSG_SELECT_NAME_STR},
	{MSG_SELECT_DATE_FROM, (STRPTR)MSG_SELECT_DATE_FROM_STR},
	{MSG_SELECT_DATE_TO, (STRPTR)MSG_SELECT_DATE_TO_STR},
	{MSG_SELECT_PROT, (STRPTR)MSG_SELECT_PROT_STR},
	{MSG_SELECT_INCLUDE, (STRPTR)MSG_SELECT_INCLUDE_STR},
	{MSG_SELECT_EXCLUDE, (STRPTR)MSG_SELECT_EXCLUDE_STR},
	{MSG_SELECT_COMPARE, (STRPTR)MSG_SELECT_COMPARE_STR},
	{MSG_SELECT_SIMPLE, (STRPTR)MSG_SELECT_SIMPLE_STR},
	{MSG_SELECT_COMPLEX, (STRPTR)MSG_SELECT_COMPLEX_STR},
	{MSG_SELECT_ENTER_PATTERN, (STRPTR)MSG_SELECT_ENTER_PATTERN_STR},
	{MSG_SELECT_PROT_OFF, (STRPTR)MSG_SELECT_PROT_OFF_STR},
	{MSG_SELECT_ENTRY_BOTH, (STRPTR)MSG_SELECT_ENTRY_BOTH_STR},
	{MSG_SELECT_ENTRY_FILES, (STRPTR)MSG_SELECT_ENTRY_FILES_STR},
	{MSG_SELECT_ENTRY_DIRS, (STRPTR)MSG_SELECT_ENTRY_DIRS_STR},
	{MSG_SELECT_ENTRY_TYPE, (STRPTR)MSG_SELECT_ENTRY_TYPE_STR},
	{MSG_DOS_ERROR, (STRPTR)MSG_DOS_ERROR_STR},
	{MSG_DOS_ERROR_CODE, (STRPTR)MSG_DOS_ERROR_CODE_STR},
	{MSG_ERROR_OCCURED, (STRPTR)MSG_ERROR_OCCURED_STR},
	{MSG_ERROR_DEST_IS_DIRECTORY, (STRPTR)MSG_ERROR_DEST_IS_DIRECTORY_STR},
	{MSG_ERROR_DEST_IS_FILE, (STRPTR)MSG_ERROR_DEST_IS_FILE_STR},
	{MSG_ERROR_UNKNOWN_CODE, (STRPTR)MSG_ERROR_UNKNOWN_CODE_STR},
	{MSG_BUTTONS_ERROR_SAVING, (STRPTR)MSG_BUTTONS_ERROR_SAVING_STR},
	{MSG_BUTTONS_ERROR_LOADING, (STRPTR)MSG_BUTTONS_ERROR_LOADING_STR},
	{MSG_ERROR_CREATING_DIRECTORY, (STRPTR)MSG_ERROR_CREATING_DIRECTORY_STR},
	{MSG_CUSTPORT_NOT_FOUND, (STRPTR)MSG_CUSTPORT_NOT_FOUND_STR},
	{MSG_ALREADY_RUNNING, (STRPTR)MSG_ALREADY_RUNNING_STR},
	{MSG_ALREADY_RUNNING_BUTTONS, (STRPTR)MSG_ALREADY_RUNNING_BUTTONS_STR},
	{MSG_UNABLE_TO_CLOSE_SCREEN, (STRPTR)MSG_UNABLE_TO_CLOSE_SCREEN_STR},
	{MSG_ERROR_INVALID_DATE, (STRPTR)MSG_ERROR_INVALID_DATE_STR},
	{MSG_UNABLE_TO_OPEN_SCREEN, (STRPTR)MSG_UNABLE_TO_OPEN_SCREEN_STR},
	{MSG_AN_ERROR_OCCURED, (STRPTR)MSG_AN_ERROR_OCCURED_STR},
	{MSG_ERROR, (STRPTR)MSG_ERROR_STR},
	{MSG_SIZE, (STRPTR)MSG_SIZE_STR},
	{MSG_DATE, (STRPTR)MSG_DATE_STR},
	{MSG_OLD, (STRPTR)MSG_OLD_STR},
	{MSG_NEW, (STRPTR)MSG_NEW_STR},
	{MSG_FILE_EXISTS, (STRPTR)MSG_FILE_EXISTS_STR},
	{MSG_ERROR_CREATING_FILE, (STRPTR)MSG_ERROR_CREATING_FILE_STR},
	{MSG_PROGRESS_OPERATION_DELETING, (STRPTR)MSG_PROGRESS_OPERATION_DELETING_STR},
	{MSG_DELETE_CONFIRM, (STRPTR)MSG_DELETE_CONFIRM_STR},
	{MSG_DELETE, (STRPTR)MSG_DELETE_STR},
	{MSG_DELETE_ALL, (STRPTR)MSG_DELETE_ALL_STR},
	{MSG_DELETE_FILE_CONFIRM, (STRPTR)MSG_DELETE_FILE_CONFIRM_STR},
	{MSG_DELETE_DIR_CONFIRM, (STRPTR)MSG_DELETE_DIR_CONFIRM_STR},
	{MSG_DELETE_PROTECTED, (STRPTR)MSG_DELETE_PROTECTED_STR},
	{MSG_UNPROTECT, (STRPTR)MSG_UNPROTECT_STR},
	{MSG_UNPROTECT_ALL, (STRPTR)MSG_UNPROTECT_ALL_STR},
	{MSG_DELETE_LINK_CONFIRM_SINGLE, (STRPTR)MSG_DELETE_LINK_CONFIRM_SINGLE_STR},
	{MSG_DELETE_LINK_CONFIRM_MULTI, (STRPTR)MSG_DELETE_LINK_CONFIRM_MULTI_STR},
	{MSG_BUTTONS_ENTER_NAME, (STRPTR)MSG_BUTTONS_ENTER_NAME_STR},
	{MSG_BUTTONS_SELECT_FILE, (STRPTR)MSG_BUTTONS_SELECT_FILE_STR},
	{MSG_BUTTONS_STATUS_TITLE, (STRPTR)MSG_BUTTONS_STATUS_TITLE_STR},
	{MSG_BUTTONS_LOADING, (STRPTR)MSG_BUTTONS_LOADING_STR},
	{MSG_ENVIRONMENT_ENTER_NAME, (STRPTR)MSG_ENVIRONMENT_ENTER_NAME_STR},
	{MSG_ENVIRONMENT_SELECT_FILE, (STRPTR)MSG_ENVIRONMENT_SELECT_FILE_STR},
	{MSG_ENVIRONMENT_LOADING, (STRPTR)MSG_ENVIRONMENT_LOADING_STR},
	{MSG_ENVIRONMENT_SAVING, (STRPTR)MSG_ENVIRONMENT_SAVING_STR},
	{MSG_BUTTONS_CHANGE_WARNING, (STRPTR)MSG_BUTTONS_CHANGE_WARNING_STR},
	{MSG_BUTTONS_CREATE_WHICH_TYPE, (STRPTR)MSG_BUTTONS_CREATE_WHICH_TYPE_STR},
	{MSG_BUTTONS_TEXT, (STRPTR)MSG_BUTTONS_TEXT_STR},
	{MSG_BUTTONS_ICONS, (STRPTR)MSG_BUTTONS_ICONS_STR},
	{MSG_SETTINGS_SELECT_FILE, (STRPTR)MSG_SETTINGS_SELECT_FILE_STR},
	{MSG_SETTINGS_LOADING, (STRPTR)MSG_SETTINGS_LOADING_STR},
	{MSG_SETTINGS_ENTER_NAME, (STRPTR)MSG_SETTINGS_ENTER_NAME_STR},
	{MSG_SETTINGS_SAVING, (STRPTR)MSG_SETTINGS_SAVING_STR},
	{MSG_ERROR_SAVING_ENV, (STRPTR)MSG_ERROR_SAVING_ENV_STR},
	{MSG_ERROR_SAVING_OPTS, (STRPTR)MSG_ERROR_SAVING_OPTS_STR},
	{MSG_LAUNCHING_PROGRAM, (STRPTR)MSG_LAUNCHING_PROGRAM_STR},
	{MSG_MEMORY_COUNTER, (STRPTR)MSG_MEMORY_COUNTER_STR},
	{MSG_MEMORY_COUNTER_LOC, (STRPTR)MSG_MEMORY_COUNTER_LOC_STR},
	{MSG_MEMORY_COUNTER_CLOCK, (STRPTR)MSG_MEMORY_COUNTER_CLOCK_STR},
	{MSG_MEMORY_COUNTER_LOC_CLOCK, (STRPTR)MSG_MEMORY_COUNTER_LOC_CLOCK_STR},
{MSG_MEMORY_FREE_OS4, (STRPTR)MSG_MEMORY_FREE_OS4_STR},
	{MSG_LISTER_STATUS_SOURCE, (STRPTR)MSG_LISTER_STATUS_SOURCE_STR},
	{MSG_LISTER_STATUS_DEST, (STRPTR)MSG_LISTER_STATUS_DEST_STR},
	{MSG_LISTER_STATUS_BUSY, (STRPTR)MSG_LISTER_STATUS_BUSY_STR},
	{MSG_LISTER_STATUS_OFF, (STRPTR)MSG_LISTER_STATUS_OFF_STR},
	{MSG_HUNT_ENTER_FILENAME, (STRPTR)MSG_HUNT_ENTER_FILENAME_STR},
	{MSG_HUNT_FOUND_FILE, (STRPTR)MSG_HUNT_FOUND_FILE_STR},
	{MSG_NEW_LISTER, (STRPTR)MSG_NEW_LISTER_STR},
	{MSG_SEARCH_COMMENTS, (STRPTR)MSG_SEARCH_COMMENTS_STR},
	{MSG_FUNC_DESC_ADDICON, (STRPTR)MSG_FUNC_DESC_ADDICON_STR},
	{MSG_FUNC_DESC_ALL, (STRPTR)MSG_FUNC_DESC_ALL_STR},
	{MSG_FUNC_DESC_CLEARBUFFERS, (STRPTR)MSG_FUNC_DESC_CLEARBUFFERS_STR},
	{MSG_FUNC_DESC_COMMENT, (STRPTR)MSG_FUNC_DESC_COMMENT_STR},
	{MSG_FUNC_DESC_COPY, (STRPTR)MSG_FUNC_DESC_COPY_STR},
	{MSG_FUNC_DESC_COPYAS, (STRPTR)MSG_FUNC_DESC_COPYAS_STR},
	{MSG_FUNC_DESC_DATESTAMP, (STRPTR)MSG_FUNC_DESC_DATESTAMP_STR},
	{MSG_FUNC_DESC_DELETE, (STRPTR)MSG_FUNC_DESC_DELETE_STR},
	{MSG_FUNC_DESC_DISKCOPY, (STRPTR)MSG_FUNC_DESC_DISKCOPY_STR},
	{MSG_FUNC_DESC_ENCRYPT, (STRPTR)MSG_FUNC_DESC_ENCRYPT_STR},
	{MSG_FUNC_DESC_FINDFILE, (STRPTR)MSG_FUNC_DESC_FINDFILE_STR},
	{MSG_FUNC_DESC_FINISHSECTION, (STRPTR)MSG_FUNC_DESC_FINISHSECTION_STR},
	{MSG_FUNC_DESC_FORMAT, (STRPTR)MSG_FUNC_DESC_FORMAT_STR},
	{MSG_FUNC_DESC_GETSIZES, (STRPTR)MSG_FUNC_DESC_GETSIZES_STR},
	{MSG_FUNC_DESC_ICONINFO, (STRPTR)MSG_FUNC_DESC_ICONINFO_STR},
	{MSG_FUNC_DESC_LEAVEOUT, (STRPTR)MSG_FUNC_DESC_LEAVEOUT_STR},
	{MSG_FUNC_DESC_LOADBUTTONS, (STRPTR)MSG_FUNC_DESC_LOADBUTTONS_STR},
	{MSG_FUNC_DESC_MAKEDIR, (STRPTR)MSG_FUNC_DESC_MAKEDIR_STR},
	{MSG_FUNC_DESC_MOVE, (STRPTR)MSG_FUNC_DESC_MOVE_STR},
	{MSG_FUNC_DESC_MOVEAS, (STRPTR)MSG_FUNC_DESC_MOVEAS_STR},
	{MSG_FUNC_DESC_NONE, (STRPTR)MSG_FUNC_DESC_NONE_STR},
	{MSG_FUNC_DESC_PLAY, (STRPTR)MSG_FUNC_DESC_PLAY_STR},
	{MSG_FUNC_DESC_PRINT, (STRPTR)MSG_FUNC_DESC_PRINT_STR},
	{MSG_FUNC_DESC_PROTECT, (STRPTR)MSG_FUNC_DESC_PROTECT_STR},
	{MSG_FUNC_DESC_READ, (STRPTR)MSG_FUNC_DESC_READ_STR},
	{MSG_FUNC_DESC_RENAME, (STRPTR)MSG_FUNC_DESC_RENAME_STR},
	{MSG_FUNC_DESC_RUN, (STRPTR)MSG_FUNC_DESC_RUN_STR},
	{MSG_FUNC_DESC_SCANDIR, (STRPTR)MSG_FUNC_DESC_SCANDIR_STR},
	{MSG_FUNC_DESC_SEARCH, (STRPTR)MSG_FUNC_DESC_SEARCH_STR},
	{MSG_FUNC_DESC_SELECT, (STRPTR)MSG_FUNC_DESC_SELECT_STR},
	{MSG_FUNC_DESC_SHOW, (STRPTR)MSG_FUNC_DESC_SHOW_STR},
	{MSG_FUNC_DESC_SMARTREAD, (STRPTR)MSG_FUNC_DESC_SMARTREAD_STR},
	{MSG_FUNC_DESC_TOGGLE, (STRPTR)MSG_FUNC_DESC_TOGGLE_STR},
	{MSG_FUNC_DESC_CHECKFIT, (STRPTR)MSG_FUNC_DESC_CHECKFIT_STR},
	{MSG_FUNC_DESC_CLEARSIZES, (STRPTR)MSG_FUNC_DESC_CLEARSIZES_STR},
	{MSG_FUNC_DESC_ANSIREAD, (STRPTR)MSG_FUNC_DESC_ANSIREAD_STR},
	{MSG_FUNC_DESC_HEXREAD, (STRPTR)MSG_FUNC_DESC_HEXREAD_STR},
	{MSG_FUNC_DESC_CLONE, (STRPTR)MSG_FUNC_DESC_CLONE_STR},
	{MSG_FUNC_DESC_LOADENVIRONMENT, (STRPTR)MSG_FUNC_DESC_LOADENVIRONMENT_STR},
	{MSG_FUNC_DESC_LOADSETTINGS, (STRPTR)MSG_FUNC_DESC_LOADSETTINGS_STR},
	{MSG_FUNC_DESC_PARENT, (STRPTR)MSG_FUNC_DESC_PARENT_STR},
	{MSG_FUNC_DESC_ROOT, (STRPTR)MSG_FUNC_DESC_ROOT_STR},
	{MSG_FUNC_DESC_USER, (STRPTR)MSG_FUNC_DESC_USER_STR},
	{MSG_FUNC_DESC_DEVICELIST, (STRPTR)MSG_FUNC_DESC_DEVICELIST_STR},
	{MSG_FUNC_DESC_BUFFERLIST, (STRPTR)MSG_FUNC_DESC_BUFFERLIST_STR},
	{MSG_FUNC_DESC_VERIFY, (STRPTR)MSG_FUNC_DESC_VERIFY_STR},
	{MSG_FUNC_DESC_PRINTDIR, (STRPTR)MSG_FUNC_DESC_PRINTDIR_STR},
	{MSG_FUNC_DESC_SET, (STRPTR)MSG_FUNC_DESC_SET_STR},
	{MSG_FUNC_DESC_CLOSEBUTTONS, (STRPTR)MSG_FUNC_DESC_CLOSEBUTTONS_STR},
	{MSG_FUNC_DESC_QUIT, (STRPTR)MSG_FUNC_DESC_QUIT_STR},
	{MSG_FUNC_DESC_HIDE, (STRPTR)MSG_FUNC_DESC_HIDE_STR},
	{MSG_FUNC_DESC_REVEAL, (STRPTR)MSG_FUNC_DESC_REVEAL_STR},
	{MSG_FUNC_DESC_RESELECT, (STRPTR)MSG_FUNC_DESC_RESELECT_STR},
	{MSG_FUNC_DESC_MAKELINK, (STRPTR)MSG_FUNC_DESC_MAKELINK_STR},
	{MSG_FUNC_DESC_MAKELINKAS, (STRPTR)MSG_FUNC_DESC_MAKELINKAS_STR},
	{MSG_FUNC_DESC_ASSIGN, (STRPTR)MSG_FUNC_DESC_ASSIGN_STR},
	{MSG_FUNC_DESC_CONFIGURE, (STRPTR)MSG_FUNC_DESC_CONFIGURE_STR},
	{MSG_FUNC_DESC_CLI, (STRPTR)MSG_FUNC_DESC_CLI_STR},
	{MSG_FUNC_DESC_SETBACKGROUND, (STRPTR)MSG_FUNC_DESC_SETBACKGROUND_STR},
	{MSG_FUNC_DESC_RESET, (STRPTR)MSG_FUNC_DESC_RESET_STR},
	{MSG_FUNC_DESC_STOPSNIFFER, (STRPTR)MSG_FUNC_DESC_STOPSNIFFER_STR},
	{MSG_FUNC_DESC_MAKEFILE, (STRPTR)MSG_FUNC_DESC_MAKEFILE_STR},
	{MSG_ICON_GROUP, (STRPTR)MSG_ICON_GROUP_STR},
	{MSG_GROUP_ENTER_NAME, (STRPTR)MSG_GROUP_ENTER_NAME_STR},
	{MSG_RXERR_1, (STRPTR)MSG_RXERR_1_STR},
	{MSG_RXERR_5, (STRPTR)MSG_RXERR_5_STR},
	{MSG_RXERR_6, (STRPTR)MSG_RXERR_6_STR},
	{MSG_RXERR_8, (STRPTR)MSG_RXERR_8_STR},
	{MSG_RXERR_10, (STRPTR)MSG_RXERR_10_STR},
	{MSG_RXERR_12, (STRPTR)MSG_RXERR_12_STR},
	{MSG_RXERR_13, (STRPTR)MSG_RXERR_13_STR},
	{MSG_RXERR_14, (STRPTR)MSG_RXERR_14_STR},
	{MSG_RXERR_15, (STRPTR)MSG_RXERR_15_STR},
	{MSG_RXERR_20, (STRPTR)MSG_RXERR_20_STR},
	{MSG_RXERR_UNKNOWN, (STRPTR)MSG_RXERR_UNKNOWN_STR},
	{MSG_DESKTOP_REALLY_DELETE, (STRPTR)MSG_DESKTOP_REALLY_DELETE_STR},
	{MSG_DESKTOP_DELETE_GROUPS, (STRPTR)MSG_DESKTOP_DELETE_GROUPS_STR},
	{MSG_DESKTOP_DELETE_ASSIGNS, (STRPTR)MSG_DESKTOP_DELETE_ASSIGNS_STR},
	{MSG_DESKTOP_DELETE_GROUP_OBJECTS, (STRPTR)MSG_DESKTOP_DELETE_GROUP_OBJECTS_STR},
	{MSG_DESKTOP_DELETE_DESKTOP_FILES, (STRPTR)MSG_DESKTOP_DELETE_DESKTOP_FILES_STR},
	{MSG_DESKTOP_DELETE_DESKTOP_DIRS, (STRPTR)MSG_DESKTOP_DELETE_DESKTOP_DIRS_STR},
	{MSG_REPLACE_SAME, (STRPTR)MSG_REPLACE_SAME_STR},
	{MSG_REPLACE_VERSION_SAME, (STRPTR)MSG_REPLACE_VERSION_SAME_STR},
	{MSG_REPLACE_VERSION_NEWER, (STRPTR)MSG_REPLACE_VERSION_NEWER_STR},
	{MSG_REPLACE_VERSION_OLDER, (STRPTR)MSG_REPLACE_VERSION_OLDER_STR},
	{MSG_REPLACE_SIZE_BIGGER, (STRPTR)MSG_REPLACE_SIZE_BIGGER_STR},
	{MSG_REPLACE_SIZE_SMALLER, (STRPTR)MSG_REPLACE_SIZE_SMALLER_STR},
	{MSG_REPLACE_SIZE_SAME, (STRPTR)MSG_REPLACE_SIZE_SAME_STR},
	{MSG_REPLACE_DATE_NEWER, (STRPTR)MSG_REPLACE_DATE_NEWER_STR},
	{MSG_REPLACE_DATE_OLDER, (STRPTR)MSG_REPLACE_DATE_OLDER_STR},
	{MSG_REPLACE_DATE_SAME, (STRPTR)MSG_REPLACE_DATE_SAME_STR},
	{MSG_FILE_ALREADY_EXISTS, (STRPTR)MSG_FILE_ALREADY_EXISTS_STR},
	{MSG_BRIEF_FILE_EXISTS, (STRPTR)MSG_BRIEF_FILE_EXISTS_STR},
	{MSG_REPLACE_VER, (STRPTR)MSG_REPLACE_VER_STR},
	{MSG_REPLACE_VER_UNKNOWN, (STRPTR)MSG_REPLACE_VER_UNKNOWN_STR},
	{MSG_GET_VERSION, (STRPTR)MSG_GET_VERSION_STR},
	{MSG_REPLACE_VERSION_TITLE, (STRPTR)MSG_REPLACE_VERSION_TITLE_STR},
	{MSG_LISTER_TITLE_NAME, (STRPTR)MSG_LISTER_TITLE_NAME_STR},
	{MSG_LISTER_TITLE_SIZE, (STRPTR)MSG_LISTER_TITLE_SIZE_STR},
	{MSG_LISTER_TITLE_PROTECT, (STRPTR)MSG_LISTER_TITLE_PROTECT_STR},
	{MSG_LISTER_TITLE_DATE, (STRPTR)MSG_LISTER_TITLE_DATE_STR},
	{MSG_LISTER_TITLE_COMMENT, (STRPTR)MSG_LISTER_TITLE_COMMENT_STR},
	{MSG_LISTER_TITLE_FILETYPE, (STRPTR)MSG_LISTER_TITLE_FILETYPE_STR},
	{MSG_LISTER_TITLE_OWNER, (STRPTR)MSG_LISTER_TITLE_OWNER_STR},
	{MSG_LISTER_TITLE_GROUP, (STRPTR)MSG_LISTER_TITLE_GROUP_STR},
	{MSG_LISTER_TITLE_NETPROT, (STRPTR)MSG_LISTER_TITLE_NETPROT_STR},
	{MSG_LISTER_TITLE_VERSION, (STRPTR)MSG_LISTER_TITLE_VERSION_STR},
	{MSG_LISTER_TITLE_VOLUME, (STRPTR)MSG_LISTER_TITLE_VOLUME_STR},
	{MSG_LISTER_TITLE_DEVICE, (STRPTR)MSG_LISTER_TITLE_DEVICE_STR},
	{MSG_LISTER_TITLE_FULL, (STRPTR)MSG_LISTER_TITLE_FULL_STR},
	{MSG_LISTER_TITLE_FREE, (STRPTR)MSG_LISTER_TITLE_FREE_STR},
	{MSG_LISTER_TITLE_USED, (STRPTR)MSG_LISTER_TITLE_USED_STR},
	{MSG_CLI_HELP, (STRPTR)MSG_CLI_HELP_STR},
	{MSG_CLI_NO_TEMPLATE, (STRPTR)MSG_CLI_NO_TEMPLATE_STR},
	{MSG_CLI_TYPE_HELP, (STRPTR)MSG_CLI_TYPE_HELP_STR},
	{MSG_CLI_BAD_CMD, (STRPTR)MSG_CLI_BAD_CMD_STR},
	{MSG_ICONPOS_WINDOW, (STRPTR)MSG_ICONPOS_WINDOW_STR},
	{MSG_ICONPOS_INSTRUCTIONS, (STRPTR)MSG_ICONPOS_INSTRUCTIONS_STR},
	{MSG_ICONPOS_APPICON, (STRPTR)MSG_ICONPOS_APPICON_STR},
	{MSG_ICONPOS_DISKS, (STRPTR)MSG_ICONPOS_DISKS_STR},
	{MSG_ICONPOS_LISTERS, (STRPTR)MSG_ICONPOS_LISTERS_STR},
	{MSG_ICONPOS_PRIORITY, (STRPTR)MSG_ICONPOS_PRIORITY_STR},
	{MSG_ICONPOS_GROUPS, (STRPTR)MSG_ICONPOS_GROUPS_STR},
	{MSG_ICONPOS_LEFTOUT, (STRPTR)MSG_ICONPOS_LEFTOUT_STR},
	{MSG_COMMAND_SAVE, (STRPTR)MSG_COMMAND_SAVE_STR},
	{MSG_SAVING_COMMAND, (STRPTR)MSG_SAVING_COMMAND_STR},
	{MSG_EDIT_COMMAND, (STRPTR)MSG_EDIT_COMMAND_STR},
	{MSG_COMMAND_DESCRIPTION, (STRPTR)MSG_COMMAND_DESCRIPTION_STR},
	{MSG_COMMAND_LEAVEOUT, (STRPTR)MSG_COMMAND_LEAVEOUT_STR},
	{MSG_START_APPEARANCE, (STRPTR)MSG_START_APPEARANCE_STR},
	{MSG_START_BORDERLESS, (STRPTR)MSG_START_BORDERLESS_STR},
	{MSG_START_DRAGBAR_LEFT, (STRPTR)MSG_START_DRAGBAR_LEFT_STR},
	{MSG_START_DRAGBAR_RIGHT, (STRPTR)MSG_START_DRAGBAR_RIGHT_STR},
	{MSG_START_IMAGE, (STRPTR)MSG_START_IMAGE_STR},
	{MSG_START_LABEL, (STRPTR)MSG_START_LABEL_STR},
	{MSG_START_SELECT_IMAGE, (STRPTR)MSG_START_SELECT_IMAGE_STR},
	{MSG_START_SELECT_LABEL, (STRPTR)MSG_START_SELECT_LABEL_STR},
	{MSG_OPEN_STARTMENU_MENU, (STRPTR)MSG_OPEN_STARTMENU_MENU_STR},
	{MSG_START_SELECT_MENU, (STRPTR)MSG_START_SELECT_MENU_STR},
	{MSG_START_FONT, (STRPTR)MSG_START_FONT_STR},
	{MSG_START_SELECT_FONT, (STRPTR)MSG_START_SELECT_FONT_STR},
	{MSG_START_SCALING, (STRPTR)MSG_START_SCALING_STR},
	{MSG_START_DRAGBAR, (STRPTR)MSG_START_DRAGBAR_STR},
	{MSG_START_DRAGBAR_OFF, (STRPTR)MSG_START_DRAGBAR_OFF_STR},
	{MSG_START_LABEL_FONT, (STRPTR)MSG_START_LABEL_FONT_STR},
	{MSG_START_PICTURE, (STRPTR)MSG_START_PICTURE_STR},
	{MSG_START_SELECT_PICTURE, (STRPTR)MSG_START_SELECT_PICTURE_STR},
	{MSG_KEYFINDER_TITLE, (STRPTR)MSG_KEYFINDER_TITLE_STR},
	{MSG_KEYFINDER_KEY, (STRPTR)MSG_KEYFINDER_KEY_STR},
	{MSG_KEYFINDER_FOUND, (STRPTR)MSG_KEYFINDER_FOUND_STR},
	{MSG_KEYFINDER_TYPE_INVALID, (STRPTR)MSG_KEYFINDER_TYPE_INVALID_STR},
	{MSG_KEYFINDER_TYPE_NONE, (STRPTR)MSG_KEYFINDER_TYPE_NONE_STR},
	{MSG_KEYFINDER_TYPE_BUTTONS, (STRPTR)MSG_KEYFINDER_TYPE_BUTTONS_STR},
	{MSG_KEYFINDER_TYPE_TOOLBAR, (STRPTR)MSG_KEYFINDER_TYPE_TOOLBAR_STR},
	{MSG_KEYFINDER_TYPE_LISTERMENU, (STRPTR)MSG_KEYFINDER_TYPE_LISTERMENU_STR},
	{MSG_KEYFINDER_TYPE_MENU, (STRPTR)MSG_KEYFINDER_TYPE_MENU_STR},
	{MSG_KEYFINDER_TYPE_START, (STRPTR)MSG_KEYFINDER_TYPE_START_STR},
	{MSG_KEYFINDER_TYPE_HOTKEYS, (STRPTR)MSG_KEYFINDER_TYPE_HOTKEYS_STR},
	{MSG_KEYFINDER_TYPE_SCRIPTS, (STRPTR)MSG_KEYFINDER_TYPE_SCRIPTS_STR},
};

#endif /* CATCOMP_ARRAY */

/****************************************************************************/

#ifdef CATCOMP_BLOCK

static const char CatCompBlock[] = {"\x00\x00\x00\x00\x00\x0C" MSG_ABORTED_STR
									"\x00\x00"
									"\x00\x00\x00\x01\x00\x0A" MSG_TRY_AGAIN_STR
									"\x00"
									"\x00\x00\x00\x02\x00\x06" MSG_RETRY_STR
									"\x00"
									"\x00\x00\x00\x03\x00\x06" MSG_ABORT_STR
									"\x00"
									"\x00\x00\x00\x04\x00\x08" MSG_REPLACE_STR
									"\x00"
									"\x00\x00\x00\x05\x00\x06" MSG_SKIP_STR
									"\x00\x00"
									"\x00\x00\x00\x06\x00\x0A" MSG_SKIP_ALL_STR
									"\x00\x00"
									"\x00\x00\x00\x07\x00\x04" MSG_ALL_STR
									"\x00"
									"\x00\x00\x00\x08\x00\x06" MSG_QUIT_STR
									"\x00\x00"
									"\x00\x00\x00\x09\x00\x06" MSG_SAVE_STR
									"\x00\x00"
									"\x00\x00\x00\x0A\x00\x06" MSG_CLOSE_STR
									"\x00"
									"\x00\x00\x00\x0B\x00\x04" MSG_YES_STR
									"\x00"
									"\x00\x00\x00\x0C\x00\x04" MSG_NO_STR
									"\x00\x00"
									"\x00\x00\x00\x0D\x00\x0A" MSG_CONTINUE_STR
									"\x00\x00"
									"\x00\x00\x00\x0E\x00\x04" MSG_OKAY_STR
									"\x00\x00"
									"\x00\x00\x00\x0F\x00\x08" MSG_CANCEL_STR
									"\x00\x00"
									"\x00\x00\x00\x10\x00\x0C" MSG_REPLACE_ALL_STR
									"\x00"
									"\x00\x00\x00\x11\x00\x08" MSG_PROCEED_STR
									"\x00"
									"\x00\x00\x00\x12\x00\x06" MSG_MOVE_STR
									"\x00\x00"
									"\x00\x00\x00\x13\x00\x08" MSG_DISCARD_STR
									"\x00"
									"\x00\x00\x00\x14\x00\x08" MSG_BLOCKS_STR
									"\x00\x00"
									"\x00\x00\x00\x15\x00\x06" MSG_FREE_STR
									"\x00\x00"
									"\x00\x00\x00\x16\x00\x0A" MSG_MAKELINK_STR
									"\x00\x00"
									"\x00\x00\x00\x17\x00\x0C" MSG_SELECT_FILE_STR
									"\x00"
									"\x00\x00\x00\x18\x00\x12" MSG_SELECT_DIR_STR
									"\x00\x00"
									"\x00\x00\x00\x19\x00\x10" MSG_ENTER_FILE_STR
									"\x00\x00"
									"\x00\x00\x00\x1A\x00\x08" MSG_OPEN_STR
									"\x00"
									"\x00\x00\x00\x1B\x00\x12" MSG_DEFAULTS_STR
									"\x00"
									"\x00\x00\x00\x1C\x00\x0C" MSG_LAST_SAVED_STR
									"\x00\x00"
									"\x00\x00\x00\x1D\x00\x08" MSG_RESTORE_STR
									"\x00"
									"\x00\x00\x00\x1E\x00\x08" MSG_PROJECT_STR
									"\x00"
									"\x00\x00\x00\x1F\x00\x06" MSG_EDIT_STR
									"\x00\x00"
									"\x00\x00\x00\x20\x00\x0A" MSG_UNKNOWN_TYPE_STR
									"\x00"
									"\x00\x00\x00\x21\x00\x50" MSG_SNIFF_CONFIRMATION_STR
									"\x00\x00"
									"\x00\x00\x00\x22\x00\x08" MSG_SNIFF_STR
									"\x00\x00"
									"\x00\x00\x00\x23\x00\x46" MSG_SCRIPTS_CHANGED_STR
									"\x00"
									"\x00\x00\x00\x24\x00\x48" MSG_LISTER_MENU_CHANGED_STR
									"\x00"
									"\x00\x00\x00\x25\x00\x4A" MSG_USER_MENU_CHANGED_STR
									"\x00\x00"
									"\x00\x00\x00\x26\x00\x46" MSG_HOTKEYS_CHANGED_STR
									"\x00"
									"\x00\x00\x00\x27\x00\x14" MSG_OPEN_NEW_WINDOW_STR
									"\x00\x00"
									"\x00\x00\x00\x28\x00\x0E" MSG_OPEN_WITH_MENU_STR
									"\x00\x00"
									"\x00\x00\x00\x29\x00\x20" MSG_SELECT_APP_STR
									"\x00"
									"\x00\x00\x00\x2A\x00\x0A" MSG_OPEN_WITH_MENU_SUB_STR
									"\x00"
									"\x00\x00\x00\x2B\x00\x0A" MSG_OPEN_WITH_MENU_OTHER_STR
									"\x00\x00"
									"\x00\x00\x03\xE8\x00\x1A" MSG_ENTER_ARGUMENTS_FOR_STR
									"\x00\x00"
									"\x00\x00\x03\xE9\x00\x0C" MSG_DIRECTORY_STR
									"\x00\x00"
									"\x00\x00\x03\xEA\x00\x2C" MSG_SELECT_UNPROTECT_STR
									"\x00\x00"
									"\x00\x00\x03\xEB\x00\x20" MSG_FOUND_A_MATCH_STR
									"\x00\x00"
									"\x00\x00\x03\xEC\x00\x0C" MSG_COMMENTING_STR
									"\x00\x00"
									"\x00\x00\x03\xED\x00\x0C" MSG_PROTECTING_STR
									"\x00\x00"
									"\x00\x00\x03\xEE\x00\x0E" MSG_DATESTAMPING_STR
									"\x00"
									"\x00\x00\x03\xEF\x00\x0A" MSG_DELETING_STR
									"\x00\x00"
									"\x00\x00\x03\xF0\x00\x0A" MSG_RENAMING_STR
									"\x00\x00"
									"\x00\x00\x03\xF1\x00\x1A" MSG_ENTER_COMMENT_STR
									"\x00"
									"\x00\x00\x03\xF2\x00\x22" MSG_ENTER_PASSWORD_STR
									"\x00\x00"
									"\x00\x00\x03\xF3\x00\x24" MSG_ENTER_DATE_AND_TIME_STR
									"\x00"
									"\x00\x00\x03\xF4\x00\x10" MSG_ENTER_ARGUMENTS_STR
									"\x00"
									"\x00\x00\x03\xF5\x00\x2C" MSG_REALLY_QUIT_STR
									"\x00\x00"
									"\x00\x00\x03\xF6\x00\x06" MSG_FILE_STR
									"\x00\x00"
									"\x00\x00\x03\xF7\x00\x18" MSG_SELECT_PROTECTION_BITS_STR
									"\x00\x00"
									"\x00\x00\x03\xF8\x00\x18" MSG_DIRECTORY_OPUS_REQUEST_STR
									"\x00\x00"
									"\x00\x00\x03\xFC\x00\x2A" MSG_MULTI_DRAG_STR
									"\x00"
									"\x00\x00\x03\xFD\x00\x22" MSG_PRESS_MOUSE_BUTTON_STR
									"\x00"
									"\x00\x00\x03\xFE\x00\x08" MSG_COMMENT_STR
									"\x00"
									"\x00\x00\x03\xFF\x00\x08" MSG_PROTECT_STR
									"\x00"
									"\x00\x00\x04\x00\x00\x0C" MSG_DATESTAMP_STR
									"\x00\x00"
									"\x00\x00\x04\x01\x00\x0A" MSG_PROGRESS_OPERATION_MOVING_STR
									"\x00"
									"\x00\x00\x04\x02\x00\x20" MSG_PROGRAM_LOADING_STR
									"\x00\x00"
									"\x00\x00\x04\x03\x00\x08" MSG_OPEN_MENU_STR
									"\x00"
									"\x00\x00\x04\x04\x00\x06" MSG_SAVE_MENU_STR
									"\x00\x00"
									"\x00\x00\x04\x05\x00\x0C" MSG_SAVEAS_MENU_STR
									"\x00\x00"
									"\x00\x00\x04\x06\x00\x22" MSG_ACT_RECURSIVELY_STR
									"\x00\x00"
									"\x00\x00\x04\x07\x00\x14" MSG_PROGRESS_OPERATION_COMMENT_STR
									"\x00"
									"\x00\x00\x04\x08\x00\x18" MSG_PROGRESS_OPERATION_PROTECT_STR
									"\x00\x00"
									"\x00\x00\x04\x09\x00\x16" MSG_PROGRESS_OPERATION_DATESTAMP_STR
									"\x00"
									"\x00\x00\x04\x0A\x00\x08" MSG_SET_PROTECT_OLD_STR
									"\x00"
									"\x00\x00\x04\x0B\x00\x04" MSG_SET_PROTECT_SET_STR
									"\x00"
									"\x00\x00\x04\x0C\x00\x06" MSG_SET_PROTECT_CLEAR_STR
									"\x00"
									"\x00\x00\x04\x0D\x00\x04" MSG_OK_BUTTON_STR
									"\x00"
									"\x00\x00\x04\x0E\x00\x06" MSG_ALL_BUTTON_STR
									"\x00\x00"
									"\x00\x00\x04\x0F\x00\x06" MSG_SKIP_BUTTON_STR
									"\x00"
									"\x00\x00\x04\x10\x00\x08" MSG_ABORT_BUTTON_STR
									"\x00\x00"
									"\x00\x00\x04\x11\x00\x0E" MSG_PROGRESS_OPERATION_ENCRYPTING_STR
									"\x00"
									"\x00\x00\x04\x12\x00\x06" MSG_USER_MENU_STR
									"\x00\x00"
									"\x00\x00\x04\x13\x00\x14" MSG_LISTER_MENU_TITLE_STR
									"\x00\x00"
									"\x00\x00\x04\x14\x00\x0C" MSG_USER_MENU_TITLE_STR
									"\x00\x00"
									"\x00\x00\x04\x15\x00\x06" MSG_EMPTY_STR
									"\x00"
									"\x00\x00\x04\x16\x00\x08" MSG_CANCEL_BUTTON_STR
									"\x00"
									"\x00\x00\x04\x17\x00\x06" MSG_OPEN_BUTTON_STR
									"\x00"
									"\x00\x00\x04\x18\x00\x06" MSG_NEW_BUTTON_STR
									"\x00\x00"
									"\x00\x00\x04\x19\x00\x12" MSG_PARENT_STR
									"\x00\x00"
									"\x00\x00\x04\x1A\x00\x10" MSG_ROOT_STR
									"\x00\x00"
									"\x00\x00\x04\x1B\x00\x0C" MSG_DEVICE_LIST_STR
									"\x00"
									"\x00\x00\x04\x1C\x00\x18" MSG_SCANNING_DIRECTORIES_STR
									"\x00"
									"\x00\x00\x04\x1D\x00\x16" MSG_READING_DIRECTORY_STR
									"\x00\x00"
									"\x00\x00\x04\x1E\x00\x16" MSG_CHECK_FIT_RESULTS_STR
									"\x00"
									"\x00\x00\x04\x1F\x00\x04" MSG_FIT_STR
									"\x00"
									"\x00\x00\x04\x20\x00\x10" MSG_BLOCKS_NEEDED_STR
									"\x00\x00"
									"\x00\x00\x04\x21\x00\x1E" MSG_EXECUTE_ENTER_COMMAND_STR
									"\x00\x00"
									"\x00\x00\x04\x22\x00\x06" MSG_TOOL_MENU_STR
									"\x00"
									"\x00\x00\x04\x23\x00\x0C" MSG_BUFFER_LIST_STR
									"\x00\x00"
									"\x00\x00\x04\x24\x00\x12" MSG_REREAD_DIR_STR
									"\x00"
									"\x00\x00\x04\x25\x00\x08" MSG_ASSIGN_STR
									"\x00\x00"
									"\x00\x00\x04\x26\x00\x06" MSG_FULL_STR
									"\x00\x00"
									"\x00\x00\x04\x27\x00\x08" MSG_USED_STR
									"\x00\x00"
									"\x00\x00\x04\x28\x00\x10" MSG_VALIDATING_STR
									"\x00"
									"\x00\x00\x04\x29\x00\x2E" MSG_ENTER_FILTER_STRING_STR
									"\x00\x00"
									"\x00\x00\x04\x2A\x00\x08" MSG_VOLUME_STR
									"\x00\x00"
									"\x00\x00\x04\x2B\x00\x0E" MSG_SELECT_SOURCE_STR
									"\x00"
									"\x00\x00\x04\x2C\x00\x14" MSG_SELECT_DESTINATION_STR
									"\x00\x00"
									"\x00\x00\x04\x2D\x00\x0C" MSG_NO_OWNER_STR
									"\x00\x00"
									"\x00\x00\x04\x2E\x00\x0C" MSG_NO_GROUP_STR
									"\x00\x00"
									"\x00\x00\x04\x2F\x00\x08" MSG_SCRIPTS_TITLE_STR
									"\x00"
									"\x00\x00\x04\x30\x00\x10" MSG_PROGRESS_OPERATION_LINKING_STR
									"\x00"
									"\x00\x00\x04\x31\x00\x12" MSG_LINKING_STR
									"\x00\x00"
									"\x00\x00\x04\x32\x00\x10" MSG_DECRYPT_STR
									"\x00\x00"
									"\x00\x00\x04\x33\x00\x0C" MSG_EMPTY_TRASH_STR
									"\x00"
									"\x00\x00\x04\x34\x00\x4C" MSG_REALLY_EMPTY_TRASH_STR
									"\x00\x00"
									"\x00\x00\x04\x35\x00\x1E" MSG_ENTER_ASSIGN_NAME_STR
									"\x00\x00"
									"\x00\x00\x04\x36\x00\x04" MSG_ADD_STR
									"\x00"
									"\x00\x00\x04\x37\x00\x06" MSG_PATH_STR
									"\x00\x00"
									"\x00\x00\x04\x38\x00\x06" MSG_DEFER_STR
									"\x00"
									"\x00\x00\x04\x39\x00\x0A" MSG_ASSIGNING_STR
									"\x00"
									"\x00\x00\x04\x3A\x00\x44" MSG_CHANGE_NEEDS_RESET_STR
									"\x00\x00"
									"\x00\x00\x04\x3B\x00\x08" MSG_OPENING_STR
									"\x00"
									"\x00\x00\x04\x3C\x00\x18" MSG_LIBRARY_ERROR_STR
									"\x00\x00"
									"\x00\x00\x04\x3D\x00\x0C" MSG_LIBRARY_ANY_VERSION_STR
									"\x00"
									"\x00\x00\x04\x3E\x00\x04" MSG_CLI_STR
									"\x00"
									"\x00\x00\x04\x3F\x00\x24" MSG_CX_DESC_STR
									"\x00"
									"\x00\x00\x04\x40\x00\x1E" MSG_ABORT_STARTUP_STR
									"\x00"
									"\x00\x00\x04\x41\x00\x10" MSG_ABORT_STARTUP_BUTTONS_STR
									"\x00\x00"
									"\x00\x00\x04\x42\x00\x08" MSG_PROTECT_TITLE_STR
									"\x00"
									"\x00\x00\x04\x43\x00\x0A" MSG_DATESTAMP_TITLE_STR
									"\x00"
									"\x00\x00\x04\x44\x00\x08" MSG_COMMENT_TITLE_STR
									"\x00"
									"\x00\x00\x04\x45\x00\x50" MSG_PATTERN_OFF_STR
									"\x00"
									"\x00\x00\x04\x46\x00\x08" MSG_THEMES_MENU_STR
									"\x00\x00"
									"\x00\x00\x04\x47\x00\x0E" MSG_THEMES_LOAD_STR
									"\x00"
									"\x00\x00\x04\x48\x00\x0E" MSG_THEMES_SAVE_STR
									"\x00"
									"\x00\x00\x04\x49\x00\x10" MSG_THEMES_BUILD_STR
									"\x00\x00"
									"\x00\x00\x04\x4A\x00\x10" MSG_ADDICONING_STR
									"\x00"
									"\x00\x00\x07\xD0\x00\x1C" MSG_REGISTERED_TO_STR
									"\x00"
									"\x00\x00\x07\xD1\x00\x0A" MSG_SERIAL_NUMBER_STR
									"\x00"
									"\x00\x00\x07\xD2\x00\x1C" MSG_UNREGISTERED_STR
									"\x00\x00"
									"\x00\x00\x07\xD3\x00\x20" MSG_YOU_SHOULD_REGISTER_STR
									"\x00"
									"\x00\x00\x07\xD4\x00\x02" MSG_TRANSLATION_BY_YOUR_NAME_STR
									"\x00\x00"
									"\x00\x00\x08\x34\x00\x26" MSG_SEARCH_DIRECTORIES_STR
									"\x00"
									"\x00\x00\x08\x35\x00\x1C" MSG_ENTER_SEARCH_STRING_STR
									"\x00"
									"\x00\x00\x08\x36\x00\x12" MSG_SEARCH_NO_CASE_STR
									"\x00"
									"\x00\x00\x08\x37\x00\x12" MSG_SEARCH_WILD_STR
									"\x00\x00"
									"\x00\x00\x08\x38\x00\x14" MSG_SEARCH_ONLYWORD_STR
									"\x00\x00"
									"\x00\x00\x08\x39\x00\x10" MSG_SEARCH_LEAVE_SELECTED_STR
									"\x00\x00"
									"\x00\x00\x08\x3A\x00\x04" MSG_SEARCH_PROMPT_STR
									"\x00"
									"\x00\x00\x08\x3B\x00\x10" MSG_SEARCH_OUTPUT_STR
									"\x00\x00"
									"\x00\x00\x08\x3C\x00\x0E" MSG_PROGRESS_SEARCHING_STR
									"\x00\x00"
									"\x00\x00\x08\x3D\x00\x18" MSG_SEARCH_OUTPUT_HEADER_STR
									"\x00"
									"\x00\x00\x08\x3E\x00\x06" MSG_SEARCH_READ_STR
									"\x00\x00"
									"\x00\x00\x08\x98\x00\x26" MSG_ENTER_NEW_NAME_STR
									"\x00"
									"\x00\x00\x08\x99\x00\x0C" MSG_PROGRESS_RENAMING_STR
									"\x00"
									"\x00\x00\x08\x9A\x00\x08" MSG_RENAME_STR
									"\x00\x00"
									"\x00\x00\x08\x9B\x00\x1C" MSG_RENAME_DISK_STR
									"\x00\x00"
									"\x00\x00\x08\x9C\x00\x18" MSG_ENTER_ANOTHER_NAME_STR
									"\x00"
									"\x00\x00\x08\xFC\x00\x16" MSG_ENTER_DIRECTORY_NAME_STR
									"\x00\x00"
									"\x00\x00\x08\xFD\x00\x0A" MSG_MAKEDIR_ICON_STR
									"\x00"
									"\x00\x00\x08\xFE\x00\x0E" MSG_MAKEDIR_NOICON_STR
									"\x00\x00"
									"\x00\x00\x08\xFF\x00\x14" MSG_DIRECTORY_CREATED_STR
									"\x00\x00"
									"\x00\x00\x09\x00\x00\x10" MSG_ENTER_FILE_NAME_STR
									"\x00"
									"\x00\x00\x09\x01\x00\x0E" MSG_FILE_CREATED_STR
									"\x00"
									"\x00\x00\x09\x60\x00\x10" MSG_PROGRESS_ADDING_ICONS_STR
									"\x00"
									"\x00\x00\x09\x61\x00\x3C" MSG_ICON_ALREADY_EXISTS_STR
									"\x00"
									"\x00\x00\x09\x62\x00\x0C" MSG_ICON_REPLACE_IMAGE_STR
									"\x00\x00"
									"\x00\x00\x0A\x28\x00\x08" MSG_COPYING_STR
									"\x00"
									"\x00\x00\x0A\x29\x00\x0C" MSG_PROGRESS_OPERATION_COPYING_STR
									"\x00\x00"
									"\x00\x00\x0A\x2A\x00\x44" MSG_CANT_OVERCOPY_FILES_STR
									"\x00"
									"\x00\x00\x0A\x2B\x00\x06" MSG_COPY_STR
									"\x00\x00"
									"\x00\x00\x0A\x2C\x00\x10" MSG_PROGRESS_OPERATION_CLONING_STR
									"\x00\x00"
									"\x00\x00\x0A\x2D\x00\x0A" MSG_DUPLICATE_STR
									"\x00"
									"\x00\x00\x0A\x2E\x00\x28" MSG_CANT_COPY_DIR_INTO_ITSELF_STR
									"\x00"
									"\x00\x00\x0A\x2F\x00\x08" MSG_MOVING_STR
									"\x00\x00"
									"\x00\x00\x0A\x30\x00\x44" MSG_CANT_OVERLINK_FILES_STR
									"\x00"
									"\x00\x00\x0A\x31\x00\x28" MSG_CANT_LINK_DIR_INTO_ITSELF_STR
									"\x00"
									"\x00\x00\x0A\x32\x00\x12" MSG_FROM_TO_STR
									"\x00"
									"\x00\x00\x0A\x33\x00\x0A" MSG_FROM_STR
									"\x00"
									"\x00\x00\x0A\x8C\x00\x06" MSG_PROJECT_MENU_STR
									"\x00\x00"
									"\x00\x00\x0A\x8D\x00\x0A" MSG_BACKDROP_MENU_STR
									"\x00\x00"
									"\x00\x00\x0A\x8E\x00\x0A" MSG_ABOUT_MENU_STR
									"\x00\x00"
									"\x00\x00\x0A\x8F\x00\x06" MSG_HIDE_MENU_STR
									"\x00\x00"
									"\x00\x00\x0A\x90\x00\x08" MSG_QUIT_MENU_STR
									"\x00"
									"\x00\x00\x0A\x91\x00\x14" MSG_EXECUTE_MENU_STR
									"\x00\x00"
									"\x00\x00\x0A\x92\x00\x06" MSG_HELP_MENU_STR
									"\x00"
									"\x00\x00\x0A\x93\x00\x0E" MSG_KEYFINDER_MENU_STR
									"\x00"
									"\x00\x00\x0A\x94\x00\x0C" MSG_NEWSHELL_MENU_STR
									"\x00"
									"\x00\x00\x0A\xF0\x00\x08" MSG_WINDOW_MENU_STR
									"\x00"
									"\x00\x00\x0A\xF1\x00\x04" MSG_NEW_LISTER_MENU_STR
									"\x00"
									"\x00\x00\x0A\xF2\x00\x06" MSG_CLOSE_LISTER_MENU_STR
									"\x00"
									"\x00\x00\x0A\xF3\x00\x0C" MSG_LISTER_SOURCE_STR
									"\x00"
									"\x00\x00\x0A\xF4\x00\x0A" MSG_LISTER_DEST_STR
									"\x00"
									"\x00\x00\x0A\xF5\x00\x10" MSG_LISTER_LOCK_SOURCE_STR
									"\x00\x00"
									"\x00\x00\x0A\xF6\x00\x0E" MSG_LISTER_LOCK_DEST_STR
									"\x00\x00"
									"\x00\x00\x0A\xF7\x00\x08" MSG_LISTER_UNLOCK_STR
									"\x00\x00"
									"\x00\x00\x0A\xF8\x00\x0A" MSG_LISTER_OFF_STR
									"\x00\x00"
									"\x00\x00\x0A\xF9\x00\x0C" MSG_LISTER_UNLOCK_ALL_STR
									"\x00\x00"
									"\x00\x00\x0A\xFA\x00\x0A" MSG_CLOSE_ALL_MENU_STR
									"\x00"
									"\x00\x00\x0A\xFB\x00\x08" MSG_EDIT_MENU_STR
									"\x00"
									"\x00\x00\x0A\xFC\x00\x18" MSG_LISTER_BUTTONS_MENU_STR
									"\x00\x00"
									"\x00\x00\x0A\xFD\x00\x14" MSG_LISTER_MENU_MENU_STR
									"\x00"
									"\x00\x00\x0A\xFE\x00\x06" MSG_TILE_MENU_STR
									"\x00\x00"
									"\x00\x00\x0A\xFF\x00\x0E" MSG_TILE_HORIZ_MENU_STR
									"\x00\x00"
									"\x00\x00\x0B\x00\x00\x0C" MSG_TILE_VERT_MENU_STR
									"\x00\x00"
									"\x00\x00\x0B\x01\x00\x08" MSG_CASCADE_MENU_STR
									"\x00"
									"\x00\x00\x0B\x02\x00\x10" MSG_LISTER_LOCK_POS_STR
									"\x00\x00"
									"\x00\x00\x0B\x03\x00\x08" MSG_LISTER_ICONIFY_STR
									"\x00"
									"\x00\x00\x0B\x04\x00\x0C" MSG_LISTER_VIEW_ICONS_STR
									"\x00\x00"
									"\x00\x00\x0B\x05\x00\x08" MSG_LISTER_VIEW_STR
									"\x00"
									"\x00\x00\x0B\x06\x00\x06" MSG_LISTER_VIEW_NAME_STR
									"\x00\x00"
									"\x00\x00\x0B\x07\x00\x06" MSG_LISTER_VIEW_ICON_STR
									"\x00\x00"
									"\x00\x00\x0B\x08\x00\x0A" MSG_LISTER_SHOW_ALL_STR
									"\x00\x00"
									"\x00\x00\x0B\x09\x00\x0C" MSG_LISTER_ICON_ACTION_STR
									"\x00"
									"\x00\x00\x0B\x0A\x00\x0E" MSG_LISTER_ARRANGE_ICONS_STR
									"\x00"
									"\x00\x00\x0B\x0B\x00\x08" MSG_LISTER_ARRANGE_NAME_STR
									"\x00"
									"\x00\x00\x0B\x0C\x00\x08" MSG_LISTER_ARRANGE_TYPE_STR
									"\x00"
									"\x00\x00\x0B\x0D\x00\x08" MSG_LISTER_ARRANGE_SIZE_STR
									"\x00"
									"\x00\x00\x0B\x0E\x00\x08" MSG_LISTER_ARRANGE_DATE_STR
									"\x00"
									"\x00\x00\x0B\x0F\x00\x0E" MSG_LISTER_NEW_DRAWER_STR
									"\x00"
									"\x00\x00\x0B\x10\x00\x0C" MSG_OPEN_PARENT_MENU_STR
									"\x00"
									"\x00\x00\x0B\x11\x00\x08" MSG_LISTER_RESTORE_STR
									"\x00"
									"\x00\x00\x0B\x12\x00\x0E" MSG_LISTER_RESIZE_FIT_STR
									"\x00"
									"\x00\x00\x0B\x13\x00\x0C" MSG_LISTER_DUAL_STR
									"\x00"
									"\x00\x00\x0B\x14\x00\x0C" MSG_LISTER_NEW_FILE_STR
									"\x00"
									"\x00\x00\x0B\x54\x00\x06" MSG_ICONS_MENU_STR
									"\x00"
									"\x00\x00\x0B\x55\x00\x06" MSG_ICON_OPEN_MENU_STR
									"\x00\x00"
									"\x00\x00\x0B\x56\x00\x10" MSG_ICON_INFO_MENU_STR
									"\x00\x00"
									"\x00\x00\x0B\x57\x00\x0A" MSG_ICON_SNAPSHOT_MENU_STR
									"\x00\x00"
									"\x00\x00\x0B\x58\x00\x0A" MSG_ICON_LEAVE_OUT_MENU_STR
									"\x00"
									"\x00\x00\x0B\x59\x00\x0A" MSG_ICON_PUT_AWAY_MENU_STR
									"\x00\x00"
									"\x00\x00\x0B\x5A\x00\x0C" MSG_ICON_SELECT_ALL_MENU_STR
									"\x00\x00"
									"\x00\x00\x0B\x5B\x00\x0A" MSG_ICON_CLEANUP_STR
									"\x00\x00"
									"\x00\x00\x0B\x5C\x00\x06" MSG_ICON_RESET_STR
									"\x00"
									"\x00\x00\x0B\x5D\x00\x0A" MSG_ICON_RENAME_STR
									"\x00"
									"\x00\x00\x0B\x5E\x00\x10" MSG_ICON_FORMAT_STR
									"\x00\x00"
									"\x00\x00\x0B\x5F\x00\x14" MSG_ICON_DISKINFO_STR
									"\x00"
									"\x00\x00\x0B\x60\x00\x0C" MSG_ICON_UNSNAPSHOT_MENU_STR
									"\x00"
									"\x00\x00\x0B\x62\x00\x06" MSG_ICON_COPY_STR
									"\x00\x00"
									"\x00\x00\x0B\x63\x00\x04" MSG_ICONS_NEW_MENU_STR
									"\x00"
									"\x00\x00\x0B\x64\x00\x0A" MSG_ICONS_MAKEDIR_MENU_STR
									"\x00"
									"\x00\x00\x0B\x65\x00\x0A" MSG_ICONS_NEWGROUP_MENU_STR
									"\x00\x00"
									"\x00\x00\x0B\x66\x00\x08" MSG_ICON_COPY_RAM_STR
									"\x00"
									"\x00\x00\x0B\x67\x00\x08" MSG_ICON_COPY_DF0_STR
									"\x00"
									"\x00\x00\x0B\x68\x00\x0C" MSG_ICON_COPY_OTHER_STR
									"\x00"
									"\x00\x00\x0B\x69\x00\x08" MSG_ICON_COPY2_STR
									"\x00"
									"\x00\x00\x0B\x6A\x00\x04" MSG_ICON_COPY_TO_STR
									"\x00\x00"
									"\x00\x00\x0B\x6B\x00\x0C" MSG_ICONS_NEWCOMMAND_MENU_STR
									"\x00\x00"
									"\x00\x00\x0B\x6C\x00\x10" MSG_ICONS_SHORTCUT_MENU_STR
									"\x00"
									"\x00\x00\x0B\x6D\x00\x10" MSG_DESKTOP_POPUP_LEFTOUT_STR
									"\x00"
									"\x00\x00\x0B\x6E\x00\x10" MSG_DESKTOP_POPUP_COPY_STR
									"\x00"
									"\x00\x00\x0B\x6F\x00\x10" MSG_DESKTOP_POPUP_MOVE_STR
									"\x00"
									"\x00\x00\x0B\x70\x00\x0C" MSG_ICON_COPY_DESKTOP_STR
									"\x00\x00"
									"\x00\x00\x0B\x71\x00\x08" MSG_ICON_LINEUP_STR
									"\x00"
									"\x00\x00\x0B\x72\x00\x10" MSG_ICON_SNAPSHOT_LISTER_STR
									"\x00"
									"\x00\x00\x0B\x73\x00\x10" MSG_ICON_SNAPSHOT_ICONS_STR
									"\x00\x00"
									"\x00\x00\x0B\x74\x00\x0E" MSG_ICON_SNAPSHOT_ALL_STR
									"\x00\x00"
									"\x00\x00\x0B\x75\x00\x10" MSG_ICON_SNAPSHOT_WINDOW_STR
									"\x00"
									"\x00\x00\x0B\x76\x00\x08" MSG_ICONS_MAKEFILE_MENU_STR
									"\x00"
									"\x00\x00\x0B\xB8\x00\x08" MSG_BUTTONS_MENU_STR
									"\x00"
									"\x00\x00\x0B\xBA\x00\x06" MSG_CLOSE_BUTTONS_MENU_STR
									"\x00"
									"\x00\x00\x0B\xBE\x00\x18" MSG_BUTTONS_NEW_GFX_MENU_STR
									"\x00\x00"
									"\x00\x00\x0B\xBF\x00\x14" MSG_BUTTONS_NEW_TEXT_MENU_STR
									"\x00"
									"\x00\x00\x0B\xC0\x00\x12" MSG_BUTTONS_NEW_STARTMENU_MENU_STR
									"\x00"
									"\x00\x00\x0C\x1C\x00\x0A" MSG_SETTINGS_MENU_STR
									"\x00\x00"
									"\x00\x00\x0C\x1D\x00\x06" MSG_CLOCK_MENU_STR
									"\x00"
									"\x00\x00\x0C\x1E\x00\x0E" MSG_CREATE_ICONS_MENU_STR
									"\x00"
									"\x00\x00\x0C\x1F\x00\x0C" MSG_ENVIRONMENT_MENU_STR
									"\x00"
									"\x00\x00\x0C\x20\x00\x0E" MSG_FILETYPES_MENU_STR
									"\x00"
									"\x00\x00\x0C\x21\x00\x0C" MSG_HOTKEYS_MENU_STR
									"\x00\x00"
									"\x00\x00\x0C\x22\x00\x0E" MSG_MENU_MENU_STR
									"\x00"
									"\x00\x00\x0C\x23\x00\x12" MSG_FILTER_MENU_STR
									"\x00\x00"
									"\x00\x00\x0C\x24\x00\x0C" MSG_OPTIONS_MENU_STR
									"\x00\x00"
									"\x00\x00\x0C\x26\x00\x12" MSG_DEFPUBSCR_MENU_STR
									"\x00"
									"\x00\x00\x0C\x28\x00\x0C" MSG_MENU_HOTKEYS_STR
									"\x00\x00"
									"\x00\x00\x0C\x29\x00\x08" MSG_HOTKEYS_TITLE_STR
									"\x00"
									"\x00\x00\x0C\x2A\x00\x0C" MSG_MENU_SCRIPTS_STR
									"\x00\x00"
									"\x00\x00\x0C\x2B\x00\x10" MSG_MENU_SYSMENU_STR
									"\x00"
									"\x00\x00\x0C\x2C\x00\x14" MSG_MENU_ICONPOS_STR
									"\x00"
									"\x00\x00\x0C\x2D\x00\x0C" MSG_ENVIRONMENT_SAVE_LAYOUT_NEW_STR
									"\x00"
									"\x00\x00\x0C\x2E\x00\x10" MSG_ENVIRONMENT_EDIT_STR
									"\x00\x00"
									"\x00\x00\x0C\x2F\x00\x14" MSG_ENVIRONMENT_LOAD_NEW_STR
									"\x00"
									"\x00\x00\x0C\x30\x00\x12" MSG_ENVIRONMENT_SAVE_NEW_STR
									"\x00\x00"
									"\x00\x00\x0C\x80\x00\x10" MSG_SELECT_TITLE_STR
									"\x00\x00"
									"\x00\x00\x0C\x81\x00\x08" MSG_SELECT_IGNORE_STR
									"\x00\x00"
									"\x00\x00\x0C\x82\x00\x06" MSG_SELECT_MATCH_STR
									"\x00"
									"\x00\x00\x0C\x83\x00\x0A" MSG_SELECT_NOMATCH_STR
									"\x00\x00"
									"\x00\x00\x0C\x84\x00\x06" MSG_SELECT_NEWER_STR
									"\x00"
									"\x00\x00\x0C\x85\x00\x06" MSG_SELECT_OLDER_STR
									"\x00"
									"\x00\x00\x0C\x86\x00\x0A" MSG_SELECT_DIFFERENT_STR
									"\x00"
									"\x00\x00\x0C\x87\x00\x06" MSG_SELECT_NAME_STR
									"\x00"
									"\x00\x00\x0C\x88\x00\x06" MSG_SELECT_DATE_FROM_STR
									"\x00"
									"\x00\x00\x0C\x89\x00\x04" MSG_SELECT_DATE_TO_STR
									"\x00\x00"
									"\x00\x00\x0C\x8A\x00\x08" MSG_SELECT_PROT_STR
									"\x00"
									"\x00\x00\x0C\x8B\x00\x08" MSG_SELECT_INCLUDE_STR
									"\x00"
									"\x00\x00\x0C\x8C\x00\x08" MSG_SELECT_EXCLUDE_STR
									"\x00"
									"\x00\x00\x0C\x8D\x00\x16" MSG_SELECT_COMPARE_STR
									"\x00\x00"
									"\x00\x00\x0C\x8E\x00\x08" MSG_SELECT_SIMPLE_STR
									"\x00\x00"
									"\x00\x00\x0C\x8F\x00\x08" MSG_SELECT_COMPLEX_STR
									"\x00"
									"\x00\x00\x0C\x90\x00\x22" MSG_SELECT_ENTER_PATTERN_STR
									"\x00"
									"\x00\x00\x0C\x91\x00\x04" MSG_SELECT_PROT_OFF_STR
									"\x00"
									"\x00\x00\x0C\x92\x00\x0E" MSG_SELECT_ENTRY_BOTH_STR
									"\x00\x00"
									"\x00\x00\x0C\x93\x00\x0C" MSG_SELECT_ENTRY_FILES_STR
									"\x00\x00"
									"\x00\x00\x0C\x94\x00\x0A" MSG_SELECT_ENTRY_DIRS_STR
									"\x00"
									"\x00\x00\x0C\x95\x00\x0C" MSG_SELECT_ENTRY_TYPE_STR
									"\x00"
									"\x00\x00\x0C\xE4\x00\x0A" MSG_DOS_ERROR_STR
									"\x00"
									"\x00\x00\x0C\xE5\x00\x0A" MSG_DOS_ERROR_CODE_STR
									"\x00"
									"\x00\x00\x0C\xE6\x00\x1C" MSG_ERROR_OCCURED_STR
									"\x00"
									"\x00\x00\x0C\xE7\x00\x22" MSG_ERROR_DEST_IS_DIRECTORY_STR
									"\x00\x00"
									"\x00\x00\x0C\xE8\x00\x1C" MSG_ERROR_DEST_IS_FILE_STR
									"\x00"
									"\x00\x00\x0C\xEA\x00\x14" MSG_ERROR_UNKNOWN_CODE_STR
									"\x00\x00"
									"\x00\x00\x0C\xEB\x00\x1A" MSG_BUTTONS_ERROR_SAVING_STR
									"\x00"
									"\x00\x00\x0C\xEC\x00\x1C" MSG_BUTTONS_ERROR_LOADING_STR
									"\x00\x00"
									"\x00\x00\x0C\xED\x00\x18" MSG_ERROR_CREATING_DIRECTORY_STR
									"\x00\x00"
									"\x00\x00\x0C\xEE\x00\x3C" MSG_CUSTPORT_NOT_FOUND_STR
									"\x00"
									"\x00\x00\x0C\xEF\x00\x90" MSG_ALREADY_RUNNING_STR
									"\x00"
									"\x00\x00\x0C\xF0\x00\x12" MSG_ALREADY_RUNNING_BUTTONS_STR
									"\x00\x00"
									"\x00\x00\x0C\xF1\x00\x56" MSG_UNABLE_TO_CLOSE_SCREEN_STR
									"\x00"
									"\x00\x00\x0C\xF2\x00\x1E" MSG_ERROR_INVALID_DATE_STR
									"\x00\x00"
									"\x00\x00\x0C\xF3\x00\x14" MSG_UNABLE_TO_OPEN_SCREEN_STR
									"\x00\x00"
									"\x00\x00\x0C\xF4\x00\x12" MSG_AN_ERROR_OCCURED_STR
									"\x00"
									"\x00\x00\x0C\xF5\x00\x08" MSG_ERROR_STR
									"\x00\x00"
									"\x00\x00\x0C\xF6\x00\x06" MSG_SIZE_STR
									"\x00\x00"
									"\x00\x00\x0C\xF7\x00\x06" MSG_DATE_STR
									"\x00\x00"
									"\x00\x00\x0C\xF8\x00\x04" MSG_OLD_STR
									"\x00"
									"\x00\x00\x0C\xF9\x00\x04" MSG_NEW_STR
									"\x00"
									"\x00\x00\x0C\xFA\x00\x1A" MSG_FILE_EXISTS_STR
									"\x00"
									"\x00\x00\x0C\xFB\x00\x12" MSG_ERROR_CREATING_FILE_STR
									"\x00"
									"\x00\x00\x0D\x48\x00\x0C" MSG_PROGRESS_OPERATION_DELETING_STR
									"\x00"
									"\x00\x00\x0D\x49\x00\x72" MSG_DELETE_CONFIRM_STR
									"\x00\x00"
									"\x00\x00\x0D\x4A\x00\x08" MSG_DELETE_STR
									"\x00\x00"
									"\x00\x00\x0D\x4B\x00\x0C" MSG_DELETE_ALL_STR
									"\x00\x00"
									"\x00\x00\x0D\x4C\x00\x0E" MSG_DELETE_FILE_CONFIRM_STR
									"\x00\x00"
									"\x00\x00\x0D\x4D\x00\x34" MSG_DELETE_DIR_CONFIRM_STR
									"\x00"
									"\x00\x00\x0D\x4E\x00\x3E" MSG_DELETE_PROTECTED_STR
									"\x00\x00"
									"\x00\x00\x0D\x4F\x00\x0A" MSG_UNPROTECT_STR
									"\x00"
									"\x00\x00\x0D\x50\x00\x0E" MSG_UNPROTECT_ALL_STR
									"\x00"
									"\x00\x00\x0D\x51\x00\x1E" MSG_DELETE_LINK_CONFIRM_SINGLE_STR
									"\x00\x00"
									"\x00\x00\x0D\x52\x00\x20" MSG_DELETE_LINK_CONFIRM_MULTI_STR
									"\x00\x00"
									"\x00\x00\x0D\xAC\x00\x0E" MSG_BUTTONS_ENTER_NAME_STR
									"\x00\x00"
									"\x00\x00\x0D\xAD\x00\x0E" MSG_BUTTONS_SELECT_FILE_STR
									"\x00\x00"
									"\x00\x00\x0D\xAE\x00\x10" MSG_BUTTONS_STATUS_TITLE_STR
									"\x00\x00"
									"\x00\x00\x0D\xAF\x00\x18" MSG_BUTTONS_LOADING_STR
									"\x00\x00"
									"\x00\x00\x0D\xB0\x00\x12" MSG_ENVIRONMENT_ENTER_NAME_STR
									"\x00\x00"
									"\x00\x00\x0D\xB1\x00\x12" MSG_ENVIRONMENT_SELECT_FILE_STR
									"\x00\x00"
									"\x00\x00\x0D\xB2\x00\x1C" MSG_ENVIRONMENT_LOADING_STR
									"\x00"
									"\x00\x00\x0D\xB3\x00\x1C" MSG_ENVIRONMENT_SAVING_STR
									"\x00\x00"
									"\x00\x00\x0D\xB4\x00\x3C" MSG_BUTTONS_CHANGE_WARNING_STR
									"\x00"
									"\x00\x00\x0D\xB5\x00\x2E" MSG_BUTTONS_CREATE_WHICH_TYPE_STR
									"\x00"
									"\x00\x00\x0D\xB6\x00\x06" MSG_BUTTONS_TEXT_STR
									"\x00\x00"
									"\x00\x00\x0D\xB7\x00\x0A" MSG_BUTTONS_ICONS_STR
									"\x00\x00"
									"\x00\x00\x0D\xB8\x00\x0E" MSG_SETTINGS_SELECT_FILE_STR
									"\x00\x00"
									"\x00\x00\x0D\xB9\x00\x18" MSG_SETTINGS_LOADING_STR
									"\x00"
									"\x00\x00\x0D\xBA\x00\x0E" MSG_SETTINGS_ENTER_NAME_STR
									"\x00\x00"
									"\x00\x00\x0D\xBB\x00\x18" MSG_SETTINGS_SAVING_STR
									"\x00\x00"
									"\x00\x00\x0D\xBC\x00\x20" MSG_ERROR_SAVING_ENV_STR
									"\x00\x00"
									"\x00\x00\x0D\xBD\x00\x1C" MSG_ERROR_SAVING_OPTS_STR
									"\x00\x00"
									"\x00\x00\x0E\x12\x00\x1E" MSG_LAUNCHING_PROGRAM_STR
									"\x00\x00"
									"\x00\x00\x0E\x13\x00\x22" MSG_MEMORY_COUNTER_STR
									"\x00"
									"\x00\x00\x0E\x14\x00\x22" MSG_MEMORY_COUNTER_LOC_STR
									"\x00"
									"\x00\x00\x0E\x15\x00\x1A" MSG_MEMORY_COUNTER_CLOCK_STR
									"\x00"
									"\x00\x00\x0E\x16\x00\x1A" MSG_MEMORY_COUNTER_LOC_CLOCK_STR
									"\x00"
									"\x00\x00\x0E\x17\x00\x0C" MSG_MEMORY_FREE_OS4_STR
									"\x00"
									"\x00\x00\x0E\x74\x00\x06" MSG_LISTER_STATUS_SOURCE_STR
									"\x00\x00"
									"\x00\x00\x0E\x75\x00\x06" MSG_LISTER_STATUS_DEST_STR
									"\x00\x00"
									"\x00\x00\x0E\x76\x00\x06" MSG_LISTER_STATUS_BUSY_STR
									"\x00\x00"
									"\x00\x00\x0E\x77\x00\x04" MSG_LISTER_STATUS_OFF_STR
									"\x00"
									"\x00\x00\x0E\xD8\x00\x22" MSG_HUNT_ENTER_FILENAME_STR
									"\x00"
									"\x00\x00\x0E\xD9\x00\x3C" MSG_HUNT_FOUND_FILE_STR
									"\x00"
									"\x00\x00\x0E\xDA\x00\x0C" MSG_NEW_LISTER_STR
									"\x00\x00"
									"\x00\x00\x0E\xDB\x00\x16" MSG_SEARCH_COMMENTS_STR
									"\x00\x00"
									"\x00\x00\x0F\x3C\x00\x14" MSG_FUNC_DESC_ADDICON_STR
									"\x00\x00"
									"\x00\x00\x0F\x3D\x00\x12" MSG_FUNC_DESC_ALL_STR
									"\x00\x00"
									"\x00\x00\x0F\x3E\x00\x16" MSG_FUNC_DESC_CLEARBUFFERS_STR
									"\x00"
									"\x00\x00\x0F\x3F\x00\x18" MSG_FUNC_DESC_COMMENT_STR
									"\x00\x00"
									"\x00\x00\x0F\x40\x00\x14" MSG_FUNC_DESC_COPY_STR
									"\x00"
									"\x00\x00\x0F\x41\x00\x14" MSG_FUNC_DESC_COPYAS_STR
									"\x00"
									"\x00\x00\x0F\x42\x00\x1A" MSG_FUNC_DESC_DATESTAMP_STR
									"\x00"
									"\x00\x00\x0F\x43\x00\x16" MSG_FUNC_DESC_DELETE_STR
									"\x00"
									"\x00\x00\x0F\x44\x00\x16" MSG_FUNC_DESC_DISKCOPY_STR
									"\x00\x00"
									"\x00\x00\x0F\x45\x00\x0E" MSG_FUNC_DESC_ENCRYPT_STR
									"\x00"
									"\x00\x00\x0F\x46\x00\x16" MSG_FUNC_DESC_FINDFILE_STR
									"\x00\x00"
									"\x00\x00\x0F\x47\x00\x16" MSG_FUNC_DESC_FINISHSECTION_STR
									"\x00"
									"\x00\x00\x0F\x48\x00\x0E" MSG_FUNC_DESC_FORMAT_STR
									"\x00\x00"
									"\x00\x00\x0F\x49\x00\x1A" MSG_FUNC_DESC_GETSIZES_STR
									"\x00"
									"\x00\x00\x0F\x4A\x00\x1C" MSG_FUNC_DESC_ICONINFO_STR
									"\x00\x00"
									"\x00\x00\x0F\x4B\x00\x1E" MSG_FUNC_DESC_LEAVEOUT_STR
									"\x00\x00"
									"\x00\x00\x0F\x4C\x00\x14" MSG_FUNC_DESC_LOADBUTTONS_STR
									"\x00\x00"
									"\x00\x00\x0F\x4D\x00\x16" MSG_FUNC_DESC_MAKEDIR_STR
									"\x00\x00"
									"\x00\x00\x0F\x4E\x00\x14" MSG_FUNC_DESC_MOVE_STR
									"\x00"
									"\x00\x00\x0F\x4F\x00\x14" MSG_FUNC_DESC_MOVEAS_STR
									"\x00"
									"\x00\x00\x0F\x50\x00\x14" MSG_FUNC_DESC_NONE_STR
									"\x00\x00"
									"\x00\x00\x0F\x51\x00\x16" MSG_FUNC_DESC_PLAY_STR
									"\x00\x00"
									"\x00\x00\x0F\x52\x00\x16" MSG_FUNC_DESC_PRINT_STR
									"\x00\x00"
									"\x00\x00\x0F\x53\x00\x18" MSG_FUNC_DESC_PROTECT_STR
									"\x00"
									"\x00\x00\x0F\x54\x00\x10" MSG_FUNC_DESC_READ_STR
									"\x00"
									"\x00\x00\x0F\x55\x00\x16" MSG_FUNC_DESC_RENAME_STR
									"\x00"
									"\x00\x00\x0F\x56\x00\x16" MSG_FUNC_DESC_RUN_STR
									"\x00"
									"\x00\x00\x0F\x57\x00\x12" MSG_FUNC_DESC_SCANDIR_STR
									"\x00\x00"
									"\x00\x00\x0F\x58\x00\x18" MSG_FUNC_DESC_SEARCH_STR
									"\x00"
									"\x00\x00\x0F\x59\x00\x16" MSG_FUNC_DESC_SELECT_STR
									"\x00"
									"\x00\x00\x0F\x5A\x00\x18" MSG_FUNC_DESC_SHOW_STR
									"\x00\x00"
									"\x00\x00\x0F\x5B\x00\x16" MSG_FUNC_DESC_SMARTREAD_STR
									"\x00\x00"
									"\x00\x00\x0F\x5C\x00\x18" MSG_FUNC_DESC_TOGGLE_STR
									"\x00\x00"
									"\x00\x00\x0F\x5D\x00\x16" MSG_FUNC_DESC_CHECKFIT_STR
									"\x00"
									"\x00\x00\x0F\x5E\x00\x16" MSG_FUNC_DESC_CLEARSIZES_STR
									"\x00"
									"\x00\x00\x0F\x5F\x00\x1C" MSG_FUNC_DESC_ANSIREAD_STR
									"\x00"
									"\x00\x00\x0F\x60\x00\x14" MSG_FUNC_DESC_HEXREAD_STR
									"\x00"
									"\x00\x00\x0F\x61\x00\x1A" MSG_FUNC_DESC_CLONE_STR
									"\x00\x00"
									"\x00\x00\x0F\x62\x00\x1A" MSG_FUNC_DESC_LOADENVIRONMENT_STR
									"\x00\x00"
									"\x00\x00\x0F\x63\x00\x16" MSG_FUNC_DESC_LOADSETTINGS_STR
									"\x00\x00"
									"\x00\x00\x0F\x64\x00\x16" MSG_FUNC_DESC_PARENT_STR
									"\x00"
									"\x00\x00\x0F\x65\x00\x14" MSG_FUNC_DESC_ROOT_STR
									"\x00"
									"\x00\x00\x0F\x66\x00\x1C" MSG_FUNC_DESC_USER_STR
									"\x00\x00"
									"\x00\x00\x0F\x67\x00\x1A" MSG_FUNC_DESC_DEVICELIST_STR
									"\x00\x00"
									"\x00\x00\x0F\x68\x00\x18" MSG_FUNC_DESC_BUFFERLIST_STR
									"\x00"
									"\x00\x00\x0F\x69\x00\x16" MSG_FUNC_DESC_VERIFY_STR
									"\x00\x00"
									"\x00\x00\x0F\x6A\x00\x1A" MSG_FUNC_DESC_PRINTDIR_STR
									"\x00\x00"
									"\x00\x00\x0F\x6B\x00\x0E" MSG_FUNC_DESC_SET_STR
									"\x00"
									"\x00\x00\x0F\x6C\x00\x14" MSG_FUNC_DESC_CLOSEBUTTONS_STR
									"\x00"
									"\x00\x00\x0F\x6D\x00\x16" MSG_FUNC_DESC_QUIT_STR
									"\x00"
									"\x00\x00\x0F\x6E\x00\x14" MSG_FUNC_DESC_HIDE_STR
									"\x00"
									"\x00\x00\x0F\x6F\x00\x24" MSG_FUNC_DESC_REVEAL_STR
									"\x00"
									"\x00\x00\x0F\x70\x00\x1A" MSG_FUNC_DESC_RESELECT_STR
									"\x00\x00"
									"\x00\x00\x0F\x71\x00\x16" MSG_FUNC_DESC_MAKELINK_STR
									"\x00"
									"\x00\x00\x0F\x72\x00\x1C" MSG_FUNC_DESC_MAKELINKAS_STR
									"\x00"
									"\x00\x00\x0F\x73\x00\x10" MSG_FUNC_DESC_ASSIGN_STR
									"\x00\x00"
									"\x00\x00\x0F\x74\x00\x1A" MSG_FUNC_DESC_CONFIGURE_STR
									"\x00\x00"
									"\x00\x00\x0F\x75\x00\x22" MSG_FUNC_DESC_CLI_STR
									"\x00"
									"\x00\x00\x0F\x76\x00\x1A" MSG_FUNC_DESC_SETBACKGROUND_STR
									"\x00"
									"\x00\x00\x0F\x77\x00\x0E" MSG_FUNC_DESC_RESET_STR
									"\x00\x00"
									"\x00\x00\x0F\x78\x00\x16" MSG_FUNC_DESC_STOPSNIFFER_STR
									"\x00"
									"\x00\x00\x0F\x79\x00\x16" MSG_FUNC_DESC_MAKEFILE_STR
									"\x00"
									"\x00\x00\x0F\xA0\x00\x10" MSG_ICON_GROUP_STR
									"\x00\x00"
									"\x00\x00\x0F\xA3\x00\x1A" MSG_GROUP_ENTER_NAME_STR
									"\x00\x00"
									"\x00\x00\x11\x94\x00\x1A" MSG_RXERR_1_STR
									"\x00\x00"
									"\x00\x00\x11\x95\x00\x18" MSG_RXERR_5_STR
									"\x00\x00"
									"\x00\x00\x11\x96\x00\x18" MSG_RXERR_6_STR
									"\x00"
									"\x00\x00\x11\x97\x00\x0E" MSG_RXERR_8_STR
									"\x00\x00"
									"\x00\x00\x11\x98\x00\x16" MSG_RXERR_10_STR
									"\x00"
									"\x00\x00\x11\x99\x00\x10" MSG_RXERR_12_STR
									"\x00"
									"\x00\x00\x11\x9A\x00\x0E" MSG_RXERR_13_STR
									"\x00\x00"
									"\x00\x00\x11\x9B\x00\x12" MSG_RXERR_14_STR
									"\x00\x00"
									"\x00\x00\x11\x9C\x00\x0A" MSG_RXERR_15_STR
									"\x00"
									"\x00\x00\x11\x9D\x00\x16" MSG_RXERR_20_STR
									"\x00"
									"\x00\x00\x11\x9E\x00\x14" MSG_RXERR_UNKNOWN_STR
									"\x00\x00"
									"\x00\x00\x17\x70\x00\x3E" MSG_DESKTOP_REALLY_DELETE_STR
									"\x00"
									"\x00\x00\x17\x71\x00\x16" MSG_DESKTOP_DELETE_GROUPS_STR
									"\x00\x00"
									"\x00\x00\x17\x75\x00\x0E" MSG_DESKTOP_DELETE_ASSIGNS_STR
									"\x00"
									"\x00\x00\x17\x76\x00\x14" MSG_DESKTOP_DELETE_GROUP_OBJECTS_STR
									"\x00"
									"\x00\x00\x17\x77\x00\x0C" MSG_DESKTOP_DELETE_DESKTOP_FILES_STR
									"\x00"
									"\x00\x00\x17\x78\x00\x0C" MSG_DESKTOP_DELETE_DESKTOP_DIRS_STR
									"\x00\x00"
									"\x00\x00\x1B\x58\x00\x26" MSG_REPLACE_SAME_STR
									"\x00\x00"
									"\x00\x00\x1B\x59\x00\x30" MSG_REPLACE_VERSION_SAME_STR
									"\x00\x00"
									"\x00\x00\x1B\x5A\x00\x30" MSG_REPLACE_VERSION_NEWER_STR
									"\x00\x00"
									"\x00\x00\x1B\x5B\x00\x32" MSG_REPLACE_VERSION_OLDER_STR
									"\x00"
									"\x00\x00\x1B\x5C\x00\x3A" MSG_REPLACE_SIZE_BIGGER_STR
									"\x00\x00"
									"\x00\x00\x1B\x5D\x00\x3A" MSG_REPLACE_SIZE_SMALLER_STR
									"\x00"
									"\x00\x00\x1B\x5E\x00\x2E" MSG_REPLACE_SIZE_SAME_STR
									"\x00"
									"\x00\x00\x1B\x5F\x00\x2C" MSG_REPLACE_DATE_NEWER_STR
									"\x00\x00"
									"\x00\x00\x1B\x60\x00\x28" MSG_REPLACE_DATE_OLDER_STR
									"\x00"
									"\x00\x00\x1B\x61\x00\x28" MSG_REPLACE_DATE_SAME_STR
									"\x00"
									"\x00\x00\x1B\x62\x00\x2A" MSG_FILE_ALREADY_EXISTS_STR
									"\x00\x00"
									"\x00\x00\x1B\x63\x00\x6A" MSG_BRIEF_FILE_EXISTS_STR
									"\x00\x00"
									"\x00\x00\x1B\x64\x00\x14" MSG_REPLACE_VER_STR
									"\x00"
									"\x00\x00\x1B\x65\x00\x0E" MSG_REPLACE_VER_UNKNOWN_STR
									"\x00"
									"\x00\x00\x1B\x66\x00\x08" MSG_GET_VERSION_STR
									"\x00"
									"\x00\x00\x1B\x67\x00\x1C" MSG_REPLACE_VERSION_TITLE_STR
									"\x00"
									"\x00\x00\x1F\x40\x00\x06" MSG_LISTER_TITLE_NAME_STR
									"\x00\x00"
									"\x00\x00\x1F\x41\x00\x06" MSG_LISTER_TITLE_SIZE_STR
									"\x00\x00"
									"\x00\x00\x1F\x42\x00\x08" MSG_LISTER_TITLE_PROTECT_STR
									"\x00\x00"
									"\x00\x00\x1F\x43\x00\x06" MSG_LISTER_TITLE_DATE_STR
									"\x00\x00"
									"\x00\x00\x1F\x44\x00\x08" MSG_LISTER_TITLE_COMMENT_STR
									"\x00"
									"\x00\x00\x1F\x45\x00\x06" MSG_LISTER_TITLE_FILETYPE_STR
									"\x00\x00"
									"\x00\x00\x1F\x46\x00\x06" MSG_LISTER_TITLE_OWNER_STR
									"\x00"
									"\x00\x00\x1F\x47\x00\x06" MSG_LISTER_TITLE_GROUP_STR
									"\x00"
									"\x00\x00\x1F\x48\x00\x04" MSG_LISTER_TITLE_NETPROT_STR
									"\x00"
									"\x00\x00\x1F\x49\x00\x08" MSG_LISTER_TITLE_VERSION_STR
									"\x00"
									"\x00\x00\x1F\xA4\x00\x08" MSG_LISTER_TITLE_VOLUME_STR
									"\x00\x00"
									"\x00\x00\x1F\xA5\x00\x08" MSG_LISTER_TITLE_DEVICE_STR
									"\x00\x00"
									"\x00\x00\x1F\xA6\x00\x06" MSG_LISTER_TITLE_FULL_STR
									"\x00\x00"
									"\x00\x00\x1F\xA7\x00\x06" MSG_LISTER_TITLE_FREE_STR
									"\x00\x00"
									"\x00\x00\x1F\xA8\x00\x08" MSG_LISTER_TITLE_USED_STR
									"\x00\x00"
									"\x00\x00\x23\x28\x01\x2A" MSG_CLI_HELP_STR
									"\x00"
									"\x00\x00\x23\x29\x00\x1A" MSG_CLI_NO_TEMPLATE_STR
									"\x00\x00"
									"\x00\x00\x23\x2A\x00\x18" MSG_CLI_TYPE_HELP_STR
									"\x00\x00"
									"\x00\x00\x23\x2B\x00\x12" MSG_CLI_BAD_CMD_STR
									"\x00\x00"
									"\x00\x00\x27\x10\x00\x16" MSG_ICONPOS_WINDOW_STR
									"\x00"
									"\x00\x00\x27\x11\x00\x5E" MSG_ICONPOS_INSTRUCTIONS_STR
									"\x00"
									"\x00\x00\x27\x12\x00\x0A" MSG_ICONPOS_APPICON_STR
									"\x00\x00"
									"\x00\x00\x27\x13\x00\x06" MSG_ICONPOS_DISKS_STR
									"\x00"
									"\x00\x00\x27\x14\x00\x10" MSG_ICONPOS_LISTERS_STR
									"\x00"
									"\x00\x00\x27\x15\x00\x0A" MSG_ICONPOS_PRIORITY_STR
									"\x00\x00"
									"\x00\x00\x27\x16\x00\x08" MSG_ICONPOS_GROUPS_STR
									"\x00\x00"
									"\x00\x00\x27\x17\x00\x0A" MSG_ICONPOS_LEFTOUT_STR
									"\x00"
									"\x00\x00\x2E\xE0\x00\x20" MSG_COMMAND_SAVE_STR
									"\x00\x00"
									"\x00\x00\x2E\xE1\x00\x10" MSG_SAVING_COMMAND_STR
									"\x00\x00"
									"\x00\x00\x2E\xE2\x00\x10" MSG_EDIT_COMMAND_STR
									"\x00"
									"\x00\x00\x2E\xE3\x00\x26" MSG_COMMAND_DESCRIPTION_STR
									"\x00"
									"\x00\x00\x2E\xE4\x00\x16" MSG_COMMAND_LEAVEOUT_STR
									"\x00\x00"
									"\x00\x00\x32\xC8\x00\x0C" MSG_START_APPEARANCE_STR
									"\x00\x00"
									"\x00\x00\x32\xC9\x00\x08" MSG_START_BORDERLESS_STR
									"\x00\x00"
									"\x00\x00\x32\xCA\x00\x06" MSG_START_DRAGBAR_LEFT_STR
									"\x00\x00"
									"\x00\x00\x32\xCB\x00\x06" MSG_START_DRAGBAR_RIGHT_STR
									"\x00"
									"\x00\x00\x32\xCC\x00\x10" MSG_START_IMAGE_STR
									"\x00"
									"\x00\x00\x32\xCD\x00\x10" MSG_START_LABEL_STR
									"\x00"
									"\x00\x00\x32\xCE\x00\x1C" MSG_START_SELECT_IMAGE_STR
									"\x00"
									"\x00\x00\x32\xCF\x00\x1C" MSG_START_SELECT_LABEL_STR
									"\x00"
									"\x00\x00\x32\xD1\x00\x14" MSG_OPEN_STARTMENU_MENU_STR
									"\x00\x00"
									"\x00\x00\x32\xD2\x00\x1A" MSG_START_SELECT_MENU_STR
									"\x00"
									"\x00\x00\x32\xD3\x00\x10" MSG_START_FONT_STR
									"\x00\x00"
									"\x00\x00\x32\xD4\x00\x1C" MSG_START_SELECT_FONT_STR
									"\x00\x00"
									"\x00\x00\x32\xD5\x00\x0E" MSG_START_SCALING_STR
									"\x00\x00"
									"\x00\x00\x32\xD6\x00\x0A" MSG_START_DRAGBAR_STR
									"\x00\x00"
									"\x00\x00\x32\xD7\x00\x04" MSG_START_DRAGBAR_OFF_STR
									"\x00"
									"\x00\x00\x32\xD8\x00\x16" MSG_START_LABEL_FONT_STR
									"\x00\x00"
									"\x00\x00\x32\xD9\x00\x16" MSG_START_PICTURE_STR
									"\x00\x00"
									"\x00\x00\x32\xDA\x00\x2A" MSG_START_SELECT_PICTURE_STR
									"\x00\x00"
									"\x00\x00\x34\xBC\x00\x0C" MSG_KEYFINDER_TITLE_STR
									"\x00\x00"
									"\x00\x00\x34\xBD\x00\x06" MSG_KEYFINDER_KEY_STR
									"\x00"
									"\x00\x00\x34\xBE\x00\x08" MSG_KEYFINDER_FOUND_STR
									"\x00\x00"
									"\x00\x00\x35\x20\x00\x0E" MSG_KEYFINDER_TYPE_INVALID_STR
									"\x00\x00"
									"\x00\x00\x35\x21\x00\x10" MSG_KEYFINDER_TYPE_NONE_STR
									"\x00"
									"\x00\x00\x35\x22\x00\x1E" MSG_KEYFINDER_TYPE_BUTTONS_STR
									"\x00"
									"\x00\x00\x35\x23\x00\x20" MSG_KEYFINDER_TYPE_TOOLBAR_STR
									"\x00\x00"
									"\x00\x00\x35\x24\x00\x1C" MSG_KEYFINDER_TYPE_LISTERMENU_STR
									"\x00"
									"\x00\x00\x35\x25\x00\x1A" MSG_KEYFINDER_TYPE_MENU_STR
									"\x00"
									"\x00\x00\x35\x26\x00\x1E" MSG_KEYFINDER_TYPE_START_STR
									"\x00\x00"
									"\x00\x00\x35\x27\x00\x18" MSG_KEYFINDER_TYPE_HOTKEYS_STR
									"\x00"
									"\x00\x00\x35\x28\x00\x18" MSG_KEYFINDER_TYPE_SCRIPTS_STR "\x00"};

#endif /* CATCOMP_BLOCK */

/****************************************************************************/

struct LocaleInfo
{
	APTR li_LocaleBase;
	APTR li_Catalog;
};

#ifdef CATCOMP_CODE

STRPTR GetString(struct LocaleInfo *li, LONG stringNum)
{
	LONG *l;
	UWORD *w;
	STRPTR builtIn;

	l = (LONG *)CatCompBlock;

	while (*l != stringNum)
	{
		w = (UWORD *)((ULONG)l + 4);
		l = (LONG *)((ULONG)l + (ULONG)*w + 6);
	}
	builtIn = (STRPTR)((ULONG)l + 6);

	#define XLocaleBase LocaleBase
	#define LocaleBase li->li_LocaleBase

	if (LocaleBase)
		return (GetCatalogStr(li->li_Catalog, stringNum, builtIn));
	#define LocaleBase XLocaleBase
	#undef XLocaleBase

	return (builtIn);
}

#endif /* CATCOMP_CODE */

/****************************************************************************/

#endif /* STRING_DATA_H */
