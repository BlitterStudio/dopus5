/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas

This program is free software; you can redistribute it and/or
modify it under the terms of the AROS Public License version 1.1.

*/

#include "ftp_sftp.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	#include <libssh2.h>
	#include <libssh2_sftp.h>

	#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
		#include "ftp.h"
		#include "ftp_opusftp.h"
		#include <dos/dos.h>
		#include <proto/dos.h>
		#include <proto/exec.h>
		#include <sys/filio.h>
		#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__)
			#include "ftp_tls.h"
		#endif
	#else
		#include <arpa/inet.h>
		#include <fcntl.h>
		#include <netdb.h>
		#include <netinet/in.h>
		#include <stdio.h>
		#include <sys/socket.h>
		#include <sys/time.h>
		#include <sys/types.h>
		#include <unistd.h>
	#endif
#endif

#define FTP_SFTP_XFER_BUFSIZE (16 * 1024)
#define FTP_SFTP_UNIX_TO_AMIGA_EPOCH 252460800UL
#define FTP_SFTP_INADDR_NONE 0xffffffffUL
#define FTP_SFTP_AMIGA_TICKS_PER_SECOND 50
#define FTP_SFTP_KNOWN_HOSTS "DOpus5:System/ftp_known_hosts"
#define FTP_SFTP_KNOWN_HOSTS_MAGIC "dopus5-sftp"
#define FTP_SFTP_KNOWN_HOSTS_LINE_BUFSIZE 4096
#define FTP_SFTP_HOSTKEY_HEX_BUFSIZE 4096

#if defined(FTP_SFTP_BACKEND_LIBSSH2)
static int ftp_sftp_libssh2_ready = 0;
static int ftp_sftp_libssh2_users = 0;

static void *ftp_sftp_buffer_alloc(size_t size)
{
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
	return AllocVec(size, MEMF_ANY);
#else
	return malloc(size);
#endif
}

static void ftp_sftp_buffer_free(void *buffer)
{
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
	FreeVec(buffer);
#else
	free(buffer);
#endif
}

static void ftp_sftp_connect_update(const struct ftp_sftp_connect_options *options, const char *text);

static int ftp_sftp_libssh2_acquire(const struct ftp_sftp_connect_options *options)
{
	int flags = 0;

	if (ftp_sftp_libssh2_users > 0)
	{
		++ftp_sftp_libssh2_users;
		return 1;
	}

	if (ftp_sftp_libssh2_ready)
	{
		++ftp_sftp_libssh2_users;
		return 1;
	}

#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__)
	ftp_sftp_connect_update(options, "SFTP: Opening AmiSSL...");
	if (!ftp_tls_backend_acquire())
		return 0;

	flags = LIBSSH2_INIT_NO_CRYPTO;
#endif

	ftp_sftp_connect_update(options, "SFTP: Initializing SSH crypto...");
	if (libssh2_init(flags))
	{
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__)
		ftp_tls_backend_release();
#endif
		return 0;
	}

	ftp_sftp_libssh2_ready = 1;
	ftp_sftp_libssh2_users = 1;
	return 1;
}

static void ftp_sftp_libssh2_release(void)
{
	if (ftp_sftp_libssh2_users <= 0)
		return;

	--ftp_sftp_libssh2_users;
	if (ftp_sftp_libssh2_users > 0)
		return;

	if (ftp_sftp_libssh2_ready)
	{
		libssh2_exit();
		ftp_sftp_libssh2_ready = 0;
	}

#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__)
	ftp_tls_backend_release();
#endif
}

#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
struct ftp_sftp_alloc_header
{
	unsigned long size;
	unsigned long pad;
};

static LIBSSH2_ALLOC_FUNC(ftp_sftp_libssh2_alloc)
{
	struct ftp_sftp_alloc_header *header;

	(void)abstract;
	if (!count)
		return NULL;

	header = AllocVec(sizeof(*header) + count, MEMF_PUBLIC);
	if (!header)
		return NULL;

	header->size = (unsigned long)count;
	header->pad = 0;
	return header + 1;
}

static LIBSSH2_FREE_FUNC(ftp_sftp_libssh2_free)
{
	(void)abstract;
	if (ptr)
		FreeVec(((struct ftp_sftp_alloc_header *)ptr) - 1);
}

static LIBSSH2_REALLOC_FUNC(ftp_sftp_libssh2_realloc)
{
	struct ftp_sftp_alloc_header *old_header;
	void *new_ptr;
	unsigned long copy_len;

	if (!ptr)
		return ftp_sftp_libssh2_alloc(count, abstract);

	if (!count)
	{
		ftp_sftp_libssh2_free(ptr, abstract);
		return NULL;
	}

	old_header = ((struct ftp_sftp_alloc_header *)ptr) - 1;
	new_ptr = ftp_sftp_libssh2_alloc(count, abstract);
	if (!new_ptr)
		return NULL;

	copy_len = old_header->size < count ? old_header->size : (unsigned long)count;
	if (copy_len)
		memcpy(new_ptr, ptr, copy_len);

	ftp_sftp_libssh2_free(ptr, abstract);
	return new_ptr;
}

static char *ftp_sftp_libssh2_strdup(const char *text, size_t len)
{
	char *copy = ftp_sftp_libssh2_alloc(len + 1, NULL);

	if (copy)
	{
		if (len)
			memcpy(copy, text, len);
		copy[len] = 0;
	}

	return copy;
}

static void ftp_sftp_libssh2_force_nonblocking(libssh2_socket_t socket)
{
	long value = 1;

	IoctlSocket((LONG)socket, FIONBIO, (char *)&value);
}

static void ftp_sftp_libssh2_yield(void)
{
	Delay(1);
}

static LIBSSH2_RECV_FUNC(ftp_sftp_libssh2_recv)
{
	ssize_t rc;
	int err;

	(void)abstract;

	ftp_sftp_libssh2_yield();
	ftp_sftp_libssh2_force_nonblocking(socket);
	errno = 0;
	rc = recv(socket, buffer, length, flags);
	if (rc > 0 || length == 0)
		return rc;
	if (rc == 0)
		return -EAGAIN;

	err = errno;
	if (!err || err == EINTR || err == EAGAIN || err == EWOULDBLOCK)
		return -EAGAIN;

	return -err;
}

static LIBSSH2_SEND_FUNC(ftp_sftp_libssh2_send)
{
	ssize_t rc;
	int err;

	(void)abstract;

	ftp_sftp_libssh2_yield();
	ftp_sftp_libssh2_force_nonblocking(socket);
	errno = 0;
	rc = send(socket, (APTR)buffer, length, flags);
	if (rc > 0 || length == 0)
		return rc;
	if (rc == 0)
		return -EAGAIN;

	err = errno;
	if (!err || err == EINTR || err == EAGAIN || err == EWOULDBLOCK)
		return -EAGAIN;

	return -err;
}
#else
static char *ftp_sftp_libssh2_strdup(const char *text, size_t len)
{
	char *copy = malloc(len + 1);

	if (copy)
	{
		if (len)
			memcpy(copy, text, len);
		copy[len] = 0;
	}

	return copy;
}
#endif

static void ftp_sftp_close_socket(int socket_fd)
{
	if (socket_fd < 0)
		return;

	#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
	CloseSocket(socket_fd);
	#else
	close(socket_fd);
	#endif
}

static void ftp_sftp_connect_update(const struct ftp_sftp_connect_options *options, const char *text)
{
	if (options && options->update && text)
	{
		options->update(options->userdata, text);
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
		Delay(1);
#endif
	}
}

static int ftp_sftp_connect_aborted(const struct ftp_sftp_connect_options *options)
{
	if (options && options->abort && options->abort(options->userdata))
	{
		errno = EINTR;
		return 1;
	}

	return 0;
}

static int ftp_sftp_timeout_secs(const struct ftp_sftp_connect_options *options)
{
	if (options && options->timeout_secs > 0)
		return options->timeout_secs;
	return 60;
}

