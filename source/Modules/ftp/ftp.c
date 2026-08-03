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

/*
 *	22-09-95	Now deletes destination files after a get where no bytes were transferred
 *	 2-10-95	get() and put() check the abort signal before each transfer instead of after
 *	21-02-96	Now works under AS225 as well as AmiTCP
 *	15-03-96	Tidied up code a bit
 *	19-03-96	"Get" is now asynchronous
 *	20-03-96	"List" is now also asynchronous
 *
 *	25-01-97	GP Changed Get/Put to improve speed.
 *			Scrapped Writes in other task, Added buffered r/w
 *	20-02-97	Rewrote sgets/c fns to add buffered reading from socket
 *			Changed (fixed) method of handling aborts from WaitSelect
 *			now uses WaitSelect flags correctly
 *
 *	-04-97		Added getput btw sites
 *
 */

/*************************************************************
	 This file controls the interaction with FTP protocol
**************************************************************/

#include "ftp.h"
#include "ftp_opusftp.h"  // update_info
#include "ftp_ipc.h"
#include "ftp_lister.h"
#include "ftp_arexx.h"
#include "ftp_recursive.h"
#include "ftp_util.h"  // for cat_bytes()
#include "ftp_parse.h"
#include "ftp_listcmd.h"
#include "ftp_utf8.h"

#ifdef __amigaos3__
// dummy TimerBase to get amiga.lib to link
// timer.device calls are inlined, so it's not actually used
struct Device *TimerBase = NULL;
#endif

#define UPDATE_BYTE_LIMIT (5 * 1024)
#define FTP_COMMAND_BUFSIZE 1024

/* Any printf can be used but this is designed for the one in 'lister.c' */
extern void STDARGS logprintf(char *fmt, ...);

static int ftp_command_error(struct ftp_info *info, int err, const char *msg);
static BOOL ftp_tls_failure_is_verify(struct ftp_tls_session *session);
static const char *ftp_tls_failure_string(struct ftp_tls_session *session);

/**
 **	Function definitions
 **/

// Change to parent dir
int ftp_cdup(struct ftp_info *info, int (*updatefn)(void *, int, char *), void *updateinfo)
{
	int reply;

	reply = _ftpa(info, FTPFLAG_ASYNCH, "CDUP");
	if (reply < 600)
		reply = _getreply(info, 0, updatefn, updateinfo);

	return reply;
}

// Change file modes
int ftp_chmod(struct ftp_info *info, unsigned int mode, char *name)
{
	int reply;
	char *utf8_name = NULL;

	*info->fi_serverr = 0;

	// Convert name to UTF-8 if server supports UTF-8
	utf8_name = ftp_convert_path_to_utf8(name, info);

	reply = ftpa(info, "SITE CHMOD %o %s", mode, utf8_name ? utf8_name : name);

	if (reply == 500 || reply == 502)
		info->fi_flags |= FTP_NO_CHMOD;
	else if (reply / 100 != COMPLETE)
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);

	// Clean up converted name
	if (utf8_name)
		ftp_codesets_free(utf8_name);

	return reply;
}

// Change dir
int ftp_cwd(struct ftp_info *info, int (*updatefn)(void *, int, char *), void *updateinfo, char *path)
{
	int reply;
	char *utf8_path = NULL;

	*info->fi_serverr = 0;

	// Convert path to UTF-8 if server supports UTF-8
	utf8_path = ftp_convert_path_to_utf8(path, info);

	reply = _ftpa(info, FTPFLAG_ASYNCH, "CWD %s", utf8_path ? utf8_path : path);
	if (reply < 600)
		reply = _getreply(info, 0, updatefn, updateinfo);

	if (reply / 100 != COMPLETE)
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);

	// Clean up converted path
	if (utf8_path)
		ftp_codesets_free(utf8_path);

	return reply;
}

// Delete file
int ftp_dele(struct ftp_info *info, char *name)
{
	int reply;
	char *utf8_name = NULL;

	*info->fi_serverr = 0;

	// Convert name to UTF-8 if server supports UTF-8
	utf8_name = ftp_convert_path_to_utf8(name, info);

	reply = ftpa(info, "DELE %s", utf8_name ? utf8_name : name);

	if (reply / 100 != COMPLETE)
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);

	// Clean up converted name
	if (utf8_name)
		ftp_codesets_free(utf8_name);

	return reply;
}

// Change transfer type to binary
int ftp_image(struct ftp_info *info)
{
	return ftp(info, "TYPE I\r\n");
}

// Return file date
int ftp_mdtm(struct ftp_info *info, char *name)
{
	int reply;
	char *utf8_name = NULL;

	*info->fi_serverr = 0;

	// Convert name to UTF-8 if server supports UTF-8
	utf8_name = ftp_convert_path_to_utf8(name, info);

	reply = ftpa(info, "MDTM %s", utf8_name ? utf8_name : name);

	if (reply >= 500 && reply <= 502)
		info->fi_flags |= FTP_NO_MDTM;
	else if (reply / 100 != COMPLETE)
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);

	// Clean up converted name
	if (utf8_name)
		ftp_codesets_free(utf8_name);

	return reply;
}

// Make new dir
int ftp_mkd(struct ftp_info *info, char *name)
{
	int reply;
	char *utf8_name = NULL;

	*info->fi_serverr = 0;

	// Convert name to UTF-8 if server supports UTF-8
	utf8_name = ftp_convert_path_to_utf8(name, info);

	reply = ftpa(info, "MKD %s", utf8_name ? utf8_name : name);

	if (reply / 100 != COMPLETE)
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);

	// Clean up converted name
	if (utf8_name)
		ftp_codesets_free(utf8_name);

	return reply;
}

//
//	Request extended passive data port details.
//
int ftp_epsv(struct ftp_info *info)
{
	int reply;

	*info->fi_serverr = 0;

	reply = ftp(info, "EPSV\r\n");

	if ((reply >= 500 && reply <= 502) || reply == 522)
	{
		D(bug("** setting NO_EPSV\n"));
		info->fi_flags |= FTP_NO_EPSV;
	}
	else if (reply / 100 != COMPLETE)
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);

	return reply;
}

//
//	Request data port details to connect to.
//
//	Note: This is also used to break data connections
//	in site-site transfers.
//
int ftp_pasv(struct ftp_info *info)
{
	int reply;

	*info->fi_serverr = 0;

	reply = ftp(info, "PASV\r\n");

	if (reply >= 500 && reply <= 502)
	{
		D(bug("** setting NO_PASV\n"));
		info->fi_flags |= FTP_NO_PASV;
	}
	else if (reply / 100 != COMPLETE)
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);

	return reply;
}

// Send PORT command
int ftp_port(struct ftp_info *info, unsigned long flags, struct sockaddr_in *addr)
{
	int reply;
	int ip1, ip2, ip3, ip4, p1, p2, port;

	if (!addr)
		return 0;

	if (!(flags & PORT_QUIET))
		*info->fi_serverr = 0;

	if (sscanf(Inet_NtoA(addr->sin_addr.s_addr), "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4) != 4)
		return 0;

	port = ntohs(addr->sin_port);
	p1 = port / 256;
	p2 = port % 256;

	reply = _ftpa(info, flags, "PORT %d,%d,%d,%d,%d,%d", ip1, ip2, ip3, ip4, p1, p2);

	if (!(flags & PORT_QUIET) && reply / 100 != COMPLETE)
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);

	return reply;
}

#if 0
int ftp_port( struct ftp_info *info, unsigned long flags, unsigned char *host, unsigned char *port )
{
int reply;

if (!port || !host)
	return 0;

if	(!(flags & PORT_QUIET))
	*info->fi_serverr = 0;

reply = _ftpa( info, flags, "PORT %d,%d,%d,%d,%d,%d",
	*host++, *host++, *host++, *host,
	*port++, *port );

if	(!(flags & PORT_QUIET) && reply / 100 != COMPLETE)
	stccpy( info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1 );

return reply;
}
#endif

// Return current dir
int ftp_pwd(struct ftp_info *info)
{
	return ftp(info, "PWD\r\n");
}

// Rename file or dir
int ftp_rename(struct ftp_info *info, char *oldname, char *newname)
{
	int reply;
	char *utf8_oldname = NULL;
	char *utf8_newname = NULL;

	*info->fi_serverr = 0;

	// Convert names to UTF-8 if server supports UTF-8
	utf8_oldname = ftp_convert_path_to_utf8(oldname, info);
	utf8_newname = ftp_convert_path_to_utf8(newname, info);

	// Can TIMEOUT
	reply = ftpa(info, "RNFR %s", utf8_oldname ? utf8_oldname : oldname);

	if (reply / 100 != CONTINUE)
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);
	else
	{
		// Can TIMEOUT
		reply = ftpa(info, "RNTO %s", utf8_newname ? utf8_newname : newname);

		if (reply / 100 != COMPLETE)
			stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);
	}

	// Clean up converted names
	if (utf8_oldname)
		ftp_codesets_free(utf8_oldname);
	if (utf8_newname)
		ftp_codesets_free(utf8_newname);

	return reply;
}

