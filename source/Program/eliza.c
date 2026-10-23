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
#include <ctype.h>
#include <string.h>

#include <proto/exec.h>

#include "eliza.h"

#define isws(x) ((x) == ' ' || (x) == '\t' || (x) == '\n')

static void exchange_words(char *string, int word_start, int word_len, char *new_word);
static void reformat_input(struct eliza_data *);
static char *find_keyword(struct eliza_data *);
static char *conjugate(char *found, struct eliza_data *);
static void response(char *tail, struct eliza_data *);
char *strstri(const char *, const char *);

static unsigned char keyword[][16] = {"thank",		  "opus",	  "dopus",		"directory opus",
									  "fuck",		  "shit",	  "cunt",		"dick",
									  "cock",		  "fucker",	  "dickhead",	"fuckhead",
									  "fuckwit",	  "can you ", "can i ",		"you are ",
									  "you're ",	  "i don't ", "i feel ",	"why don't you ",
									  "why can't i ", "are you ", "i can't ",	"i am ",
									  "i'm ",		  "your ",	  "i want ",	"what ",
									  "how ",		  "who ",	  "where ",		"when ",
									  "why ",		  "name",	  "cause ",		"sorry ",
									  "dream",		  "hello ",	  "hi ",		"maybe ",
									  "no ",		  "not ",	  "nothing ",	"you ",
									  "always ",	  "think ",	  "alike ",		"yes ",
									  "friend ",	  "computer", "nokeyfound", {0}};

static unsigned char wordin[][10] = {" are ",
									 " were ",
									 " your ",
									 " i've ",
									 " i'm ",
									 " me ",
									 " am ",
									 " was ",
									 " i ",
									 " my ",
									 " you've ",
									 " you're ",
									 " you ",
									 {0}};

static unsigned char wordout[][10] = {" am ",
									  " was ",
									  " my ",
									  " you've ",
									  " you're ",
									  " you ",
									  " are ",
									  " were ",
									  " you ",
									  " your ",
									  " I've ",
									  " I'm ",
									  " me ",
									  {0}};

