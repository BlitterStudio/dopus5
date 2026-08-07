/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas (dopus5 fork)

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

#include "dopus.h"
#include <proto/input.h>
#include <proto/SysInfo.h>
#include <libraries/SysInfo.h>

#define TITLE_ERROR 1
#define ERROR_TIMEOUT 4
#define TITLE_SIZE 256

#if defined(__amigaos3__) || defined(USE_SCREENTITLE)
#define CLOCK_USE_SCREENTITLE
#endif

void calc_daymonyr(long days, long *day, long *month, long *year);

#define MOON_BIG_SIZE 13
#define MOON_SMALL_SIZE 9

#ifndef CLOCK_USE_SCREENTITLE
struct ClockTitleBarMetrics
{
	short height;
	short text_y;
	short fill_top;
	short fill_bottom;
};

static struct ClockTitleBarMetrics clock_titlebar_metrics = {0, 0, 0, 0};

#ifdef __amigaos3__
#ifndef BARCONTOURPEN
#define BARCONTOURPEN (0x000C)
#endif

static BOOL clock_titlebar_has_contour_pen(struct DrawInfo *drawinfo)
{
	return drawinfo && drawinfo->dri_Pens && drawinfo->dri_NumPens > BARCONTOURPEN &&
		   drawinfo->dri_Pens[BARCONTOURPEN] != drawinfo->dri_Pens[BARBLOCKPEN];
}
#endif

static void clock_titlebar_set_metrics(struct Screen *screen, struct RastPort *rp, struct DrawInfo *drawinfo)
{
	short height = rp->TxHeight + 2;
	short text_y = rp->TxBaseline + 1;
	short fill_top = 0;
	short fill_bottom = height - 1;

#ifdef __amigaos3__
	if (screen && ((struct Library *)IntuitionBase)->lib_Version >= 47 && screen->BarHeight > 0)
	{
		height = screen->BarHeight + 1;
		fill_bottom = height - 2;

		if (height > rp->TxHeight)
		{
			fill_top = (height - rp->TxHeight) >> 1;
			text_y = fill_top + rp->TxBaseline;
			fill_bottom = fill_top + rp->TxHeight - 1;
		}
	}

	if (clock_titlebar_has_contour_pen(drawinfo) && fill_top == 0)
		fill_top = 1;
#endif

	clock_titlebar_metrics.height = height;
	clock_titlebar_metrics.text_y = text_y;
	clock_titlebar_metrics.fill_top = fill_top;
	clock_titlebar_metrics.fill_bottom = fill_bottom;
}

static short clock_titlebar_image_y(short image_height)
{
	if (clock_titlebar_metrics.height > image_height)
		return (clock_titlebar_metrics.height - image_height) >> 1;

	return 0;
}
#endif

#if defined(__amigaos3__) && defined(CLOCK_USE_SCREENTITLE)
static void clock_append_screen_title_clock(struct RastPort *rp, short clock_x, char *titlebuf)
{
	short title_len = strlen(GUI->screen_title);

	if (title_len >= TITLE_SIZE - 1)
		return;

	if (rp && clock_x > 0)
	{
		short title_width = TextLength(rp, GUI->screen_title, title_len);
		short space_width = TextLength(rp, " ", 1);
		short target_x = clock_x - 5;

		if (space_width > 0)
		{
			while (title_len < TITLE_SIZE - 1 && title_width + space_width < target_x)
			{
				GUI->screen_title[title_len++] = ' ';
				GUI->screen_title[title_len] = 0;
				title_width += space_width;
			}
		}
	}

	if (title_len < TITLE_SIZE - 1)
	{
		GUI->screen_title[title_len++] = ' ';
		GUI->screen_title[title_len] = 0;
		stccpy(GUI->screen_title + title_len, titlebuf, TITLE_SIZE - title_len);
	}
}
#endif

struct ClockFormatBuffer
{
	char *buffer;
	short size;
	short pos;
};

static unsigned long clock_format_append(struct Hook *hook, ULONG ch)
{
	struct ClockFormatBuffer *data = (struct ClockFormatBuffer *)hook->h_Data;

	if (data && data->pos < data->size - 1)
	{
		data->buffer[data->pos++] = (char)ch;
		data->buffer[data->pos] = 0;
	}
	return 0;
}

// FormatDate() calls this with A0=hook, A2=locale, A1=character, so the parameters must be
// declared in the standard hook order (hook, object, message). m68k picks the registers up from
// the REG() annotations whatever the position, but everywhere else REG() expands to nothing and
// the declaration order *is* the calling convention. Listing the character second made OS4 and
// MorphOS read the locale pointer as the character, painting the clock as a run of one repeated
// garbage glyph (issue #170).
static unsigned long ASM clock_format_hook(REG(a0, struct Hook *hook), REG(a2, APTR dummy), REG(a1, IPTR ch))
{
	(void)dummy;
	return clock_format_append(hook, (ULONG)ch);
}

static BOOL clock_format_date_raw(char *buffer, short size, char *format, struct DateStamp *stamp)
{
	struct Hook hook;
	struct ClockFormatBuffer data;

	if (!buffer || size < 1)
		return FALSE;
	buffer[0] = 0;

	if (!format || !format[0] || !stamp || !locale.li_LocaleBase)
		return FALSE;

	data.buffer = buffer;
	data.size = size;
	data.pos = 0;

#if defined(__AROS__) || defined(__MORPHOS__)
	hook.h_Entry = (HOOKFUNC)HookEntry;
	hook.h_SubEntry = (HOOKFUNC)clock_format_hook;
#else
	hook.h_Entry = (ULONG(*)())clock_format_hook;
#endif
	hook.h_Data = (APTR)&data;

#define LocaleBase locale.li_LocaleBase
	FormatDate(locale.li_Locale, format, stamp, &hook);
#undef LocaleBase

	return (buffer[0] != 0);
}

#ifdef __AROS__
static void clock_format_append_text(char *buffer, short size, short *pos, char *text)
{
	if (!buffer || !pos || !text)
		return;

	while (*text && *pos < size - 1)
	{
		buffer[(*pos)++] = *text++;
		buffer[*pos] = 0;
	}
}