static void ftp_sftp_copy_text(char *out, int outsize, const char *text)
{
	size_t len;

	if (!out || outsize <= 0)
		return;

	if (!text)
		text = "";

	len = strlen(text);
	if (len >= (size_t)outsize)
		len = (size_t)outsize - 1;
	if (len)
		memcpy(out, text, len);
	out[len] = 0;
}

static void ftp_sftp_set_detail(struct ftp_sftp_session *session, const char *text)
{
	if (session)
		ftp_sftp_copy_text(session->last_detail, sizeof(session->last_detail), text);
}

static void ftp_sftp_set_host_detail(struct ftp_sftp_session *session, const char *prefix, const char *host)
{
	size_t prefix_len;
	size_t host_len;

	if (!session)
		return;

	if (!prefix)
		prefix = "";
	if (!host)
		host = "";

	prefix_len = strlen(prefix);
	if (prefix_len > FTP_SFTP_ERROR_BUFSIZE)
		prefix_len = FTP_SFTP_ERROR_BUFSIZE;

	memcpy(session->last_detail, prefix, prefix_len);
	session->last_detail[prefix_len] = 0;

	host_len = strlen(host);
	if (host_len > FTP_SFTP_ERROR_BUFSIZE - prefix_len)
		host_len = FTP_SFTP_ERROR_BUFSIZE - prefix_len;
	if (host_len)
		memcpy(session->last_detail + prefix_len, host, host_len);
	session->last_detail[prefix_len + host_len] = 0;
}

static time_t ftp_sftp_now(void)
{
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
	struct DateStamp stamp;

	DateStamp(&stamp);
	return (time_t)(((unsigned long)stamp.ds_Days * 24UL * 60UL * 60UL) +
					((unsigned long)stamp.ds_Minute * 60UL) +
					((unsigned long)stamp.ds_Tick / FTP_SFTP_AMIGA_TICKS_PER_SECOND));
#else
	return time(NULL);
#endif
}

static time_t ftp_sftp_deadline(const struct ftp_sftp_connect_options *options)
{
	return ftp_sftp_now() + ftp_sftp_timeout_secs(options);
}

static int ftp_sftp_wait_seconds(time_t deadline)
{
	time_t now = ftp_sftp_now();
	int remaining;

	if (deadline <= now)
		return 0;

	remaining = (int)(deadline - now);
	return remaining > 1 ? 1 : remaining;
}

static int ftp_sftp_set_socket_nonblocking(int socket_fd, int enabled)
{
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
	long value = enabled ? 1 : 0;
	return IoctlSocket(socket_fd, FIONBIO, (char *)&value) == 0;
#else
	int flags = fcntl(socket_fd, F_GETFL, 0);

	if (flags < 0)
		return 0;

	if (enabled)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;

	return fcntl(socket_fd, F_SETFL, flags) == 0;
#endif
}

#if !(defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__))
static int ftp_sftp_socket_would_block(void)
{
	return errno == EINPROGRESS || errno == EWOULDBLOCK || errno == EAGAIN;
}
#endif

static int ftp_sftp_wait_socket(int socket_fd,
								LIBSSH2_SESSION *ssh,
								int for_connect,
								time_t deadline,
								const struct ftp_sftp_connect_options *options)
{
	if (socket_fd < 0)
		return 0;

	for (;;)
	{
		fd_set rd;
		fd_set wd;
		fd_set ex;
		struct timeval timeout;
		int seconds;
		int rc;
		int directions = 0;

		if (ftp_sftp_connect_aborted(options))
			return 0;

		seconds = ftp_sftp_wait_seconds(deadline);
		if (seconds <= 0)
		{
			errno = ETIMEDOUT;
			return 0;
		}

#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
		if (!for_connect)
		{
			Delay(1);
			return !ftp_sftp_connect_aborted(options);
		}
#endif

		FD_ZERO(&rd);
		FD_ZERO(&wd);
		FD_ZERO(&ex);
		FD_SET(socket_fd, &ex);

		if (for_connect)
			FD_SET(socket_fd, &wd);
		else
		{
			directions = libssh2_session_block_directions(ssh);
			if (!directions || (directions & LIBSSH2_SESSION_BLOCK_INBOUND))
				FD_SET(socket_fd, &rd);
			if (directions & LIBSSH2_SESSION_BLOCK_OUTBOUND)
				FD_SET(socket_fd, &wd);
		}

		timeout.tv_sec = seconds;
		timeout.tv_usec = 0;

#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
		{
			ULONG signals = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D;
			rc = WaitSelect(socket_fd + 1, &rd, &wd, &ex, &timeout, &signals);
			if (signals & (SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D))
			{
				errno = EINTR;
				return 0;
			}
		}
#else
		rc = select(socket_fd + 1, &rd, &wd, &ex, &timeout);
#endif

		if (rc < 0)
			return 0;

		if (ftp_sftp_connect_aborted(options))
			return 0;

		if (rc == 0)
		{
			if (ftp_sftp_now() >= deadline)
			{
				errno = ETIMEDOUT;
				return 0;
			}
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
			return 1;
#else
			continue;
#endif
		}

		if (FD_ISSET(socket_fd, &ex))
		{
			errno = ECONNABORTED;
			return 0;
		}

		if (FD_ISSET(socket_fd, &rd) || FD_ISSET(socket_fd, &wd))
			return 1;
	}
}

#if !(defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__))
static int ftp_sftp_socket_connected(int socket_fd)
{
	int error = 0;
	socklen_t error_len = sizeof(error);

	if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, (char *)&error, &error_len) < 0)
		return 0;

	if (error)
	{
		errno = error;
		return 0;
	}

	return 1;
}
#endif