// Restart next transfer at this offset
int ftp_rest(struct ftp_info *info, unsigned int offset)
{
	int reply;

	*info->fi_serverr = 0;

	reply = ftpa(info, "REST %lu", offset);

	// Microsoft servers always say, "504 Reply marker must be 0."
	if ((reply >= 500 && reply <= 502) || reply == 504)
		info->fi_flags |= FTP_NO_REST;
	else if (reply / 100 != CONTINUE)
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);

	return reply;
}

// Remove directory
int ftp_rmd(struct ftp_info *info, char *name)
{
	int reply;
	char *utf8_name = NULL;

	*info->fi_serverr = 0;

	// Convert name to UTF-8 if server supports UTF-8
	utf8_name = ftp_convert_path_to_utf8(name, info);

	reply = ftpa(info, "RMD %s", utf8_name ? utf8_name : name);

	if (reply / 100 != COMPLETE)
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);

	// Clean up converted name
	if (utf8_name)
		ftp_codesets_free(utf8_name);

	return reply;
}

// Return file size
int ftp_size(struct ftp_info *info, char *name)
{
	int reply;
	char *utf8_name = NULL;

	*info->fi_serverr = 0;

	// Convert name to UTF-8 if server supports UTF-8
	utf8_name = ftp_convert_path_to_utf8(name, info);

	reply = ftpa(info, "SIZE %s", utf8_name ? utf8_name : name);

	if (reply >= 500 && reply <= 502)
		info->fi_flags |= FTP_NO_SIZE;
	else if (reply / 100 != COMPLETE)
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);

	// Clean up converted name
	if (utf8_name)
		ftp_codesets_free(utf8_name);

	return reply;
}

// Return system type
int ftp_syst(struct ftp_info *info)
{
	return ftp(info, "SYST\r\n");
}

/*************************************************************
 *	3.3.99 GP
 *	Send the ftp command string
 *
 *	This function expects the cmd to be terminated by \r\n
 *
 *	The alternative does not work on all servers. The \r\n MUST be sent as
 *	part of the main command
 *
 *	Not work: send( info->fi_cs, (char *)cmd, len, 0 );
 *		  send( info->fi_cs, "\r\n", 2, 0 );
 *
 **************************************************************/

static BOOL ftp_tls_session_active(struct ftp_tls_session *session)
{
	return session && session->active;
}

static int ftp_socket_write_all(struct ftp_tls_session *tls_session, int socket, const char *buf, int len)
{
	int done = 0;

	while (done < len)
	{
		int written;

		if (ftp_tls_session_active(tls_session))
			written = ftp_tls_write(tls_session, buf + done, len - done);
		else
			written = send(socket, (char *)buf + done, len - done, 0);

		if (written <= 0)
			return 0;

		done += written;
	}

	return 1;
}

static int _ftp(struct ftp_info *info, unsigned long flags, const char *cmd)
{
	struct opusftp_globals *ogp;
	int len;

	// Valid?
	if (!info)
		return 600;

	if (!cmd)
		return ftp_command_error(info, FTPERR_BAD_COMMAND, "FTP command is empty");

	ogp = info->fi_og;

	// Socket used by non-owner task?
	if (info->fi_task != FindTask(0))
		return ftp_command_error(info, FTPERR_BAD_COMMAND, "FTP command used by wrong task");

	// Log outgoing commands if debugging is turned on
	// NOOPs are never logged, passwords are hidden
	if (!info->fi_doing_noop && ogp && ogp->og_oc.oc_log_debug)
		logprintf("--> %s\n", strnicmp(cmd, "PASS ", 5) ? cmd : "PASS .....");

	len = strlen(cmd);
	if (!ftp_socket_write_all(&info->fi_control_tls, info->fi_cs, cmd, len))
		return ftp_command_error(info, FTPERR_SOCKET_FAIL, "Could not write FTP command");

	// Don't get reply if this is an asynchronous command, simply return 0
	if (flags & FTPFLAG_ASYNCH)
		return 0;

	// Can TIMEOUT
	return getreply(info);
}

int ftp(struct ftp_info *info, const char *cmd)
{
	return _ftp(info, 0, cmd);
}

/*************************************************************/

static int ftp_command_error(struct ftp_info *info, int err, const char *msg)
{
	if (info)
	{
		info->fi_reply = 600;
		info->fi_errno = err;

		if (err != FTPERR_SOCKET_FAIL)
			errno = 0;

		if (msg)
		{
			stccpy(info->fi_iobuf, msg, IOBUFSIZE + 1);
			stccpy(info->fi_serverr, msg, IOBUFSIZE + 1);
		}
	}

	return 600;
}

/*************************************************************/

static const char *ftp_tls_failure_string(struct ftp_tls_session *session)
{
	if (ftp_tls_failure_is_verify(session))
		return GetString(locale, MSG_FTP_TLS_VERIFY_FAILED);

	return GetString(locale, MSG_FTP_TLS_FAILED);
}

static BOOL ftp_tls_failure_is_verify(struct ftp_tls_session *session)
{
	switch (ftp_tls_session_error(session))
	{
	case FTP_TLS_ERROR_VERIFY_PATHS:
	case FTP_TLS_ERROR_HOSTNAME:
	case FTP_TLS_ERROR_VERIFY_RESULT:
		return TRUE;
	}

	return FALSE;
}

/*************************************************************/

//
//	Send an FTP command and return it's reply code.
//	Store the result string in info->fi_iobuf
//
//	Return error 600 for local formatting errors
//
static int _vftpa(struct ftp_info *info, unsigned long flags, const char *fmt, va_list ap)
{
	char *buf;
	int reply;
	int written;

	if (!fmt)
		return ftp_command_error(info, FTPERR_BAD_COMMAND, "FTP command is empty");

	if ((buf = AllocVec(FTP_COMMAND_BUFSIZE, MEMF_CLEAR)))
	{
		written = vsnprintf(buf, FTP_COMMAND_BUFSIZE - 2, fmt, ap);

		if (written < 0 || written >= FTP_COMMAND_BUFSIZE - 2)
		{
			reply = ftp_command_error(info, FTPERR_COMMAND_TOO_LONG, "FTP command is too long");
		}
		else if (ftp_parse_has_eol(buf))
		{
			reply = ftp_command_error(info, FTPERR_BAD_COMMAND, "FTP command contains a line break");
		}
		else
		{
			// add cr lf to terminate command for ftp()
			strcat(buf, "\r\n");

			reply = _ftp(info, flags, buf);
		}

		FreeVec(buf);
	}
	else
		reply = ftp_command_error(info, FTPERR_NO_MEM, "Out of memory");

	return reply;
}

// Calls internal vftpa

static int vftpa(struct ftp_info *info, const char *fmt, va_list ap)
{
	return _vftpa(info, 0, fmt, ap);
}

// Internal ftpa - supports flags

int _ftpa(struct ftp_info *info, unsigned long flags, const char *fmt, ...)
{
	va_list ap;
	int reply;

	va_start(ap, fmt);
	reply = _vftpa(info, flags, fmt, ap);
	va_end(ap);

	return reply;
}

int ftpa(struct ftp_info *info, const char *fmt, ...)
{
	va_list ap;
	int reply;

	va_start(ap, fmt);
	reply = vftpa(info, fmt, ap);
	va_end(ap);

	return reply;
}

/*************************************************************
 *	abort_recv() -  Abort a receive transaction
 *
 *	From the Berkeley FTP code:
 *	Send IAC in urgent mode instead of DM because UNIX places oob mark
 *	after urgent byte rather than before as now is protocol
 */

void ftp_abor(struct ftp_info *info)
{
	unsigned char iac_ip[] = {IAC, IP, IAC, DM, 'A', 'B', 'O', 'R', '\r', '\n'};
	struct opusftp_globals *ogp = info->fi_og;

	if (ftp_tls_session_active(&info->fi_control_tls))
	{
		_ftpa(info, FTPFLAG_ASYNCH, "ABOR");
		return;
	}

	D(bug("--> IAC,IP\n"));
	send(info->fi_cs, iac_ip, 2, 0);

	D(bug("--> IAC (MSG_OOB)\n"));
	send(info->fi_cs, iac_ip + 2, 1, MSG_OOB);

	D(bug("--> DM\n"));

	if (ogp->og_oc.oc_log_debug)
		logprintf("--> ABOR\n");

	send(info->fi_cs, iac_ip + 3, 7, 0);
}