static void clock_format_append_12hour(char *buffer, short size, short *pos, struct DateStamp *stamp, BOOL leading_zero)
{
	char timebuf[8];
	short hour = (stamp->ds_Minute / 60) % 12;

	if (hour == 0)
		hour = 12;

	if (leading_zero)
		lsprintf(timebuf, "%02ld", (long)hour);
	else
		lsprintf(timebuf, "%ld", (long)hour);

	clock_format_append_text(buffer, size, pos, timebuf);
}

static void clock_format_append_ampm(char *buffer, short size, short *pos, struct DateStamp *stamp)
{
	char *ampm = 0;

#define LocaleBase locale.li_LocaleBase
	ampm = (char *)GetLocaleStr(locale.li_Locale, ((stamp->ds_Minute / 60) > 11) ? PM_STR : AM_STR);
#undef LocaleBase
	if (!ampm)
		ampm = "";

	clock_format_append_text(buffer, size, pos, ampm);
}

// AROS locale.library expands 12-hour tokens as 0-11; patch those and delegate the rest.
static BOOL clock_format_date_aros(char *buffer, short size, char *format, struct DateStamp *stamp)
{
	char *ptr;
	short pos = 0;

	if (!buffer || size < 1)
		return FALSE;
	buffer[0] = 0;

	if (!format || !format[0] || !stamp || !locale.li_LocaleBase)
		return FALSE;

	for (ptr = format; *ptr && pos < size - 1; ptr++)
	{
		char token_format[3];
		char tokenbuf[TITLE_SIZE];

		if (*ptr != '%')
		{
			buffer[pos++] = *ptr;
			buffer[pos] = 0;
			continue;
		}

		if (!*(++ptr))
			break;

		switch (*ptr)
		{
		case '%':
			buffer[pos++] = '%';
			buffer[pos] = 0;
			break;

		case 'I':
			clock_format_append_12hour(buffer, size, &pos, stamp, TRUE);
			break;

		case 'Q':
			clock_format_append_12hour(buffer, size, &pos, stamp, FALSE);
			break;

		case 'r': {
			char timebuf[16];

			clock_format_append_12hour(buffer, size, &pos, stamp, TRUE);
			lsprintf(timebuf, ":%02ld:%02ld ", stamp->ds_Minute % 60, stamp->ds_Tick / TICKS_PER_SECOND);
			clock_format_append_text(buffer, size, &pos, timebuf);
			clock_format_append_ampm(buffer, size, &pos, stamp);
			break;
		}

		default:
			token_format[0] = '%';
			token_format[1] = *ptr;
			token_format[2] = 0;
			if (clock_format_date_raw(tokenbuf, sizeof(tokenbuf), token_format, stamp))
				clock_format_append_text(buffer, size, &pos, tokenbuf);
			break;
		}
	}

	return (buffer[0] != 0);
}
#endif

static BOOL clock_format_date(char *buffer, short size, char *format, struct DateStamp *stamp)
{
#ifdef __AROS__
	return clock_format_date_aros(buffer, size, format, stamp);
#else
	return clock_format_date_raw(buffer, size, format, stamp);
#endif
}

static void clock_build_manual_time(struct DateStamp *stamp, char *timebuf, BOOL show_seconds)
{
	short hours = stamp->ds_Minute / 60;
	short minutes = stamp->ds_Minute % 60;
	short seconds = stamp->ds_Tick / TICKS_PER_SECOND;

	if (environment->env->settings.date_flags & DATE_12HOUR)
	{
		char *ampm = 0;
		char *ampm_sep;

		if (locale.li_LocaleBase)
		{
#define LocaleBase locale.li_LocaleBase
			ampm = (char *)GetLocaleStr(locale.li_Locale, (hours > 11) ? PM_STR : AM_STR);
#undef LocaleBase
		}
		if (hours > 11)
			hours -= 12;
		if (hours == 0)
			hours = 12;
		if (!ampm)
			ampm = "";
		ampm_sep = (ampm[0]) ? " " : "";

		if (show_seconds)
			lsprintf(timebuf, "%2ld:%02ld:%02ld%s%s", hours, minutes, seconds, ampm_sep, ampm);
		else
			lsprintf(timebuf, "%2ld:%02ld%s%s", hours, minutes, ampm_sep, ampm);
	}
	else if (show_seconds)
		lsprintf(timebuf, "%02ld:%02ld:%02ld", hours, minutes, seconds);
	else
		lsprintf(timebuf, "%02ld:%02ld", hours, minutes);
}

static void clock_build_title_text(struct DateTime *date, char *datebuf, char *titlebuf, BOOL show_seconds)
{
	char timebuf[80];
	char *format = environment->env->clock_format;

	if (format[0] && clock_format_date(titlebuf, TITLE_SIZE, format, &date->dat_Stamp))
		return;

	format = 0;
	if (environment->env->settings.date_flags & DATE_12HOUR)
		format = (show_seconds) ? "%Q:%M:%S %p" : "%Q:%M %p";
	else
		format = (show_seconds) ? "%H:%M:%S" : "%H:%M";

	if (!clock_format_date(timebuf, sizeof(timebuf), format, &date->dat_Stamp))
		clock_build_manual_time(&date->dat_Stamp, timebuf, show_seconds);

	lsprintf(titlebuf, "%s  %s  ", datebuf, timebuf);
}

static BOOL clock_title_format_uses_clock(char *format)
{
	char *ptr;

	for (ptr = format; ptr && *ptr; ptr++)
	{
		if (*ptr != '%')
			continue;

		if (*(ptr + 1) == '%')
		{
			++ptr;
			continue;
		}

		if (*(ptr + 1) == 't' && *(ptr + 2) != 'm' && *(ptr + 2) != 'c' && *(ptr + 2) != 'f' &&
			*(ptr + 2) != 'a')
			return TRUE;
	}
	return FALSE;
}

static long clock_memory_message(struct RastPort *rp, short clock_x)
{
	short width;
	char *ptr;

	if (!rp)
		return MSG_MEMORY_COUNTER;

	ptr = GetString(&locale, MSG_MEMORY_COUNTER);
	width = TextLength(rp, ptr, (WORD)strlen(ptr));
	width += TextLength(rp, "999999999999", 12);
	width += TextLength(rp, dopus_name, strlen(dopus_name));

	return (5 + width >= clock_x) ? MSG_MEMORY_COUNTER_CLOCK : MSG_MEMORY_COUNTER;
}