static int ftp_sftp_open_socket(const char *host, int port, const struct ftp_sftp_connect_options *options)
{
	struct sockaddr_in remote_addr;
	struct hostent *he;
	int socket_fd;
	time_t deadline;

	if (ftp_sftp_connect_aborted(options))
		return -1;

	memset(&remote_addr, 0, sizeof(remote_addr));
	ftp_sftp_connect_update(options, "SFTP: Looking up...");
	remote_addr.sin_addr.s_addr = inet_addr((char *)host);
	if ((unsigned long)remote_addr.sin_addr.s_addr != FTP_SFTP_INADDR_NONE)
		remote_addr.sin_family = AF_INET;
	else if ((he = gethostbyname((char *)host)))
	{
		remote_addr.sin_family = he->h_addrtype;
		memcpy(&remote_addr.sin_addr, he->h_addr_list[0], he->h_length);
	}
	else
		return -1;

	if (ftp_sftp_connect_aborted(options))
		return -1;

	ftp_sftp_connect_update(options, "SFTP: Host found");

	if (port <= 0)
		port = 22;
	remote_addr.sin_port = htons(port);

	ftp_sftp_connect_update(options, "SFTP: Opening socket...");
	socket_fd = socket(remote_addr.sin_family, SOCK_STREAM, 0);
	if (socket_fd < 0)
		return -1;

	if (!ftp_sftp_set_socket_nonblocking(socket_fd, 1))
	{
		ftp_sftp_close_socket(socket_fd);
		return -1;
	}

	ftp_sftp_connect_update(options, "SFTP: TCP connecting...");
	deadline = ftp_sftp_deadline(options);
	errno = 0;
	if (connect(socket_fd, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0)
	{
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
		ftp_sftp_connect_update(options, "SFTP: TCP waiting...");
		if (!ftp_sftp_wait_socket(socket_fd, NULL, 1, deadline, options))
		{
			ftp_sftp_close_socket(socket_fd);
			return -1;
		}
#else
		if (!ftp_sftp_socket_would_block() ||
			!ftp_sftp_wait_socket(socket_fd, NULL, 1, deadline, options) ||
			!ftp_sftp_socket_connected(socket_fd))
		{
			ftp_sftp_close_socket(socket_fd);
			return -1;
		}
#endif
	}

	errno = 0;
	ftp_sftp_connect_update(options, "SFTP: TCP connected");
	return socket_fd;
}

static int ftp_sftp_handshake(LIBSSH2_SESSION *ssh, int socket_fd, const struct ftp_sftp_connect_options *options)
{
	int rc;
	time_t deadline;

	ftp_sftp_connect_update(options, "SFTP: Starting SSH...");
	deadline = ftp_sftp_deadline(options);
	while ((rc = libssh2_session_handshake(ssh, socket_fd)) == LIBSSH2_ERROR_EAGAIN)
	{
		if (!ftp_sftp_wait_socket(socket_fd, ssh, 0, deadline, options))
			return 0;
	}

	return rc == 0;
}

static int ftp_sftp_auth_password(LIBSSH2_SESSION *ssh,
								  int socket_fd,
								  const char *user,
								  const char *pass,
								  const struct ftp_sftp_connect_options *options)
{
	int rc;
	time_t deadline;

	ftp_sftp_connect_update(options, "SFTP: Authenticating...");
	deadline = ftp_sftp_deadline(options);
	while ((rc = libssh2_userauth_password(ssh, user, pass)) == LIBSSH2_ERROR_EAGAIN)
	{
		if (!ftp_sftp_wait_socket(socket_fd, ssh, 0, deadline, options))
			return 0;
	}

	return rc == 0;
}

static LIBSSH2_USERAUTH_KBDINT_RESPONSE_FUNC(ftp_sftp_keyboard_interactive_response)
{
	const char *pass = abstract && *abstract ? (const char *)*abstract : "";
	int i;

	(void)name;
	(void)name_len;
	(void)instruction;
	(void)instruction_len;

	for (i = 0; i < num_prompts; ++i)
	{
		size_t len = prompts && prompts[i].echo ? 0 : strlen(pass);

		responses[i].length = (unsigned int)len;
		responses[i].text = ftp_sftp_libssh2_strdup(pass, len);
		if (!responses[i].text)
			responses[i].length = 0;
	}
}

static int ftp_sftp_auth_keyboard_interactive(LIBSSH2_SESSION *ssh,
											  int socket_fd,
											  const char *user,
											  const char *pass,
											  const struct ftp_sftp_connect_options *options)
{
	int rc;
	time_t deadline;
	void **abstract;
	void *old_abstract = NULL;

	if (!ssh || !user || !*user || !pass || !*pass)
		return 0;

	ftp_sftp_connect_update(options, "SFTP: Keyboard auth...");
	abstract = libssh2_session_abstract(ssh);
	if (abstract)
	{
		old_abstract = *abstract;
		*abstract = (void *)pass;
	}

	deadline = ftp_sftp_deadline(options);
	while ((rc = libssh2_userauth_keyboard_interactive(ssh, user, ftp_sftp_keyboard_interactive_response)) ==
		   LIBSSH2_ERROR_EAGAIN)
	{
		if (!ftp_sftp_wait_socket(socket_fd, ssh, 0, deadline, options))
		{
			if (abstract)
				*abstract = old_abstract;
			return 0;
		}
	}

	if (abstract)
		*abstract = old_abstract;

	return rc == 0;
}

static LIBSSH2_SFTP *ftp_sftp_init_subsystem(LIBSSH2_SESSION *ssh,
											 int socket_fd,
											 const struct ftp_sftp_connect_options *options)
{
	LIBSSH2_SFTP *sftp;
	time_t deadline;
#if !(defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__))
	time_t start;
	time_t now;
#endif
	int timeout_secs;
	int waits = 0;
	int last_elapsed = -1;
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
	int max_waits;
#endif

	ftp_sftp_connect_update(options, "SFTP: Starting SFTP...");
	timeout_secs = ftp_sftp_timeout_secs(options);
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
	max_waits = timeout_secs * FTP_SFTP_AMIGA_TICKS_PER_SECOND;
	if (max_waits <= 0)
		max_waits = FTP_SFTP_AMIGA_TICKS_PER_SECOND;
#endif
#if !(defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__))
	start = ftp_sftp_now();
#endif
	deadline = ftp_sftp_deadline(options);
	for (;;)
	{
		int elapsed;

#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
		elapsed = waits / FTP_SFTP_AMIGA_TICKS_PER_SECOND;
		if (elapsed != last_elapsed || (waits % FTP_SFTP_AMIGA_TICKS_PER_SECOND) == 0)
		{
			char status[64];

			sprintf(status, "SFTP: Waiting SFTP %lds", (long)elapsed);
			ftp_sftp_connect_update(options, status);
			last_elapsed = elapsed;
		}

		libssh2_session_set_blocking(ssh, 0);
		sftp = libssh2_sftp_init(ssh);
		if (sftp)
			return sftp;

		if (libssh2_session_last_errno(ssh) != LIBSSH2_ERROR_EAGAIN)
			return NULL;

		if (++waits >= max_waits)
		{
			errno = ETIMEDOUT;
			libssh2_session_set_last_error(ssh,
										   LIBSSH2_ERROR_TIMEOUT,
										   "Timed out starting SFTP subsystem");
			return NULL;
		}
#else
		sftp = libssh2_sftp_init(ssh);
		if (sftp)
			return sftp;

		if (libssh2_session_last_errno(ssh) != LIBSSH2_ERROR_EAGAIN)
			return NULL;

		now = ftp_sftp_now();
		elapsed = (int)(now - start);
		if (elapsed < 0)
			elapsed = 0;

		if (elapsed != last_elapsed || (waits++ % 10) == 0)
		{
			char status[64];

			sprintf(status, "SFTP: Waiting SFTP %lds", (long)elapsed);
			ftp_sftp_connect_update(options, status);
			last_elapsed = elapsed;
		}

		if (elapsed >= timeout_secs)
		{
			errno = ETIMEDOUT;
			libssh2_session_set_last_error(ssh,
										   LIBSSH2_ERROR_TIMEOUT,
										   "Timed out starting SFTP subsystem");
			return NULL;
		}
#endif

		if (!ftp_sftp_wait_socket(socket_fd, ssh, 0, deadline, options))
			return NULL;
	}
}

static int ftp_sftp_knownhost_key_type(int hostkey_type)
{
	switch (hostkey_type)
	{
	case LIBSSH2_HOSTKEY_TYPE_RSA:
		return hostkey_type;
	case LIBSSH2_HOSTKEY_TYPE_DSS:
		return hostkey_type;
#ifdef LIBSSH2_HOSTKEY_TYPE_ECDSA_256
	case LIBSSH2_HOSTKEY_TYPE_ECDSA_256:
		return hostkey_type;
#endif
#ifdef LIBSSH2_HOSTKEY_TYPE_ECDSA_384
	case LIBSSH2_HOSTKEY_TYPE_ECDSA_384:
		return hostkey_type;
#endif
#ifdef LIBSSH2_HOSTKEY_TYPE_ECDSA_521
	case LIBSSH2_HOSTKEY_TYPE_ECDSA_521:
		return hostkey_type;
#endif
#ifdef LIBSSH2_HOSTKEY_TYPE_ED25519
	case LIBSSH2_HOSTKEY_TYPE_ED25519:
		return hostkey_type;
#endif
	default:
		break;
	}

	return 0;
}

static const char *ftp_sftp_default_known_hosts_path(void)
{
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
	return FTP_SFTP_KNOWN_HOSTS;
#else
	static char path[512];
	const char *home = getenv("HOME");
	const char *name = "/.dopus5_ftp_known_hosts";

	if (home && *home && strlen(home) + strlen(name) < sizeof(path))
	{
		sprintf(path, "%s%s", home, name);
		return path;
	}

	return "dopus5_ftp_known_hosts";
#endif
}

static const char *ftp_sftp_known_hosts_path(const struct ftp_sftp_connect_options *options)
{
	if (options && options->known_hosts_path && *options->known_hosts_path)
		return options->known_hosts_path;

	return ftp_sftp_default_known_hosts_path();
}

static void ftp_sftp_trim_eol(char *line)
{
	size_t len;

	if (!line)
		return;

	len = strlen(line);
	while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
		line[--len] = 0;
}

static char *ftp_sftp_next_field(char **cursor)
{
	char *field;
	char *tab;

	if (!cursor || !*cursor)
		return NULL;

	field = *cursor;
	tab = strchr(field, '\t');
	if (tab)
	{
		*tab++ = 0;
		*cursor = tab;
	}
	else
		*cursor = NULL;

	return field;
}

static int ftp_sftp_hostkey_hex(char *out, int outsize, const char *hostkey, size_t hostkey_len)
{
	static const char hex[] = "0123456789abcdef";
	size_t i;

	if (!out || outsize <= 0 || !hostkey)
		return 0;

	if (hostkey_len > ((size_t)outsize - 1) / 2)
		return 0;

	for (i = 0; i < hostkey_len; ++i)
	{
		unsigned char byte = (unsigned char)hostkey[i];

		out[i * 2] = hex[(byte >> 4) & 0x0f];
		out[(i * 2) + 1] = hex[byte & 0x0f];
	}
	out[hostkey_len * 2] = 0;
	return 1;
}

static int ftp_sftp_known_hosts_check_line(char *line,
										   const char *host,
										   int port,
										   int key_type,
										   const char *key_hex)
{
	char *cursor;
	char *magic;
	char *line_host;
	char *line_port;
	char *line_type;
	char *line_key;

	ftp_sftp_trim_eol(line);
	cursor = line;
	magic = ftp_sftp_next_field(&cursor);
	line_host = ftp_sftp_next_field(&cursor);
	line_port = ftp_sftp_next_field(&cursor);
	line_type = ftp_sftp_next_field(&cursor);
	line_key = ftp_sftp_next_field(&cursor);

	if (!magic || !line_host || !line_port || !line_type || !line_key)
		return 0;
	if (strcmp(magic, FTP_SFTP_KNOWN_HOSTS_MAGIC))
		return 0;
	if (strcmp(line_host, host) || atoi(line_port) != port || atoi(line_type) != key_type)
		return 0;

	return strcmp(line_key, key_hex) ? -1 : 1;
}

#if defined(FTP_SFTP_TESTING)
int ftp_sftp_test_known_hosts_check_line(char *line,
										 const char *host,
										 int port,
										 int key_type,
										 const char *key_hex)
{
	return ftp_sftp_known_hosts_check_line(line, host, port, key_type, key_hex);
}
#endif

static int ftp_sftp_known_hosts_scan_file(struct ftp_sftp_session *session,
										  const char *path,
										  const char *host,
										  int port,
										  int key_type,
										  const char *key_hex,
										  int *matched,
										  char *line,
										  size_t line_size)
{
	int line_match;

	if (matched)
		*matched = 0;

	if (!path || !*path)
		return 1;

#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
	{
		BPTR file = Open((char *)path, MODE_OLDFILE);

		if (!file)
		{
			if (IoErr() == ERROR_OBJECT_NOT_FOUND)
				return 1;

			ftp_sftp_set_host_detail(session, "Could not read SFTP known-hosts file: ", path);
			return 0;
		}

		while (FGets(file, line, line_size))
		{
			line_match = ftp_sftp_known_hosts_check_line(line, host, port, key_type, key_hex);
			if (line_match > 0)
			{
				Close(file);
				if (matched)
					*matched = 1;
				return 1;
			}
			if (line_match < 0)
			{
				Close(file);
				ftp_sftp_set_host_detail(session, "SFTP host key changed for ", host);
				return 0;
			}
		}

		Close(file);
	}
#else
	{
		FILE *file = fopen(path, "r");

		if (!file)
		{
			if (errno == ENOENT)
				return 1;

			ftp_sftp_set_host_detail(session, "Could not read SFTP known-hosts file: ", path);
			return 0;
		}

		while (fgets(line, (int)line_size, file))
		{
			line_match = ftp_sftp_known_hosts_check_line(line, host, port, key_type, key_hex);
			if (line_match > 0)
			{
				fclose(file);
				if (matched)
					*matched = 1;
				return 1;
			}
			if (line_match < 0)
			{
				fclose(file);
				ftp_sftp_set_host_detail(session, "SFTP host key changed for ", host);
				return 0;
			}
		}

		fclose(file);
	}
#endif

	return 1;
}

static int ftp_sftp_known_hosts_check_file(struct ftp_sftp_session *session,
										   const char *path,
										   const char *host,
										   int port,
										   int key_type,
										   const char *key_hex,
										   int *matched)
{
	char *line;
	int result;

	if (matched)
		*matched = 0;

	line = ftp_sftp_buffer_alloc(FTP_SFTP_KNOWN_HOSTS_LINE_BUFSIZE);
	if (!line)
	{
		ftp_sftp_set_detail(session, "Out of memory reading SFTP known-hosts file");
		return 0;
	}

	result = ftp_sftp_known_hosts_scan_file(session, path, host, port, key_type, key_hex, matched,
											line, FTP_SFTP_KNOWN_HOSTS_LINE_BUFSIZE);
	ftp_sftp_buffer_free(line);
	return result;
}

#if defined(FTP_SFTP_TESTING)
int ftp_sftp_test_known_hosts_check_file(struct ftp_sftp_session *session, const char *path, int *matched)
{
	return ftp_sftp_known_hosts_check_file(session, path, "example.com", 22, 1, "abcdef", matched);
}
#endif

static int ftp_sftp_known_hosts_write_line(const char *path, const char *line)
{
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
	{
		BPTR file = Open((char *)path, MODE_READWRITE);
		LONG ok;

		if (!file)
			file = Open((char *)path, MODE_NEWFILE);
		if (!file)
			return 0;

		Seek(file, 0, OFFSET_END);
		ok = FPuts(file, (char *)line);
		Close(file);
		if (ok < 0)
			return 0;
	}
#else
	{
		FILE *file = fopen(path, "a");

		if (!file)
			return 0;
		if (fputs(line, file) < 0)
		{
			fclose(file);
			return 0;
		}
		fclose(file);
	}
#endif

	return 1;
}

static int ftp_sftp_known_hosts_append(const char *path, const char *host, int port, int key_type, const char *key_hex)
{
	char *line;
	int result;

	if (!path || !*path || !host || !*host || !key_hex || !*key_hex)
		return 0;

	if (strlen(host) + strlen(key_hex) + 80 >= FTP_SFTP_KNOWN_HOSTS_LINE_BUFSIZE)
		return 0;

	line = ftp_sftp_buffer_alloc(FTP_SFTP_KNOWN_HOSTS_LINE_BUFSIZE);
	if (!line)
		return 0;

	sprintf(line, "%s\t%s\t%ld\t%ld\t%s\n", FTP_SFTP_KNOWN_HOSTS_MAGIC, host, (long)port, (long)key_type, key_hex);
	result = ftp_sftp_known_hosts_write_line(path, line);
	ftp_sftp_buffer_free(line);
	return result;
}

static void ftp_sftp_format_fingerprint(char *out, int outsize, const char *prefix, const char *hash, int hash_len)
{
	static const char hex[] = "0123456789abcdef";
	int pos = 0;
	int i;

	if (!out || outsize <= 0)
		return;

	out[0] = 0;
	if (!prefix || !hash || hash_len <= 0)
		return;

	while (*prefix && pos < outsize - 1)
		out[pos++] = *prefix++;

	for (i = 0; i < hash_len && pos < outsize - 2; ++i)
	{
		unsigned char byte = (unsigned char)hash[i];

		out[pos++] = hex[(byte >> 4) & 0x0f];
		out[pos++] = hex[byte & 0x0f];
	}
	out[pos] = 0;
}

static void ftp_sftp_hostkey_fingerprint(LIBSSH2_SESSION *ssh, char *out, int outsize)
{
	const char *hash;

	if (!out || outsize <= 0)
		return;

	ftp_sftp_copy_text(out, outsize, "unavailable");
	if (!ssh)
		return;

	hash = libssh2_hostkey_hash(ssh, LIBSSH2_HOSTKEY_HASH_SHA256);
	if (hash)
	{
		ftp_sftp_format_fingerprint(out, outsize, "SHA256:", hash, 32);
		return;
	}

	hash = libssh2_hostkey_hash(ssh, LIBSSH2_HOSTKEY_HASH_SHA1);
	if (hash)
		ftp_sftp_format_fingerprint(out, outsize, "SHA1:", hash, 20);
}

static int ftp_sftp_verify_hostkey_hex(struct ftp_sftp_session *session,
									   const char *host,
									   int port,
									   const struct ftp_sftp_connect_options *options,
									   char *key_hex,
									   size_t key_hex_size)
{
	size_t len = 0;
	int type = 0;
	int key_type;
	int matched = 0;
	const char *hostkey;
	const char *path;
	char fingerprint[80];

	if (!session || !session->ssh)
		return 0;

	hostkey = libssh2_session_hostkey((LIBSSH2_SESSION *)session->ssh, &len, &type);
	if (!hostkey || len == 0)
	{
		ftp_sftp_set_detail(session, "SFTP host did not provide a host key");
		return 0;
	}

	key_type = ftp_sftp_knownhost_key_type(type);
	if (!key_type)
	{
		ftp_sftp_set_detail(session, "SFTP host key type is not supported");
		return 0;
	}

	if (port <= 0)
		port = 22;

	if (!ftp_sftp_hostkey_hex(key_hex, (int)key_hex_size, hostkey, len))
	{
		ftp_sftp_set_detail(session, "SFTP host key is too large to store");
		return 0;
	}

	path = ftp_sftp_known_hosts_path(options);
	if (!ftp_sftp_known_hosts_check_file(session, path, host, port, key_type, key_hex, &matched))
		return 0;

	if (matched)
		return 1;

	ftp_sftp_hostkey_fingerprint((LIBSSH2_SESSION *)session->ssh, fingerprint, sizeof(fingerprint));
	if (!options || !options->hostkey)
	{
		ftp_sftp_set_host_detail(session, "SFTP host key is not trusted for ", host);
		return 0;
	}

	if (!options->hostkey(options->userdata, host, port, fingerprint, path))
	{
		ftp_sftp_set_host_detail(session, "SFTP host key was not trusted for ", host);
		return 0;
	}

	ftp_sftp_connect_update(options, "SFTP: Trusting host key...");
	if (!ftp_sftp_known_hosts_append(path, host, port, key_type, key_hex))
	{
		ftp_sftp_set_host_detail(session, "Could not write SFTP known-hosts file: ", path);
		return 0;
	}

	return 1;
}

static int ftp_sftp_verify_hostkey(struct ftp_sftp_session *session,
								   const char *host,
								   int port,
								   const struct ftp_sftp_connect_options *options)
{
	char *key_hex;
	int result;

	key_hex = ftp_sftp_buffer_alloc(FTP_SFTP_HOSTKEY_HEX_BUFSIZE);
	if (!key_hex)
	{
		ftp_sftp_set_detail(session, "Out of memory checking the SFTP host key");
		return 0;
	}

	result = ftp_sftp_verify_hostkey_hex(session, host, port, options, key_hex, FTP_SFTP_HOSTKEY_HEX_BUFSIZE);
	ftp_sftp_buffer_free(key_hex);
	return result;
}

static int ftp_sftp_auth_agent(LIBSSH2_SESSION *ssh, const char *user)
{
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
	(void)ssh;
	(void)user;
	return 0;
#else
	LIBSSH2_AGENT *agent;
	struct libssh2_agent_publickey *identity;
	struct libssh2_agent_publickey *prev_identity;
	int authenticated = 0;

	if (!ssh || !user || !*user)
		return 0;

	agent = libssh2_agent_init(ssh);
	if (!agent)
		return 0;

	if (libssh2_agent_connect(agent))
		goto cleanup;

	if (libssh2_agent_list_identities(agent))
		goto disconnect;

	prev_identity = NULL;
	while (libssh2_agent_get_identity(agent, &identity, prev_identity) == 0)
	{
		if (libssh2_agent_userauth(agent, user, identity) == 0)
		{
			authenticated = 1;
			break;
		}
		prev_identity = identity;
	}

disconnect:
	libssh2_agent_disconnect(agent);

cleanup:
	libssh2_agent_free(agent);
	return authenticated;
#endif
}
#endif

static void ftp_sftp_set_error(struct ftp_sftp_session *session, int error)
{
	if (session)
	{
		session->last_error = error;
		if (error == FTP_SFTP_ERROR_NONE)
		{
			session->last_status = 0;
			session->last_detail[0] = 0;
		}
#if defined(FTP_SFTP_BACKEND_LIBSSH2)
		else if (error == FTP_SFTP_ERROR_HOSTKEY && session->last_detail[0])
			;
		else if (session->sftp)
			session->last_status = libssh2_sftp_last_error((LIBSSH2_SFTP *)session->sftp);
		else if (session->ssh)
		{
			char *message = NULL;
			int message_len = 0;
			int ssh_error;

			ssh_error = libssh2_session_last_error((LIBSSH2_SESSION *)session->ssh, &message, &message_len, 0);
			if (message && message_len > 0)
			{
				size_t prefix_len;

				sprintf(session->last_detail, "libssh2 %ld: ", (long)ssh_error);
				prefix_len = strlen(session->last_detail);
				if ((size_t)message_len > FTP_SFTP_ERROR_BUFSIZE - prefix_len)
					message_len = (int)(FTP_SFTP_ERROR_BUFSIZE - prefix_len);
				memcpy(session->last_detail + prefix_len, message, message_len);
				session->last_detail[prefix_len + message_len] = 0;
			}
			else
				sprintf(session->last_detail, "libssh2 %ld", (long)ssh_error);
		}
#endif
	}
}

const char *ftp_sftp_backend_name(void)
{
#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	return "libssh2";
#else
	return "none";
#endif
}

void ftp_sftp_session_init(struct ftp_sftp_session *session)
{
	if (!session)
		return;

	session->ssh = 0;
	session->sftp = 0;
	session->socket = -1;
	strcpy(session->cwd, ".");
	session->active = 0;
	session->backend_acquired = 0;
	session->last_error = FTP_SFTP_ERROR_NONE;
	session->last_status = 0;
	session->last_detail[0] = 0;
}

void ftp_sftp_session_cleanup(struct ftp_sftp_session *session)
{
	if (!session)
		return;

#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	if (session->sftp)
	{
		if (session->active)
			libssh2_sftp_shutdown((LIBSSH2_SFTP *)session->sftp);
		session->sftp = 0;
	}
	if (session->ssh)
	{
		if (session->active)
			libssh2_session_disconnect((LIBSSH2_SESSION *)session->ssh, "Normal Shutdown");
		else
			libssh2_session_set_blocking((LIBSSH2_SESSION *)session->ssh, 0);
		libssh2_session_free((LIBSSH2_SESSION *)session->ssh);
	}
	if (session->socket >= 0)
		ftp_sftp_close_socket(session->socket);
	if (session->backend_acquired)
		ftp_sftp_libssh2_release();
#endif

	session->ssh = 0;
	session->sftp = 0;
	session->socket = -1;
	strcpy(session->cwd, ".");
	session->active = 0;
	session->backend_acquired = 0;
}

int ftp_sftp_session_error(const struct ftp_sftp_session *session)
{
	return session ? session->last_error : FTP_SFTP_ERROR_NONE;
}

unsigned long ftp_sftp_status(const struct ftp_sftp_session *session)
{
#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	if (session && session->sftp && session->last_error != FTP_SFTP_ERROR_NONE)
		return libssh2_sftp_last_error((LIBSSH2_SFTP *)session->sftp);
#endif

	return session ? session->last_status : 0;
}

const char *ftp_sftp_error_message(const struct ftp_sftp_session *session)
{
	unsigned long status = ftp_sftp_status(session);

	if (!session)
		return "SFTP error";

	switch (session->last_error)
	{
	case FTP_SFTP_ERROR_BACKEND:
		if (session->last_detail[0])
			return session->last_detail;
		return "SFTP support is not available in this build";
	case FTP_SFTP_ERROR_SESSION:
		if (session->last_detail[0])
			return session->last_detail;
		return "Could not create SSH session";
	case FTP_SFTP_ERROR_CONNECT:
		if (session->last_detail[0])
			return session->last_detail;
		return "Could not connect to SFTP host";
	case FTP_SFTP_ERROR_AUTH:
		if (session->last_detail[0])
			return session->last_detail;
		return "SFTP authentication failed";
	case FTP_SFTP_ERROR_INIT:
		if (session->last_detail[0])
			return session->last_detail;
		return "Could not start SFTP subsystem";
	case FTP_SFTP_ERROR_PATH:
		if (status == 2)
			return "SFTP path not found";
		if (status == 3)
			return "SFTP permission denied";
		return "SFTP path error";
	case FTP_SFTP_ERROR_OP:
		switch (status)
		{
		case 2:
		case 10:
			return "SFTP file or directory not found";
		case 3:
			return "SFTP permission denied";
		case 4:
			return "SFTP operation failed";
		case 6:
			return "SFTP connection is not open";
		case 7:
			return "SFTP connection lost";
		case 8:
			return "SFTP operation not supported";
		case 11:
			return "SFTP file already exists";
		case 14:
			return "SFTP no space left on remote filesystem";
		default:
			return "SFTP operation failed";
		}
	case FTP_SFTP_ERROR_HOSTKEY:
		if (session->last_detail[0])
			return session->last_detail;
		return "SFTP host key check failed";
	case FTP_SFTP_ERROR_ABORTED:
		return "SFTP operation aborted";
	case FTP_SFTP_ERROR_NONE:
	default:
		break;
	}

	return "SFTP error";
}

int ftp_sftp_connect_with_options(struct ftp_sftp_session *session,
								  const char *host,
								  int port,
								  const char *user,
								  const char *pass,
								  const struct ftp_sftp_connect_options *options)
{
	if (!session || !host || !*host)
		return 0;

	ftp_sftp_session_cleanup(session);
	ftp_sftp_session_init(session);

#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	if (ftp_sftp_connect_aborted(options))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_CONNECT);
		return 0;
	}

	session->socket = ftp_sftp_open_socket(host, port, options);
	if (session->socket < 0)
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_CONNECT);
		ftp_sftp_session_cleanup(session);
		return 0;
	}

	ftp_sftp_connect_update(options, "SFTP: Initializing SSH...");
	if (!ftp_sftp_libssh2_acquire(options))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_BACKEND);
		ftp_sftp_session_cleanup(session);
		return 0;
	}
	session->backend_acquired = 1;

	ftp_sftp_connect_update(options, "SFTP: Allocating SSH session...");
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
	session->ssh = libssh2_session_init_ex(ftp_sftp_libssh2_alloc,
										   ftp_sftp_libssh2_free,
										   ftp_sftp_libssh2_realloc,
										   NULL);