static int open_data_socket(struct ftp_info *info)
{
	int ts;

	if ((ts = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		info->fi_errno = FTPERR_SOCKET_FAIL;

	return ts;
}

static int ftp_socket_read(struct ftp_tls_session *tls_session, int socket, void *buf, int len)
{
	if (ftp_tls_session_active(tls_session))
		return ftp_tls_read(tls_session, buf, len);

	return recv(socket, buf, len, 0);
}

static int ftp_socket_read_pending(struct ftp_tls_session *tls_session, int socket, void *buf, int len)
{
	if (!ftp_tls_session_active(tls_session) || ftp_tls_pending(tls_session) <= 0)
	{
		(void)socket;
		(void)buf;
		(void)len;
		return -2;
	}

	return ftp_socket_read(tls_session, socket, buf, len);
}

static void ftp_close_data_socket(struct ftp_tls_session *tls_session, int socket)
{
	ftp_tls_session_cleanup(tls_session);
	CloseSocket(socket);
}

static const char *ftp_data_mode_name(int mode)
{
	switch (mode)
	{
	case FTP_DATAMODE_EPSV:
		return "EPSV";
	case FTP_DATAMODE_PASV:
		return "PASV";
	case FTP_DATAMODE_PORT:
		return "PORT";
	default:
		return "unknown";
	}
}

static void ftp_log_data_mode(struct opusftp_globals *ogp, struct ftp_info *info)
{
	const char *mode = ftp_data_mode_name(info ? info->fi_data_mode : FTP_DATAMODE_NONE);

	D(bug("FTP data connection mode: %s\n", mode));

	if (ogp && ogp->og_oc.oc_log_debug)
		logprintf("-- Data connection: %s\n", mode);
}

static void ftp_ipv4_to_octets(struct in_addr addr, unsigned int octets[4])
{
	unsigned long value = ntohl(addr.s_addr);

	octets[0] = (value >> 24) & 255;
	octets[1] = (value >> 16) & 255;
	octets[2] = (value >> 8) & 255;
	octets[3] = value & 255;
}

static void ftp_ipv4_to_string(struct in_addr addr, char *buffer)
{
	unsigned int octets[4];

	ftp_ipv4_to_octets(addr, octets);
	sprintf(buffer, "%u.%u.%u.%u", octets[0], octets[1], octets[2], octets[3]);
}

static BOOL ftp_get_control_peer_address(struct ftp_info *info, struct sockaddr_in *peer_addr)
{
#ifdef __amigaos4__
	socklen_t peer_len = sizeof(*peer_addr);
#else
	LONG peer_len = sizeof(*peer_addr);
#endif

	if (!info || !peer_addr)
		return FALSE;

	return getpeername(info->fi_cs, (struct sockaddr *)peer_addr, &peer_len) >= 0;
}

static BOOL ftp_use_pasv_control_peer(struct opusftp_globals *ogp,
									  struct ftp_info *info,
									  struct sockaddr_in *address,
									  BOOL force_retry)
{
	struct sockaddr_in peer_addr;
	unsigned int pasv[4];
	unsigned int peer[4];
	BOOL use_peer;

	if (!info || !address)
		return FALSE;

	if (!ftp_get_control_peer_address(info, &peer_addr))
		return FALSE;

	ftp_ipv4_to_octets(address->sin_addr, pasv);
	ftp_ipv4_to_octets(peer_addr.sin_addr, peer);

	use_peer = force_retry || ftp_parse_pasv_should_use_control_peer(pasv, peer);
	if (!use_peer)
		return FALSE;

	{
		char pasv_text[16];
		char peer_text[16];

		ftp_ipv4_to_string(address->sin_addr, pasv_text);
		ftp_ipv4_to_string(peer_addr.sin_addr, peer_text);

		if (!strcmp(pasv_text, peer_text))
			return FALSE;

		if (force_retry)
		{
			D(bug("PASV retry using control peer %s instead of %s\n", peer_text, pasv_text));

			if (ogp && ogp->og_oc.oc_log_debug)
				logprintf("-- PASV retry using control peer %s instead of %s\n", peer_text, pasv_text);
		}
		else
		{
			D(bug("PASV advertised %s, using control peer %s\n", pasv_text, peer_text));

			if (ogp && ogp->og_oc.oc_log_debug)
				logprintf("-- PASV advertised %s, using control peer %s\n", pasv_text, peer_text);
		}

		address->sin_family = peer_addr.sin_family;
		address->sin_addr = peer_addr.sin_addr;
		return TRUE;
	}

	return FALSE;
}

//
//	Open data connection
//	Returns -1 upon failure; a socket descriptor upon success.
//
static int opendataconn(struct opusftp_globals *ogp, struct ftp_info *info)
{
	int ts = -1;							// Temporary socket
	LONG len = sizeof(struct sockaddr_in);	// Length of socket address
	int reply;								// Reply from FTP commands
	int bad_pasv = FALSE;					// Passive unavailable?

	info->fi_data_mode = FTP_DATAMODE_NONE;

#if defined(__AROS__) && defined(__x86_64__)
	if (!(info->fi_flags & FTP_NO_PASV))
		info->fi_flags |= FTP_PASSIVE;
#endif

	// PASV known not to work on this site?
	if (info->fi_flags & FTP_NO_PASV)
		bad_pasv = TRUE;

	// PASV option not enabled?
	if (!(info->fi_flags & FTP_PASSIVE))
		bad_pasv = TRUE;

	// Create temporary socket
	if ((ts = open_data_socket(info)) >= 0)
	{
		// Passive mode?  Try it first.
		if (!bad_pasv)
		{
			// Prefer EPSV. It avoids the NAT-sensitive host address in PASV replies.
			if (!(info->fi_flags & FTP_NO_EPSV))
			{
				int epsvreply;
				unsigned int port;
				struct sockaddr_in epsv_addr;
#ifdef __amigaos4__
				socklen_t peer_len = sizeof(epsv_addr);
#else
				LONG peer_len = sizeof(epsv_addr);
#endif

				epsvreply = ftp_epsv(info);

				switch (epsvreply)
				{
				case 229:  // correct reply
					if (epsv_to_port(&port, info->fi_iobuf) &&
						getpeername(info->fi_cs, (struct sockaddr *)&epsv_addr, &peer_len) >= 0)
					{
						epsv_addr.sin_port = htons(port);

						if (connect(ts, (struct sockaddr *)&epsv_addr, sizeof(epsv_addr)) >= 0)
						{
							info->fi_addr = epsv_addr;
							info->fi_data_mode = FTP_DATAMODE_EPSV;
							ftp_log_data_mode(ogp, info);
							return (ts);
						}
					}

					info->fi_flags |= FTP_NO_EPSV;
					CloseSocket(ts);
					ts = -1;
					break;

				case 421:  // socket closed by timeout
					CloseSocket(ts);
					return (-1);

				default:
					D(bug("Epsv failed returns %ld\n", (long)epsvreply));
				}
			}

			if (ts < 0 && (ts = open_data_socket(info)) < 0)
				return (-1);

			// Send PASV command to FTP server
			if (!(info->fi_flags & FTP_NO_PASV))
			{
				int pasvreply;

				pasvreply = ftp_pasv(info);

				switch (pasvreply)
				{
				case 227:  // correct reply
					if (pasv_to_address(&info->fi_addr, info->fi_iobuf))
					{
						ftp_use_pasv_control_peer(ogp, info, &info->fi_addr, FALSE);

						if (connect(ts, (struct sockaddr *)&info->fi_addr, sizeof(info->fi_addr)) >= 0)
						{
							info->fi_data_mode = FTP_DATAMODE_PASV;
							ftp_log_data_mode(ogp, info);
							return (ts);
						}

						if (ftp_use_pasv_control_peer(ogp, info, &info->fi_addr, TRUE))
						{
							CloseSocket(ts);
							ts = -1;

							if ((ts = open_data_socket(info)) >= 0 &&
								connect(ts, (struct sockaddr *)&info->fi_addr, sizeof(info->fi_addr)) >= 0)
							{
								info->fi_data_mode = FTP_DATAMODE_PASV;
								ftp_log_data_mode(ogp, info);
								return (ts);
							}
						}
					}

					bad_pasv = TRUE;
					if (ts >= 0)
						CloseSocket(ts);
					ts = -1;
					break;

				case 421:  // socket closed by timeout

					CloseSocket(ts);
					return (-1);

				default:
					D(bug("Pasv failed returns %ld\n", (long)pasvreply));
					bad_pasv = TRUE;
				}
			}
		}

		// Sendport mode - Passive may have just failed above

		if (bad_pasv || (info->fi_flags & FTP_NO_PASV))
		{
			if (ts < 0 && (ts = open_data_socket(info)) < 0)
				return (-1);

			getsockname(info->fi_cs, (struct sockaddr *)&info->fi_addr, &len);

			// Set port to zero so the system will pick one
			info->fi_addr.sin_port = 0;

			if (bind(ts, (struct sockaddr *)&info->fi_addr, sizeof(info->fi_addr)) >= 0)
			{
				// The system will now fill in the port number it picked
				if (getsockname(ts, (struct sockaddr *)&info->fi_addr, &len) >= 0)
				{
					if (listen(ts, 1) >= 0)
					{
						// Can TIMEOUT
						// reply = ftp_port( info, 0, (char *)&info->fi_addr.sin_addr, (char *)&info->fi_addr.sin_port
						// );
						reply = ftp_port(info, 0, &info->fi_addr);

						if (reply / 100 == COMPLETE)
						{
							info->fi_data_mode = FTP_DATAMODE_PORT;
							ftp_log_data_mode(ogp, info);
							return (ts);
						}
					}
					else
					{
						D(bug("** listen fail\n"));
						info->fi_errno = FTPERR_LISTEN_FAIL;
					}
				}
				else
				{
					D(bug("** getsockname fail\n"));
					info->fi_errno = FTPERR_GETSOCKNAME_FAIL;
				}
			}
			else
			{
				D(bug("** bind fail\n"));
				info->fi_errno = FTPERR_BIND_FAIL;
			}
		}

		CloseSocket(ts);
	}
	else
		info->fi_errno = FTPERR_SOCKET_FAIL;

	return (-1);
}

//
//	To transfer data, we do these things in order as specifed by
//	the RFC.
//
//	First, we tell the other side to set up a data line.  This
//	is done by opendataconn() which sets up  the socket.  When we do that,
//	the other side detects a connection  attempt, so it knows we're
//	there.  Then tell the other side (by using listen()) that we're
//	willing to receive a connection going to our side.
//
//	Initialize the data connection prior to a transfer operation
//	Send the PORT command followed by the optional REST command for a resume
//	Finally send the LIST, RETR, or STOR command
//
//	Returns >= 0 (socket) for success
//	Returns -2 if cmd failed
//	Returns -3 under some (forgotten) circumstances
//	Returns -1 for other errors
//
static int dataconna(struct ftp_info *info,
					 struct ftp_tls_session *data_tls_session,
					 int restart,
					 BOOL allow_complete_reply,
					 const char *fmt,
					 ...)
{
	struct opusftp_globals *ogp = info->fi_og;
	va_list ap;						  // For varargs
	int ts, ds = -1;				  // Temporary socket, Data socket
	struct linger linger = {1, 120};  // Socket option
	int tos = IPTOS_THROUGHPUT;		  // Another socket option
	struct sockaddr_in from;		  // Socket address
	int reply;
	BOOL okay = FALSE;

	if ((ts = opendataconn(ogp, info)) >= 0)
	{
		okay = TRUE;

		// Have the system make an effort to deliver any unsent data
		// even after we close the connection
		setsockopt(ts, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));

		// Data connection is a non-interactive data stream, so
		// high throughput is desired, at the expense of low
		// response time
		setsockopt(ts, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(tos));

		// Resuming a transfer?
		if (restart)
		{
			// Can we use REST command?
			if (!(info->fi_flags & FTP_NO_REST))
			{
				// Can TIMEOUT
				ftp_rest(info, restart);
			}
		}

		// Result if something failed after opendataconn and before here
		ds = -3;
	}

	if (okay)
	{
		va_start(ap, fmt);

		// Send main command - Can TIMEOUT
		reply = vftpa(info, fmt, ap);
		va_end(ap);

		if (reply / 100 == PRELIM ||
			(allow_complete_reply && (info->fi_flags & FTP_FEAT_MLST) && reply == 200))
		{
			LONG len = sizeof(from);

			// Sendport mode?
			if (info->fi_data_mode == FTP_DATAMODE_PORT)
			{
				ds = accept(ts, (struct sockaddr *)&from, &len);

				CloseSocket(ts);
			}

			// Passive mode?
			else
			{
				ds = ts;
			}

			if (ds >= 0 && data_tls_session && ftp_tls_mode_uses_data_tls(info->fi_tls_mode))
			{
				if (!ftp_tls_connect_reuse(data_tls_session,
										   ds,
										   info->fi_tls_host,
										   info->fi_tls_verify_peer,
										   &info->fi_control_tls))
				{
					errno = 0;
					info->fi_errno =
						ftp_tls_failure_is_verify(data_tls_session) ? FTPERR_TLS_VERIFY_FAIL : FTPERR_TLS_FAIL;
					stccpy(info->fi_serverr, ftp_tls_failure_string(data_tls_session), IOBUFSIZE + 1);
					CloseSocket(ds);
					ds = -1;
				}
			}
		}
		else
		{
			CloseSocket(ts);
			ds = -2;
		}
	}

	return ds;
}

#ifdef DEBUG
/*static void timeit( struct update_info *ui, int bytes )
{
unsigned char sizebuf[48] = "";
int  time;
int  rate;

#ifdef __AROS__
struct Device *TimerBase = (struct Device *)GetTimerBase();
#else
struct Library *TimerBase = GetTimerBase();
#endif
#ifdef __amigaos4__
struct TimerIFace *ITimer = (struct TimerIFace *)GetInterface(TimerBase,"main",1,NULL);
#endif

GetSysTime( &ui->ui_curr );
//gettimeofday(&ui->ui_curr, NULL);

if	((time = ui->ui_curr.tv_secs - ui->ui_start.tv_secs) == 0)
	time = 1;

rate = bytes / time;
cat_bytes( sizebuf,bytes );
strcat( sizebuf, " " );
cat_bytes( sizebuf, rate );
strcat( sizebuf, "/s" );

D(bug("time %ld done %s\n",time,sizebuf));

#ifdef __amigaos4__
DropInterface((struct Interface *)ITimer);
#endif
}*/
#endif

/*********************************************************************
 *
 *	Set network timeout for tcp stack calls
 *	Ensure is is a valid value and NOT 0 since this will
 *	cause a busy wait type state for the TCP get data loop
 *	and hog processor time
 *********************************************************************/

static VOID set_timeout(struct ftp_info *info, struct timeval *timer)
{
	timer->tv_secs = info->fi_timeout;
	timer->tv_micro = 0;

	// check to make sure there is a valid timeout

	if (timer->tv_secs == 0)
		timer->tv_secs = 60;
}

#define WBUFSIZE (16 * 1024)  // disk buffer size for buffered read

/*************************************************************/

//
//	Get a remote file, calling the update function after each block arrives
//	Returns the total bytes received.
//	fi_errno MUST be checked for errors
//	Returns -2 for AmigaDos errors - Use IoErr() to report/handle them
//	Returns -3 for FTP server errors - Use the fi_reply and fi_iobuf fields to report/handle them
//	Returns -1 for rare unlikely errors - Could cause DisplayBeep() etc...
//	now uses IOBUFSIZE
//
//	Update callback must specify 0xffffffff for total and length if REST fails
//	Update callback must specify 0xffffffff for unknown total
//	Update callback must specify 0 for length of final call
//
unsigned int get(struct ftp_info *info,
				 int (*updatefn)(void *, unsigned int, unsigned int),
				 void *updateinfo,
				 char *remote_path,
				 char *local_path,
				 BOOL restart)
{
	// Needed because socket library base is in our task's tc_userdata field
	// struct opusftp_globals *ogp = info->fi_og;
	unsigned int total = 0xffffffff;  // Length of file
	unsigned int bytes = 0;			  // Bytes received so far
	APTR f;							  // Output file
	int ds;							  // Data socket
	int b;							  // Byte count
	int reply;						  // FTP reply
	fd_set rd, ex;
	ULONG flags;
	struct timeval timer = {0};
	int display_bytes;
	int done;
	struct ftp_tls_session data_tls;

	D(bug("get() '%s' -> '%s'\n", remote_path, local_path));

	// Valid?
	if (!info)
		return 0;

	// No abort/error yet
	info->fi_aborted = 0;
	info->fi_errno = 0;
	info->fi_ioerr = 0;
	*info->fi_serverr = 0;
	errno = 0;

	// More validity
	if (!remote_path || !local_path)
	{
		info->fi_errno |= FTPERR_XFER_RARERR;
		return 0;
	}

	// init counters etc
	display_bytes = done = 0;
	ftp_tls_session_init(&data_tls);

	// open output file for writing
	if ((f = OpenBuf((char *)local_path, restart ? MODE_OLDFILE : MODE_NEWFILE, WBUFSIZE)))
	{
		// Resuming?  So find out where to resume from
		if (restart)
		{
			SeekBuf(f, 0, OFFSET_END);
			bytes = SeekBuf(f, 0, OFFSET_CURRENT);
		}

		// get connected  - bytes is a market flag 0/x for RETR/REST
		if ((ds = dataconna(info, &data_tls, bytes, FALSE, "RETR %s", remote_path)) < 0)
		{
			// Source (server error)?
			if (ds == -2 || ds == -3)
			{
				info->fi_errno |= FTPERR_XFER_SRCERR;
				stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);
			}
			else
				info->fi_errno |= FTPERR_XFER_RARERR;
		}
		else
		{
			char *p;

			// Reset bytes if REST failed
			if (bytes && (info->fi_flags & FTP_NO_REST))
			{
				if (updatefn)
					(*updatefn)(updateinfo, 0xffffffff, 0xffffffff);

				SeekBuf(f, 0, OFFSET_BEGINNING);
				bytes = 0;
			}

			// First update tells callback where we're starting from
			if (updatefn)
				(*updatefn)(updateinfo, total, bytes);

			// Does the RETR reply contain the file length?
			if ((p = strstr(info->fi_iobuf, " bytes)")))
				if (isdigit(*--p))
				{
					while (isdigit(*p--))
						;
					total = atoi(p + 2);
				}

			// do the transfer

			FD_ZERO(&rd);
			FD_ZERO(&ex);

			// set network timeout for the select wait call
			set_timeout(info, &timer);

			// loop fetch tcp data and save it
			while (!done)
			{
				BOOL have_data = FALSE;

				// Note: these masks must be set before every call and
				// are all cleared by select wait. Examine the masks
				// afterwards to see what was set

				b = ftp_socket_read_pending(&data_tls, ds, info->fi_iobuf, IOBUFSIZE);

				if (b != -2)
					have_data = TRUE;
				else
				{
					FD_SET(ds, &rd);
					FD_SET(ds, &ex);
					flags = SIGBREAKF_CTRL_D;

					if (WaitSelect(ds + 1, &rd, NULL, &ex, &timer, &flags) >= 0)
					{
						// Is there some data ready for us?
						if (FD_ISSET(ds, &rd))
						{
							// then get it and store it
							b = ftp_socket_read(&data_tls, ds, info->fi_iobuf, IOBUFSIZE);
							have_data = TRUE;
						}

						// did we get a signal to abort?
						if (!done && (flags & SIGBREAKF_CTRL_D))
						{
							D(bug("*** get() CTRL-D SIGNAL ***\n"));
							info->fi_abortsignals = 0;
							info->fi_aborted = 1;
							ftp_abor(info);	 // NEEDED why not just close socket?
							done = TRUE;
						}

						// did we get an exception? Other end closed connection maybe
						if (FD_ISSET(ds, &ex))
						{
							D(bug("** get() socket exception\n"));

							// has been aborted from remote ?
							done = TRUE;
						}
					}
					else
					{
						// some socket error -ve a 0 == timeout
						D(bug("** get() WaitSelect error\n"));
						done = TRUE;
					}
				}

				if (have_data)
				{
					if (b > 0)
					{
						// save data
						if (WriteBuf(f, info->fi_iobuf, b) == b)
						{
							bytes += b;

							// progress bar uprate
							display_bytes += b;

							if (display_bytes >= UPDATE_BYTE_LIMIT)
							{
								if (updatefn)
									(*updatefn)(updateinfo, total, display_bytes);

								display_bytes = 0;
							}
						}
						// Write Error
						else
						{
							info->fi_errno |= FTPERR_XFER_DSTERR;
							info->fi_ioerr = IoErr();
							info->fi_aborted = 1;
							ftp_abor(info);
							done = TRUE;
						}
					}
					else
						done = TRUE;
				}
			}

			// Final progress bar redraw at 100% (if it finished)
			if (!info->fi_aborted && !info->fi_errno && updatefn)
				(*updatefn)(updateinfo, total, 0);

			// D(bug( "--> close(%ld)\n", ds ));
			ftp_close_data_socket(&data_tls, ds);

#ifdef DEBUG
//			if	(ui)
//			timeit( ui, bytes );
#endif

			// Get reply to socket closure  Can TIMEOUT
			reply = getreply(info);

			// If transfer was aborted, read the ABOR reply (426)
			// (This could get the reply to RETR (226) but it don't matter)
			if (info->fi_aborted && reply != 421)
				reply = getreply(info);

			// RETR successful? - Don't set error if we forced it!
			// (This could get the reply to ABOR (225) but it don't matter)
			if (reply / 100 != COMPLETE)
				if (!info->fi_errno)
				{
					info->fi_errno |= FTPERR_XFER_SRCERR;
					stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);
				}
		}

		CloseBuf(f);

		// Delete empty files if there was an error and we created it
		if (info->fi_errno && bytes == 0 && !restart)
			DeleteFile((char *)local_path);
	}
	else
	{
		info->fi_errno |= FTPERR_XFER_DSTERR;
		info->fi_ioerr = IoErr();
	}

	ftp_tls_session_cleanup(&data_tls);
	return bytes;
}

