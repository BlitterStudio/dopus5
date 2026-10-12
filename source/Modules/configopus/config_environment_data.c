#include "config_lib.h"
#include "config_environment.h"

// environment window dimensions
const ConfigWindow _config_environment_window = {{POS_CENTER, POS_CENTER, 68, 14}, {0, 0, 36, 64}};

// Palette slider callback hook
#if defined(__MORPHOS__)
static void _palette_slider_callback_entry(struct TagItem *tag, struct Window *window);

static void _palette_slider_callback_trampoline()
{
	struct TagItem *tag = (struct TagItem *)REG_A1;
	struct Window *window = (struct Window *)REG_A2;
	_palette_slider_callback_entry(tag, window);
}

static struct EmulLibEntry _palette_slider_callback = {TRAP_LIBNR,
													   0,
													   (void (*)()) & _palette_slider_callback_trampoline};

static void _palette_slider_callback_entry(struct TagItem *tag, struct Window *window)
#else
static void ASM _palette_slider_callback(REG(a1, struct TagItem *tag), REG(a2, struct Window *window))
#endif
{
	// struct Library *GfxBase;

	// GfxBase=data->GfxBase;

	// Change tag to max pixel length
	tag->ti_Tag = GTSL_MaxPixelLen;
	tag->ti_Data = TextLength(window->RPort, "8888", 4);
}

// Labels
const UWORD _environment_listeredit_labels[] = {MSG_LISTEREDIT_DISABLED,
												MSG_LISTEREDIT_LEFT,
												MSG_LISTEREDIT_MIDDLE,
												MSG_LISTEREDIT_BOTH,
												0},

			_environment_hidden_labels[] = {MSG_ENVIRONMENT_HIDDEN_DRIVES, MSG_ENVIRONMENT_HIDDEN_BAD_DRIVES, 0},

			_environment_default_labels[] = {MSG_ENVIRONMENT_DEFAULT_NONE,
											 MSG_ENVIRONMENT_DEFAULT_LEFTOUT,
											 MSG_ENVIRONMENT_DEFAULT_MOVE,
											 MSG_ENVIRONMENT_DEFAULT_COPY,
											 0},

			_environment_dateformat_labels[] = {MSG_SETTINGS_DATEFORMAT_DDMMMYY,
												MSG_SETTINGS_DATEFORMAT_YYMMDD,
												MSG_SETTINGS_DATEFORMAT_MMDDYY,
												MSG_SETTINGS_DATEFORMAT_DDMMYY,
												0},

			_environment_hidemethod_labels[] = {MSG_SETTINGS_SUB_HIDE_CLOCK,
												MSG_SETTINGS_SUB_HIDE_NONE,
												MSG_SETTINGS_SUB_HIDE_ICON,
												MSG_SETTINGS_SUB_HIDE_MENU,
												0},

			_environment_iconsettings_labels[] = {MSG_ENVIRONMENT_CACHE_ICONS,
												  MSG_ENVIRONMENT_REMAP_ICONS,
												  MSG_ENVIRONMENT_BORDERS_ON,
												  MSG_ENVIRONMENT_FULL_TRANSPARENCY,
												  MSG_ENVIRONMENT_ARROW_ON,
												  MSG_ENVIRONMENT_SPLIT_LABELS,
												  MSG_ENVIRONMENT_OPTIONS_ICONPOS,
												  MSG_ENVIRONMENT_OPTIONS_REMOVE_OPUS,
												  MSG_ENVIRONMENT_OPTIONS_REALTIME_SCROLL,
												  MSG_SETTINGS_PERFORM_ALL_ACTIONS,
												  MSG_SETTINGS_SELECT_ICONS_AUTOMATICALLY,
												  MSG_SETTINGS_TRAP_MORE,
												  MSG_ENVIRONMENT_DISABLE_NOLABELS,
												  MSG_ENVIRONMENT_SMART_ICON_COPY,
												  MSG_ENVIRONMENT_FORCE_LABELSPLIT,
												  MSG_ENVIRONMENT_QUICK_ICON_DRAG,
												  MSG_ENVIRONMENT_CUSTOM_DRAG,
												  0};

