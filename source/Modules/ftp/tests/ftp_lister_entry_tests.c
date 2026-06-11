/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas

This program is free software; you can redistribute it and/or
modify it under the terms of the AROS Public License version 1.1.

*/

#include "../ftp_lister_entry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

static void check_true(const char *name, int value)
{
	if (!value)
	{
		printf("FAIL: %s\n", name);
		++failures;
	}
}

static void check_false(const char *name, int value)
{
	if (value)
	{
		printf("FAIL: %s\n", name);
		++failures;
	}
}

static void check_string(const char *name, const char *actual, const char *expected)
{
	if (strcmp(actual, expected) != 0)
	{
		printf("FAIL: %s: got '%s' expected '%s'\n", name, actual, expected);
		++failures;
	}
}

static void check_ulong(const char *name, unsigned long actual, unsigned long expected)
{
	if (actual != expected)
	{
		printf("FAIL: %s: got %lu expected %lu\n", name, actual, expected);
		++failures;
	}
}

static void test_parse_entry_info(void)
{
	struct ftp_lister_entry_info info;

	check_true("parses normal entry",
			   ftp_lister_parse_entry_info(
				   "simple.txt 1234 -3 0 4567 -------- comment text", "simple.txt", &info));
	check_string("normal name", info.name, "simple.txt");
	check_ulong("normal size", info.size, 1234);
	check_ulong("normal seconds", info.seconds, 4567);
	check_string("normal comment", info.comment, " comment text");

	check_true("parses name with spaces",
			   ftp_lister_parse_entry_info(
				   "Project Notes.txt 42 -3 0 99 -------- ", "Project Notes.txt", &info));
	check_string("space name", info.name, "Project Notes.txt");

	check_false("rejects missing size separator",
				ftp_lister_parse_entry_info("simple.txt", "simple.txt", &info));
	check_false("rejects wrong name prefix",
				ftp_lister_parse_entry_info("other.txt 1 -3 0 1 -------- ", "simple.txt", &info));
	check_false("rejects truncated protection",
				ftp_lister_parse_entry_info("simple.txt 1 -3 0 1 ---", "simple.txt", &info));
}

static void test_quote_entry_name(void)
{
	char quoted[FTP_LISTER_QUOTED_NAME_BUFSIZE];

	check_true("quotes plain name", ftp_lister_quote_entry_name("simple.txt", quoted, sizeof(quoted)));
	check_string("plain quoted", quoted, "\"simple.txt\"");

	check_true("uses single quotes for double quote",
			   ftp_lister_quote_entry_name("a \"quoted\" file.txt", quoted, sizeof(quoted)));
	check_string("double quote escaped", quoted, "'a \"quoted\" file.txt'");

	check_false("rejects both quote types",
				ftp_lister_quote_entry_name("both ' and \" quotes.txt", quoted, sizeof(quoted)));
	check_false("rejects short output",
				ftp_lister_quote_entry_name("simple.txt", quoted, 8));
}

int main(void)
{
	test_parse_entry_info();
	test_quote_entry_name();

	if (failures)
	{
		printf("%d ftp_lister_entry test(s) failed\n", failures);
		return EXIT_FAILURE;
	}

	printf("ftp_lister_entry tests passed\n");
	return EXIT_SUCCESS;
}