/*************************************************************
 *
 * Wait until data ready from socket and fill buffer
 *
 * Function returns number of chars read for success or 0,-1 for timeouts/errors.
 * also can return 0 for EOF condition I think!
 *
 */

static int iread(struct ftp_info *info, int skt, BOOL checkbreak, struct ftp_tls_session *tls_session)
{
	// struct opusftp_globals *ogp = info->fi_og;
	int retval = -1;
	fd_set rd, ex;
	ULONG flags;
	struct timeval timer = {0};

	retval = ftp_socket_read_pending(tls_session, skt, info->fi_bufiobuf, BUFIOBUFSIZE);
	if (retval != -2)
		return retval;

	// set network timeout for the select wait call
	set_timeout(info, &timer);

	FD_ZERO(&rd);
	FD_ZERO(&ex);

	FD_SET(skt, &rd);
	FD_SET(skt, &ex);
	flags = checkbreak ? SIGBREAKF_CTRL_D : 0;

	//  WaitSelect returns number of ready file descriptors if
	//  successful.  Zero (0) if a timeout occurred.
	//  (-1) upon error.

	if (WaitSelect(skt + 1, &rd, 0L, &ex, &timer, &flags) >= 0)
	{
		D(bug("recv - "));

		// is data available from the socket?
		if (FD_ISSET(skt, &rd))
		{
			// number of bytes read if successful else -1.
			// can be 0 for EOF
			retval = ftp_socket_read(tls_session, skt, info->fi_bufiobuf, BUFIOBUFSIZE);
		}

#ifdef DEBUG
		// has the socket been closed?
		if (FD_ISSET(skt, &ex))
			D(bug("** iread() socket exception\n"));
#endif

		// whatever check if user has hit CTRL_D
		if (flags & SIGBREAKF_CTRL_D)
		{
			info->fi_abortsignals = 0;
			D(bug("*** iread() CTRL-D SIGNAL ***\n"));
			retval = -1;
		}
	}

	return (retval);
}

