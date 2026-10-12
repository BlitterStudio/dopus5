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

#include "dopuslib.h"

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	BPTR file;		   // File handle
	char *buffer;	   // Buffer
	long buffer_size;  // Size of buffer
	long buffer_pos;   // Position in buffer
	long buffer_fill;  // Amount of data in buffer
	long file_pos;	   // Position in file
	long write_total;  // Amount of write data
	void *dos;		   // DOS library pointer
} BufFile;

#ifndef __amigaos3__
	#pragma pack()
#endif

// Open a buffered file
APTR LIBFUNC L_OpenBuf(REG(a0, char *name), REG(d0, long mode), REG(d1, long buffer_size))
{
	BufFile *file;

	// Allocate file handle
	if (!(file = AllocVec(sizeof(BufFile), MEMF_CLEAR)))
		return 0;

	// Open file
	if (!(file->file = Open(name, mode)))
	{
		FreeVec(file);
		return 0;
	}

	// Store DOS pointer
	file->dos = DOSBase;

	// Allocate buffer
	if (buffer_size > 0)
	{
		if ((file->buffer = AllocVec(buffer_size, 0)))
			file->buffer_size = buffer_size;
	}

	return file;
}

#define DOSBase (((BufFile *)file)->dos)

// Close a buffered file
long LIBFUNC L_CloseBuf(REG(a0, APTR file))
{
	long total = 0;

	if (file)
	{
		// Flush buffer
		total = L_FlushBuf(file);

		// Free buffer
		FreeVec(((BufFile *)file)->buffer);

		// Close file
		Close(((BufFile *)file)->file);

		// Free handle
		FreeVec(file);
	}

	return total;
}

// Read data from a buffered file
long LIBFUNC L_ReadBuf(REG(a0, APTR file), REG(a1, char *data), REG(d0, long size))
{
	long read_size = 0;

	// Invalid size?
	if (size < 1)
		return (size < 0) ? -1 : 0;

	// Is there any write data in the buffer?
	if (((BufFile *)file)->write_total > 0)
	{
		// Flush file
		L_FlushBuf(file);
	}

	// Is there any data in the buffer?
	if (((BufFile *)file)->buffer_pos < ((BufFile *)file)->buffer_fill)
	{
		// How much do we copy from the buffer?
		read_size = ((BufFile *)file)->buffer_fill - ((BufFile *)file)->buffer_pos;
		if (read_size > size)
			read_size = size;

		// Copy data from buffer
		if (read_size > 0)
			memcpy(data, ((BufFile *)file)->buffer + ((BufFile *)file)->buffer_pos, read_size);

		// Bump buffer pointer
		((BufFile *)file)->buffer_pos += read_size;
		size -= read_size;
		data += read_size;

		// Got all the data we want?
		if (size == 0)
			return read_size;
	}

	// Buffer is empty. Is read request bigger than buffer size?
	if (size > ((BufFile *)file)->buffer_size)
	{
		long read;

		// Read straight into buffer
		if ((read = Read(((BufFile *)file)->file, data, size)) > 0)
		{
			// Update file position
			((BufFile *)file)->file_pos += read;
			read_size += read;
		}

		// Error?
		else if (read == -1 && !read_size)
			return -1;

		return read_size;
	}

	// Read data into buffer
	((BufFile *)file)->buffer_fill =
		Read(((BufFile *)file)->file, ((BufFile *)file)->buffer, ((BufFile *)file)->buffer_size);

	// Got some data?
	if (((BufFile *)file)->buffer_fill > 0)
	{
		// Update file position
		((BufFile *)file)->file_pos += ((BufFile *)file)->buffer_fill;

		// Not enough data?
		if (((BufFile *)file)->buffer_fill < size)
			size = ((BufFile *)file)->buffer_fill;

		// Copy data from buffer
		if (size > 0)
			memcpy(data, ((BufFile *)file)->buffer, size);

		// Bump buffer pointer
		((BufFile *)file)->buffer_pos = size;
		read_size += size;
	}

	// Error?
	else if (((BufFile *)file)->buffer_fill == -1 && !read_size)
	{
		((BufFile *)file)->buffer_fill = 0;
		return -1;
	}

	return read_size;
}

// Write data to a buffered file
long LIBFUNC L_WriteBuf(REG(a0, APTR file), REG(a1, const char *data), REG(d0, long size))
{
	long write_size;

	// Size==-1 means do strlen
	if (size == -1)
		size = strlen(data);

	// Is write request bigger than total buffer size?
	if (size > ((BufFile *)file)->buffer_size)
	{
		// Flush buffer if there's anything in it
		if (((BufFile *)file)->write_total > 0)
		{
			// Flush buffer
			if (L_FlushBuf(file) < 0)
			{
				// Error!
				return -1;
			}
		}

		// Write data straight to disk
		write_size = Write(((BufFile *)file)->file, (APTR)data, size);

		// Success?
		if (write_size > 0)
		{
			// Update file position
			((BufFile *)file)->file_pos += write_size;
		}
		return write_size;
	}

	// Copy as much of write request as possible to the buffer
	write_size = ((BufFile *)file)->buffer_size - ((BufFile *)file)->write_total;
	if (write_size > size)
		write_size = size;

	// Copy to buffer
	memcpy(((BufFile *)file)->buffer + ((BufFile *)file)->write_total, data, write_size);

	// Bump buffer position
	((BufFile *)file)->write_total += write_size;

	// Is buffer full?
	if (((BufFile *)file)->write_total >= ((BufFile *)file)->buffer_size)
	{
		long error;

		// Flush buffer
		error = L_FlushBuf(file);

		// Do we still have data left to write?
		if (write_size < size)
		{
			// Copy to buffer
			memcpy(((BufFile *)file)->buffer, data + write_size, size - write_size);

			// Bump buffer position
			((BufFile *)file)->write_total += size - write_size;
		}

		// Error?
		if (error < 0)
			size = error;
	}

	return size;
}