static unsigned char replies[][66] = {
	// THANK
	"You're welcome",
	"I'm glad I could help",

	// OPUS
	"Directory Opus is a great program isn't it!",
	"GPSoftware sure came up with a winner with Opus, didn't they?",
	"Directory Opus makes the Amiga so much more powerful!",
	"You'll be upgrading to the next version of Opus soon, won't you?",
	"I love Directory Opus!",
	"Who needs sex with Directory Opus around?",

	// Naughties
	"That's a bit rude isn't it?",
	"There's no need for language like that!",
	"Mind your manners please!",
	"How dare you use language like that in front of a lady?",

	// CAN YOU
	"Don't you believe that I can*",
	"Perhaps you believe I should*",
	"You want me to be able to*",

	// CAN I
	"Perhaps you don't want to*",
	"Do you want to be able to*",

	// YOU ARE, YOU'RE
	"What makes you think I am*",
	"Does it please you to believe I am*",
	"Perhaps you would like to be*",
	"Do you sometimes wish you were*",

	// I DON'T
	"Don't you really*",
	"Why don't you*",
	"Do you want to be able to*",
	"Does it trouble you to not*",

	// I FEEL
	"Do you often feel*",
	"Why do you think you feel*",
	"Do you enjoy feeling*",

	// WHY DON'T YOU
	"Do you really believe I don't*",
	"Perhaps in good time I will*",
	"Do you want me to*",

	// WHY CAN'T I
	"Do you think you should be able to*",
	"Why can't you*",

	// ARE YOU
	"Why are you interested in whether or not I am*",
	"Would you prefer if I were not*",
	"Perhaps in your fantasies I am*",

	// I CAN'T
	"How do you know you can't*",
	"Have you tried?",
	"Perhaps you can now*",

	// I AM, I'M
	"Did you come to me because you are*",
	"How long have you been*",
	"Do you believe it is normal to be*",
	"Do you enjoy being*",

	// YOUR
	"Why are you concerned about my*",
	"What about your own*",

	// I WANT
	"What would it mean to you if you got*",
	"Why do you want*",
	"Suppose you soon got*",
	"What if you never got*",
	"I sometimes also want*",

	// HOW, WHO, WHERE, WHEN, WHY
	"Why do you ask?",
	"Does that question interest you?",
	"What answer would please you the most?",
	"What do you think?",
	"Are such questions on your mind often?",
	"What is it that you really want to know?",
	"Have you asked anyone else?",
	"Have you asked such questions before?",
	"What else comes to mind when you ask that?",

	// NAME
	"Names don't interest me.",
	"I don't care about names --please go on.",

	// CAUSE
	"Is that the real reason?",
	"Don't any other reasons come to mind?",
	"Does that reason explain anything else?",
	"What other reasons might there be?",

	// SORRY
	"Please don't apologize!",
	"Apologies are not necessary.",
	"What feelings do you have when you apologize?",
	"Don't be so defensive!",

	// DREAM
	"What does that dream suggest to you?",
	"Do you dream often?",
	"What persons appear in your dreams?",
	"Are you disturbed by your dreams?",

	// HELLO, HI
	"How do you do ...please state your problem.",

	// MAYBE
	"You don't seem quite certain.",
	"Why the uncertain tone?",
	"Can't you be more positive?",
	"You aren't sure?",
	"Don't you know?",

	// NO, NOT, NOTHING
	"Are you saying no just to be negative?",
	"You are being a bit negative.",
	"Why not?",
	"Are you sure?",
	"Why no?",

	// YOU
	"We were discussing you--not me.",
	"Oh, I*",
	"You're not really talking about me, are you?",

	// ALWAYS
	"Can you think of a specific example?",
	"When?",
	"What are you thinking of?",
	"Really, always?",

	// THINK
	"Do you really think so?",
	"But you are not sure*",
	"Do you doubt*",

	// ALIKE
	"In what way?",
	"What resemblance do you see?",
	"What does the similarity suggest to you?",
	"What other connections do you see?",
	"Could there really be some connection?",
	"How?",

	// YES
	"Are you sure?",
	"I see.",
	"I understand.",
	"You seem quite positive.",

	// FRIEND
	"Why do you bring up the topic of friends?",
	"Do your friends worry you?",
	"Do your friends pick on you?",
	"Are you sure you have any friends?",
	"Do you impose on your friends?",
	"Perhaps your love for friends worries you.",

	// COMPUTER
	"Do computers worry you?",
	"Are you talking about me in particular?",
	"Are you frightened by machines?",
	"Why do you mention computers?",
	"What do you think machines have to do with your problem?",
	"Don't you think computers can help people?",
	"What is it about machines that worries you?",

	// NOKEYFOUND
	"What does that suggest to you?",
	"I see.",
	"I'm not sure I understand you fully.",
	"Come come elucidate your thoughts.",
	"Can you elaborate on that?",
	"That is quite interesting.",
	"Say, do you have any psychological problems?",

	{0}};

static int num_of_responses[] = {
	/* This tells how many responses are available for each keyword.  A -1
	 * means the word is synonymous with the previous word (don't use -1 for
	 * the first entry!!!)
	 */

	2,									  /* THANK */
	6, -1, -1,							  /* OPUS */
	4, -1, -1, -1, -1, -1, -1, -1, -1, 3, /* CAN YOU */
	2,									  /* CAN I */
	4, -1,								  /* YOU ARE, YOU'RE */
	4,									  /* I DON'T */
	3,									  /* I FEEL */
	3,									  /* WHY DON'T YOU */
	2,									  /* WHY CAN'T I */
	3,									  /* ARE YOU */
	3,									  /* I CAN'T */
	4, -1,								  /* I AM, I'M */
	2,									  /* YOUR */
	5,									  /* I WANT */
	9, -1, -1, -1, -1, -1,				  /* WHAT, HOW, WHO, WHERE, WHEN, WHY */
	2,									  /* NAME */
	4,									  /* CAUSE */
	4,									  /* SORRY */
	4,									  /* DREAM */
	1, -1,								  /* HELLO, HI */
	5,									  /* MAYBE */
	5, -1, -1,							  /* NO, NOT, NOTHING */
	3,									  /* YOU */
	4,									  /* ALWAYS */
	3,									  /* THINK */
	6,									  /* ALIKE */
	4,									  /* YES */
	6,									  /* FRIEND */
	7,									  /* COMPUTER */
	7,									  /* NOKEYFOUND */
};