/*************************************************************
 *
 *	Flush the input socket buffer
 */

static void flush_socket_buffer(struct ftp_info *info)
{
	info->fi_buffer_left = 0;
	info->fi_buffer_pos = info->fi_bufiobuf;
}

/*************************************************************
 *
 *	The function for reading from socket skt, char-by-char. If
 *	there is anything in the buffer, the character is returned from the
 *	buffer. Otherwise, refill the buffer and return the first
 *	character.
 *
 * 	Function returns 1 for success or 0,-1 for timeouts/errors.
 */

int buf_sgetc(struct ftp_info *info, int skt, BOOL checkbreak, struct ftp_tls_session *tls_session, char *ret)
{
	int res;

	if (info->fi_buffer_left)
	{
		--info->fi_buffer_left;
		*ret = *info->fi_buffer_pos++;
	}
	else
	{
		info->fi_buffer_pos = info->fi_bufiobuf;
		info->fi_buffer_left = 0;

		// Fill buffer - res no chars read, 0 = timeout, -1 = error
		if ((res = iread(info, skt, checkbreak, tls_session)) <= 0)
			return (res);

		info->fi_buffer_left = res - 1;
		*ret = *info->fi_buffer_pos++;
	}

	return 1;
}

/*************************************************************
 *
 *	The sgets function gets a string from the specified socket.
 *	Characters are copied from the socket to the buffer until a
 *	new line (\n) has been copied, or length-1 characters have been
 *	copied, or the end-of-file is hit.  In any case, if the read succeeds,
 *	the buffer is terminated with a trailing null byte (\0).
 *	** If the read fails, the buffer will not be modified. **
 *	If the end-of-file is hit before any bytes are read, a NULL
 *	pointer is returned.  If an I/O error occurs, a NULL pointer is
 *	returned and additional information is placed in the external
 *	integer  errno. (GJP Now not specifically checked.)
 *	If no I/O error occurs and at least one byte was read from
 *	 the file, the buffer argument is returned.
 */

static char *buf_sgets(struct ftp_info *info, int bytes, int skt, BOOL checkbreak, struct ftp_tls_session *tls_session)
{
	char *buf, *retval, *p;
	char c;

	buf = info->fi_iobuf;
	retval = p = buf;

	do
	{
		// Buffer full?
		if (p - buf == bytes - 1)
			break;

		//  EOF or error?
		if (buf_sgetc(info, skt, checkbreak, tls_session, &c) <= 0)
		{
			retval = NULL;
			break;
		}
		*p++ = c;

	} while (c != '\n');

	// Null terminate
	*p = 0;

	return (retval);
}

/*******************old_functions ************************************/