static IPTR clock_other_free_memory(IPTR graphics_mem)
{
	IPTR any_mem;
#ifdef __AROS__
	IPTR fast_mem;

	fast_mem = AvailMem(MEMF_FAST);
	if (fast_mem > 0)
		return fast_mem;
#endif

	any_mem = AvailMem(MEMF_ANY);
	if (any_mem > graphics_mem)
		return any_mem - graphics_mem;

	return AvailMem(MEMF_FAST);
}

static UQUAD clock_used_memory(UQUAD total_mem, UQUAD free_mem)
{
	return (total_mem > free_mem) ? total_mem - free_mem : 0;
}

static void clock_memory_value_string(char *buf, int buf_size, IPTR memory, char sep)
{
	UQUAD value = memory;

	ItoaU64(&value, buf, buf_size, sep);
}

static void clock_memory_format_string(char *buf, int buf_size, char *format)
{
	char *out = buf;
	char *end;

	if (buf_size < 1)
		return;

	end = buf + buf_size - 1;

	while (*format && out < end)
	{
		if (*format == '%' && *(format + 1) == '%' && out + 1 < end)
		{
			*out++ = *format++;
			*out++ = *format++;
		}
		else if (*format == '%' && *(format + 1) == 'l' &&
				 (*(format + 2) == 'd' || *(format + 2) == 'u' || *(format + 2) == 'U') && out + 1 < end)
		{
			*out++ = '%';
			*out++ = 's';
			format += 3;
		}
		else
			*out++ = *format++;
	}

	*out = 0;
}

static void clock_memory_percent_string(char *buf, int buf_size, UQUAD memval, UQUAD memtotal)
{
	UQUAD percent;

	if (memtotal == 0)
	{
		strcpy(buf, "0");
		return;
	}

	percent = (memval / memtotal) * 100;
	percent += ((memval % memtotal) * 100) / memtotal;
	ItoaU64(&percent, buf, buf_size, 0);
}

#ifdef __amigaos4__
BOOL clock_show_custom_title(struct RastPort *rp,
							 long clock_x,
							 long days,
							 char *clock_text,
							 struct SysInfo *si,
							 struct Library *SysInfoBase,
							 struct SysInfoIFace *ISysInfo);
#else
BOOL clock_show_custom_title(struct RastPort *rp,
							 long clock_x,
							 long days,
							 char *clock_text,
							 struct SysInfo *si,
							 struct Library *SysInfoBase);
#endif