#else
	session->ssh = libssh2_session_init();
#endif
	if (!session->ssh)
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_SESSION);
		ftp_sftp_session_cleanup(session);
		return 0;
	}

	ftp_sftp_connect_update(options, "SFTP: Configuring SSH session...");
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
	libssh2_session_callback_set2((LIBSSH2_SESSION *)session->ssh,
								  LIBSSH2_CALLBACK_RECV,
								  (libssh2_cb_generic *)ftp_sftp_libssh2_recv);
	libssh2_session_callback_set2((LIBSSH2_SESSION *)session->ssh,
								  LIBSSH2_CALLBACK_SEND,
								  (libssh2_cb_generic *)ftp_sftp_libssh2_send);
#endif
	libssh2_session_set_blocking((LIBSSH2_SESSION *)session->ssh, 0);
	libssh2_session_set_timeout((LIBSSH2_SESSION *)session->ssh, (long)ftp_sftp_timeout_secs(options) * 1000);

	if (!ftp_sftp_handshake((LIBSSH2_SESSION *)session->ssh, session->socket, options))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_CONNECT);
		ftp_sftp_session_cleanup(session);
		return 0;
	}

	ftp_sftp_connect_update(options, "SFTP: Checking host key...");
	if (!ftp_sftp_verify_hostkey(session, host, port, options))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_HOSTKEY);
		ftp_sftp_session_cleanup(session);
		return 0;
	}

	if (!user || !*user ||
		(!ftp_sftp_auth_agent((LIBSSH2_SESSION *)session->ssh, user) &&
		 (!pass || !*pass ||
		  (!ftp_sftp_auth_password((LIBSSH2_SESSION *)session->ssh, session->socket, user, pass, options) &&
		   !ftp_sftp_auth_keyboard_interactive((LIBSSH2_SESSION *)session->ssh,
											   session->socket,
											   user,
											   pass,
											   options)))))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_AUTH);
		ftp_sftp_session_cleanup(session);
		return 0;
	}

	session->sftp = ftp_sftp_init_subsystem((LIBSSH2_SESSION *)session->ssh, session->socket, options);
	if (!session->sftp)
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_INIT);
		ftp_sftp_session_cleanup(session);
		return 0;
	}

	libssh2_session_set_blocking((LIBSSH2_SESSION *)session->ssh, 1);
	ftp_sftp_set_socket_nonblocking(session->socket, 0);

	session->active = 1;
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_NONE);
	if (!ftp_sftp_cwd(session, "."))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_INIT);
		ftp_sftp_session_cleanup(session);
		return 0;
	}
	return 1;