//
//	Single character get char function
//	Returns char for success
//	Returns -1 for error
//	Returns -2 for timeout
//	Returns -3 for control-D break
//
//	checkabort_time is set either to default or special time for
//	getput only on dest STOR command
//
static int sgetc(struct ftp_info *info, int skt, int checkabort_time, struct ftp_tls_session *tls_session)
{
	// struct opusftp_globals *ogp = info->fi_og;
	int retval = -1;
	unsigned char c;
	fd_set rd, ex;
	ULONG flags;
	int n, nds;
	struct timeval t = {0};

	// Valid?
	if (!info || skt < 0)
	{
		D(bug("** sgetc invalid!\n"));
		return retval;
	}

	n = ftp_socket_read_pending(tls_session, skt, &c, 1);
	if (n == 1)
		return c;
	if (n != -2)
		return retval;

	// set network timeout for the select wait call

	if (checkabort_time == 2)
		t.tv_secs = 5;	// special return soon to update progress bar
	else
		set_timeout(info, &t);

	t.tv_micro = 0;

	FD_ZERO(&rd);
	FD_ZERO(&ex);

	FD_SET(skt, &rd);
	FD_SET(skt, &ex);

	flags = checkabort_time ? SIGBREAKF_CTRL_D : 0;

	//	WaitSelect returns number of ready file descriptors if
	//	successful.  Zero (0) if a timeout occurred.
	//	(-1) upon error.

	if ((nds = WaitSelect(skt + 1, &rd, 0L, &ex, &t, &flags)) >= 0)
	{
		if (nds == 0)  // timeout or abort
		{
			D(bug("** sgetc() WaitSelect timeout\n"));
			retval = -2;
		}

		// abort button hit?
		if (flags & SIGBREAKF_CTRL_D)
		{
			info->fi_abortsignals = 0;
			D(bug("*** sgetc() CTRL-D SIGNAL ***\n"));
			retval = -3;
		}

		// is data ready?
		if (FD_ISSET(skt, &rd))
			if ((n = ftp_socket_read(tls_session, skt, &c, 1)) == 1)
				retval = c;

#ifdef DEBUG
		// has the socket been closed?
		if (FD_ISSET(skt, &ex))
			D(bug("** sgetc() socket exception\n"));
#endif
	}

	return retval;
}

//
//	Older single character based sgets function
//
//	Returns buffer pointer for success
//	Returns 0 for failure
//
//	info->fi_reply = 800 for timeout
//	info->fi_reply = 700 for control-D abort
//
static char *sgets(
	struct ftp_info *info, char *iobuf, int bytes, int skt, int checkabort_time, struct ftp_tls_session *tls_session)
{
	char *retval, *p;
	int c;

	if (bytes < 1)
		return NULL;

	retval = p = iobuf;

	do
	{
		// Buffer full?
		if (p - iobuf == bytes - 1)
			break;

		// Read a character
		c = sgetc(info, skt, checkabort_time, tls_session);

		// EOF or error?
		if (c == -1)
		{
			retval = 0;
			break;
		}
		// Timeout?
		else if (c == -2)
		{
			info->fi_reply = 800;
			retval = 0;
			break;
		}
		// Aborted?
		else if (c == -3)
		{
			info->fi_reply = 700;
			retval = 0;
			break;
		}

		*p++ = c;

	} while (c != '\n');

	// Null terminate
	*p = 0;

	return retval;
}

//
//	Get a remote directory one line at a time,
//	calling the update function for each line
//
//	Returns 0 for success, -2 for command failure, -1 for other error
//
//	Update function and structure can now be defined as you like
//	(So please don't change the interface to this function)
//
//	#define QUOTE_HACK and 'ls foo bar' will become 'ls "foo bar"'.
//	This has never been any help for me but it's left in in case
//	it ever helps for any site.  Also to prove to users who sometimes
//	demand it, that it doesn't work.
//
int list(struct ftp_info *info,
		 int (*updatefn)(void *, const char *),
		 void *updateinfo,
		 const char *cmd,
		 const char *path)
{
	// Needed because socket library base is in our task's tc_userdata field
	// struct opusftp_globals *ogp = info->fi_og;
	int retval = -1;	// error - 0 is no error
	int ds;				// Data socket
	int reply;			// FTP reply
	int updateret = 1;	// Return value from update function
	BOOL allow_complete_reply;
	struct ftp_tls_session data_tls;

#ifdef QUOTE_HACK
	// For quote checking
	char env;
#endif

	// Valid?
	if (!info || !cmd)
		return -1;

	// No abort/error yet
	info->fi_aborted = 0;
	info->fi_errno = 0;
	info->fi_ioerr = 0;
	*info->fi_serverr = 0;
	errno = 0;

	ftp_tls_session_init(&data_tls);
	allow_complete_reply = FALSE;

// Establish data connection
#ifdef QUOTE_HACK
	if (path && *path && GetVar("DOpus/ftp_quotes", &env, 1, 0) != -1)
		ds = dataconna(info, &data_tls, 0, allow_complete_reply, "%s \"%s\"", cmd, path);
	else
#endif
		if (path && *path)
		ds = dataconna(info, &data_tls, 0, allow_complete_reply, "%s %s", cmd, path);
	else
		ds = dataconna(info, &data_tls, 0, allow_complete_reply, "%s", cmd);

	if (ds >= 0)
	{
		retval = 0;

		// Safety
		flush_socket_buffer(info);

		// Read a line and call the callback function
		while (updateret > 0 && buf_sgets(info, IOBUFSIZE, ds, TRUE, &data_tls))
		{
			if (updatefn)
			{
				updateret = (*updatefn)(updateinfo, info->fi_iobuf);

				if (updateret <= 0)
				{
					D(bug("** list update %ld\n", (long)updateret));
					retval = updateret;
				}
			}
		}

		// D(bug( "--> close(%ld)\n", ds ));
		ftp_close_data_socket(&data_tls, ds);

		// Get reply to socket closure -  Can TIMEOUT
		reply = getreply(info);

		// Server error during list?  (should be pretty rare)
		if (reply / 100 != COMPLETE)
		{
			info->fi_errno |= FTPERR_XFER_SRCERR;
			stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);
		}

		retval = ftp_listcmd_result_after_reply(retval, reply);
	}
	else
	{
		if (ds == -2)
			retval = -2;
	}

	ftp_tls_session_cleanup(&data_tls);
	return retval;
}

/*************************************************************
 *	Put a file, calling the update function after each block arrives
 *		Returns the total bytes sent or -1 for failure
 *		'abort' will be true if aborted, false if an error occurred
 *	1/97 changed FRead to Read
 */

unsigned int put(struct ftp_info *info,
				 int (*updatefn)(void *, unsigned int, unsigned int),
				 void *updateinfo,
				 char *local_path,
				 char *remote_path,
				 unsigned int restart)
{
	// struct opusftp_globals *ogp = info->fi_og;
	unsigned int bytes = 0;
	APTR f;	 // Output file
	int ds;	 // Data socket
	int b;	 // Byte count
	fd_set wd, ex;
	ULONG flags = SIGBREAKF_CTRL_D;
	struct timeval timer = {0};
	BOOL done = FALSE;
	int display_bytes = 0;
	struct ftp_tls_session data_tls;

	// Valid?
	if (!info)
		return 0;

	// No abort/error yet
	info->fi_aborted = 0;
	info->fi_errno = 0;
	info->fi_ioerr = 0;
	*info->fi_serverr = 0;
	errno = 0;

	// More validity
	if (!remote_path || !local_path)
	{
		info->fi_errno |= FTPERR_XFER_RARERR;
		return 0;
	}

	ftp_tls_session_init(&data_tls);

	if ((f = OpenBuf((char *)local_path, MODE_OLDFILE, WBUFSIZE)))
	{
		// Can TIMEOUT
		if ((ds = dataconna(info, &data_tls, restart, FALSE, "STOR %s", remote_path)) < 0)
		{
			// Destination (server) error?
			if (ds == -2 || ds == -3)
			{
				info->fi_errno |= FTPERR_XFER_DSTERR;
				stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);
			}
			else
				info->fi_errno |= FTPERR_XFER_RARERR;
		}
		else
		{
			// Resuming a transfer?
			if (restart)
			{
				// Start at 0 if REST failed
				if (info->fi_flags & FTP_NO_REST)
				{
					if (updatefn)
						(*updatefn)(updateinfo, 0xffffffff, 0xffffffff);

					bytes = restart = 0;
				}

				// Otherwise seek to restart position
				else
				{
					SeekBuf(f, restart, OFFSET_BEGINNING);
					bytes = restart;
				}
			}

			// First update tells callback where we're starting from
			if (updatefn)
				(*updatefn)(updateinfo, 0xffffffff, bytes);

			// Transfer

			FD_ZERO(&wd);
			FD_ZERO(&ex);

			// set network timeout for the select wait call
			set_timeout(info, &timer);

			while (!done)
			{
				// Note: these masks must be set before every call and
				// are all cleared by select wait. Examine the masks
				// afterwards to see what was set

				FD_SET(ds, &wd);
				FD_SET(ds, &ex);
				flags = SIGBREAKF_CTRL_D;

				if (WaitSelect(ds + 1, 0L, &wd, &ex, &timer, &flags) >= 0)
				{
					if (FD_ISSET(ds, &wd))
					{
						if ((b = ReadBuf(f, info->fi_iobuf, IOBUFSIZE)) > 0)
						{
							if (!ftp_socket_write_all(&data_tls, ds, info->fi_iobuf, b))
							{
								info->fi_errno |= FTPERR_XFER_DSTERR;
								done = TRUE;
								continue;
							}
							bytes += b;

							if ((display_bytes += b) >= UPDATE_BYTE_LIMIT || bytes < UPDATE_BYTE_LIMIT)
							{
								if (updatefn)
									(*updatefn)(updateinfo, 0xffffffff, display_bytes);

								display_bytes = 0;
							}
						}
						else
						{
							done = TRUE;

							if (b < 0)
							{
								info->fi_errno |= FTPERR_XFER_SRCERR;
								info->fi_ioerr = IoErr();
							}
						}
					}

					if (!done && (flags & SIGBREAKF_CTRL_D))
					{
						D(bug("*** put() CTRL-D SIGNAL ***\n"));
						info->fi_abortsignals = 0;
						info->fi_aborted = TRUE;
						done = TRUE;
					}

					if (FD_ISSET(ds, &ex))
					{
						D(bug("** put() socket exception\n"));
						info->fi_abortsignals = 0;
						done = TRUE;
					}
				}
				else
				{
					// some socket error -ve  a 0== timeout
					D(bug("** put() WaitSelect error\n"));
					done = TRUE;
				}
			}

			// Final progress bar redraw at 100% (if it finished)
			if (!info->fi_aborted && !info->fi_errno && updatefn)
				(*updatefn)(updateinfo, 0xffffffff, 0);

			// Close data socket
			// D(bug( "--> close(%ld)\n", ds ));
			ftp_close_data_socket(&data_tls, ds);

			// Get reply to socket closure -  Can TIMEOUT
			if (getreply(info) / 100 != COMPLETE)
			{
				info->fi_errno |= FTPERR_XFER_DSTERR;
				stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);
			}
		}
		CloseBuf(f);
	}
	else
	{
		info->fi_errno |= FTPERR_XFER_SRCERR;
		info->fi_ioerr = IoErr();
	}

	ftp_tls_session_cleanup(&data_tls);
	return bytes;
}

