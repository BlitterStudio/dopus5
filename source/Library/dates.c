/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software

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

#include <stdlib.h>

#include "dopuslib.h"

#define DATE_PARSE_MAX_CHARS 30

static char *date_english_months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
										"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static BOOL date_is_token_boundary(struct Locale *locale, char ch)
{
	if (!ch)
		return 1;

	if (LocaleBase)
		return !IsAlNum(locale, (UBYTE)ch);

	return !isalnum((unsigned char)ch);
}

static BOOL date_replace_locale_month(struct Locale *locale,
									  char *date,
									  char *buffer,
									  ULONG buffer_size,
									  ULONG locale_id,
									  char *english)
{
	char *month, *ptr;
	long month_len, prefix_len, replace_len, suffix_len;

	if (!date || !buffer || !buffer_size)
		return 0;

	if (!(month = (char *)GetLocaleStr(locale, locale_id)) || !*month)
		return 0;

	month_len = strlen(month);
	replace_len = strlen(english);

	for (ptr = date; *ptr; ptr++)
	{
		if (!date_is_token_boundary(locale, (ptr == date) ? 0 : *(ptr - 1)) ||
			strnicmp(ptr, month, month_len) != 0 || !date_is_token_boundary(locale, ptr[month_len]))
			continue;

		prefix_len = ptr - date;
		suffix_len = strlen(ptr + month_len);
		if (prefix_len + replace_len + suffix_len >= buffer_size)
			return 0;

		strncpy(buffer, date, prefix_len);
		buffer[prefix_len] = 0;
		strcat(buffer, english);
		strcat(buffer, ptr + month_len);
		return 1;
	}

	return 0;
}

static BOOL date_normalise_locale_month(char *date, char *buffer, ULONG buffer_size)
{
	struct Locale *locale;
	short month;
	BOOL ret = 0;

	if (!LocaleBase || !(locale = OpenLocale(0)))
		return 0;

	for (month = 0; month < 12 && !ret; month++)
	{
		ret = date_replace_locale_month(locale, date, buffer, buffer_size, MON_1 + month, date_english_months[month]);

#ifdef ALTMON_1
		if (!ret)
			ret = date_replace_locale_month(
				locale, date, buffer, buffer_size, ALTMON_1 + month, date_english_months[month]);
#endif

		if (!ret)
			ret = date_replace_locale_month(locale, date, buffer, buffer_size, ABMON_1 + month, date_english_months[month]);
	}

	CloseLocale(locale);
	return ret;
}