// Flush a buffered file
long LIBFUNC L_FlushBuf(REG(a0, APTR file))
{
	long total = 0;

	// Any write data to flush?
	if (((BufFile *)file)->write_total > 0)
	{
		// Write data to disk
		total = Write(((BufFile *)file)->file, ((BufFile *)file)->buffer, ((BufFile *)file)->write_total);

		// Update file position
		if (total > 0)
			((BufFile *)file)->file_pos += total;
	}

	// Reset buffer positions
	((BufFile *)file)->buffer_pos = 0;
	((BufFile *)file)->buffer_fill = 0;
	((BufFile *)file)->write_total = 0;

	return total;
}

// Seek a buffered file
long LIBFUNC L_SeekBuf(REG(a0, APTR file), REG(d0, long offset), REG(d1, long mode))
{
	long old_pos, new_pos;

	// Get old position
	old_pos = ((BufFile *)file)->file_pos;

	// Any data in write buffer?
	if (((BufFile *)file)->write_total > 0)
		old_pos += ((BufFile *)file)->write_total;

	// Any data in read buffer?
	else if (((BufFile *)file)->buffer_fill > 0)
	{
		old_pos -= ((BufFile *)file)->buffer_fill;
		old_pos += ((BufFile *)file)->buffer_pos;
	}

	// Only want current position?
	if (offset == 0 && mode == OFFSET_CURRENT)
		return old_pos;

	// Is there any write data?
	if (((BufFile *)file)->write_total > 0)
	{
		// Flush buffer
		L_FlushBuf(file);

		// Seek to the new position
		new_pos = Seek(((BufFile *)file)->file, offset, mode);

		// Valid?
		if (new_pos > -1)
		{
			// Save position
			((BufFile *)file)->file_pos = Seek(((BufFile *)file)->file, 0, OFFSET_CURRENT);
		}

		return new_pos;
	}

	// If there's any read data, we can only handle current and beginning seeks
	if (((BufFile *)file)->buffer_fill > 0)
	{
		// Current/beginning?
		if (mode == OFFSET_CURRENT || mode == OFFSET_BEGINNING)
		{
			// Calculate new position
			if (mode == OFFSET_CURRENT)
				new_pos = old_pos + offset;
			else
				new_pos = offset;

			// Would that leave us within the buffer?
			if (new_pos >= ((BufFile *)file)->file_pos - ((BufFile *)file)->buffer_fill &&
				new_pos < ((BufFile *)file)->file_pos)
			{
				// Adjust position within buffer
				((BufFile *)file)->buffer_pos =
					new_pos - (((BufFile *)file)->file_pos - ((BufFile *)file)->buffer_fill);

				// Return old offset
				return old_pos;
			}

			// If moving current, adjust offset
			if (mode == OFFSET_CURRENT)
			{
				offset += old_pos;
				mode = OFFSET_BEGINNING;
			}
		}

		// Flush read buffer
		((BufFile *)file)->buffer_pos = 0;
		((BufFile *)file)->buffer_fill = 0;
	}

	// Seek to the new position
	new_pos = Seek(((BufFile *)file)->file, offset, mode);

	// Valid?
	if (new_pos > -1)
	{
		// Save position
		((BufFile *)file)->file_pos = Seek(((BufFile *)file)->file, 0, OFFSET_CURRENT);
	}

	return new_pos;	 /////// was old_pos
}

// Examine a file
long LIBFUNC L_ExamineBuf(REG(a0, APTR file), REG(a1, struct FileInfoBlock *fib))
{
	return ExamineFH(((BufFile *)file)->file, fib);
}

// Get DOS file handle
BPTR LIBFUNC L_FHFromBuf(REG(a0, APTR file))
{
	return ((BufFile *)file)->file;
}

// Read a line from a buffered file
long LIBFUNC L_ReadBufLine(REG(a0, APTR file), REG(a1, char *data), REG(d0, long size))
{
	long read_size = 0;

	// Invalid size?
	if (size < 1)
		return -1;

	// Initialise line buffer
	*data = 0;

	// Read into line buffer
	while (read_size < size)
	{
		char ch;

		// Get next character
		if ((L_ReadBuf(file, &ch, 1)) < 1)
		{
			if (read_size == 0)
				read_size = -1;
			break;
		}

		// End of line?
		if (ch == '\n')
			break;

		// Store character in line
		*(data++) = ch;
		*data = 0;

		// Increment read size
		++read_size;
	}

	// Return length
	return read_size;
}