// Clock task
IPC_EntryCode(clock_proc)
{
	IPCData *ipc;
	struct Window *window = 0;
	struct AppWindow *appwindow = 0;
	struct Layer *layer = 0;
	TimerHandle *timer;
	char datebuf[LEN_DATSTRING], titlebuf[TITLE_SIZE];
	struct DateTime date;
	BOOL hide_clock;
	short clock_x = 0;
	struct RastPort clock_rp = {0};
	struct Screen *pubscreen = NULL;
	long mem_msg = 0, error_count = 0, days;
	char *error_txt = 0;
	short bar_x = 0, last_x = 0, clock_on = 0, error_time = 0;
	struct Library *SysInfoBase;
#ifdef __amigaos4__
	struct SysInfoIFace *ISysInfo;
#endif
	struct SysInfo *si = 0;
	BOOL titlebar = FALSE;
#if defined(__amigaos3__) && defined(CLOCK_USE_SCREENTITLE)
	char last_screen_title[TITLE_SIZE] = {0};
	struct Window *last_title_window = 0;
#endif

	// Do startup
	if ((ipc = IPC_ProcStartup(0, 0)))
	{
		// Try to open SysInfo library
#ifdef __amigaos4__
		if (OpenLibIFace(SYSINFONAME, (APTR)&SysInfoBase, (APTR)&ISysInfo, SYSINFOVERSION))
#else
		if ((SysInfoBase = OpenLibrary(SYSINFONAME, SYSINFOVERSION)))
#endif
		{
			si = InitSysInfo();
		}

		// Open timer
		if ((timer = AllocTimer(UNIT_VBLANK, 0)))
		{
			// Send first timer request
			StartTimer(timer, 0, 0);

			// Event loop
			FOREVER
			{
				IPCMessage *imsg;
				struct IntuiMessage *msg;
				BOOL update = 0, quit = 0;

				// IPC messages?
				while ((imsg = (IPCMessage *)GetMsg(ipc->command_port)))
				{
					// Look at message
					switch (imsg->command)
					{
					// Quit
					case IPC_QUIT:
						quit = 1;
						break;

					// Show
					case IPC_SHOW:
						// D(bug("IPC_SHOW received\n"));

						// Not already open?
						if (!window && !titlebar)
						{
							short len;
							BOOL show_seconds = TRUE;
							struct Screen *screen;

							// Get screen pointer
							screen = (struct Screen *)imsg->data;
							hide_clock = imsg->flags;

							// No screen?
							if (!screen)
							{
								// Lock public screen
								if (!(pubscreen = LockPubScreen(0)))
								{
									quit = 1;
									break;
								}
								screen = pubscreen;
							}

							// Otherwise, lock screen if using title bar
							else if (!hide_clock)
							{
#if defined(__amigaos3__) && defined(CLOCK_USE_SCREENTITLE)
								// Screen title mode does not touch the screen layers directly.
#else
								// Try to lock screen
								if (FindPubScreen(screen, TRUE))
									pubscreen = screen;

								// Failed
								else
								{
									quit = 1;
									break;
								}
#endif
							}

							// Calculate length of window we need
							len = TextLength(&screen->RastPort, "88:88:88pm  88-WWW-88 ", 21);
#if defined(__amigaos3__) && defined(CLOCK_USE_SCREENTITLE)
							if (!hide_clock)
								show_seconds = FALSE;
#endif
							if (environment->env->clock_format[0])
							{
								struct DateTime now;
								char now_date[LEN_DATSTRING], now_title[TITLE_SIZE];

								DateStamp(&now.dat_Stamp);
								now.dat_Format = environment->env->settings.date_format;
								now.dat_Flags = 0;
								now.dat_StrDay = 0;
								now.dat_StrDate = now_date;
								now.dat_StrTime = 0;
								DateToStr(&now);
								clock_build_title_text(&now, now_date, now_title, show_seconds);
								len = TextLength(&screen->RastPort, now_title, (WORD)strlen(now_title));
							}

							// Displaying in title bar?
							if (!hide_clock)
							{
#if defined(__amigaos3__) && defined(CLOCK_USE_SCREENTITLE)
								struct Gadget *gadget;
								// Get position to render clock
								bar_x = screen->Width - 16;
								if ((gadget = FindGadgetType(screen->FirstGadget, GTYP_SDEPTH)))
									bar_x -= gadget->Width;
								clock_x = bar_x - len;

								titlebar = TRUE;
								last_screen_title[0] = 0;
								last_title_window = 0;

								mem_msg = clock_memory_message(&screen->RastPort, clock_x);
#else
								struct DrawInfo *drawinfo;
								struct Gadget *gadget;

								// Copy rastport
								layer = screen->BarLayer;
								clock_rp = *layer->rp;

								// Get draw info
								drawinfo = GetScreenDrawInfo(screen);

								// Set pens
								SetAPen(&clock_rp,
										drawinfo->dri_Pens[(drawinfo->dri_Version >= 2) ? BARDETAILPEN : DETAILPEN]);
								SetBPen(&clock_rp,
										drawinfo->dri_Pens[(drawinfo->dri_Version >= 2) ? BARBLOCKPEN : BLOCKPEN]);
								SetDrMd(&clock_rp, JAM2);

#ifndef CLOCK_USE_SCREENTITLE
								clock_titlebar_set_metrics(screen, &clock_rp, drawinfo);
#endif

								// Free draw info
								FreeScreenDrawInfo(screen, drawinfo);

								// Get position to render clock
								bar_x = screen->Width - 16;
								if ((gadget = FindGadgetType(screen->FirstGadget, GTYP_SDEPTH)))
									bar_x -= gadget->Width;
								clock_x = bar_x - len;

								mem_msg = clock_memory_message(&screen->RastPort, clock_x);

								titlebar = TRUE;
#endif
							}

							// Otherwise, open window
							else if (!(window =
										   OpenWindowTags(0,
														  WA_Left,
														  environment->env->clock_left,
														  WA_Top,
														  environment->env->clock_top,
														  WA_Width,
														  len + 50,
														  WA_Height,
														  screen->WBorTop + screen->RastPort.Font->tf_YSize + 1,
														  WA_IDCMP,
														  IDCMP_CLOSEWINDOW | IDCMP_CHANGEWINDOW | IDCMP_MOUSEBUTTONS,
														  WA_DragBar,
														  TRUE,
														  WA_DepthGadget,
														  TRUE,
														  WA_CloseGadget,
														  TRUE,
														  WA_SimpleRefresh,
														  TRUE,
														  WA_NoCareRefresh,
														  TRUE,
														  WA_NewLookMenus,
														  TRUE,
														  WA_RMBTrap,
														  TRUE,
														  WA_PubScreen,
														  screen,
														  WA_PubScreenFallBack,
														  TRUE,
														  WA_WindowName,
														  DOPUS_WIN_NAME,
														  TAG_END)))
								quit = 1;

							// Successful?
							if (!quit)
							{
								// Set flag to update window
								update = 1;
							}

							// Did we lock a screen?
							if (pubscreen && !layer)
							{
								UnlockPubScreen(0, pubscreen);
								pubscreen = 0;
							}

							// Got a window?
							if (window)
							{
								// Make an AppWindow
								appwindow = AddAppWindowA(0x12345678, 0, window, GUI->appmsg_port, 0);
							}
						}
						// D(bug("IPC_SHOW done\n"));
						break;

					// Hide
					case IPC_HIDE:
						// D(bug("IPC_HIDE received\n"));

						// Close window if it's open
						if (window)
						{
							// Remove AppWindow
							if (appwindow)
							{
								RemoveAppWindow(appwindow);
								appwindow = 0;
							}

							// Close window
							CloseWindow(window);
							window = 0;
						}

						// Unlock public screen if it's locked
						if (pubscreen)
						{
							UnlockPubScreen(0, pubscreen);
							pubscreen = 0;
						}
						layer = 0;
						titlebar = FALSE;
#if defined(__amigaos3__) && defined(CLOCK_USE_SCREENTITLE)
						last_screen_title[0] = 0;
						last_title_window = 0;
#endif
						// D(bug("IPC_HIDE done\n"));

						break;

					// Error message
					case TITLE_ERROR:

						// Existing error text?
						if (error_txt)
							FreeVec(error_txt);

						// Get error text and timeout, flag for update
						error_txt = imsg->data_free;
						error_time = imsg->flags;
						imsg->data_free = 0;
						error_count = 0;
						update = 1;

						// Default time
						if (error_time == 0)
							error_time = ERROR_TIMEOUT;
						break;
					}

					// Reply the message
					IPC_Reply(imsg);
				}

				// Intuition messages
				if (window)
				{
					while ((msg = (struct IntuiMessage *)GetMsg(window->UserPort)))
					{
						struct IntuiMessage copy_msg;

						// Copy message and reply
						copy_msg = *msg;
						ReplyMsg((struct Message *)msg);

						// Close window?
						if (copy_msg.Class == IDCMP_CLOSEWINDOW)
						{
							if (!(misc_startup("dopus_quit", MENU_QUIT, window, 0, 1)))
								quit = 1;
						}

						// Change window event
						else if (copy_msg.Class == IDCMP_CHANGEWINDOW)
						{
							// Store new position
							environment->env->clock_left = window->LeftEdge;
							environment->env->clock_top = window->TopEdge;
						}

						// Right button press
						else if (copy_msg.Class == IDCMP_MOUSEBUTTONS && copy_msg.Code == MENUDOWN)
						{
							// Signal main program
							IPC_Command(&main_ipc, IPC_SHOW, 0, 0, 0, 0);
						}
					}
				}

				// Check quit flag
				if (quit)
					break;

				// Timer event
				if (CheckTimer(timer))
				{
					// Send next timer request
					StartTimer(timer, 0, 800000);

					// Set flag to update window
					update = 1;
				}

				//// Update clock?
				if (update)
				{
					// D(bug("Updating title\n"));
					// If error text, update error count
					if (error_txt)
					{
						++error_count;

						// Clear text when count reaches limit
						if (error_count > error_time)
						{
							// Free text
							FreeVec(error_txt);
							error_txt = 0;
#if defined(__amigaos3__) && defined(CLOCK_USE_SCREENTITLE)
							last_screen_title[0] = 0;
							last_title_window = 0;
#endif
						}
					}

					// Valid window?
					if (window || titlebar)
					{
						BOOL show_seconds = TRUE;

						// Get current datestamp
						DateStamp(&date.dat_Stamp);

						// Convert to string
						date.dat_Format = environment->env->settings.date_format;
						date.dat_Flags = 0;
						date.dat_StrDay = 0;
						date.dat_StrDate = datebuf;
						date.dat_StrTime = 0;

						// Convert date to string
						DateToStr(&date);

						// Save days value
						days = date.dat_Stamp.ds_Days;

#if defined(__amigaos3__) && defined(CLOCK_USE_SCREENTITLE)
						if (titlebar)
							show_seconds = FALSE;
#endif
						clock_build_title_text(&date, datebuf, titlebuf, show_seconds);

#if defined(__amigaos3__) && defined(CLOCK_USE_SCREENTITLE)
						if (titlebar && GUI->screen_pointer)
						{
							short clock_width =
								TextLength(&GUI->screen_pointer->RastPort, titlebuf, (WORD)strlen(titlebuf));

							clock_x = (clock_width < bar_x) ? bar_x - clock_width : 0;
						}
#endif

						// Update window?
						if (window)
						{
							// Show title
							SetWindowTitles(window, titlebuf, (char *)-1);
						}

						// Or render to screen bar?
						else if (titlebar)
						{
							ULONG lock;
							BOOL ok = 0;
							struct Window *active_window = 0;
							struct Window *tit_window = 0;

							// Lock Intuition
							lock = LockIBase(0);

							// Check that the active window is an Opus window
							active_window = IntuitionBase->ActiveWindow;
							if (active_window && GetWindowID(active_window) != WINDOW_UNKNOWN)
							{
								// Set ok flag
								ok = 1;

								// Rendering to screen title bar, in Workbench compatible way?
								if (environment->env->display_options & DISPOPTF_WB_TITLE)
								{
									// Save window pointer
									tit_window = active_window;
								}
							}

							// Unlock Intuition
							UnlockIBase(lock);

							// Check that the right button isn't down
							if (ok && InputBase && PeekQualifier() & IEQUALIFIER_RBUTTON)
								ok = 0;

							// Ok to render?
							if (ok)
							{
								static short done_tit = 0;

								// Got window pointer?
								if (tit_window)
								{
									// Not done title?
									if (done_tit++ >= 10)
									{
										// Call SetWindowTitles which will be patched
										SetWindowTitles(tit_window, (char *)-1, "Amiga Workbench");
										done_tit = 0;
									}
								}

								// Rendering as usual
								else
								{
									BOOL custom_title_uses_clock = FALSE;

									// Reset title flag
									done_tit = 0;
									if (environment->env->scr_title_text[0] && !error_txt)
										custom_title_uses_clock = clock_title_format_uses_clock(environment->env->scr_title_text);

									// Is clock on?
									if ((GUI->flags & GUIF_CLOCK) && !custom_title_uses_clock)
									{
#ifndef CLOCK_USE_SCREENTITLE
										Move(&clock_rp, clock_x, clock_titlebar_metrics.text_y);
										Text(&clock_rp, titlebuf, strlen(titlebuf));
#endif

										clock_on = 1;
									}

									// Was it on last time?
									else if (clock_on)
									{
										// Clear flag, erase to end of display
										clock_on = 0;
										last_x = bar_x;
									}

									mem_msg = clock_memory_message(
										(GUI->screen_pointer) ? &GUI->screen_pointer->RastPort : &clock_rp,
										(clock_on) ? clock_x : last_x);

									// Custom title?
									if (environment->env->scr_title_text[0] && !error_txt)
									{
										// Show custom title
#ifdef __amigaos4__
										clock_show_custom_title(&clock_rp,
																(clock_on) ? clock_x : last_x,
																days,
																(custom_title_uses_clock) ? titlebuf : 0,
																si,
																SysInfoBase,
																ISysInfo);
#else
										clock_show_custom_title(&clock_rp,
																(clock_on) ? clock_x : last_x,
																days,
																(custom_title_uses_clock) ? titlebuf : 0,
																si,
																SysInfoBase);
#endif
									}

									// Normal title with memory monitor
									else
									{
										// Show free memory count
										clock_show_memory(&clock_rp,
														  mem_msg,
														  (clock_on) ? clock_x : last_x,
														  error_txt);
									}

#ifdef CLOCK_USE_SCREENTITLE
									if (clock_on)
									{
#if defined(__amigaos3__)
										clock_append_screen_title_clock(
											(GUI->screen_pointer) ? &GUI->screen_pointer->RastPort : 0, clock_x, titlebuf);
#else
										strcat(GUI->screen_title, " - ");
										strcat(GUI->screen_title, titlebuf);
#endif
									}
#if defined(__amigaos3__)
									if (strcmp(last_screen_title, GUI->screen_title) != 0 || last_title_window != active_window)
									{
										SetWindowTitles(active_window, (char *)-1, (char *)GUI->screen_title);
										stccpy(last_screen_title, GUI->screen_title, TITLE_SIZE);
										last_title_window = active_window;
									}
#else
									SetWindowTitles(IntuitionBase->ActiveWindow, (char *)-1, (char *)GUI->screen_title);
#endif
#endif

									// Remember position
									last_x = clock_rp.cp_x;
								}
							}
						}
					}
				}

				// Wait for an event
				Wait(1 << ipc->command_port->mp_SigBit | 1 << timer->port->mp_SigBit |
					 ((window) ? (1 << window->UserPort->mp_SigBit) : 0));
			}

			// Close window if it's open
			if (window)
			{
				// Remove AppWindow
				if (appwindow)
					RemoveAppWindow(appwindow);

				// Close window
				CloseWindow(window);

				// Signal main process
				IPC_Command(&main_ipc, MAINCMD_OPEN_DISPLAY, 0, 0, 0, 0);
			}

			// Unlock screen if it's locked
			if (pubscreen)
			{
				UnlockPubScreen(0, pubscreen);
			}

			// Free timer
			FreeTimer(timer);
		}

		// Free SysInfo
		if (si)
			FreeSysInfo(si);
		if (SysInfoBase)
		{
#ifdef __amigaos4__
			DropInterface((struct Interface *)ISysInfo);
#endif
			CloseLibrary(SysInfoBase);
		}
	}

	// Outstanding error text?
	if (error_txt)
		FreeVec(error_txt);

	// Send goodbye message
	IPC_Goodbye(ipc, &main_ipc, 0);

	// Exit
	IPC_Free(ipc);
}