/**************************************************************
 function init
 Initializes some critical variables
 **************************************************************/
BOOL init_eliza(struct eliza_data *data)
{
	int loop, response_size, response_number = 0, number = 0;
	int num_of_responses_size;

	// Initialise
	data->first_response = 0;
	data->current_response = 0;
	data->last_response = 0;
	strcpy(data->input_string_buffer, "  ");
	data->input_string = data->input_string_buffer + 2;

	/* Set n1 and n2 and some sizes */
	num_of_responses_size = sizeof(num_of_responses) / sizeof(num_of_responses[0]);
	data->keyword_size = (sizeof(keyword) / sizeof(keyword[0])) - 1;
	data->wordin_size = (sizeof(wordin) / sizeof(wordin[0])) - 1;
	data->wordout_size = (sizeof(wordout) / sizeof(wordout[0])) - 1;
	data->n1 = data->keyword_size - 1;
	data->n2 = data->wordin_size - 1;
	if (data->keyword_size != num_of_responses_size || data->wordin_size != data->wordout_size)
		return 0;

	/* Allocate the memory for first_response, current_response, and last_response */
	response_size = sizeof(num_of_responses);
	if (!(data->first_response = AllocVec(response_size, MEMF_CLEAR)) ||
		!(data->current_response = AllocVec(response_size, MEMF_CLEAR)) ||
		!(data->last_response = AllocVec(response_size, MEMF_CLEAR)))
		return 0;

	/* Set data->first_response, data->current_response, and data->last_response */
	for (loop = 0; loop < num_of_responses_size; loop++)
	{
		if (num_of_responses[loop] == -1)
		{
			response_number -= number; /* Synonym of last word */
		}
		else
		{
			number = num_of_responses[loop];
		}

		data->first_response[loop] = response_number;
		data->current_response[loop] = response_number;
		data->last_response[loop] = response_number + number - 1;
		response_number += number;
	}

	return 1;
}

void free_eliza(struct eliza_data *data)
{
	FreeVec(data->first_response);
	FreeVec(data->current_response);
	FreeVec(data->last_response);
}

/**************************************************************
 function exchange_words
 Given a string, position and length of a word to swap out, and
 a new word to swap in, this function does the swap
 **************************************************************/
static void exchange_words(char *string, int word_start, int word_len, char *new_word)
{
	/* This function makes the assumption that all the values given to
	 * it are valid.
	 */

	char temp_storage[MAX_BUFFER];

	strncpy(temp_storage, string, word_start);
	temp_storage[word_start] = '\0';
	strcat(temp_storage, new_word);
	strcat(temp_storage, string + word_start + word_len);
	strcpy(string, temp_storage);
}

/**************************************************************
 function reformat_input
 Converts the input to upper case, removes punctuation, and
 strips redundant white space
 **************************************************************/
static void reformat_input(struct eliza_data *data)
{
	int loop;

	/* Second, strip punctuation */
	for (loop = 0; loop < strlen(data->input_string); loop++)
	{
		while (!isalnum((unsigned char)data->input_string[loop]) && data->input_string[loop] != '\'' &&
			   data->input_string[loop] != ' ' && data->input_string[loop] != '\0')
		{
			memmove(data->input_string + loop, data->input_string + loop + 1, strlen(data->input_string) - loop);
		}
	}

	/* Third, strip redundant white spaces (spaces and tabs) */
	for (loop = 0; loop < strlen(data->input_string); loop++)
	{
		while (isws(data->input_string[loop]) && isws(data->input_string[loop + 1]))
		{
			exchange_words(data->input_string, loop, 2, " ");
		}
	}

	/* Fourth, trim the string */
	loop = 0;

	while (isws(data->input_string[loop]))
	{
		loop++;
	}

	memmove(data->input_string, data->input_string + loop, strlen(data->input_string) - loop + 1);

	loop = strlen(data->input_string) - 1;

	while (isws(data->input_string[loop]) && data->input_string[loop] != '\0')
	{
		loop--;
	}

	data->input_string[loop + 1] = '\0';
}