#else
	(void)port;
	(void)user;
	(void)pass;
	(void)options;
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_BACKEND);
	return 0;
#endif
}

int ftp_sftp_connect(struct ftp_sftp_session *session, const char *host, int port, const char *user, const char *pass)
{
	return ftp_sftp_connect_with_options(session, host, port, user, pass, NULL);
}

int ftp_sftp_path_join(char *out, int outsize, const char *cwd, const char *path)
{
	int len;

	if (!out || outsize <= 0)
		return 0;

	if (!path || !*path)
		path = ".";

	if (*path == '/')
	{
		if ((int)strlen(path) >= outsize)
			return 0;
		strcpy(out, path);
		return 1;
	}

	if (!cwd || !*cwd)
		cwd = ".";

	if (!strcmp(path, "."))
	{
		if ((int)strlen(cwd) >= outsize)
			return 0;
		strcpy(out, cwd);
		return 1;
	}

	if (!strcmp(cwd, "."))
	{
		if ((int)strlen(path) >= outsize)
			return 0;
		strcpy(out, path);
		return 1;
	}

	if (!strcmp(cwd, "/"))
		len = 1 + strlen(path);
	else
		len = strlen(cwd) + 1 + strlen(path);

	if (len >= outsize)
		return 0;

	strcpy(out, cwd);
	if (strcmp(out, "/"))
		strcat(out, "/");
	strcat(out, path);

	return 1;
}

