#include "config_lib.h"
#include "config_environment.h"

// Icon label draw modes beyond JAM1/JAM2 (positions in the mode list below)
#define ICON_MODE_SHADOW 2
#define ICON_MODE_OUTLINE 3

void _env_select_icon_font(config_env_data *data,
						   char *name,
						   UBYTE *size,
						   UBYTE *fpen,
						   UBYTE *bpen,
						   UBYTE *mode,
						   short bufsize)
{
	short initial_fpen, initial_bpen;
	char *mode_list[6];
	ULONG flags = FOF_PRIVATEIDCMP | FOF_DOFRONTPEN | FOF_DOBACKPEN | FOF_DODRAWMODE;
	ULONG initial_mode = *mode, initial_spec = 0;
	Tag mode_list_tag = ASLFO_ModeList, spec_init_tag = TAG_IGNORE, spec_tag = TAG_IGNORE;

	// No requester?
	if (!data->icon_font_req)
		return;

	// Fill out mode list array
	mode_list[0] = GetString(locale, MSG_FONT_DRAWMODE);
	mode_list[1] = GetString(locale, MSG_FONT_TEXT);
	mode_list[2] = GetString(locale, MSG_FONT_FIELD_TEXT);
	mode_list[3] = GetString(locale, MSG_FONT_SHADOW);
	mode_list[4] = GetString(locale, MSG_FONT_OUTLINE);
	mode_list[5] = 0;

#ifdef FO_SPECIALMODE_OUTLINE
	// asl.library V47+ has its own shadow/outline modes and previews them correctly;
	// our substitute mode list would be previewed as COMPLEMENT draw modes instead
	if (AslBase->lib_Version >= 47)
	{
		flags &= ~FOF_DODRAWMODE;
		mode_list_tag = TAG_IGNORE;
		spec_init_tag = ASLFO_InitialSpecMode;
		spec_tag = ASLFO_DoSpecialMode;

		if (*mode == ICON_MODE_SHADOW)
		{
			initial_mode = JAM1;
			initial_spec = FO_SPECIALMODE_SHADOW;
		}
		else if (*mode == ICON_MODE_OUTLINE)
		{
			initial_mode = JAM1;
			initial_spec = FO_SPECIALMODE_OUTLINE;
		}
	}
#endif

	// Fix initial pens
	initial_fpen = *fpen;
	initial_bpen = *bpen;
	if (initial_fpen >= 4 && initial_fpen < 252 && data->palette_table)
		initial_fpen = data->palette_table[initial_fpen - 4];
	if (initial_bpen >= 4 && initial_bpen < 252 && data->palette_table)
		initial_bpen = data->palette_table[initial_bpen - 4];

	// Make window busy
	SetWindowBusy(data->window);

	// Show font requester
	if (AslRequestTags(data->icon_font_req,
					   ASLFO_Window,
					   data->window,
					   ASLFO_TitleText,
					   GetString(locale, MSG_SELECT_FONT),
					   ASLFO_InitialName,
					   name,
					   ASLFO_InitialSize,
					   (ULONG)*size,
					   ASLFO_InitialFrontPen,
					   initial_fpen,
					   ASLFO_InitialBackPen,
					   initial_bpen,
					   ASLFO_InitialDrawMode,
					   initial_mode,
					   spec_init_tag,
					   initial_spec,
					   ASLFO_Flags,
					   flags,
					   spec_tag,
					   TRUE,
					   ASLFO_MaxFrontPen,
					   data->font_pen_count,
					   ASLFO_MaxBackPen,
					   data->font_pen_count,
					   ASLFO_FrontPens,
					   data->font_pen_table,
					   ASLFO_BackPens,
					   data->font_pen_table,
					   mode_list_tag,
					   mode_list,
					   TAG_END))
	{
		// Store name and size
		stccpy(name, data->icon_font_req->fo_Attr.ta_Name, bufsize);
		*size = data->icon_font_req->fo_Attr.ta_YSize;

		// Store colours
		*fpen = _env_map_font_colour(data, data->icon_font_req->fo_FrontPen);
		*bpen = _env_map_font_colour(data, data->icon_font_req->fo_BackPen);
		*mode = data->icon_font_req->fo_DrawMode;

#ifdef FO_SPECIALMODE_OUTLINE
		// Map the native special modes back to ours
		if (spec_tag != TAG_IGNORE)
		{
			if (data->icon_font_req->fo_SpecialDrawMode == FO_SPECIALMODE_SHADOW)
				*mode = ICON_MODE_SHADOW;
			else if (data->icon_font_req->fo_SpecialDrawMode == FO_SPECIALMODE_OUTLINE)
				*mode = ICON_MODE_OUTLINE;
		}
#endif
	}

	// Make window unbusy
	ClearWindowBusy(data->window);
}