/*************************************************************/

//
//	Attempt to get the host by address or by name
//
int gethost(struct opusftp_globals *ogp, struct sockaddr_in *remote_addr, char *host)
{
	BOOL retval = FALSE;
	struct hostent *he;

	remote_addr->sin_addr.s_addr = inet_addr((char *)host);

	if (remote_addr->sin_addr.s_addr != -1)
	{
		remote_addr->sin_family = AF_INET;

		retval = TRUE;
	}
	else if ((he = gethostbyname((char *)host)))
	{
		remote_addr->sin_family = he->h_addrtype;

		memcpy(&remote_addr->sin_addr, he->h_addr_list[0], he->h_length);

		retval = TRUE;
	}

	return retval;
}

/*************************************************************/

static int ftp_secure_control(struct ftp_info *info,
							  int (*updatefn)(void *, int, char *),
							  void *updateinfo,
							  const char *host)
{
	int reply;

	if (updatefn)
		(*updatefn)(updateinfo, 0 /*attempt*/, GetString(locale, MSG_FTP_SECURING_CONNECTION));

	*info->fi_serverr = 0;

	reply = _ftpa(info, FTPFLAG_ASYNCH, "AUTH TLS");
	if (reply < 600)
		reply = _getreply(info, 0, updatefn, updateinfo);

	if (reply != 234)
	{
		errno = 0;
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);
		return 0;
	}

	if (!ftp_tls_connect(&info->fi_control_tls, info->fi_cs, host, info->fi_tls_verify_peer))
	{
		errno = 0;
		info->fi_errno =
			ftp_tls_failure_is_verify(&info->fi_control_tls) ? FTPERR_TLS_VERIFY_FAIL : FTPERR_TLS_FAIL;
		stccpy(info->fi_iobuf, ftp_tls_failure_string(&info->fi_control_tls), IOBUFSIZE + 1);
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);
		return 0;
	}

	flush_socket_buffer(info);
	return 1;
}

static int ftp_prepare_protected_data(struct ftp_info *info,
									  int (*updatefn)(void *, int, char *),
									  void *updateinfo)
{
	int reply;

	if (!ftp_tls_mode_uses_data_tls(info->fi_tls_mode))
		return 1;

	reply = _ftpa(info, FTPFLAG_ASYNCH, "PBSZ 0");
	if (reply < 600)
		reply = _getreply(info, 0, updatefn, updateinfo);

	if (reply / 100 != COMPLETE)
	{
		errno = 0;
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);
		return 0;
	}

	reply = _ftpa(info, FTPFLAG_ASYNCH, "PROT P");
	if (reply < 600)
		reply = _getreply(info, 0, updatefn, updateinfo);

	if (reply / 100 != COMPLETE)
	{
		errno = 0;
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);
		return 0;
	}

	return 1;
}

/*************************************************************/

//
//	Connect a remote host to our local socket
//	local_addr will be used as data_addr in later calls
//
//	Returns  1 for successful connection
//	Returns  0 for failure that can be retried
//	Returns -1 for fatal failure
//	Returns -2 for (fatal) failure where gethost fails with no error number
//
int connect_host(struct ftp_info *info, int (*updatefn)(void *, int, char *), void *updateinfo, char *host, int port)
{
	struct opusftp_globals *ogp = info->fi_og;
	struct servent *se;
	int retval = 0;
	struct sockaddr_in remote_addr = {0};
	LONG len = sizeof(struct sockaddr_in);
	int reply;	// FTP reply

	if (info->fi_cs >= 0)
	{
		ftp_tls_session_cleanup(&info->fi_control_tls);
		CloseSocket(info->fi_cs);
	}
	else
		ftp_tls_session_cleanup(&info->fi_control_tls);

	info->fi_cs = -1;
	info->fi_errno = 0;
	*info->fi_serverr = 0;
	errno = 0;

	if (host)
		stccpy(info->fi_tls_host, host, FTP_TLS_HOST_BUFSIZE);
	else
		*info->fi_tls_host = 0;

	if (updatefn)
		(*updatefn)(updateinfo, 0 /*attempt*/, GetString(locale, MSG_LOOKING_UP));

	// Lookup the FTP server's address
	if (gethost(ogp, &remote_addr, host))
	{
		if (updatefn)
			(*updatefn)(updateinfo, 0 /*attempt*/, GetString(locale, MSG_HOST_FOUND));

		// If no port specified, use the standard ftp port 21
		if (!port)
		{
			if ((se = getservbyname("ftp", "tcp")))
				port = se->s_port;
			else
				port = htons(21);
		}
		else
			port = htons(port);

		// Specify the port in the address structure
		remote_addr.sin_port = port;

		// Create the control socket
		if ((info->fi_cs = socket(remote_addr.sin_family, SOCK_STREAM, 0)) >= 0)
		{
			// Connect the control socket to the FTP server
			// D(bug( "--> connect()\n" ));
			D(bug("** control connect(0x%08lx)\n", (unsigned long)remote_addr.sin_addr.s_addr));

			if (connect(info->fi_cs, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) >= 0)
			{
				// Now get the FTP server's address from the socket itself
				// Since the connection may have come via firewalls, this name is more meaningful
				// and possibly different to that used to establish the connection
				if (getsockname(info->fi_cs, (struct sockaddr *)&info->fi_addr, &len) >= 0)
				{
					int tos = IPTOS_LOWDELAY;
					D(bug("  conhost: %lx\n", (unsigned long)info->fi_addr.sin_addr.s_addr));

					// Control connection is somewhat interactive, so quick response
					// is desired
					setsockopt(info->fi_cs, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(tos));

					// We want Out-of-band data to appear in the regular stream,
					// since we can handle TELNET
					// (actually, we can't handle it yet so it's disabled)
					// setsockopt( info->fi_cs, SOL_SOCKET, SO_OOBINLINE, (char *)&on, sizeof(on) );

					if (updatefn)
						(*updatefn)(updateinfo, 0 /*attempt*/, GetString(locale, MSG_READING_STARTUP));

					// Read FTP protocol startup message - Can TIMEOUT
					while ((reply = _getreply(info, 0, updatefn, updateinfo)) == 120)
						;

					if (reply == 220)
					{
						if (!ftp_tls_mode_uses_control_tls(info->fi_tls_mode) ||
							ftp_secure_control(info, updatefn, updateinfo, host))
							retval = 1;
					}
				}
			}

			// If something went wrong close and invalidate the control socket
			if (retval <= 0)
			{
				ftp_tls_session_cleanup(&info->fi_control_tls);
				CloseSocket(info->fi_cs);
				info->fi_cs = -1;
			}
		}
	}
	else
	{
		// Call to gethost() failed, but errno not set
		if (errno == 0)
			retval = -2;
	}

	if (retval <= 0)
	{
		// Connect may have been aborted by user - reset control-c signals
		SetSignal(0L, SIGBREAKF_CTRL_C);

		if (retval != -2)
		{
			switch (errno)
			{
			case ENETDOWN:
			case ENETUNREACH:
			case ECONNABORTED:
			case ETIMEDOUT:
			case ECONNREFUSED:
			case EHOSTDOWN:
				retval = 0;
				break;

			default:
				retval = -1;
				break;
			}
		}
	}

	return retval;
}