// Parse a date/time string into separate date and time buffers
char *LIBFUNC L_ParseDateStrings(REG(a0, char *string),
								 REG(a1, char *date_buffer),
								 REG(a2, char *time_buffer),
								 REG(a3, long *range))
{
	char *date_ptr = 0, *time_ptr = 0, *end_ptr;
	char work_buffer[80], cur_date_buf[20], cur_time_buf[20];
	struct DateTime datetime;
	long str_pos, str_len, temp, time_first = 0;

	// Clear buffers and range code
	date_buffer[0] = 0;
	time_buffer[0] = 0;
	if (range)
		*range = 0;

	// Copy string and get length
	strcpy(work_buffer, string);
	str_len = strlen(work_buffer);

	// Strip leading spaces
	for (str_pos = 0; str_pos < str_len && isspace((unsigned char)work_buffer[str_pos]); str_pos++)
		;
	if (str_pos == str_len)
		goto getout;

	// After a date
	if (range && work_buffer[str_pos] == '>')
	{
		++str_pos;
		*range = RANGE_AFTER;
		goto getout;
	}

	// Get pointer to start of date string
	date_ptr = &work_buffer[str_pos];

	// Find first space; null-terminate there
	for (++str_pos; str_pos < str_len; str_pos++)
	{
		if (isspace((unsigned char)work_buffer[str_pos]))
		{
			work_buffer[str_pos] = 0;
			break;
		}

		// If we find a colon, time must be listed first
		if (work_buffer[str_pos] == ':')
			time_first = 1;
	}
	if (str_pos >= str_len)
		goto getout;

	// Strip leading spaces
	for (++str_pos; str_pos < str_len && isspace((unsigned char)work_buffer[str_pos]); str_pos++)
		;
	if (str_pos >= str_len)
		goto getout;

	// Between two dates
	if (range && work_buffer[str_pos] == '>')
	{
		++str_pos;
		*range = RANGE_BETWEEN;
		goto getout;
	}

	// Get pointer to start of time string
	time_ptr = &work_buffer[str_pos];

	// Find first space; null-terminate there
	for (++str_pos; str_pos < str_len; str_pos++)
	{
		if (isspace((unsigned char)work_buffer[str_pos]))
		{
			work_buffer[str_pos] = 0;
			break;
		}
	}
	if (str_pos == str_len)
		goto getout;

	// Scan for range indicator
	if (range)
	{
		// Store old position
		temp = str_pos;

		for (++str_pos; str_pos < str_len; str_pos++)
		{
			if (work_buffer[str_pos] == '>')
			{
				temp = str_pos + 1;
				*range = RANGE_BETWEEN;
				break;
			}
		}

		// Restore position
		str_pos = temp;
	}

getout:

	// Get pointer to end of string we deal with
	end_ptr = &string[str_pos];

	// Swap date/time pointers?
	if (time_first)
	{
		char *temp;

		temp = date_ptr;
		date_ptr = time_ptr;
		time_ptr = temp;
	}

	// Get current date
	DateStamp(&datetime.dat_Stamp);

	// Initialise DateTime structure
	datetime.dat_Format = FORMAT_DOS;
	datetime.dat_StrDay = 0;
	datetime.dat_StrDate = cur_date_buf;
	datetime.dat_StrTime = cur_time_buf;
	datetime.dat_Flags = 0;

	// Convert to string
	DateToStr(&datetime);

	// Fucknose
	temp = 0;

	// Valid date string supplied?
	if (date_ptr && (str_pos = strlen(date_ptr)) < DATE_PARSE_MAX_CHARS && str_pos > 4)
		strcpy(date_buffer, date_ptr);

	// Otherwise
	else
	{
		// Range is not after?
		if (range && (*range) != RANGE_AFTER)
		{
			temp = 1;

			// Get current date
			strcpy(date_buffer, cur_date_buf);
		}

		// Otherwise, default date 1/1/78
		else
		{
			strcpy(date_buffer, "1-Jan-78");
		}
	}

	// Valid time supplied?
	if (time_ptr && (str_pos = strlen(time_ptr)) < 9 && str_pos > 2)
	{
		strcpy(time_buffer, time_ptr);
		if (range && !(*range))
			*range = RANGE_WEIRD;
	}

	// Nope
	else
	{
		// Current time?
		if (temp)
			strcpy(time_buffer, cur_time_buf);

		// Default to midnight
		else
			strcpy(time_buffer, "00:00:00");
	}

	// Return pointer to end of date/time string
	return end_ptr;
}

// Convert a string to a datestamp
BOOL LIBFUNC L_DateFromStrings(REG(a0, char *date), REG(a1, char *time), REG(a2, struct DateStamp *ds))
{
	return L_DateFromStringsNew(date, time, ds, FORMAT_DOS);
}

// Convert a string to a datestamp
BOOL LIBFUNC L_DateFromStringsNew(REG(a0, char *date),
								  REG(a1, char *time),
								  REG(a2, struct DateStamp *ds),
								  REG(d0, ULONG method))
{
	struct DateTime datetime;
	char tempdate[80], temptime[12], *ptr;
	BOOL ret;

	// Initialise DateTime structure
	datetime.dat_Format = method;
	datetime.dat_Flags = DTF_FUTURE;
	datetime.dat_StrDay = 0;
	datetime.dat_StrDate = date;
	datetime.dat_StrTime = temptime;

	// Copy time
	strcpy(temptime, time);

	// If time is in 12 hour format, fix it
	if ((ptr = strchr(temptime, 'a')) || (ptr = strchr(temptime, 'p')))
	{
		short hour;

		// Get hour
		hour = atoi(temptime);

		// Fix am
		if (*ptr == 'a')
		{
			if (hour == 12)
				hour = 0;
		}

		// Or pm
		else
		{
			if (hour < 12)
				hour += 12;
		}

		// Rebuild time string
		lsprintf(temptime, "%ld%s", hour, (IPTR)strchr(time, ':'));
	}

	// Do conversion
	ret = StrToDate(&datetime);
	if (!ret && date_normalise_locale_month(date, tempdate, sizeof(tempdate)))
	{
#if defined(__amigaos4__)
		datetime.dat_StrDate = (STRPTR)tempdate;
#else
		datetime.dat_StrDate = (UBYTE *)tempdate;
#endif
		ret = StrToDate(&datetime);
	}

	// Copy stamp
	*ds = datetime.dat_Stamp;
	return ret;
}