// Tags for gadgets
const static struct TagItem

	// Option lister
	_environment_lister_taglist[] = {{DLV_ShowSelected, 0},
									 //		{DLV_ScrollLeft,1},
									 {DLV_TopJustify, 1},
									 {TAG_END, 0}},

	// Relative to area tags[]={
	_environment_relative_taglist[] = {{GTCustom_LayoutRel, GAD_ENVIRONMENT_EDIT_AREA}, {TAG_END, 0}},

	// Screenmode selector
	_environment_screenmode_taglist[] = {{DLV_ShowSelected, 0}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Screen colours taglist
	_environment_colors_taglist[] = {{GA_RelVerify, TRUE},
									 {GA_Immediate, TRUE},
									 {GTSL_Min, 2},
									 {TAG_MORE, (IPTR)_environment_relative_taglist}},

	_environment_colors_display_taglist[] = {{GTCustom_Borderless, TRUE},
											 {GTCustom_Justify, JUSTIFY_LEFT},
											 {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Palette colour selector
	_environment_palette_taglist[] = {{TAG_MORE, (IPTR)_environment_relative_taglist}},

	// RGB sliders
	_environment_colour_slider_taglist[] = {{GA_RelVerify, TRUE},
											{GA_Immediate, TRUE},
											{GTSL_MaxLevelLen, 5},
											{GTSL_LevelFormat, (IPTR) "%ld  "},
											{GTSL_LevelPlace, PLACETEXT_RIGHT},
											{GTCustom_CallBack, (IPTR)&_palette_slider_callback},
											{TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Colour count slider
	_environment_colour_count_taglist[] = {{GTSL_Max, 8}, {TAG_MORE, (IPTR)_environment_colour_slider_taglist}},

	// Lister colour items
	_environment_lister_colour_taglist[] = {{DLV_ShowSelected, 0}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Backdrop preferences
	_environment_buttons[] = {{GTCustom_Control, GAD_ENVIRONMENT_MAIN_WINDOW_FIELD},
							  {DFB_DefPath, (IPTR) "env:sys/WBPattern.prefs"},
							  {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Backdrop prefs file
	_environment_buttons_field[] = {{GTST_MaxChars, 80}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Themes preferences
	_environment_themes_location[] = {{GTCustom_Control, GAD_ENVIRONMENT_THEMES_FIELD},
									  {DFB_DefPath, (IPTR) "DOpus5:Themes/"},
									  {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Themes location field
	_environment_themes_field[] = {{GTST_MaxChars, 256}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Set output window name
	_environment_output_name_tags[] = {{GTST_MaxChars, 60}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Set default stack
	_environment_stack_tags[] = {{GTIN_MaxChars, 7}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Command line length
	_environment_cll_tags[] = {{GTIN_MaxChars, 4}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Icon spacing/grid (small unsigned integers, max 4 digits is plenty)
	_environment_icon_int_tags[] = {{GTIN_MaxChars, 4}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Status bar text
	_environment_status_taglist[] = {{GTST_MaxChars, 80}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Screen title text
	_environment_screen_title_taglist[] =
		{
			//		{GTST_MaxChars,120},
			{GTST_MaxChars, 188},
			{TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Clock format text
	_environment_clock_format_taglist[] = {{GTST_MaxChars, 80}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Screen font
	_environment_font_taglist[] = {{GTCustom_Control, GAD_ENVIRONMENT_SCREENMODE_FONTNAME},
								   {TAG_MORE, (IPTR)_environment_relative_taglist}},

	_environment_fontname_taglist[] = {{GTST_MaxChars, 50}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	_environment_fontsize_taglist[] = {{GTIN_MaxChars, 2}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Hideen drives cycle
	_environment_hidden_drives_cycle[] = {{GTCustom_LocaleLabels, (IPTR)_environment_hidden_labels},
										  {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Hidden drives lister
	_environment_hidden_drives[] = {{DLV_MultiSelect, TRUE},
									{DLV_TopJustify, 1},
									{TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Lister editing
	_environment_listeredit_taglist[] = {{GTCustom_LocaleLabels, (IPTR)_environment_listeredit_labels},
										 {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Lister font
	_environment_lister_font_taglist[] = {{GTCustom_Control, GAD_ENVIRONMENT_LISTER_FONTNAME},
										  //		{GTCustom_FixedWidthOnly,TRUE},
										  {GTCustom_CopyTags, TRUE},
										  {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Desktop popup
	_environment_desktop_popup[] = {{GTCustom_Control, GAD_ENVIRONMENT_DESKTOP_LOCATION},
									{TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Desktop folder location
	_environment_desktop_folder[] = {{GTST_MaxChars, 240}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Default action
	_environment_desktop_default[] = {{GTCustom_LocaleLabels, (IPTR)_environment_default_labels},
									  {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Precision
	_environment_precision_taglist[] = {{GTIN_MaxChars, 3}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Desktop picture
	_environment_desktop_picture[] = {{GTCustom_Control, GAD_ENVIRONMENT_PICTURE_DESK_FIELD},
									  {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Desktop picture field
	_environment_desktop_picture_field[] = {{GTST_MaxChars, 256}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Lister picture
	_environment_lister_picture[] = {{GTCustom_Control, GAD_ENVIRONMENT_PICTURE_LISTER_FIELD},
									 {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Lister picture field
	_environment_lister_picture_field[] = {{GTST_MaxChars, 256}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Requester picture
	_environment_requester_picture[] = {{GTCustom_Control, GAD_ENVIRONMENT_PICTURE_REQ_FIELD},
										{TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Requester picture field
	_environment_requester_picture_field[] = {{GTST_MaxChars, 256}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Maximum number of cached directories
	_environment_caching_buffers_taglist[] = {{GTIN_MaxChars, 5}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Disable caching
	_environment_disable_caching_taglist[] = {{GTCustom_Control, GAD_SETTINGS_CACHING_MAX_BUFFERS},
											  {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Date format
	_environment_dateformat_taglist[] = {{GTCustom_LocaleLabels, (IPTR)_environment_dateformat_labels},
										 {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Hide method
	_environment_hide_taglist[] = {{GTCustom_LocaleLabels, (IPTR)_environment_hidemethod_labels},
								   {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Popkey
	_environment_popkey_taglist[] = {{GTST_MaxChars, 80}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Priority
	_environment_priority_taglist[] = {{GTIN_MaxChars, 4},
									   {GTCustom_MinMax, (127 << 16) | ((unsigned short)-128)},
									   {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Icon settings
	_environment_icon_settings_taglist[] = {{DLV_MultiSelect, 1}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Path list
	_environment_pathlist_taglist[] = {{DLV_ShowSelected, 0},
									   {DLV_DragNotify, 2},
									   {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Sounds
	_environment_soundlist_taglist[] = {{DLV_ShowSelected, 0}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	_environment_soundlist_popup_taglist[] = {{GTCustom_Control, GAD_SETTINGS_SOUNDLIST_PATH},
											  {DFB_DefPath, (IPTR) "DOpus5:Sounds/"},
											  {TAG_MORE, (IPTR)_environment_relative_taglist}},

	_environment_soundlist_path_taglist[] = {{GTST_MaxChars, 255}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	_environment_volume_taglist[] = {{GTIN_MaxChars, 3}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	_environment_volslider_taglist[] = {{GA_RelVerify, TRUE},
										{GA_Immediate, TRUE},
										{GTSL_Max, 64},
										{TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Path list popup
	_environment_pathlist_popup_taglist[] = {{GTCustom_Control, GAD_SETTINGS_PATHLIST_PATH},
											 {TAG_MORE, (IPTR)_environment_relative_taglist}},

	// Path
	_environment_pathlist_path_taglist[] = {{GTST_MaxChars, 255}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	_environment_max_openwith_taglist[] = {{GTIN_MaxChars, 3}, {TAG_MORE, (IPTR)_environment_relative_taglist}},

	_environment_popup_delay_taglist[] = {{GTIN_MaxChars, 2}, {TAG_MORE, (IPTR)_environment_relative_taglist}};

// Flags objects
const ObjectDef _config_environment_objects[] =
	{

		// Config item lister
		{OD_GADGET,
		 MY_LISTVIEW_KIND,
		 {0, 0, 18, 13},
		 {2, 2, 4, 52},
		 0,
		 LISTVIEWFLAG_CURSOR_KEYS,
		 GAD_ENVIRONMENT_ITEM_LISTER,
		 _environment_lister_taglist},

		// Edit area
		{OD_AREA,
		 0,
		 {18, 0, SIZE_MAXIMUM, 13},
		 {8, 2, -2, 52},
		 0,
		 AREAFLAG_RECESSED | AREAFLAG_ERASE,
		 GAD_ENVIRONMENT_EDIT_AREA,
		 0},

		// Save
		{OD_GADGET,
		 BUTTON_KIND,
		 {0, POS_RIGHT_JUSTIFY, 16, 1},
		 {2, -2, 4, 6},
		 MSG_SAVE,
		 BUTTONFLAG_OKAY_BUTTON,
		 GAD_ENVIRONMENT_SAVE,
		 0},

		// Use
		{OD_GADGET,
		 BUTTON_KIND,
		 {POS_CENTER, POS_RIGHT_JUSTIFY, 16, 1},
		 {2, -2, 4, 6},
		 MSG_USE,
		 0,
		 GAD_ENVIRONMENT_USE,
		 0},

		// Cancel
		{OD_GADGET,
		 BUTTON_KIND,
		 {POS_RIGHT_JUSTIFY, POS_RIGHT_JUSTIFY, 16, 1},
		 {-2, -2, 4, 6},
		 MSG_CANCEL,
		 BUTTONFLAG_CANCEL_BUTTON,
		 GAD_ENVIRONMENT_CANCEL,
		 0},

		{OD_END}},

				_environment_display_gadgets[] =
					{

						// Display mode lister
						{OD_GADGET,
						 MY_LISTVIEW_KIND,
						 {2, 1, SIZE_MAX_LESS - 2, 8},
						 {8, 8, -8, 4},
						 MSG_ENVIRONMENT_SCREENMODE_MODE,
						 PLACETEXT_ABOVE,
						 GAD_ENVIRONMENT_SCREENMODE_MODE,
						 _environment_screenmode_taglist},

						// Width entry
						{OD_GADGET,
						 INTEGER_KIND,
						 {12, 9, 5, 1},
						 {12, 16, 12, 4},
						 MSG_ENVIRONMENT_SCREENMODE_WIDTH,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_SCREENMODE_WIDTH,
						 _environment_relative_taglist},

						// Use default width
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {17, 9, 0, 1},
						 {28, 16, 26, 4},
						 MSG_ENVIRONMENT_SCREENMODE_DEFAULT,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_SCREENMODE_DEFWIDTH,
						 _environment_relative_taglist},

						// Height entry
						{OD_GADGET,
						 INTEGER_KIND,
						 {12, 10, 5, 1},
						 {12, 21, 12, 4},
						 MSG_ENVIRONMENT_SCREENMODE_HEIGHT,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_SCREENMODE_HEIGHT,
						 _environment_relative_taglist},

						// Use default height
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {17, 10, 0, 1},
						 {28, 21, 26, 4},
						 MSG_ENVIRONMENT_SCREENMODE_DEFAULT,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_SCREENMODE_DEFHEIGHT,
						 _environment_relative_taglist},

						// Colors slider
						{OD_GADGET,
						 SLIDER_KIND,
						 {12, 11, SIZE_MAX_LESS - 8, 1},
						 {12, 28, -14, 2},
						 MSG_ENVIRONMENT_SCREENMODE_COLORS,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_SCREENMODE_COLORS,
						 _environment_colors_taglist},

						// Colors display
						{OD_GADGET,
						 TEXT_KIND,
						 {POS_RIGHT_JUSTIFY - 1, 11, 6, 1},
						 {-14, 28, 0, 2},
						 0,
						 0,
						 GAD_ENVIRONMENT_COLORS_DISPLAY,
						 _environment_colors_display_taglist},

						// Font popup
						{OD_GADGET,
						 FONT_BUTTON_KIND,
						 {12, 12, 0, 1},
						 {12, 33, 28, 6},
						 MSG_ENVIRONMENT_SCREENMODE_FONT,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_SCREENMODE_FONT,
						 _environment_font_taglist},

						// Font name
						{OD_GADGET,
						 STRING_KIND,
						 {12, 12, SIZE_MAX_LESS - 7, 1},
						 {40, 33, -8, 6},
						 0,
						 0,
						 GAD_ENVIRONMENT_SCREENMODE_FONTNAME,
						 _environment_fontname_taglist},

						// Font size
						{OD_GADGET,
						 INTEGER_KIND,
						 {POS_RIGHT_JUSTIFY - 2, 12, 5, 1},
						 {-8, 33, 0, 6},
						 0,
						 0,
						 GAD_ENVIRONMENT_SCREENMODE_FONTSIZE,
						 _environment_fontsize_taglist},

						{OD_END}},

				_environment_palette_gadgets[] =
					{

						// Number of colours
						{OD_GADGET,
						 SLIDER_KIND,
						 {11, 1, 31, 1},
						 {6, 8, 12, 4},
						 MSG_ENVIRONMENT_PALETTE_COUNT,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_PALETTE_COUNT,
						 _environment_colour_count_taglist},

						// Palette selector
						{OD_GADGET,
						 PALETTE_KIND,
						 {11, 2, 31, 6},
						 {6, 14, 12, 10},
						 0,
						 0,
						 GAD_ENVIRONMENT_PALETTE_PALETTE,
						 _environment_palette_taglist},

						// Red slider
						{OD_GADGET,
						 SLIDER_KIND,
						 {11, 8, 31, 1},
						 {6, 26, 12, 2},
						 MSG_ENVIRONMENT_PALETTE_RED,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_PALETTE_RED,
						 _environment_colour_slider_taglist},

						// Green slider
						{OD_GADGET,
						 SLIDER_KIND,
						 {11, 9, 31, 1},
						 {6, 30, 12, 2},
						 MSG_ENVIRONMENT_PALETTE_GREEN,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_PALETTE_GREEN,
						 _environment_colour_slider_taglist},

						// Blue slider
						{OD_GADGET,
						 SLIDER_KIND,
						 {11, 10, 31, 1},
						 {6, 34, 12, 2},
						 MSG_ENVIRONMENT_PALETTE_BLUE,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_PALETTE_BLUE,
						 _environment_colour_slider_taglist},

						// Restore
						{OD_GADGET,
						 BUTTON_KIND,
						 {11, 11, 10, 1},
						 {6, 38, 4, 6},
						 MSG_ENVIRONMENT_PALETTE_RESET,
						 0,
						 GAD_ENVIRONMENT_PALETTE_RESET,
						 _environment_relative_taglist},

						// Presets
						{OD_GADGET,
						 BUTTON_KIND,
						 {21, 11, 11, 1},
						 {10, 38, 4, 6},
						 MSG_ENVIRONMENT_PALETTE_PRESETS,
						 BUTTONFLAG_IMMEDIATE,
						 GAD_ENVIRONMENT_PALETTE_PRESETS,
						 _environment_relative_taglist},

						// Load
						{OD_GADGET,
						 BUTTON_KIND,
						 {32, 11, 10, 1},
						 {14, 38, 4, 6},
						 MSG_ENVIRONMENT_PALETTE_LOAD,
						 0,
						 GAD_ENVIRONMENT_PALETTE_LOAD,
						 _environment_relative_taglist},

						{OD_END}},

				_environment_lister_gadgets[] =
					{

						// Heading
						{OD_TEXT,
						 TEXTPEN,
						 {4, 4, 0, 1},
						 {0, 0, 0, 0},
						 MSG_ENVIRONMENT_LISTER_SETTINGS,
						 0,
						 0,
						 _environment_relative_taglist},

						// Font popup
						{OD_GADGET,
						 FONT_BUTTON_KIND,
						 {14, 5, 0, 1},
						 {5, 4, 28, 6},
						 MSG_ENVIRONMENT_SCREENMODE_FONT,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_LISTER_FONT,
						 _environment_lister_font_taglist},

						// Font name
						{OD_GADGET,
						 STRING_KIND,
						 {14, 5, SIZE_MAX_LESS - 2, 1},
						 {33, 4, -36, 6},
						 0,
						 0,
						 GAD_ENVIRONMENT_LISTER_FONTNAME,
						 _environment_fontname_taglist},

						// Font size
						{OD_GADGET,
						 INTEGER_KIND,
						 {POS_RIGHT_JUSTIFY - 2, 5, 0, 1},
						 {-8, 4, 28, 6},
						 0,
						 0,
						 GAD_ENVIRONMENT_LISTER_FONTSIZE,
						 _environment_fontsize_taglist},

						// Status text
						{OD_GADGET,
						 STRING_KIND,
						 {14, 6, SIZE_MAX_LESS - 2, 1},
						 {5, 12, -36, 6},
						 MSG_ENVIRONMENT_LISTER_STATUS,
						 0,
						 GAD_ENVIRONMENT_LISTER_STATUS,
						 _environment_status_taglist},

						// Status text popup
						{OD_GADGET,
						 FILE_BUTTON_KIND,
						 {POS_RIGHT_JUSTIFY - 2, 6, 0, 1},
						 {-8, 12, 28, 6},
						 0,
						 0,
						 GAD_ENVIRONMENT_LISTER_STATUS_LIST,
						 _environment_relative_taglist},

						// Field titles
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {14, 7, 0, 1},
						 {5, 20, 26, 6},
						 MSG_ENVIRONMENT_FIELD_TITLES,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_FIELD_TITLES,
						 _environment_relative_taglist},

						// Hide padlock gadget
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {14, 8, 0, 1},
						 {5, 28, 26, 6},
						 MSG_ENVIRONMENT_HIDE_PADLOCK,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_HIDE_PADLOCK,
						 _environment_relative_taglist},

						// DOS patches: live folder updates (bound to dopus/DOSPatch
						// ENVARC: var; behavioural toggle whose effect is most
						// visible in lister auto-refresh, hence on this tab next
						// to the other lister-display flags rather than Misc).
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {14, 9, 0, 1},
						 {5, 36, 26, 6},
						 MSG_ENVIRONMENT_DOS_PATCH,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_DOS_PATCH,
						 _environment_relative_taglist},

						// Open new listers in dual mode
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {14, 10, 0, 1},
						 {5, 44, 26, 6},
						 MSG_ENVIRONMENT_DUAL_DEFAULT,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_DUAL_DEFAULT,
						 _environment_relative_taglist},

						// Striped rows
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {14, 11, 0, 1},
						 {5, 52, 26, 6},
						 MSG_ENVIRONMENT_STRIPED_ROWS,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_STRIPED_ROWS,
						 _environment_relative_taglist},

						{OD_END}},

				_environment_options_gadgets[] =
					{

						// Workbench patches
						{OD_TEXT,
						 TEXTPEN,
						 {1, 1, 0, 1},
						 {5, 0, 0, 0},
						 MSG_ENVIRONMENT_PATCH_OPTIONS,
						 0,
						 0,
						 _environment_relative_taglist},

						// Display AppIcons
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 2, 0, 1},
						 {5, 4, 26, 4},
						 MSG_ENVIRONMENT_OPTIONS_APPICONS,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_OPTIONS_APPICONS,
						 _environment_relative_taglist},

						// Display AppMenuItems
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 3, 0, 1},
						 {5, 9, 26, 4},
						 MSG_ENVIRONMENT_OPTIONS_APPMENU,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_OPTIONS_APPMENU,
						 _environment_relative_taglist},

						// Convert AppIcons to Tools menu
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 4, 0, 1},
						 {5, 14, 26, 4},
						 MSG_ENVIRONMENT_OPTIONS_APPTOOLS,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_OPTIONS_APPTOOLS,
						 _environment_relative_taglist},

						// Hide bad disks
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 5, 0, 1},
						 {5, 19, 26, 4},
						 MSG_ENVIRONMENT_OPTIONS_HIDEBAD,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_OPTIONS_HIDEBAD,
						 _environment_relative_taglist},

						// Show WB leftouts
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 6, 0, 1},
						 {5, 24, 26, 4},
						 MSG_ENVIRONMENT_OPTIONS_SHOW_WBLEFTOUTS,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_OPTIONS_SHOW_WBLEFTOUTS,
						 _environment_relative_taglist},

						// Workbench-compatible title bar mode (lets WB-style patches use it)
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 7, 0, 1},
						 {5, 29, 26, 4},
						 MSG_ENVIRONMENT_OPTIONS_WB_TITLE,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_OPTIONS_WB_TITLE,
						 _environment_relative_taglist},

						// Use Workbench Information (route through WBInfo() for SwazInfo / RAWBInfo)
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 8, 0, 1},
						 {5, 34, 26, 4},
						 MSG_ENVIRONMENT_OPTIONS_USE_WBINFO,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_OPTIONS_USE_WBINFO,
						 _environment_relative_taglist},

						// Enable extra desktop popup keyboard shortcuts
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 9, 0, 1},
						 {5, 39, 26, 4},
						 MSG_ENVIRONMENT_OPTIONS_ENABLE_SHORTCUTS,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_OPTIONS_ENABLE_SHORTCUTS,
						 _environment_relative_taglist},

						// Show: prefer datatypes IFF over the built-in IFF reader
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 10, 0, 1},
						 {5, 44, 26, 4},
						 MSG_ENVIRONMENT_OPTIONS_SHOW_DT_FIRST,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_OPTIONS_SHOW_DT_FIRST,
						 _environment_relative_taglist},

						{OD_END}},

				// Output window
	_environment_output_window[] =
		{

			// Output window label
			{OD_TEXT,
			 TEXTPEN,
			 {2, 2, 0, 1},
			 {0, 0, 0, 0},
			 MSG_ENVIRONMENT_OUTPUT_WINDOW,
			 0,
			 0,
			 _environment_relative_taglist},

			// Name
			{OD_GADGET,
			 STRING_KIND,
			 {12, 3, SIZE_MAX_LESS - 2, 1},
			 {0, 4, -8, 6},
			 MSG_ENVIRONMENT_OUTPUT_NAME,
			 PLACETEXT_LEFT,
			 GAD_ENVIRONMENT_OUTPUT_NAME,
			 _environment_output_name_tags},

			// Handler
			{OD_GADGET,
			 STRING_KIND,
			 {12, 4, SIZE_MAX_LESS - 2, 1},
			 {0, 12, -8, 6},
			 MSG_ENVIRONMENT_OUTPUT_DEVICE,
			 PLACETEXT_LEFT,
			 GAD_ENVIRONMENT_OUTPUT_DEVICE,
			 _environment_output_name_tags},

			// Dimensions text
			{OD_TEXT,
			 TEXTPEN,
			 {11, 5, 0, 1},
			 {0, 27, 0, 0},
			 MSG_ENVIRONMENT_OUTPUT_DIMENSIONS,
			 TEXTFLAG_RIGHT_JUSTIFY,
			 0,
			 _environment_relative_taglist},

			// Position display
			{OD_AREA,
			 TEXTPEN,
			 {12, 5, SIZE_MAX_LESS - 2, 1},
			 {0, 24, -8, 6},
			 0,
			 AREAFLAG_RECESSED | AREAFLAG_OPTIM | AREAFLAG_ERASE | TEXTFLAG_CENTER | AREAFLAG_FILL_COLOUR,
			 GAD_ENVIRONMENT_OUTPUT_DIMENSIONS,
			 _environment_relative_taglist},

			// Set
			{OD_GADGET,
			 BUTTON_KIND,
			 {12, 6, SIZE_MAX_LESS - 2, 1},
			 {0, 30, -8, 6},
			 MSG_ENVIRONMENT_OUTPUT_SET,
			 0,
			 GAD_ENVIRONMENT_OUTPUT_SET,
			 _environment_relative_taglist},

			// AmigaDOS label
			{OD_TEXT,
			 TEXTPEN,
			 {2, 8, 0, 1},
			 {0, 40, 0, 0},
			 MSG_ENVIRONMENT_AMIGADOS,
			 0,
			 0,
			 _environment_relative_taglist},

			// Default stack
			{OD_GADGET,
			 INTEGER_KIND,
			 {12, 9, 8, 1},
			 {0, 44, 4, 6},
			 MSG_ENVIRONMENT_STACK,
			 PLACETEXT_LEFT,
			 GAD_ENVIRONMENT_STACK,
			 _environment_stack_tags},

			// Command line length
			{OD_GADGET,
			 INTEGER_KIND,
			 {12, 10, 8, 1},
			 {0, 52, 4, 6},
			 MSG_ENVIRONMENT_CLL,
			 PLACETEXT_LEFT,
			 GAD_ENVIRONMENT_CLL,
			 _environment_cll_tags},

			{OD_END}},

				// Lister options
	_environment_listeroptions[] =
		{

			// Options title
			{OD_TEXT,
			 TEXTPEN,
			 {1, 1, 0, 1},
			 {5, -4, 0, 0},
			 MSG_ENVIRONMENT_LISTER_OPTIONS,
			 0,
			 0,
			 _environment_relative_taglist},

			// Device list
			{OD_GADGET,
			 CHECKBOX_KIND,
			 {4, 2, 0, 1},
			 {5, 0, 26, 4},
			 MSG_ENVIRONMENT_NEW_DEVICELIST,
			 PLACETEXT_RIGHT,
			 GAD_ENVIRONMENT_NEW_DEVICELIST,
			 _environment_relative_taglist},

			// RMB 2xclick editing
			{OD_GADGET,
			 CHECKBOX_KIND,
			 {4, 3, 0, 1},
			 {5, 6, 26, 4},
			 MSG_ENVIRONMENT_2XCLICK_EDITING,
			 PLACETEXT_RIGHT,
			 GAD_ENVIRONMENT_2XCLICK_EDITING,
			 _environment_relative_taglist},

			// Simple refresh
			{OD_GADGET,
			 CHECKBOX_KIND,
			 {4, 4, 0, 1},
			 {5, 12, 26, 4},
			 MSG_ENVIRONMENT_SIMPLE_REFRESH,
			 PLACETEXT_RIGHT,
			 GAD_ENVIRONMENT_SIMPLE_REFRESH,
			 _environment_relative_taglist},

			// Use Snapshot
			{OD_GADGET,
			 CHECKBOX_KIND,
			 {4, 5, 0, 1},
			 {5, 18, 26, 4},
			 MSG_ENVIRONMENT_USE_SNAPSHOT,
			 PLACETEXT_RIGHT,
			 GAD_ENVIRONMENT_USE_SNAPSHOT,
			 _environment_relative_taglist},

			// Name mode popup
			{OD_GADGET,
			 CHECKBOX_KIND,
			 {4, 6, 0, 1},
			 {5, 24, 26, 4},
			 MSG_ENVIRONMENT_NAME_POPUP,
			 PLACETEXT_RIGHT,
			 GAD_ENVIRONMENT_NAME_POPUP,
			 _environment_relative_taglist},

			// Drop into sub-dirs
			{OD_GADGET,
			 CHECKBOX_KIND,
			 {4, 7, 0, 1},
			 {5, 30, 26, 4},
			 MSG_ENVIRONMENT_DROP_SUBDIR,
			 PLACETEXT_RIGHT,
			 GAD_ENVIRONMENT_DROP_SUBDIR,
			 _environment_relative_taglist},

			// No file select on window activation
			{OD_GADGET,
			 CHECKBOX_KIND,
			 {4, 8, 0, 1},
			 {5, 36, 26, 4},
			 MSG_ENVIRONMENT_NO_ACTIVE_SELECT,
			 PLACETEXT_RIGHT,
			 GAD_ENVIRONMENT_NO_ACTIVE_SELECT,
			 _environment_relative_taglist},

			// Ben Vost zoom mode
			{OD_GADGET,
			 CHECKBOX_KIND,
			 {4, 9, 0, 1},
			 {5, 42, 26, 4},
			 MSG_ENVIRONMENT_VOSTY_ZOOM,
			 PLACETEXT_RIGHT,
			 GAD_ENVIRONMENT_VOSTY_ZOOM,
			 _environment_relative_taglist},

			// Full path in lister title
			{OD_GADGET,
			 CHECKBOX_KIND,
			 {4, 10, 0, 1},
			 {5, 48, 26, 4},
			 MSG_ENVIRONMENT_FULL_PATH,
			 PLACETEXT_RIGHT,
			 GAD_ENVIRONMENT_FULL_PATH,
			 _environment_relative_taglist},

			// Lister editing
			{OD_GADGET,
			 CYCLE_KIND,
			 {4, 11, 16, 1},
			 {5, 54, 24, 6},
			 MSG_ENVIRONMENT_LISTER_EDITING,
			 PLACETEXT_RIGHT,
			 GAD_ENVIRONMENT_LISTER_EDITING,
			 _environment_listeredit_taglist},

			{OD_END}},

				// Lister size
	_environment_listersize[] =
		{

			// Default lister settings
			{OD_TEXT,
			 TEXTPEN,
			 {2, 4, 0, 1},
			 {4, 0, 0, 0},
			 MSG_ENVIRONMENT_LISTER_DEFAULT,
			 0,
			 0,
			 _environment_relative_taglist},

			// Dimensions text
			{OD_TEXT,
			 TEXTPEN,
			 {18, 6, 0, 1},
			 {0, 4, 0, 0},
			 MSG_ENVIRONMENT_DEFAULT_SIZE,
			 TEXTFLAG_RIGHT_JUSTIFY,
			 0,
			 _environment_relative_taglist},

			// Position display
			{OD_AREA,
			 TEXTPEN,
			 {19, 6, 25, 1},
			 {0, 1, 4, 6},
			 0,
			 AREAFLAG_RECESSED | AREAFLAG_OPTIM | AREAFLAG_ERASE | TEXTFLAG_CENTER | AREAFLAG_FILL_COLOUR,
			 GAD_ENVIRONMENT_DEFAULT_SIZE,
			 _environment_relative_taglist},

			// Set
			{OD_GADGET,
			 BUTTON_KIND,
			 {19, 7, 25, 1},
			 {0, 7, 4, 6},
			 MSG_ENVIRONMENT_OUTPUT_SET,
			 0,
			 GAD_ENVIRONMENT_OUTPUT_SETSIZE,
			 _environment_relative_taglist},

			// Format text
			{OD_TEXT,
			 TEXTPEN,
			 {18, 9, 0, 1},
			 {0, 13, 0, 0},
			 MSG_ENVIRONMENT_DEFAULT_FORMAT,
			 TEXTFLAG_RIGHT_JUSTIFY,
			 0,
			 _environment_relative_taglist},

			// Set
			{OD_GADGET,
			 BUTTON_KIND,
			 {19, 9, 25, 1},
			 {0, 10, 4, 6},
			 MSG_ENVIRONMENT_FORMAT_SET,
			 0,
			 GAD_ENVIRONMENT_LISTER_EDIT_FORMAT,
			 _environment_relative_taglist},

			{OD_END}},

				// Desktop
	_environment_desktop[] =
		{

			// Hidden drives cycle
			{OD_GADGET,
			 CYCLE_KIND,
			 {2, 1, SIZE_MAX_LESS - 2, 1},
			 {8, 4, -8, 6},
			 0,
			 0,
			 GAD_ENVIRONMENT_HIDDEN_DRIVES_CYCLE,
			 _environment_hidden_drives_cycle},

			// Hidden drives
			{OD_GADGET,
			 MY_LISTVIEW_KIND,
			 {2, 2, SIZE_MAX_LESS - 2, SIZE_MAX_LESS - 4},
			 {8, 12, -8, -28},
			 0,
			 0,
			 GAD_ENVIRONMENT_HIDDEN_DRIVES,
			 _environment_hidden_drives},

			// Folder popup
			{OD_GADGET,
			 DIR_BUTTON_KIND,
			 {16, POS_RIGHT_JUSTIFY - 3, 0, 1},
			 {4, -20, 26, 6},
			 MSG_ENVIRONMENT_DESKTOP_LOCATION,
			 PLACETEXT_LEFT,
			 GAD_ENVIRONMENT_DESKTOP_POPUP,
			 _environment_desktop_popup},

			// Desktop folder location
			{OD_GADGET,
			 STRING_KIND,
			 {16, POS_RIGHT_JUSTIFY - 3, SIZE_MAX_LESS - 2, 1},
			 {30, -20, -8, 6},
			 0,
			 0,
			 GAD_ENVIRONMENT_DESKTOP_LOCATION,
			 _environment_desktop_folder},

			// Popup enabled
			{OD_GADGET,
			 CHECKBOX_KIND,
			 {16, POS_RIGHT_JUSTIFY - 2, 0, 1},
			 {4, -12, 26, 6},
			 MSG_ENVIRONMENT_DESKTOP_ENABLE,
			 PLACETEXT_LEFT,
			 GAD_ENVIRONMENT_DESKTOP_ENABLE,
			 _environment_relative_taglist},

			// Default action
			{OD_GADGET,
			 CYCLE_KIND,
			 {16, POS_RIGHT_JUSTIFY - 1, SIZE_MAX_LESS - 2, 1},
			 {4, -4, -8, 6},
			 MSG_ENVIRONMENT_DESKTOP_DEFAULT,
			 PLACETEXT_LEFT,
			 GAD_ENVIRONMENT_DESKTOP_DEFAULT,
			 _environment_desktop_default},

			{OD_END}},

				// Icons
	_environment_icons[] =
		{

			// Icon settings
			{OD_GADGET,
			 MY_LISTVIEW_KIND,
			 {1, 2, SIZE_MAX_LESS - 1, SIZE_MAX_LESS - 1},
			 {4, 8, -4, -8},
			 MSG_ENVIRONMENT_ICON_SETTINGS,
			 PLACETEXT_ABOVE,
			 GAD_ENVIRONMENT_ICON_SETTINGS,
			 _environment_icon_settings_taglist},

			{OD_END}},

				// Icon layout (was dopus/IconSpace*, dopus/IconGrid* and dopus/ReturnOfBenify env vars)
	_environment_icon_layout[] =
		{

			// Page title
			{OD_TEXT,
			 TEXTPEN,
			 {1, 1, 0, 1},
			 {5, 0, 0, 0},
			 MSG_ENVIRONMENT_ICON_LAYOUT_TITLE,
			 0,
			 0,
			 _environment_relative_taglist},

			// Icon spacing X
			{OD_GADGET,
			 INTEGER_KIND,
			 {16, 3, 6, 1},
			 {2, 14, 8, 6},
			 MSG_ENVIRONMENT_ICON_SPACE_X,
			 PLACETEXT_LEFT,
			 GAD_ENVIRONMENT_ICON_SPACE_X,
			 _environment_icon_int_tags},

			// Icon spacing Y
			{OD_GADGET,
			 INTEGER_KIND,
			 {16, 4, 6, 1},
			 {2, 22, 8, 6},
			 MSG_ENVIRONMENT_ICON_SPACE_Y,
			 PLACETEXT_LEFT,
			 GAD_ENVIRONMENT_ICON_SPACE_Y,
			 _environment_icon_int_tags},

			// Icon grid X (1 = no grid)
			{OD_GADGET,
			 INTEGER_KIND,
			 {16, 6, 6, 1},
			 {2, 36, 8, 6},
			 MSG_ENVIRONMENT_ICON_GRID_X,
			 PLACETEXT_LEFT,
			 GAD_ENVIRONMENT_ICON_GRID_X,
			 _environment_icon_int_tags},

			// Icon grid Y (1 = no grid)
			{OD_GADGET,
			 INTEGER_KIND,
			 {16, 7, 6, 1},
			 {2, 44, 8, 6},
			 MSG_ENVIRONMENT_ICON_GRID_Y,
			 PLACETEXT_LEFT,
			 GAD_ENVIRONMENT_ICON_GRID_Y,
			 _environment_icon_int_tags},

			// Drop on desktop saves position permanently
			{OD_GADGET,
			 CHECKBOX_KIND,
			 {4, 9, 0, 1},
			 {5, 58, 26, 4},
			 MSG_ENVIRONMENT_BENIFY,
			 PLACETEXT_RIGHT,
			 GAD_ENVIRONMENT_BENIFY,
			 _environment_relative_taglist},

			{OD_END}},

				_environment_listercolours[] =
					{

						// Lister colour items
						{OD_GADGET,
						 MY_LISTVIEW_KIND,
						 {2, 2, 16, SIZE_MAX_LESS - 1},
						 {4, 12, 12, -4},
						 MSG_ENVIRONMENT_LISTER_COLOUR_ITEMS,
						 PLACETEXT_ABOVE,
						 GAD_ENVIRONMENT_LISTER_COLOUR_ITEMS,
						 _environment_lister_colour_taglist},

						// Colour example
						{OD_AREA,
						 0,
						 {18, 2, SIZE_MAX_LESS - 2, SIZE_MAX_LESS - 2},
						 {20, 12, -8, -12},
						 MSG_ENVIRONMENT_LISTER_EXAMPLE,
						 AREAFLAG_ERASE | AREAFLAG_RECESSED | TEXTFLAG_CENTER | AREAFLAG_FILL_COLOUR,
						 GAD_ENVIRONMENT_LISTER_EXAMPLE,
						 _environment_relative_taglist},

						// Modify colour
						{OD_GADGET,
						 BUTTON_KIND,
						 {18, POS_RIGHT_JUSTIFY - 1, SIZE_MAX_LESS - 2, 1},
						 {20, -4, -8, 6},
						 MSG_ENVIRONMENT_COLOURS_MODIFY,
						 0,
						 GAD_ENVIRONMENT_COLOURS_MODIFY,
						 _environment_relative_taglist},

						{OD_END}},

				_environment_icon_colours[] =
					{

						// NewIcons heading
						{OD_TEXT,
						 TEXTPEN,
						 {1, 2, 0, 1},
						 {5, 6, 0, 0},
						 MSG_ENVIRONMENT_NEWICONS,
						 0,
						 0,
						 _environment_relative_taglist},

						// Enable NewIcons
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 3, 0, 1},
						 {5, 10, 26, 4},
						 MSG_ENVIRONMENT_NEWICONS_ENABLE,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_NEWICONS_ENABLE,
						 _environment_relative_taglist},

						// Discourage NewIcons
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 4, 0, 1},
						 {5, 16, 26, 4},
						 MSG_ENVIRONMENT_NEWICONS_DISCOURAGE,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_NEWICONS_DISCOURAGE,
						 _environment_relative_taglist},

						// Dithering
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 5, 0, 1},
						 {5, 22, 26, 4},
						 MSG_ENVIRONMENT_NEWICONS_DITHERING,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_NEWICONS_DITHERING,
						 _environment_relative_taglist},

						// RTG
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 6, 0, 1},
						 {5, 28, 26, 4},
						 MSG_ENVIRONMENT_NEWICONS_RTGMODE,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_NEWICONS_RTGMODE,
						 _environment_relative_taglist},

						// Pen precision
						{OD_GADGET,
						 INTEGER_KIND,
						 {4, 7, 5, 1},
						 {5, 34, 8, 4},
						 MSG_ENVIRONMENT_NEWICONS_PRECISION,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_NEWICONS_PRECISION,
						 _environment_precision_taglist},

						// Icon settings
						{OD_TEXT,
						 TEXTPEN,
						 {1, 9, 0, 1},
						 {5, 40, 0, 0},
						 MSG_ENVIRONMENT_ICON_FONTCOL,
						 0,
						 0,
						 _environment_relative_taglist},

						// Desktop icon font
						{OD_GADGET,
						 BUTTON_KIND,
						 {4, 10, 16, 1},
						 {5, 44, 4, 6},
						 MSG_ENVIRONMENT_ICON_FONT_DESKTOP,
						 0,
						 GAD_ENVIRONMENT_ICON_FONT_DESKTOP,
						 _environment_relative_taglist},

						// Windows icon font
						{OD_GADGET,
						 BUTTON_KIND,
						 {22, 10, 16, 1},
						 {9, 44, 4, 6},
						 MSG_ENVIRONMENT_ICON_FONT_WINDOWS,
						 0,
						 GAD_ENVIRONMENT_ICON_FONT_WINDOWS,
						 _environment_relative_taglist},

						{OD_END}},

				_environment_pictures[] =
					{

						// Disable backdrop
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {12, 0, 0, 1},
						 {4, 4, 26, 6},
						 MSG_ENVIRONMENT_OPTIONS_ENABLE_PATTERN,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_OPTIONS_DISPLAY_BACKDROP,
						 _environment_relative_taglist},

						// Desktop picture
						{OD_GADGET,
						 FILE_BUTTON_KIND,
						 {12, 1, 0, 1},
						 {4, 12, 26, 6},
						 MSG_ENVIRONMENT_PICTURE_DESKTOP,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_PICTURE_DESK,
						 _environment_desktop_picture},

						// Desktop picture field
						{OD_GADGET,
						 STRING_KIND,
						 {12, 1, SIZE_MAX_LESS - 2, 1},
						 {30, 12, -34, 6},
						 0,
						 0,
						 GAD_ENVIRONMENT_PICTURE_DESK_FIELD,
						 _environment_desktop_picture_field},

						// Desktop picture prefs
						{OD_GADGET,
						 BUTTON_KIND,
						 {POS_RIGHT_JUSTIFY - 2, 1, 0, 1},
						 {-8, 12, 26, 6},
						 (IPTR) "?",
						 TEXTFLAG_TEXT_STRING | BUTTONFLAG_IMMEDIATE,
						 GAD_ENVIRONMENT_PICTURE_DESK_PREFS,
						 _environment_relative_taglist},

						// Lister picture
						{OD_GADGET,
						 FILE_BUTTON_KIND,
						 {12, 2, 0, 1},
						 {4, 20, 26, 6},
						 MSG_ENVIRONMENT_PICTURE_LISTER,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_PICTURE_LISTER,
						 _environment_lister_picture},

						// Lister picture field
						{OD_GADGET,
						 STRING_KIND,
						 {12, 2, SIZE_MAX_LESS - 2, 1},
						 {30, 20, -34, 6},
						 0,
						 0,
						 GAD_ENVIRONMENT_PICTURE_LISTER_FIELD,
						 _environment_lister_picture_field},

						// Lister picture prefs
						{OD_GADGET,
						 BUTTON_KIND,
						 {POS_RIGHT_JUSTIFY - 2, 2, 0, 1},
						 {-8, 20, 26, 6},
						 (IPTR) "?",
						 TEXTFLAG_TEXT_STRING | BUTTONFLAG_IMMEDIATE,
						 GAD_ENVIRONMENT_PICTURE_LISTER_PREFS,
						 _environment_relative_taglist},

						// Requester picture
						{OD_GADGET,
						 FILE_BUTTON_KIND,
						 {12, 3, 0, 1},
						 {4, 28, 26, 6},
						 MSG_ENVIRONMENT_PICTURE_REQUESTER,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_PICTURE_REQ,
						 _environment_requester_picture},

						// Requester picture field
						{OD_GADGET,
						 STRING_KIND,
						 {12, 3, SIZE_MAX_LESS - 2, 1},
						 {30, 28, -34, 6},
						 0,
						 0,
						 GAD_ENVIRONMENT_PICTURE_REQ_FIELD,
						 _environment_requester_picture_field},

						// Requester picture prefs
						{OD_GADGET,
						 BUTTON_KIND,
						 {POS_RIGHT_JUSTIFY - 2, 3, 0, 1},
						 {-8, 28, 26, 6},
						 (IPTR) "?",
						 TEXTFLAG_TEXT_STRING | BUTTONFLAG_IMMEDIATE,
						 GAD_ENVIRONMENT_PICTURE_REQ_PREFS,
						 _environment_relative_taglist},

						// WBPattern title
						{OD_TEXT,
						 TEXTPEN,
						 {2, 5, 0, 1},
						 {4, 34, 0, 0},
						 MSG_ENVIRONMENT_PICTURE_WBPATTERN,
						 0,
						 0,
						 _environment_relative_taglist},

						// Use WBPattern settings
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {12, 6, 0, 1},
						 {4, 36, 26, 6},
						 MSG_ENVIRONMENT_PICTURE_USE_WBPATTERN,
						 PLACETEXT_RIGHT,
						 GAD_ENVIRONMENT_PICTURE_USE_WBPATTERN,
						 _environment_relative_taglist},

						// Backdrop prefs file
						{OD_GADGET,
						 FILE_BUTTON_KIND,
						 {12, 7, 0, 1},
						 {4, 44, 26, 6},
						 MSG_ENVIRONMENT_BUTTONS_FIELD,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_MAIN_WINDOW_BUTTONS,
						 _environment_buttons},

						// Backdrop prefs field
						{OD_GADGET,
						 STRING_KIND,
						 {12, 7, SIZE_MAX_LESS - 2, 1},
						 {30, 44, -34, 6},
						 0,
						 0,
						 GAD_ENVIRONMENT_MAIN_WINDOW_FIELD,
						 _environment_buttons_field},

						// Edit prefs
						{OD_GADGET,
						 BUTTON_KIND,
						 {POS_RIGHT_JUSTIFY - 2, 7, 0, 1},
						 {-8, 44, 26, 6},
						 (IPTR) "_!",
						 TEXTFLAG_TEXT_STRING,
						 GAD_ENVIRONMENT_EDIT_PATTERN,
						 _environment_relative_taglist},

						// Theme folder title
						{OD_TEXT,
						 TEXTPEN,
						 {2, 9, 0, 1},
						 {4, 50, 0, 0},
						 MSG_ENVIRONMENT_THEMES_PATH,
						 0,
						 0,
						 _environment_relative_taglist},

						// Theme folder popup
						{OD_GADGET,
						 DIR_BUTTON_KIND,
						 {12, 10, 0, 1},
						 {4, 52, 26, 6},
						 MSG_ENVIRONMENT_THEMES_LOCATION,
						 PLACETEXT_LEFT,
						 GAD_ENVIRONMENT_THEMES_LOCATION,
						 _environment_themes_location},

						// Theme folder field
						{OD_GADGET,
						 STRING_KIND,
						 {12, 10, SIZE_MAX_LESS - 2, 1},
						 {30, 52, -8, 6},
						 0,
						 0,
						 GAD_ENVIRONMENT_THEMES_FIELD,
						 _environment_themes_field},

						{OD_END}},

				_environment_copy_gadgets[] =
					{

						// When copying files and directories...
						{OD_TEXT,
						 TEXTPEN,
						 {1, 0, 0, 0},
						 {5, 11, 0, 0},
						 MSG_SETTINGS_WHEN_COPYING_FILES,
						 0,
						 0,
						 _environment_relative_taglist},

						// Update destination space
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 1, 0, 1},
						 {5, 15, 26, 4},
						 MSG_SETTINGS_UPDATE_FREE_SPACE,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_COPY_UPDATE,
						 _environment_relative_taglist},

						// Set archive bit
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 2, 0, 1},
						 {5, 20, 26, 4},
						 MSG_SETTINGS_SET_ARCHIVE_BIT,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_COPY_SETARCHIVE,
						 _environment_relative_taglist},

						// Also copy source's...
						{OD_TEXT,
						 TEXTPEN,
						 {1, 3, 0, 0},
						 {5, 32, 0, 0},
						 MSG_SETTINGS_ALSO_COPY_SOURCE,
						 0,
						 0,
						 _environment_relative_taglist},

						// Copy datestamp
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 4, 0, 1},
						 {5, 36, 26, 4},
						 MSG_SETTINGS_COPY_DATESTAMP,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_COPY_DATESTAMP,
						 _environment_relative_taglist},

						// Copy protection bits
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 5, 0, 1},
						 {5, 41, 26, 4},
						 MSG_SETTINGS_COPY_PROTECTION,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_COPY_PROTECTION,
						 _environment_relative_taglist},

						// Copy protection bits
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 6, 0, 1},
						 {5, 46, 26, 4},
						 MSG_SETTINGS_COPY_COMMENT,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_COPY_COMMENT,
						 _environment_relative_taglist},

						// Replace title
						{OD_TEXT,
						 TEXTPEN,
						 {1, 7, 0, 0},
						 {5, 58, 0, 0},
						 MSG_SETTINGS_SUB_REPLACE_TITLE,
						 0,
						 0,
						 _environment_relative_taglist},

						// Verbose replace requester
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 8, 0, 1},
						 {5, 62, 26, 4},
						 MSG_SETTINGS_VERBOSE_REPLACE,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_VERBOSE_REPLACE,
						 _environment_relative_taglist},

						// Check version
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 9, 0, 1},
						 {5, 67, 26, 4},
						 MSG_SETTINGS_CHECK_VERSION,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_CHECK_VERSION,
						 _environment_relative_taglist},

						{OD_END}},

				_environment_delete_gadgets[] =
					{

						// Ask before...
						{OD_TEXT,
						 TEXTPEN,
						 {1, 3, 0, 0},
						 {5, 5, 0, 0},
						 MSG_SETTINGS_ASK_BEFORE,
						 0,
						 0,
						 _environment_relative_taglist},

						// Commencing delete
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 4, 0, 1},
						 {5, 9, 26, 4},
						 MSG_SETTINGS_COMMENCING_DELETE,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_ASKDELETE_BEFORE,
						 _environment_relative_taglist},

						// Deleting files
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 5, 0, 1},
						 {5, 14, 26, 4},
						 MSG_SETTINGS_DELETING_FILES,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_ASKDELETE_FILES,
						 _environment_relative_taglist},

						// Deleting non-empty directories
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 6, 0, 1},
						 {5, 19, 26, 4},
						 MSG_SETTINGS_DELETING_DIRS,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_ASKDELETE_DIRS,
						 _environment_relative_taglist},

						{OD_END}},

				_environment_caching_gadgets[] =
					{

						// Caching title
						{OD_TEXT,
						 TEXTPEN,
						 {1, 2, 0, 1},
						 {5, 5, 0, 0},
						 MSG_ENVIRONMENT_CACHING_TITLE,
						 0,
						 0,
						 _environment_relative_taglist},

						// Maximum buffers
						{OD_GADGET,
						 INTEGER_KIND,
						 {4, 3, 2, 1},
						 {5, 9, 26, 4},
						 MSG_SETTINGS_CACHING_MAX_BUFFERS,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_CACHING_MAX_BUFFERS,
						 _environment_caching_buffers_taglist},

						// Disable caching
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 4, 0, 1},
						 {5, 14, 26, 4},
						 MSG_SETTINGS_DISABLE_CACHING,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_DISABLE_CACHING,
						 _environment_disable_caching_taglist},

						// Re-read changed buffers
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 5, 0, 1},
						 {5, 19, 26, 4},
						 MSG_SETTINGS_REREAD_CHANGED_BUFFERS,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_REREAD_CHANGED_BUFFERS,
						 _environment_relative_taglist},

						// Miscellaneous title
						{OD_TEXT,
						 TEXTPEN,
						 {1, 7, 0, 1},
						 {5, 26, 0, 0},
						 MSG_SETTINGS_SUB_MISC_TITLE,
						 0,
						 0,
						 _environment_relative_taglist},

						// Enable MUFS
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 8, 0, 1},
						 {5, 30, 26, 4},
						 MSG_SETTINGS_ENABLE_MUFS,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_ENABLE_MUFS,
						 _environment_relative_taglist},

						// Max filename length
						{OD_GADGET,
						 INTEGER_KIND,
						 {4, 9, 5, 1},
						 {5, 38, 8, 6},
						 MSG_SETTINGS_MAX_FILENAME,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_MAX_FILENAME,
						 _environment_max_openwith_taglist},

						{OD_END}},

				_environment_date_gadgets[] =
					{

						// Date format...
							{OD_TEXT,
							 TEXTPEN,
							 {1, 0, 0, 0},
							 {5, 2, 0, 0},
							 MSG_SETTINGS_DATE_FORMAT,
							 0,
							 0,
						 _environment_relative_taglist},

						// Date format gadget
							{OD_GADGET,
							 MX_KIND,
							 {4, 1, 1, 1},
							 {5, 6, 7, 1},
							 0,
							 PLACETEXT_RIGHT,
							 GAD_SETTINGS_DATE_FORMAT,
						 _environment_dateformat_taglist},

						// Misc. flags...
							{OD_TEXT,
							 TEXTPEN,
							 {1, 6, 0, 0},
							 {5, 10, 0, 0},
							 MSG_SETTINGS_MISC,
							 0,
							 0,
						 _environment_relative_taglist},

						// Name substitution
							{OD_GADGET,
							 CHECKBOX_KIND,
							 {4, 7, 0, 1},
							 {5, 14, 26, 4},
							 MSG_SETTINGS_NAME_SUBSTITUTION,
							 PLACETEXT_RIGHT,
							 GAD_SETTINGS_DATE_NAMESUB,
						 _environment_relative_taglist},

						// 12 hour clock
							{OD_GADGET,
							 CHECKBOX_KIND,
							 {4, 8, 0, 1},
							 {5, 19, 26, 4},
							 MSG_SETTINGS_12_HOUR_CLOCK,
							 PLACETEXT_RIGHT,
							 GAD_SETTINGS_DATE_12HOUR,
						 _environment_relative_taglist},

						// Thousands separator
							{OD_GADGET,
							 CHECKBOX_KIND,
							 {4, 9, 0, 1},
							 {5, 24, 26, 4},
							 MSG_SETTINGS_THOUSANDS_SEPS,
							 PLACETEXT_RIGHT,
							 GAD_SETTINGS_THOUSANDS_SEPS,
						 _environment_relative_taglist},

						// Clock format label
							{OD_TEXT,
							 TEXTPEN,
							 {1, 11, 0, 0},
							 {5, 29, 0, 0},
							 MSG_ENVIRONMENT_CLOCK_FORMAT,
							 0,
							 0,
						 _environment_relative_taglist},

						// Clock format popup
							{OD_GADGET,
							 FILE_BUTTON_KIND,
							 {4, 12, 0, 1},
							 {5, 35, 28, 6},
							 0,
							 0,
							 GAD_ENVIRONMENT_CLOCK_FORMAT_LIST,
						 _environment_relative_taglist},

						// Clock format text
							{OD_GADGET,
							 STRING_KIND,
							 {4, 12, SIZE_MAX_LESS - 2, 1},
							 {33, 35, -8, 6},
							 0,
							 0,
							 GAD_ENVIRONMENT_CLOCK_FORMAT,
						 _environment_clock_format_taglist},

						{OD_END}},

				_environment_hide_gadgets[] =
					{

						// Hide method title
						{OD_TEXT,
						 TEXTPEN,
						 {1, 2, 0, 0},
						 {5, 4, 0, 0},
						 MSG_SETTINGS_SUB_HIDE_TITLE,
						 0,
						 0,
						 _environment_relative_taglist},

						// Date format gadget
						{OD_GADGET,
						 MX_KIND,
						 {4, 3, 1, 1},
						 {5, 8, 7, 1},
						 0,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_HIDE_METHOD,
						 _environment_hide_taglist},

						// Popkey
						{OD_GADGET,
						 HOTKEY_KIND,
						 {10, 8, SIZE_MAX_LESS - 1, 1},
						 {4, 16, -4, 4},
						 MSG_SETTINGS_POPKEY,
						 PLACETEXT_LEFT,
						 GAD_SETTINGS_POPKEY,
						 _environment_popkey_taglist},

						{OD_END}},

				_environment_misc_gadgets[] =
					{

						// Misc title
						{OD_TEXT,
						 TEXTPEN,
						 {1, 0, 0, 0},
						 {5, 3, 0, 0},
						 MSG_SETTINGS_SUB_MISC_TITLE,
						 0,
						 0,
						 _environment_relative_taglist},

						// Right button hook
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 1, 0, 1},
						 {5, 6, 26, 4},
						 MSG_SETTINGS_MOUSE_HOOK,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_MOUSE_HOOK,
						 _environment_relative_taglist},

						// Quick quit
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 2, 0, 1},
						 {5, 11, 26, 4},
						 MSG_SETTINGS_QUICK_QUIT,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_QUICK_QUIT,
						 _environment_relative_taglist},

						// Extended lister key selection
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 3, 0, 1},
						 {5, 16, 26, 4},
						 MSG_SETTINGS_EXTENDED_KEY,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_EXTENDED_KEY,
						 _environment_relative_taglist},

						// Filetype sniffer
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 4, 0, 1},
						 {5, 21, 26, 4},
						 MSG_SETTINGS_FILETYPE_SNIFFER,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_FILETYPE_SNIFFER,
						 _environment_relative_taglist},

						// Thin borders
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {4, 5, 0, 1},
						 {5, 26, 26, 4},
						 MSG_SETTINGS_THIN_BORDERS,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_THIN_BORDERS,
						 _environment_relative_taglist},

						// Popup menu delay
						{OD_GADGET,
						 INTEGER_KIND,
						 {4, 6, 5, 1},
						 {5, 32, 8, 6},
						 MSG_SETTINGS_POPUP_DELAY,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_POPUP_DELAY,
						 _environment_popup_delay_taglist},

						// Max open with
						{OD_GADGET,
						 INTEGER_KIND,
						 {4, 7, 5, 1},
						 {5, 40, 8, 6},
						 MSG_SETTINGS_MAX_OPENWITH,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_MAX_OPENWITH,
						 _environment_max_openwith_taglist},

						// Mouse wheel scroll lines
						{OD_GADGET,
						 INTEGER_KIND,
						 {4, 8, 5, 1},
						 {5, 48, 8, 6},
						 MSG_SETTINGS_WHEEL_SCROLL_LINES,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_WHEEL_SCROLL_LINES,
						 _environment_icon_int_tags},

						// Screen title
						{OD_TEXT,
						 TEXTPEN,
						 {1, 9, 0, 0},
						 {5, 59, 0, 0},
						 MSG_ENVIRONMENT_SCREEN_TITLE,
						 0,
						 0,
						 _environment_relative_taglist},

						// Status text popup
						{OD_GADGET,
						 FILE_BUTTON_KIND,
						 {4, 10, 0, 1},
						 {5, 62, 28, 6},
						 0,
						 0,
						 GAD_ENVIRONMENT_SCREEN_TITLE_LIST,
						 _environment_relative_taglist},

						// Screen title text
						{OD_GADGET,
						 STRING_KIND,
						 {4, 10, SIZE_MAX_LESS - 2, 1},
						 {33, 62, -8, 6},
						 0,
						 0,
						 GAD_ENVIRONMENT_SCREEN_TITLE,
						 _environment_screen_title_taglist},

						{OD_END}},

				_environment_pri_gadgets[] =
					{

						// Priority title
						{OD_TEXT,
						 TEXTPEN,
						 {1, 3, 0, 0},
						 {5, 11, 0, 0},
						 MSG_SETTINGS_SUB_PRI_TITLE,
						 0,
						 0,
						 _environment_relative_taglist},

						// Main priority
						{OD_GADGET,
						 INTEGER_KIND,
						 {18, 4, 6, 1},
						 {5, 14, 8, 6},
						 MSG_SETTINGS_PRI_MAIN,
						 PLACETEXT_LEFT,
						 GAD_SETTINGS_PRI_MAIN,
						 _environment_priority_taglist},

						// Main priority when busy
						{OD_GADGET,
						 INTEGER_KIND,
						 {42, 4, 6, 1},
						 {14, 14, 8, 6},
						 MSG_SETTINGS_PRI_WHEN_BUSY,
						 PLACETEXT_LEFT,
						 GAD_SETTINGS_PRI_MAIN_BUSY,
						 _environment_priority_taglist},

						// Lister priority
						{OD_GADGET,
						 INTEGER_KIND,
						 {18, 5, 6, 1},
						 {5, 22, 8, 6},
						 MSG_SETTINGS_PRI_LISTER,
						 PLACETEXT_LEFT,
						 GAD_SETTINGS_PRI_LISTER,
						 _environment_priority_taglist},

						// Lister priority when busy
						{OD_GADGET,
						 INTEGER_KIND,
						 {42, 5, 6, 1},
						 {14, 22, 8, 6},
						 MSG_SETTINGS_PRI_WHEN_BUSY,
						 PLACETEXT_LEFT,
						 GAD_SETTINGS_PRI_LISTER_BUSY,
						 _environment_priority_taglist},

						{OD_END}},

				_environment_pathlist_gadgets[] =
					{

						// Use path list
						{OD_GADGET,
						 CHECKBOX_KIND,
						 {2, 1, 0, 1},
						 {5, 1, 26, 6},
						 MSG_SETTINGS_USE_PATHLIST,
						 PLACETEXT_RIGHT,
						 GAD_SETTINGS_USE_PATHLIST,
						 _environment_relative_taglist},

						// Reset
						{OD_GADGET,
						 BUTTON_KIND,
						 {POS_RIGHT_JUSTIFY - 2, 1, 8, 1},
						 {-5, 1, 4, 6},
						 MSG_SETTINGS_RESET_PATHLIST,
						 0,
						 GAD_SETTINGS_PATHLIST_RESET,
						 _environment_relative_taglist},

						// Path list list
						{OD_GADGET,
						 MY_LISTVIEW_KIND,
						 {2, 2, SIZE_MAX_LESS - 2, SIZE_MAX_LESS - 2},
						 {5, 9, -5, -8},
						 0,
						 0,
						 GAD_SETTINGS_PATHLIST,
						 _environment_pathlist_taglist},

						// Add
						{OD_GADGET,
						 BUTTON_KIND,
						 {2, POS_RIGHT_JUSTIFY - 1, 8, 1},
						 {5, -2, 4, 6},
						 MSG_SETTINGS_PATHLIST_ADD,
						 0,
						 GAD_SETTINGS_PATHLIST_ADD,
						 _environment_relative_taglist},

						// Delete
						{OD_GADGET,
						 BUTTON_KIND,
						 {10, POS_RIGHT_JUSTIFY - 1, 8, 1},
						 {9, -2, 4, 6},
						 MSG_SETTINGS_PATHLIST_DEL,
						 0,
						 GAD_SETTINGS_PATHLIST_DELETE,
						 _environment_relative_taglist},

						// File popup
						{OD_GADGET,
						 DIR_BUTTON_KIND,
						 {18, POS_RIGHT_JUSTIFY - 1, 0, 1},
						 {13, -2, 28, 6},
						 0,
						 0,
						 GAD_SETTINGS_PATHLIST_POPUP,
						 _environment_pathlist_popup_taglist},

						// Path
						{OD_GADGET,
						 STRING_KIND,
						 {18, POS_RIGHT_JUSTIFY - 1, SIZE_MAX_LESS - 2, 1},
						 {41, -2, -5, 6},
						 0,
						 0,
						 GAD_SETTINGS_PATHLIST_PATH,
						 _environment_pathlist_path_taglist},

						{OD_END}},

				_environment_soundlist_gadgets[] = {

					// Sound list list
					{OD_GADGET,
					 MY_LISTVIEW_KIND,
					 {2, 1, SIZE_MAX_LESS - 2, SIZE_MAX_LESS - 4},
					 {5, 4, -5, -20},
					 0,
					 0,
					 GAD_SETTINGS_SOUNDLIST,
					 _environment_soundlist_taglist},

					// File popup
					{OD_GADGET,
					 FILE_BUTTON_KIND,
					 {2, POS_RIGHT_JUSTIFY - 3, 0, 1},
					 {5, -14, 28, 6},
					 0,
					 0,
					 GAD_SETTINGS_SOUNDLIST_POPUP,
					 _environment_soundlist_popup_taglist},

					// Path
					{OD_GADGET,
					 STRING_KIND,
					 {2, POS_RIGHT_JUSTIFY - 3, SIZE_MAX_LESS - 10, 1},
					 {33, -14, 0, 6},
					 0,
					 0,
					 GAD_SETTINGS_SOUNDLIST_PATH,
					 _environment_soundlist_path_taglist},

					// Clear
					{OD_GADGET,
					 BUTTON_KIND,
					 {POS_RIGHT_JUSTIFY - 3, POS_RIGHT_JUSTIFY - 3, 8, 1},
					 {2, -14, -5, 6},
					 MSG_SETTINGS_RESET_PATHLIST,
					 0,
					 GAD_SETTINGS_SOUNDLIST_RESET,
					 _environment_relative_taglist},

					// Volume
					{OD_GADGET,
					 INTEGER_KIND,
					 {10, POS_RIGHT_JUSTIFY - 2, 6, 1},
					 {33, -6, 0, 6},
					 MSG_SETTINGS_VOLUME,
					 PLACETEXT_LEFT,
					 GAD_SETTINGS_VOLUME,
					 _environment_volume_taglist},

					// Volume slider
					{OD_GADGET,
					 SLIDER_KIND,
					 {16, POS_RIGHT_JUSTIFY - 2, 0, 1},
					 {33, -6, 70, 6},
					 0,
					 0,
					 GAD_SETTINGS_VOLUME_SLIDER,
					 _environment_volslider_taglist},

					// Test
					{OD_GADGET,
					 BUTTON_KIND,
					 {17, POS_RIGHT_JUSTIFY - 2, 8, 1},
					 {105, -6, 0, 6},
					 MSG_SETTINGS_TEST,
					 0,
					 GAD_SETTINGS_TEST_SOUND,
					 _environment_relative_taglist},

					// Exclusive startup sound
					{OD_GADGET,
					 CHECKBOX_KIND,
					 {2, POS_RIGHT_JUSTIFY - 1, 0, 1},
					 {5, 1, 28, 6},
					 MSG_SETTINGS_EXCLUSIVE_SOUND,
					 PLACETEXT_RIGHT,
					 GAD_SETTINGS_EXCLUSIVE_SOUND,
					 _environment_relative_taglist},

					{OD_END}};