// Show free memory
void clock_show_memory(struct RastPort *rp, long msg, long clock_x, char *error)
{
#ifndef __amigaos4__
	IPTR graphics_mem;
	char graphics_buf[32], other_buf[32];
	char format[TITLE_SIZE];
	char sep = 0;
#endif

	// Error text?
	if (error)
		strcpy(GUI->screen_title, error);

	// Update memory string
	else
	{
#ifdef __amigaos4__
		// On OS4, chip/fast split is meaningless — show total free as KB/MB/GB
		unsigned long freemem = AvailMem(MEMF_ANY);
		char membuf[20];
		char sep = (environment->env->settings.date_flags & DATE_1000SEP) ? GUI->decimal_sep : 0;

		BytesToString(freemem, membuf, 1, sep);
		lsprintf(GUI->screen_title, GetString(&locale, MSG_MEMORY_FREE_OS4), dopus_name, membuf);
#else
		// Thousands separator?
		if (environment->env->settings.date_flags & DATE_1000SEP && GUI->flags & GUIF_LOCALE_OK)
		{
			++msg;
			sep = GUI->decimal_sep;
		}

		// Get graphics/chip memory and build title string
		graphics_mem = AvailMem(MEMF_CHIP);
		clock_memory_value_string(graphics_buf, sizeof(graphics_buf), graphics_mem, sep);
		clock_memory_value_string(other_buf, sizeof(other_buf), clock_other_free_memory(graphics_mem), sep);
		clock_memory_format_string(format, sizeof(format), GetString(&locale, msg));
		lsprintf(GUI->screen_title,
				 format,
				 dopus_name,
				 graphics_buf,
				 other_buf);
#endif
	}

#ifndef CLOCK_USE_SCREENTITLE
	// Render text
	Move(rp, 5, clock_titlebar_metrics.text_y);
	Text(rp, GUI->screen_title, strlen(GUI->screen_title));

	// Erase to start of clock text
	if (clock_x > rp->cp_x)
	{
		unsigned char fp;

		// Save front pen
		fp = rp->FgPen;
		SetAPen(rp, rp->BgPen);
		RectFill(rp, rp->cp_x, clock_titlebar_metrics.fill_top, clock_x - 1, clock_titlebar_metrics.fill_bottom);
		SetAPen(rp, fp);
	}
#endif
}