int ftp_sftp_parent_path(char *out, int outsize, const char *path)
{
	char *slash;

	if (!out || outsize <= 0 || !path || !*path)
		return 0;

	if ((int)strlen(path) >= outsize)
		return 0;

	strcpy(out, path);

	slash = strrchr(out, '/');
	if (!slash || slash == out)
	{
		strcpy(out, "/");
		return 1;
	}

	*slash = 0;
	return 1;
}

unsigned long ftp_sftp_unix_to_amiga_seconds(unsigned long unix_seconds)
{
	if (unix_seconds < FTP_SFTP_UNIX_TO_AMIGA_EPOCH)
		return 0;

	return unix_seconds - FTP_SFTP_UNIX_TO_AMIGA_EPOCH;
}

#if defined(FTP_SFTP_BACKEND_LIBSSH2)
#if defined(__amigaos3__) || defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
typedef BPTR ftp_sftp_local_file;

static ftp_sftp_local_file ftp_sftp_local_open_read(const char *path)
{
	return Open((char *)path, MODE_OLDFILE);
}

static ftp_sftp_local_file ftp_sftp_local_open_write(const char *path, int restart)
{
	return Open((char *)path, restart ? MODE_READWRITE : MODE_NEWFILE);
}

static int ftp_sftp_local_read(ftp_sftp_local_file file, void *buf, int len)
{
	return Read(file, buf, len);
}

static int ftp_sftp_local_write(ftp_sftp_local_file file, const void *buf, int len)
{
	return Write(file, (APTR)buf, len);
}