// Sub options
const SubOptionHandle _environment_options[] = {
	{ENVIRONMENT_DISPLAY, MSG_ENVIRONMENT_SUB_DISPLAY, _environment_display_gadgets},
	{ENVIRONMENT_PALETTE, MSG_ENVIRONMENT_SUB_PALETTE, _environment_palette_gadgets},
	{ENVIRONMENT_LISTER_DISPLAY, MSG_ENVIRONMENT_SUB_LISTER_DISPLAY, _environment_lister_gadgets},
	{ENVIRONMENT_DISPLAY_OPTIONS, MSG_ENVIRONMENT_SUB_WBEMULATION, _environment_options_gadgets},
	{ENVIRONMENT_OUTPUT_WINDOW, MSG_ENVIRONMENT_SUB_LAUNCHING, _environment_output_window},
	{ENVIRONMENT_LISTER_OPTIONS, MSG_ENVIRONMENT_SUB_LISTER_OPTIONS, _environment_listeroptions},
	{ENVIRONMENT_LISTER_SIZE, MSG_ENVIRONMENT_SUB_LISTER_DEFAULT, _environment_listersize},
	{ENVIRONMENT_DESKTOP, MSG_ENVIRONMENT_SUB_DESKTOP, _environment_desktop},
	{ENVIRONMENT_ICONS, MSG_ENVIRONMENT_SUB_ICONS, _environment_icons},
	{ENVIRONMENT_ICON_LAYOUT, MSG_ENVIRONMENT_SUB_ICON_LAYOUT, _environment_icon_layout},
	{ENVIRONMENT_LISTER_COLOURS, MSG_ENVIRONMENT_SUB_LISTER_COLOURS, _environment_listercolours},
	{ENVIRONMENT_ICON_COLOURS, MSG_ENVIRONMENT_SUB_ICON_COLOURS, _environment_icon_colours},
	{ENVIRONMENT_PICTURES, MSG_ENVIRONMENT_SUB_PICTURES, _environment_pictures},
	{ENVIRONMENT_COPY, MSG_SETTINGS_SUB_COPY, _environment_copy_gadgets},
	{ENVIRONMENT_DELETE, MSG_SETTINGS_SUB_DELETE, _environment_delete_gadgets},
	{ENVIRONMENT_CACHING, MSG_SETTINGS_SUB_DIRECTORIES, _environment_caching_gadgets},
	{ENVIRONMENT_DATE, MSG_SETTINGS_SUB_DATE_FORMAT, _environment_date_gadgets},
	{ENVIRONMENT_HIDE, MSG_SETTINGS_SUB_HIDE, _environment_hide_gadgets},
	{ENVIRONMENT_MISC, MSG_SETTINGS_SUB_MISC, _environment_misc_gadgets},
	{ENVIRONMENT_PRI, MSG_SETTINGS_SUB_PRI, _environment_pri_gadgets},
	{ENVIRONMENT_PATHLIST, MSG_SETTINGS_SUB_PATHLIST, _environment_pathlist_gadgets},
	{ENVIRONMENT_SOUNDLIST, MSG_SETTINGS_SUB_SOUNDLIST, _environment_soundlist_gadgets},
	{-1}};