// Title-bar error message
void title_error(char *txt, short time)
{
	char *copy = 0;
	IPCData *ipc;

	// Lock process list
	GetSemaphore(&GUI->process_list.lock, SEMF_SHARED, 0);

	// Find clock process
	if ((ipc = IPC_FindProc(&GUI->process_list, NAME_CLOCK, 0, 0)))
	{
		// Allocate copy of text
		if (!txt || (copy = AllocVec(strlen(txt) + 1, 0)))
		{
			// Copy text
			if (copy)
				strcpy(copy, txt);

			// Send command
			IPC_Command(ipc, TITLE_ERROR, time, 0, copy, 0);
		}
	}

	// Unlock process list
	FreeSemaphore(&GUI->process_list.lock);
}

// Show custom title
#ifdef __amigaos4__
BOOL clock_show_custom_title(struct RastPort *rp,
							 long clock_x,
							 long days,
							 char *clock_text,
							 struct SysInfo *si,
							 struct Library *SysInfoBase,
							 struct SysInfoIFace *ISysInfo)
#else
BOOL clock_show_custom_title(struct RastPort *rp,
							 long clock_x,
							 long days,
							 char *clock_text,
							 struct SysInfo *si,
							 struct Library *SysInfoBase)
#endif
{
	char *ptr, *title_buffer;
	short pos = 0;
	BOOL used_clock = FALSE;
#ifndef CLOCK_USE_SCREENTITLE
	short moon_day = -1, moon_pos = 0;
	struct BitMap bm;
#endif

	title_buffer = GUI->screen_title;

	// Go through title text
	for (ptr = environment->env->scr_title_text; *ptr && pos < TITLE_SIZE - 1; ptr++)
	{
		short esc = 0;
		UQUAD memval = 0, memtotal = 0;
		BOOL got_memval = FALSE, got_memtotal = FALSE;
		char buf[TITLE_SIZE];

		// Clear buffer
		buf[0] = 0;

		// Escape command?
		if (*ptr == '%')
		{
			// Literal percent character?
			if (*(ptr + 1) == '%')
			{
				title_buffer[pos++] = '%';
				esc = 1;
			}

			// Directory Opus version
			else if (*(ptr + 1) == 'd' && *(ptr + 2) == 'v')
			{
				extern short version_num;
				lsprintf(buf, "5.%ld", version_num);
				esc = 2;
			}

			// Kickstart version
			else if (*(ptr + 1) == 'k' && *(ptr + 2) == 'v')
			{
				strcpy(buf, GUI->ver_kickstart);
				esc = 2;
			}

			// Workbench version
			else if (*(ptr + 1) == 'w' && *(ptr + 2) == 'v')
			{
				strcpy(buf, GUI->ver_workbench);
				esc = 2;
			}

			// CPU type
			else if (*(ptr + 1) == 'p' && *(ptr + 2) == 'r')
			{
				strcpy(buf, GUI->ver_cpu);
				esc = 2;
			}

			// Coprocessor type
			else if (*(ptr + 1) == 'c' && *(ptr + 2) == 'p')
			{
				strcpy(buf, GUI->ver_fpu);
				esc = 2;
			}

			// Chip set
			else if (*(ptr + 1) == 'c' && *(ptr + 2) == 's')
			{
				strcpy(buf, GUI->ver_chips);
				esc = 2;
			}

			// Clock text
			else if (*(ptr + 1) == 't' && *(ptr + 2) != 'm' && *(ptr + 2) != 'c' && *(ptr + 2) != 'f' &&
					 *(ptr + 2) != 'a')
			{
				if (clock_text)
				{
					stccpy(buf, clock_text, sizeof(buf));
					used_clock = TRUE;
				}
				esc = 1;
			}

			// Total memory
			else if (*(ptr + 1) == 't' && *(ptr + 2) == 'm')
			{
				memval = AvailMem(MEMF_TOTAL | MEMF_ANY);
				got_memval = TRUE;
				esc = 2;
			}

			// Total chip memory
			else if (*(ptr + 1) == 't' && *(ptr + 2) == 'c')
			{
				memval = AvailMem(MEMF_TOTAL | MEMF_CHIP);
				got_memval = TRUE;
				esc = 2;
			}

			// Total fast memory
			else if (*(ptr + 1) == 't' && *(ptr + 2) == 'f')
			{
				memval = AvailMem(MEMF_TOTAL | MEMF_FAST);
				got_memval = TRUE;
				esc = 2;
			}

			// Free memory
			else if (*(ptr + 1) == 'f' && *(ptr + 2) == 'm')
			{
				memval = AvailMem(MEMF_ANY);
				if (*(ptr + 3) == '%')
				{
					memtotal = AvailMem(MEMF_ANY | MEMF_TOTAL);
					got_memtotal = TRUE;
				}
				got_memval = TRUE;
				esc = 2;
			}

			// Free chip memory
			else if (*(ptr + 1) == 'f' && *(ptr + 2) == 'c')
			{
				memval = AvailMem(MEMF_CHIP);
				if (*(ptr + 3) == '%')
				{
					memtotal = AvailMem(MEMF_CHIP | MEMF_TOTAL);
					got_memtotal = TRUE;
				}
				got_memval = TRUE;
				esc = 2;
			}

			// Free fast memory
			else if (*(ptr + 1) == 'f' && *(ptr + 2) == 'f')
			{
				memval = AvailMem(MEMF_FAST);
				if (*(ptr + 3) == '%')
				{
					memtotal = AvailMem(MEMF_FAST | MEMF_TOTAL);
					got_memtotal = TRUE;
				}
				got_memval = TRUE;
				esc = 2;
			}

			// Used memory
			else if (*(ptr + 1) == 'u' && *(ptr + 2) == 'm')
			{
				memtotal = AvailMem(MEMF_TOTAL | MEMF_ANY);
				memval = clock_used_memory(memtotal, AvailMem(MEMF_ANY));
				got_memval = TRUE;
				got_memtotal = TRUE;
				esc = 2;
			}

			// Used chip memory
			else if (*(ptr + 1) == 'u' && *(ptr + 2) == 'c')
			{
				memtotal = AvailMem(MEMF_TOTAL | MEMF_CHIP);
				memval = clock_used_memory(memtotal, AvailMem(MEMF_CHIP));
				got_memval = TRUE;
				got_memtotal = TRUE;
				esc = 2;
			}

			// Used fast memory
			else if (*(ptr + 1) == 'u' && *(ptr + 2) == 'f')
			{
				memtotal = AvailMem(MEMF_TOTAL | MEMF_FAST);
				memval = clock_used_memory(memtotal, AvailMem(MEMF_FAST));
				got_memval = TRUE;
				got_memtotal = TRUE;
				esc = 2;
			}

			// Task count
			else if (*(ptr + 1) == 't' && *(ptr + 2) == 'a')
			{
				lsprintf(buf, "%ld", GetStatistics(STATID_TASKCOUNT));
				esc = 2;
			}

			// Processor usage
			else if (*(ptr + 1) == 'p' && *(ptr + 2) == 'u')
			{
				if (si && si->cpu_usage_implemented & CPU_USAGEF_LASTSEC_IMPLEMENTED)
				{
					struct SI_CpuUsage cpu;
					GetCpuUsage(si, &cpu);
					lsprintf(
						buf, "%ld", UDivMod32(UMult32(100, cpu.used_cputime_lastsec), cpu.used_cputime_lastsec_hz));
				}
				else
					lsprintf(buf, "%ld", GetStatistics(STATID_CPU_USAGE) / 10);
				esc = 2;
			}

			// Screen count
			else if (*(ptr + 1) == 's' && *(ptr + 2) == 'c')
			{
				ULONG lock;
				short count;
				struct Screen *screen;

				// Lock Intuition and count screens
				lock = LockIBase(0);
				for (count = 0, screen = IntuitionBase->FirstScreen; screen; count++, screen = screen->NextScreen)
					;
				UnlockIBase(lock);
				lsprintf(buf, "%ld", count);
				esc = 2;
			}

			// Free pen count
			else if (*(ptr + 1) == 'f' && *(ptr + 2) == 'p')
			{
				struct ColorMap *cm;
				if (GUI->screen_pointer && (cm = GUI->screen_pointer->ViewPort.ColorMap) &&
					cm->Type >= COLORMAP_TYPE_V39 && cm->PalExtra)
					lsprintf(buf, "%ld", cm->PalExtra->pe_NFree);
				else
					strcpy(buf, "n/a");
				esc = 2;
			}

			// Phase of the moon
			else if (*(ptr + 1) == 'p' && *(ptr + 2) == 'm')
			{
				long day, month, year, sub = 4;

				// Calculate day, month, year from number of days
				calc_daymonyr(days, &day, &month, &year);
				if (year > 100)
				{
					year -= 100;
					sub = 8;
				}

				// Calculate phase of the moon
				year -= (year / 19) * 19;  // Take modulo 19 of year
				if (year > 9)
					year -= 19;	 // If year>9, subtract 19 to give -9>=y>=9
				year *= 11;		 // Multiply by 11
				if (year < 0)
					year = -((-year) - (((-year) / 30) * 30));
				else  // Reduce modulo 30 to give -29>=y>=29
					year -= (year / 30) * 30;
				year += (month < 3) ? month + 2 : month;  // Add month number (except jan/feb = 3 and 4)
				year += day;							  // Add day of month
				year -= sub;							  // Subtract 4 (or 8 for 2000+)
				year -= (year / 30) * 30;				  // Reduce modulo 30 to give 0>=y>=29

				// If font is big enough, we have a graphical moon
#ifndef CLOCK_USE_SCREENTITLE
				if (rp->TxHeight >= 8)
				{
					// Save moon day and position
					moon_day = year;
					moon_pos = pos;
					strcpy(buf, "  ");
				}
				else
#endif
					lsprintf(buf, "%ld", year);
				esc = 2;
			}

			// Environment variable
			else if (*(ptr + 1) == 'e' && *(ptr + 2) == 'v')
			{
				char name[32], *p;

				// Get variable name
				stccpy(name, ptr + 3, 30);
				if ((p = strchr(name, '%')))
					*p = 0;

				// Get variable
				GetVar(name, buf, sizeof(buf) - 1, GVF_GLOBAL_ONLY);
				if ((p = strchr(buf, '\n')))
					*p = 0;

				// Get length to skip
				if ((p = strchr(ptr + 2, '%')))
					esc = p - ptr;
			}

			// Memory value?
			if (got_memval)
			{
				// As kilobytes/megabytes/smart/bytes
				if (*(ptr + 3) == 'K' || *(ptr + 3) == 'k')
				{
					DivideToString64(
						buf,
						sizeof(buf),
						&memval,
						1024,
						(*(ptr + 3) == 'K') ? 1 : 0,
						(environment->env->settings.date_flags & DATE_1000SEP) ? GUI->decimal_sep : 0);
					++esc;
				}
				else if (*(ptr + 3) == 'M' || *(ptr + 3) == 'm')
				{
					DivideToString64(
						buf,
						sizeof(buf),
						&memval,
						1024 * 1024,
						(*(ptr + 3) == 'M') ? 1 : 0,
						(environment->env->settings.date_flags & DATE_1000SEP) ? GUI->decimal_sep : 0);
					++esc;
				}
				else if (*(ptr + 3) == 'G' || *(ptr + 3) == 'g')
				{
					DivideToString64(
						buf,
						sizeof(buf),
						&memval,
						1024 * 1024 * 1024,
						(*(ptr + 3) == 'G') ? 1 : 0,
						(environment->env->settings.date_flags & DATE_1000SEP) ? GUI->decimal_sep : 0);
					++esc;
				}
				else if (*(ptr + 3) == 'S' || *(ptr + 3) == 's')
				{
					BytesToString64(&memval,
									buf,
									sizeof(buf),
									(*(ptr + 3) == 'S') ? 1 : 0,
									(environment->env->settings.date_flags & DATE_1000SEP) ? GUI->decimal_sep : 0);
					++esc;
				}
				else if (*(ptr + 3) == '%' && got_memtotal)
				{
					// Get percentage string
					clock_memory_percent_string(buf, sizeof(buf), memval, memtotal);
					++esc;
				}
				else
				{
					// memory values should always be unsigned
					ItoaU64(&memval,
							buf,
							sizeof(buf),
							(environment->env->settings.date_flags & DATE_1000SEP) ? GUI->decimal_sep : 0);
				}
			}

			// Got a string to add?
			if (buf[0])
			{
				// Add on to title string
				stccpy(title_buffer + pos, buf, TITLE_SIZE - pos);
				pos = strlen(title_buffer);
			}
		}

		// Skip characters from escape sequence?
		if (esc)
			ptr += esc;

		// Otherwise, add literal character to string
		else
			title_buffer[pos++] = *ptr;
	}

	// Null-terminate buffer
	title_buffer[pos] = 0;

	// Render text
#ifndef CLOCK_USE_SCREENTITLE
	Move(rp, 5, clock_titlebar_metrics.text_y);
	Text(rp, title_buffer, (moon_day > -1) ? moon_pos : strlen(title_buffer));

	// Moon to show?
	if (moon_day > -1)
	{
		short x, image, size;

		// Find position for moon
		x = rp->cp_x;

		// Map moon day to image
		if (moon_day < 2)
			image = 0;
		else if (moon_day < 6)
			image = 1;
		else if (moon_day < 10)
			image = 2;
		else if (moon_day < 13)
			image = 3;
		else if (moon_day < 16)
			image = 4;	// Full
		else if (moon_day < 19)
			image = 5;
		else if (moon_day < 23)
			image = 6;
		else if (moon_day < 28)
			image = 7;
		else
			image = 0;

		// Big moon?
		if (rp->TxHeight >= 13)
		{
			InitBitMap(&bm, 2, MOON_BIG_SIZE, MOON_BIG_SIZE);
			/*#ifdef __amigaos3__
						bm.Planes[0]=(PLANEPTR)moon_big_data_chip[image][0];
						bm.Planes[1]=(PLANEPTR)moon_big_data_chip[image][1];
			#else*/
			bm.Planes[0] = (PLANEPTR)moon_big_data[image][0];
			bm.Planes[1] = (PLANEPTR)moon_big_data[image][1];
			//#endif
			size = MOON_BIG_SIZE;
		}

		// Small moon
		else
		{
			InitBitMap(&bm, 2, MOON_SMALL_SIZE, MOON_SMALL_SIZE);
			/*#ifdef __amigaos3__
						bm.Planes[0]=(PLANEPTR)moon_small_data_chip[image][0];
						bm.Planes[1]=(PLANEPTR)moon_small_data_chip[image][1];
			#else*/
			bm.Planes[0] = (PLANEPTR)moon_small_data[image][0];
			bm.Planes[1] = (PLANEPTR)moon_small_data[image][1];
			//#endif
			size = MOON_SMALL_SIZE;
		}

		// Draw moon
		BltBitMapRastPort(&bm, 0, 0, rp, x, clock_titlebar_image_y(size), size, size, 0xc0);

		// Draw the rest of the text
		Move(rp, x + size, clock_titlebar_metrics.text_y);
		if ((x = strlen(title_buffer) - moon_pos - 2) > 0)
			Text(rp, title_buffer + moon_pos + 2, x);
	}

	// Erase to start of clock text
	if (clock_x > rp->cp_x)
	{
		unsigned char fp;

		// Save front pen
		fp = rp->FgPen;
		SetAPen(rp, rp->BgPen);
		RectFill(rp, rp->cp_x, clock_titlebar_metrics.fill_top, clock_x - 1, clock_titlebar_metrics.fill_bottom);
		SetAPen(rp, fp);
	}
#endif

	return used_clock;
}

static char mondays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Calculate day, month and year from number of days since 1978-01-11
void calc_daymonyr(long days, long *day, long *month, long *year)
{
	long i, mdays, ldays = 1461, year2 = 78;

	year2 += (days / ldays) * 4;
	days %= ldays;
	while (days)
	{
		mdays = 365;
		if ((year2 & 3) == 0)
			mdays++;
		if (days < mdays)
			break;
		days -= mdays;
		year2++;
	}
	for (i = 0, days++; i < 12; i++)
	{
		mdays = mondays[i];
		if (i == 1 && (year2 & 3) == 0)
			mdays++;
		if (days <= mdays)
			break;
		days -= mdays;
	}

	// Return information
	*day = days;
	*month = i + 1;
	*year = year2;
}