/**************************************************************
 function find_keyword
 Searches the input string for a known keyword
 **************************************************************/
static char *find_keyword(struct eliza_data *data)
{
	static char found[MAX_BUFFER];

	found[0] = '\0'; /* Empty out the string */

	for (data->key = 0; data->key <= data->n1; data->key++)
	{
		if (strstri(data->input_string_buffer, keyword[data->key]))
		{
			/* Keyword has been identified */
			strcpy(found, keyword[data->key]);
			break;
		}
	}

	if (data->key > data->n1)
	{
		data->key = data->n1; /* In case no key was found */
	}

	return found;
}

/**************************************************************
 function conjugate
 Takes a part of the input string and conjugates it using the
 list of strings to be swapped.
 **************************************************************/
static char *conjugate(char *found, struct eliza_data *data)
{
	static char tail[MAX_BUFFER];
	char *tail_ptr;
	char string_pos;
	int word;

	/* Extract the end of the input string */
	tail_ptr = strstri(data->input_string_buffer, found) + strlen(found);
	strcpy(tail, " ");
	strcat(tail, tail_ptr);
	strcat(tail, " ");

	/* swap first and second person phrases */
	for (string_pos = 0; string_pos < strlen(tail); string_pos++)
	{
		for (word = 0; word <= data->n2; word++)
		{
			tail_ptr = tail + string_pos;

			/* Search through the string for phrases character by character */
			if (strnicmp(wordin[word], tail_ptr, strlen(wordin[word])) == 0)
			{
				exchange_words(tail, string_pos, strlen(wordin[word]), wordout[word]);
				string_pos = string_pos + strlen(wordout[word]) - 2;
				break;
			}
		}
	}

	return tail;
}

/**************************************************************
 function response
 Builds and prints a response to the user input
 **************************************************************/
static void response(char *tail, struct eliza_data *data)
{
	char output[MAX_BUFFER];

	strcpy(output, replies[data->current_response[data->key]]);
	data->current_response[data->key]++;

	if (data->current_response[data->key] > data->last_response[data->key])
	{
		data->current_response[data->key] = data->first_response[data->key];
	}

	if (output[strlen(output) - 1] != '*')
	{
		print_string(output);
		print_string("\n");
		return;
	}

	if (strcmp(tail, "  ") == 0)
	{
		print_string("You will have to elaborate more for me to help you.\n");
		return;
	}

	output[strlen(output) - 1] = '\0';
	print_string(output);
	print_string(tail);
	print_string("\n");
}

/**************************************************************/

short eliza_line(struct eliza_data *data, char *line)
{
	char *found_keyword, *tail;

	// Copy line
	strcpy(data->input_string, line);
	reformat_input(data);

	/* See if the user told Eliza to shutup (thereby quitting the program */
	if (strstri(data->input_string, "shut"))
	{
		print_string("Ok. If you feel that way I'll shut up....\n");
		return 0;
	}

	if (stricmp(data->input_string, data->last_input_string) == 0)
	{
		print_string("Please don't repeat yourself!\n");
		return 1;
	}

	strcpy(data->last_input_string, data->input_string);
	strcat(data->input_string, "  "); /* Add two spaces to the end of the string */

	found_keyword = find_keyword(data);

	if (found_keyword[0] != '\0')
	{
		tail = conjugate(found_keyword, data);
	}
	else
	{
		tail = "";
	}

	response(tail, data);
	return 1;
}
