#include "config_lib.h"

// Palette box window dimensions
ConfigWindow _palette_box_window = {{POS_RIGHT_JUSTIFY, POS_RIGHT_JUSTIFY, 16, 5}, {0, 0, 76, 28}},

			 _palette_box_cust_window = {{POS_RIGHT_JUSTIFY, POS_RIGHT_JUSTIFY, 20, 8}, {0, 0, 74, 66}};

// Tag lists
struct TagItem _palette_box_tags[] = {{GTCustom_LayoutRel, GAD_PALETTE_LAYOUT}, {TAG_END, 0}},

			   _palette_slider_tags[] = {{GA_RelVerify, TRUE},
										 {GA_Immediate, TRUE},
										 {TAG_MORE, (IPTR)_palette_box_tags}};

// Palette box objects
ObjectDef
	_palette_box_objects[] =
		{

			// Layout object
			{OD_AREA,
			 0,
			 {0, 0, SIZE_MAXIMUM, SIZE_MAXIMUM},
			 {4, 4, -4, -4},
			 0,
			 AREAFLAG_RECESSED | AREAFLAG_ERASE,
			 GAD_PALETTE_LAYOUT,
			 0},

			// Foreground
			{OD_GADGET,
			 PALETTE_KIND,
			 {0, 1, 8, SIZE_MAXIMUM},
			 {4, 8, 24, -8},
			 MSG_PALETTE_FOREGROUND,
			 PLACETEXT_ABOVE,
			 GAD_PALETTE_FOREGROUND,
			 _palette_box_tags},

			// Background
			{OD_GADGET,
			 PALETTE_KIND,
			 {8, 1, 8, SIZE_MAXIMUM},
			 {36, 8, 24, -8},
			 MSG_PALETTE_BACKGROUND,
			 PLACETEXT_ABOVE,
			 GAD_PALETTE_BACKGROUND,
			 _palette_box_tags},

			{OD_END}},

	_palette_box_cust_objects[] = {

		// Layout object
		{OD_AREA,
		 0,
		 {0, 0, SIZE_MAXIMUM, SIZE_MAX_LESS - 1},
		 {2, 2, -2, -10},
		 0,
		 AREAFLAG_RECESSED | AREAFLAG_ERASE,
		 GAD_PALETTE_LAYOUT,
		 0},

		// Foreground
		{OD_GADGET,
		 PALETTE_KIND,
		 {0, 1, 10, 5},
		 {4, 4, 24, 8},
		 MSG_PALETTE_FOREGROUND,
		 PLACETEXT_ABOVE,
		 GAD_PALETTE_FOREGROUND,
		 _palette_box_tags},

		// Background
		{OD_GADGET,
		 PALETTE_KIND,
		 {10, 1, 10, 5},
		 {36, 4, 24, 8},
		 MSG_PALETTE_BACKGROUND,
		 PLACETEXT_ABOVE,
		 GAD_PALETTE_BACKGROUND,
		 _palette_box_tags},

		// Custom colour caption foreground
		{OD_TEXT, TEXTPEN, {0, 6, 10, 1}, {4, 14, 24, 0}, MSG_PALETTE_CUSTOM, TEXTFLAG_CENTER, 0, _palette_box_tags},

		// Custom colour caption background
		{OD_TEXT, TEXTPEN, {10, 6, 10, 1}, {36, 14, 24, 0}, MSG_PALETTE_CUSTOM, TEXTFLAG_CENTER, 0, _palette_box_tags},

		// Red slider foreground
		{OD_GADGET,
		 SLIDER_KIND,
		 {2, 7, 8, 0},
		 {4, 16, 24, 10},
		 MSG_PALETTE_RED,
		 PLACETEXT_LEFT,
		 GAD_PALETTE_RED_FORE,
		 _palette_slider_tags},

		// Green slider foreground
		{OD_GADGET,
		 SLIDER_KIND,
		 {2, 7, 8, 0},
		 {4, 27, 24, 10},
		 MSG_PALETTE_GREEN,
		 PLACETEXT_LEFT,
		 GAD_PALETTE_GREEN_FORE,
		 _palette_slider_tags},

		// Blue slider foreground
		{OD_GADGET,
		 SLIDER_KIND,
		 {2, 7, 8, 0},
		 {4, 38, 24, 10},
		 MSG_PALETTE_BLUE,
		 PLACETEXT_LEFT,
		 GAD_PALETTE_BLUE_FORE,
		 _palette_slider_tags},

		// Red slider background
		{OD_GADGET,
		 SLIDER_KIND,
		 {12, 7, 8, 0},
		 {36, 16, 24, 10},
		 MSG_PALETTE_RED,
		 PLACETEXT_LEFT,
		 GAD_PALETTE_RED_BACK,
		 _palette_slider_tags},

		// Green slider background
		{OD_GADGET,
		 SLIDER_KIND,
		 {12, 7, 8, 0},
		 {36, 27, 24, 10},
		 MSG_PALETTE_GREEN,
		 PLACETEXT_LEFT,
		 GAD_PALETTE_GREEN_BACK,
		 _palette_slider_tags},

		// Blue slider background
		{OD_GADGET,
		 SLIDER_KIND,
		 {12, 7, 8, 0},
		 {36, 38, 24, 10},
		 MSG_PALETTE_BLUE,
		 PLACETEXT_LEFT,
		 GAD_PALETTE_BLUE_BACK,
		 _palette_slider_tags},

		// Use
		{OD_GADGET,
		 BUTTON_KIND,
		 {0, POS_RIGHT_JUSTIFY, POS_PROPORTION + 48, 1},
		 {2, -2, 0, 6},
		 MSG_USE,
		 BUTTONFLAG_OKAY_BUTTON,
		 GAD_PALETTE_USE,
		 0},

		// Cancel
		{OD_GADGET,
		 BUTTON_KIND,
		 {POS_RIGHT_JUSTIFY, POS_RIGHT_JUSTIFY, POS_PROPORTION + 48, 1},
		 {-2, -2, 0, 6},
		 MSG_CANCEL,
		 BUTTONFLAG_CANCEL_BUTTON,
		 GAD_PALETTE_CANCEL,
		 0},

		{OD_END}};