static unsigned int ftp_sftp_local_seek_end(ftp_sftp_local_file file)
{
	Seek(file, 0, OFFSET_END);
	return Seek(file, 0, OFFSET_CURRENT);
}

static void ftp_sftp_local_seek_start(ftp_sftp_local_file file, unsigned int offset)
{
	Seek(file, offset, OFFSET_BEGINNING);
}

static void ftp_sftp_local_close(ftp_sftp_local_file file)
{
	Close(file);
}
#else
typedef FILE *ftp_sftp_local_file;

static ftp_sftp_local_file ftp_sftp_local_open_read(const char *path)
{
	return fopen(path, "rb");
}

static ftp_sftp_local_file ftp_sftp_local_open_write(const char *path, int restart)
{
	return fopen(path, restart ? "r+b" : "wb");
}

static int ftp_sftp_local_read(ftp_sftp_local_file file, void *buf, int len)
{
	return (int)fread(buf, 1, len, file);
}

static int ftp_sftp_local_write(ftp_sftp_local_file file, const void *buf, int len)
{
	return (int)fwrite(buf, 1, len, file);
}

static unsigned int ftp_sftp_local_seek_end(ftp_sftp_local_file file)
{
	fseek(file, 0, SEEK_END);
	return (unsigned int)ftell(file);
}

static void ftp_sftp_local_seek_start(ftp_sftp_local_file file, unsigned int offset)
{
	fseek(file, offset, SEEK_SET);
}

static void ftp_sftp_local_close(ftp_sftp_local_file file)
{
	fclose(file);
}
#endif

static void ftp_sftp_entry_from_attrs(struct ftp_sftp_entry *entry,
									  const char *name,
									  const LIBSSH2_SFTP_ATTRIBUTES *attrs)
{
	memset(entry, 0, sizeof(*entry));

	if (name)
	{
		strncpy(entry->name, name, FTP_SFTP_NAME_BUFSIZE);
		entry->name[FTP_SFTP_NAME_BUFSIZE] = 0;
	}

	if (attrs)
	{
		if (attrs->flags & LIBSSH2_SFTP_ATTR_SIZE)
			entry->size = (unsigned long)attrs->filesize;
		if (attrs->flags & LIBSSH2_SFTP_ATTR_ACMODTIME)
			entry->seconds = (unsigned long)attrs->mtime;
		if (attrs->flags & LIBSSH2_SFTP_ATTR_PERMISSIONS)
		{
			entry->unixprot = (unsigned long)attrs->permissions;
			entry->type = LIBSSH2_SFTP_S_ISDIR(attrs->permissions) ? 1 : -1;
		}
		else
			entry->type = -1;
	}
	else
		entry->type = -1;
}

static int ftp_sftp_resolve_path(struct ftp_sftp_session *session, const char *path, char *out, int outsize)
{
	if (!session || !session->active)
		return 0;

	if (!ftp_sftp_path_join(out, outsize, session->cwd, path))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_PATH);
		return 0;
	}

	return 1;
}
#endif

int ftp_sftp_cwd(struct ftp_sftp_session *session, const char *path)
{
#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	char full[FTP_SFTP_PATH_BUFSIZE + 1];
	char canonical[FTP_SFTP_PATH_BUFSIZE + 1];
	const char *newcwd;
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	int len;

	if (!ftp_sftp_resolve_path(session, path, full, sizeof(full)))
		return 0;

	newcwd = full;
	len = libssh2_sftp_realpath((LIBSSH2_SFTP *)session->sftp, full, canonical, FTP_SFTP_PATH_BUFSIZE);
	if (len > 0)
	{
		canonical[len < FTP_SFTP_PATH_BUFSIZE ? len : FTP_SFTP_PATH_BUFSIZE] = 0;
		newcwd = canonical;
	}

	memset(&attrs, 0, sizeof(attrs));
	if (libssh2_sftp_stat((LIBSSH2_SFTP *)session->sftp, newcwd, &attrs) ||
		!(attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) ||
		!LIBSSH2_SFTP_S_ISDIR(attrs.permissions))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_PATH);
		return 0;
	}

	strncpy(session->cwd, newcwd, FTP_SFTP_PATH_BUFSIZE);
	session->cwd[FTP_SFTP_PATH_BUFSIZE] = 0;
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_NONE);
	return 1;
#else
	(void)path;
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_BACKEND);
	return 0;
#endif
}

int ftp_sftp_cdup(struct ftp_sftp_session *session)
{
	char parent[FTP_SFTP_PATH_BUFSIZE + 1];

	if (!session || !ftp_sftp_parent_path(parent, sizeof(parent), session->cwd))
		return 0;

	return ftp_sftp_cwd(session, parent);
}

int ftp_sftp_pwd(struct ftp_sftp_session *session, char *out, int outsize)
{
	if (!session || !out || outsize <= 0 || (int)strlen(session->cwd) >= outsize)
		return 0;

	strcpy(out, session->cwd);
	return 1;
}

int ftp_sftp_list(struct ftp_sftp_session *session,
				  const char *path,
				  ftp_sftp_list_callback callback,
				  void *userdata)
{
#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	char full[FTP_SFTP_PATH_BUFSIZE + 1];
	char name[FTP_SFTP_NAME_BUFSIZE + 1];
	LIBSSH2_SFTP_HANDLE *dir;
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	struct ftp_sftp_entry entry;
	long bytes = 0;
	int callback_result;
	int failed = 0;
	int callback_failed = 0;
	int aborted = 0;

	if (!callback || !ftp_sftp_resolve_path(session, path, full, sizeof(full)))
		return 0;

	dir = libssh2_sftp_opendir((LIBSSH2_SFTP *)session->sftp, full);
	if (!dir)
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}

	while ((bytes = (long)libssh2_sftp_readdir(dir, name, FTP_SFTP_NAME_BUFSIZE, &attrs)) > 0)
	{
		name[bytes < FTP_SFTP_NAME_BUFSIZE ? bytes : FTP_SFTP_NAME_BUFSIZE] = 0;
		if (strcmp(name, ".") && strcmp(name, ".."))
		{
			ftp_sftp_entry_from_attrs(&entry, name, &attrs);
			callback_result = callback(userdata, &entry);
			if (callback_result <= 0)
			{
				if (callback_result < 0)
					callback_failed = 1;
				else
					aborted = 1;
				break;
			}
		}
	}

	if (bytes < 0)
		failed = 1;

	libssh2_sftp_closedir(dir);
	if (failed)
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}
	if (callback_failed)
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}
	if (aborted)
	{
		errno = EINTR;
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_ABORTED);
		return 0;
	}

	ftp_sftp_set_error(session, FTP_SFTP_ERROR_NONE);
	return 1;
#else
	(void)path;
	(void)callback;
	(void)userdata;
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_BACKEND);
	return 0;
#endif
}

int ftp_sftp_stat(struct ftp_sftp_session *session, const char *path, struct ftp_sftp_entry *entry)
{
#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	char full[FTP_SFTP_PATH_BUFSIZE + 1];
	LIBSSH2_SFTP_ATTRIBUTES attrs;

	if (!entry || !ftp_sftp_resolve_path(session, path, full, sizeof(full)))
		return 0;

	memset(&attrs, 0, sizeof(attrs));
	if (libssh2_sftp_stat((LIBSSH2_SFTP *)session->sftp, full, &attrs))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}

	ftp_sftp_entry_from_attrs(entry, 0, &attrs);
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_NONE);
	return 1;
#else
	(void)path;
	(void)entry;
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_BACKEND);
	return 0;
#endif
}

int ftp_sftp_mkdir(struct ftp_sftp_session *session, const char *path)
{
#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	char full[FTP_SFTP_PATH_BUFSIZE + 1];

	if (!ftp_sftp_resolve_path(session, path, full, sizeof(full)))
		return 0;

	if (libssh2_sftp_mkdir((LIBSSH2_SFTP *)session->sftp, full, 0777))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}

	ftp_sftp_set_error(session, FTP_SFTP_ERROR_NONE);
	return 1;