/*************************************************************/

//
//	This is safe to call if already disconnected and socket is already closed
//	although if we know the connection has been lost we should use shutdown()
//	instead as close() will attempt to first send all queued data
//
void disconnect_host(struct ftp_info *info)
{
	// struct opusftp_globals *ogp = info->fi_og;

	if (info->fi_cs >= 0)
	{
		ftp_tls_session_cleanup(&info->fi_control_tls);
		CloseSocket(info->fi_cs);
		info->fi_cs = -1;
	}
}

/*************************************************************/

//
//	This must be called when a connection is disconnected by the server (421 response)
//
void lostconn(struct ftp_info *info)
{
	// struct opusftp_globals *ogp = info->fi_og;

	if (info->fi_cs >= 0)
	{
		ftp_tls_session_cleanup(&info->fi_control_tls);
		shutdown(info->fi_cs, 1 + 1);  // Send and receive disallowed
		CloseSocket(info->fi_cs);	   // Close control socket
		info->fi_cs = -1;			   // Descriptor invalid
	}
}

/*************************************************************/

//
//	Login
//
//	Returns  0 if okay
//	Returns -1 if USER failed
//	Returns -2 if PASS failed
//	Returns -3 if TLS data protection setup failed
//
int login(struct ftp_info *info, int (*updatefn)(void *, int, char *), void *updateinfo, char *user, char *passwd)
{
	BOOL retval = 0;
	int reply;

	errno = 0;

	// Clear previous error text
	*info->fi_serverr = 0;

	reply = _ftpa(info, FTPFLAG_ASYNCH, "USER %s", user);
	if (reply < 600)
		reply = _getreply(info, 0, updatefn, updateinfo);
	reply /= 100;

	if (reply != CONTINUE && reply != COMPLETE)
	{
		stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);
		retval = -1;
	}

	if (reply == CONTINUE)
	{
		reply = _ftpa(info, FTPFLAG_ASYNCH, "PASS %s", passwd);
		if (reply < 600)
			reply = _getreply(info, 0, updatefn, updateinfo);
		reply /= 100;

		if (reply != COMPLETE)
		{
			stccpy(info->fi_serverr, info->fi_iobuf, IOBUFSIZE + 1);
			retval = -2;
		}
	}

	if (retval == 0 && !ftp_prepare_protected_data(info, updatefn, updateinfo))
		retval = -3;

	return retval;
}

void logout(struct ftp_info *info)
{
	// Can TIMEOUT - do we care?
	ftp(info, "QUIT\r\n");
}

/*************************************************************/

//
//	Internal version of getreply(), takes additional flags
//	(Used in ftp_recursive.c)
//
//	Get reply to an FTP command
//	This version works properly with multi-line replies
//
//	Results:
//		Success:	Returns the reply code from the ftp server - for multi-line
//			replies, this will be the code from the last line
//		Failure:	600 = Error, 700 = Break, 800 = Timeout
//
//		421 "Service not available, remote server has closed connection"
//		 is now returned for any error as in berkley ftp code
//
//	What do we do in situation for Timeout on read? AD simply calls this
//	an error and reports 421 which closes connection.
//	Should this not handle this and maybe just ignore it or resend command?
//
int _getreply(struct ftp_info *info, unsigned long flags, int (*updatefn)(void *, int, char *), void *updateinfo)
{
	struct opusftp_globals *ogp = info->fi_og;
	int actual;				  // Bytes received
	int first = 1;			  // Is this the first line?
	int multi = 0;			  // Multi-line reply
	int currcode;			  // Current line's code
	int checkabort_time = 0;  // By default don't check for aborts on replies
	char *iobuf;

	// Need to allocate buffer for quiet io?
	if (flags & GETREPLY_QUIET)
	{
		if (!(iobuf = AllocVec(IOBUFSIZE + 1, MEMF_CLEAR)))
		{
			info->fi_reply = 600;
			return info->fi_reply;
		}
	}
	else
		iobuf = info->fi_iobuf;

	if (info->fi_reply < 0 || (info->fi_abortsignals & SIGBREAKF_CTRL_D))	 // not accept timeouts
	{
		checkabort_time = 1;

		// -2 is special marker for DEST STOR to return often to update progress bar
		if (info->fi_reply == -2)
			checkabort_time = 2;
	}
	else
		info->fi_reply = 600;  // Error by default

	// Safety
	flush_socket_buffer(info);

	// Read one line at a time
	do
	{
		// EOF or error
		if ((sgets(info, iobuf, IOBUFSIZE, info->fi_cs, checkabort_time, &info->fi_control_tls)) == 0)
		{
			if (info->fi_reply == 800)
				info->fi_errno = FTPERR_TIMEOUT;
			else
				info->fi_errno = FTPERR_FAKE_421;

			if (info->fi_reply < 700)
				info->fi_reply = 600;

			break;
		}

		actual = strlen(iobuf);

		// NULL terminate
		iobuf[actual] = 0;

		// Got at least reply code?
		if (actual >= 3 && isdigit(iobuf[0]) && isdigit(iobuf[1]) && isdigit(iobuf[2]))
			currcode = iobuf[0] * 100 + iobuf[1] * 10 + iobuf[2] - '0' * 111;
		else
			currcode = 0;

		// Output reply, omitting the code if debug is off
		// No output if doing_noop
		if (!info->fi_doing_noop)
		{
			if (!ogp->og_oc.oc_log_debug && currcode)
			{
				if (updatefn && updateinfo)
					(*updatefn)(updateinfo, -1, iobuf + 4);

				logprintf(iobuf + 4);
			}
			else
			{
				if (updatefn && updateinfo)
					(*updatefn)(updateinfo, -1, iobuf);

				logprintf(iobuf);
			}
		}

		if (currcode)
		{
			// Invalid code on any line ?
			if (currcode < 100 || currcode > 559)
			{
				info->fi_reply = 600;
				break;
			}

			// Code is okay so update it
			info->fi_reply = currcode;

			// Start of multi-line reply?
			if (iobuf[3] == '-')
			{
				if (!multi)
					multi = 1;
			}
			// End of multi-line reply?
			else if (multi)
				multi = 0;
		}

		// First block?
		if (first)
			first = 0;

		// Mangled code on last line of reply
		if (!multi && !currcode)
		{
			info->fi_reply = 600;
			break;
		}

	} while (multi);

	// Convert ERROR codes to 'connection lost' code
	if (info->fi_reply >= 600)
		info->fi_reply = 421;

	// Need to free buffer for quiet io?
	if (flags & GETREPLY_QUIET)
		if (iobuf)
			FreeVec(iobuf);

	return info->fi_reply;
}

/*************************************************************/

//
//	Get reply to an FTP command
//
int getreply(struct ftp_info *info)
{
	return _getreply(info, 0, 0, 0);
}

/*************************************************************/

//
//	Convert EPSV reply to port
//
BOOL epsv_to_port(unsigned int *port, const char *buf)
{
	return ftp_parse_epsv_port(buf, port) ? TRUE : FALSE;
}

//
//	Convert PASV reply to address
//
BOOL pasv_to_address(struct sockaddr_in *address, const char *buf)
{
	unsigned int values[6];
	int port;
	char data_ip_addr[16];

	if (!address || !buf)
		return FALSE;

	if (!ftp_parse_pasv_tuple(buf, values))
		return FALSE;

	sprintf(data_ip_addr, "%u.%u.%u.%u", values[0], values[1], values[2], values[3]);
	port = (values[4] << 8) + values[5];

	if (port <= 0 || port > 65535)
		return FALSE;

	address->sin_family = AF_INET;
	address->sin_addr.s_addr = inet_addr(data_ip_addr);
	address->sin_port = htons(port);

	return TRUE;
}

#if 0
BOOL pasv_to_address( struct sockaddr_in *address, const char *buf )
{
int            n;		// Scratch for address numbers
const char    *in;		// Convert address from here
unsigned char *out;		// Convert address to here
BOOL           good = TRUE;

if (!strncmp( "227 Entering Passive Mode (", buf, 27 ))
	{
	in = buf + 27;
	out = (unsigned char *)&address->sin_addr;

	// Parse it...
	while	(good)
		{
		if	(!isdigit(*in))
			good = FALSE;

		for	(n = 0; good; ++in)
			{
			if	(!isdigit(*in))
				break;

			n = n * 10 + *in - '0';

			if	(n > 255 || n <0)
				good = FALSE;
			}

		if	(!good)
			break;

		*out++ = n;

		if	(out == (unsigned char *)(1+&address->sin_addr))
			out = (unsigned char *)&address->sin_port;
		if	(out == (unsigned char *)(1+&address->sin_port))
			break;

		if	(*in++ != ',')
			good = FALSE;
		}
	}

return good;
}
#endif