#else
	(void)path;
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_BACKEND);
	return 0;
#endif
}

int ftp_sftp_delete(struct ftp_sftp_session *session, const char *path)
{
#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	char full[FTP_SFTP_PATH_BUFSIZE + 1];

	if (!ftp_sftp_resolve_path(session, path, full, sizeof(full)))
		return 0;

	if (libssh2_sftp_unlink((LIBSSH2_SFTP *)session->sftp, full))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}

	ftp_sftp_set_error(session, FTP_SFTP_ERROR_NONE);
	return 1;
#else
	(void)path;
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_BACKEND);
	return 0;
#endif
}

int ftp_sftp_rmdir(struct ftp_sftp_session *session, const char *path)
{
#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	char full[FTP_SFTP_PATH_BUFSIZE + 1];

	if (!ftp_sftp_resolve_path(session, path, full, sizeof(full)))
		return 0;

	if (libssh2_sftp_rmdir((LIBSSH2_SFTP *)session->sftp, full))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}

	ftp_sftp_set_error(session, FTP_SFTP_ERROR_NONE);
	return 1;
#else
	(void)path;
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_BACKEND);
	return 0;
#endif
}

int ftp_sftp_rename(struct ftp_sftp_session *session, const char *oldpath, const char *newpath)
{
#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	char oldfull[FTP_SFTP_PATH_BUFSIZE + 1];
	char newfull[FTP_SFTP_PATH_BUFSIZE + 1];

	if (!ftp_sftp_resolve_path(session, oldpath, oldfull, sizeof(oldfull)) ||
		!ftp_sftp_resolve_path(session, newpath, newfull, sizeof(newfull)))
		return 0;

	if (libssh2_sftp_rename((LIBSSH2_SFTP *)session->sftp, oldfull, newfull))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}

	ftp_sftp_set_error(session, FTP_SFTP_ERROR_NONE);
	return 1;
#else
	(void)oldpath;
	(void)newpath;
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_BACKEND);
	return 0;
#endif
}

int ftp_sftp_chmod(struct ftp_sftp_session *session, const char *path, unsigned long mode)
{
#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	char full[FTP_SFTP_PATH_BUFSIZE + 1];
	LIBSSH2_SFTP_ATTRIBUTES attrs;

	if (!ftp_sftp_resolve_path(session, path, full, sizeof(full)))
		return 0;

	memset(&attrs, 0, sizeof(attrs));
	attrs.flags = LIBSSH2_SFTP_ATTR_PERMISSIONS;
	attrs.permissions = mode;
	if (libssh2_sftp_setstat((LIBSSH2_SFTP *)session->sftp, full, &attrs))
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}

	ftp_sftp_set_error(session, FTP_SFTP_ERROR_NONE);
	return 1;
#else
	(void)path;
	(void)mode;
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_BACKEND);
	return 0;
#endif
}

unsigned int ftp_sftp_get(struct ftp_sftp_session *session,
						  ftp_sftp_xfer_callback callback,
						  void *userdata,
						  const char *remote,
						  const char *local,
						  int restart)
{
#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	char full[FTP_SFTP_PATH_BUFSIZE + 1];
	char *buffer;
	LIBSSH2_SFTP_HANDLE *in;
	ftp_sftp_local_file out;
	long bytes = 0;
	int failed = 0;
	int aborted = 0;
	unsigned int done = 0;
	unsigned int total = 0;
	struct ftp_sftp_entry entry;

	if (!ftp_sftp_resolve_path(session, remote, full, sizeof(full)))
		return 0;

	if (ftp_sftp_stat(session, remote, &entry))
		total = (unsigned int)entry.size;

	in = libssh2_sftp_open((LIBSSH2_SFTP *)session->sftp, full, LIBSSH2_FXF_READ, 0);
	if (!in)
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}

	out = ftp_sftp_local_open_write(local, restart);
	if (!out)
	{
		libssh2_sftp_close(in);
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}

	buffer = ftp_sftp_buffer_alloc(FTP_SFTP_XFER_BUFSIZE);
	if (!buffer)
	{
		ftp_sftp_local_close(out);
		libssh2_sftp_close(in);
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}

	if (restart)
	{
		done = ftp_sftp_local_seek_end(out);
		libssh2_sftp_seek64(in, done);
	}

	if (callback && callback(userdata, total, done))
		aborted = 1;

	while (!aborted && (bytes = (long)libssh2_sftp_read(in, buffer, FTP_SFTP_XFER_BUFSIZE)) > 0)
	{
		if (ftp_sftp_local_write(out, buffer, bytes) != bytes)
		{
			failed = 1;
			break;
		}
		done += bytes;
		if (callback && callback(userdata, total, bytes))
			aborted = 1;
	}

	ftp_sftp_buffer_free(buffer);
	ftp_sftp_local_close(out);
	libssh2_sftp_close(in);

	if (aborted)
	{
		errno = EINTR;
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_ABORTED);
	}
	else if (bytes < 0 || failed)
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
	else
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_NONE);
		if (callback)
			callback(userdata, total, 0);
	}

	return done;
#else
	(void)callback;
	(void)userdata;
	(void)remote;
	(void)local;
	(void)restart;
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_BACKEND);
	return 0;
#endif
}

unsigned int ftp_sftp_put(struct ftp_sftp_session *session,
						  ftp_sftp_xfer_callback callback,
						  void *userdata,
						  const char *local,
						  const char *remote,
						  unsigned int restart)
{
#if defined(FTP_SFTP_BACKEND_LIBSSH2)
	char full[FTP_SFTP_PATH_BUFSIZE + 1];
	char *buffer;
	char *outpos;
	LIBSSH2_SFTP_HANDLE *out;
	ftp_sftp_local_file in;
	long bytes = 0;
	long written;
	long written_total;
	unsigned long open_flags;
	int failed = 0;
	int aborted = 0;
	unsigned int done = restart;

	if (!ftp_sftp_resolve_path(session, remote, full, sizeof(full)))
		return 0;

	in = ftp_sftp_local_open_read(local);
	if (!in)
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}

	open_flags = LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT;
	if (!restart)
		open_flags |= LIBSSH2_FXF_TRUNC;

	out = libssh2_sftp_open((LIBSSH2_SFTP *)session->sftp, full, open_flags, 0666);
	if (!out)
	{
		ftp_sftp_local_close(in);
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}

	buffer = ftp_sftp_buffer_alloc(FTP_SFTP_XFER_BUFSIZE);
	if (!buffer)
	{
		ftp_sftp_local_close(in);
		libssh2_sftp_close(out);
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
		return 0;
	}

	if (restart)
	{
		ftp_sftp_local_seek_start(in, restart);
		libssh2_sftp_seek64(out, restart);
	}

	if (callback && callback(userdata, 0xffffffff, done))
		aborted = 1;

	while (!aborted && (bytes = ftp_sftp_local_read(in, buffer, FTP_SFTP_XFER_BUFSIZE)) > 0)
	{
		outpos = buffer;
		written_total = 0;
		while (!aborted && written_total < bytes)
		{
			written = (long)libssh2_sftp_write(out, outpos, bytes - written_total);
			if (written <= 0)
			{
				failed = 1;
				break;
			}
			written_total += written;
			outpos += written;
		}

		if (failed || aborted)
			break;

		done += bytes;
		if (callback && callback(userdata, 0xffffffff, bytes))
			aborted = 1;
	}

	ftp_sftp_buffer_free(buffer);
	ftp_sftp_local_close(in);
	libssh2_sftp_close(out);

	if (aborted)
	{
		errno = EINTR;
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_ABORTED);
	}
	else if (bytes < 0 || failed)
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_OP);
	else
	{
		ftp_sftp_set_error(session, FTP_SFTP_ERROR_NONE);
		if (callback)
			callback(userdata, 0xffffffff, 0);
	}

	return done;
#else
	(void)callback;
	(void)userdata;
	(void)local;
	(void)remote;
	(void)restart;
	ftp_sftp_set_error(session, FTP_SFTP_ERROR_BACKEND);
	return 0;
#endif
}
