#include "libportytcp4.h"
/*
    libporty - collection of random system-dependent network code
    Copyright (C) 2005 gpmi-extension project
    Copyright (C) 2010..2012  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Project page: http://repo.hu/projects/libporty
    Author: libporty@igor2.repo.hu
*/

#include <stdio.h>

int P_net_close(P_net_socket s)
{
#ifdef P_NET_CLOSESOCKET
	if (P_net_shutdown(s, P_SHUT_RDWR) != 0)
		return -1;
	return closesocket(s);
#else
	return close(s);
#endif
}


int P_net_set_nonblocking(int sock)
{
#ifdef WIN32
	u_long nonblocking = 1;
#else
	int nonblocking = 1;
#endif
	return P_ioctl_socket(sock, FIONBIO, &nonblocking);
}

int P_net_set_blocking(int sock)
{
#ifdef WIN32
	u_long nonblocking = 0;
#else
	int nonblocking = 0;
#endif

	return P_ioctl_socket(sock, FIONBIO, &nonblocking);
}

/* UDP only! */
int P_net_enable_broadcast(int sock)
{
	/* Enable broadcast */
	unsigned long val = 1;
	return setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&val , sizeof(val));
}

/* alternative poll() implementations, in case we don't have native poll() */
#ifndef P_NET_POLL

#ifdef P_NET_SELECT
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
int P_poll_select(struct P_pollfd *fds, nfds_t nfds, int timeout)
{
	fd_set rd, wr;
	int snfds, n, ret;
	struct timeval to;

	snfds = -1;
	FD_ZERO(&rd);
	FD_ZERO(&wr);
# warning TODO: reproduce odd behavior of poll() returning HUP even if events is 0
	for(n = 0; n < nfds; n++) {
		if ((fds[n].events != 0) && (fds[n].fd >= 0)) {
			if (fds[n].fd >= snfds)
				snfds = fds[n].fd;
			if (fds[n].events & P_POLLIN)
				FD_SET(fds[n].fd, &rd);
			if (fds[n].events & P_POLLOUT)
				FD_SET(fds[n].fd, &wr);
		} else {
			fds[n].revents = 0;
		}
# warning add support for POLLPRI?
	}
	if (snfds == -1) {
		P_usleep(timeout * 1000);
		return 0;
	}

	to.tv_sec  = timeout / 1000;
	to.tv_usec = (timeout - (to.tv_sec * 1000)) * 1000;
	ret = select(snfds + 1, &rd, &wr, NULL, &to);
	if (ret < 0)
		return ret;
	ret = 0;
	for(n = 0; n < nfds; n++) {
		fds[n].revents = 0;
		if (FD_ISSET(fds[n].fd, &rd))
			fds[n].revents |= P_POLLIN;
		if (FD_ISSET(fds[n].fd, &wr))
			fds[n].revents |= P_POLLOUT;
		if (fds[n].revents != 0)
			ret++;
# warning add support for the other standard poll outputs
	}
	return ret;
}

#else
int P_poll_busyloop(struct P_pollfd *fds, nfds_t nfds, int timeout)
{
	fprintf(stderr, "P_poll_busyloop() unimplemented yet (TODO)\n");
	abort();
}
#endif

#endif


#ifndef WIN32

/* Sockets can be written and read; async stdio may not be a socket
   thus recv() and send() would fail */
P_size_t P_net_read(P_net_socket s, void *buf, P_size_t count)
{
	return read(s, buf, count);
}

P_size_t P_net_write(P_net_socket s, void *buf, P_size_t count)
{
	return write(s, buf, count);
}

#else

/* Sockets can not be handled by read() and write(); we made sure
   async stdio has a real socket and a thread/fork. */
P_size_t P_net_read(P_net_socket s, void *buf, P_size_t count)
{
	return recv(s, buf, count, 0);
}

P_size_t P_net_write(P_net_socket s, void *buf, P_size_t count)
{
	return send(s, buf, count, 0);
}
#endif


int P_net_printf_minbuff = 1024;
int P_net_printf_maxbuff = 65536;

#ifdef P_HAVE_VDPRINTF
#include <stdarg.h>
P_size_t P_net_printf(P_net_socket s, const char *format, ...)
{
	int ret;
	va_list ap;
	va_start(ap, format);
	ret = vdprintf(s, format, ap);
	va_end(ap);
	return ret;
}
#else
/* we can't use fdopen as an alternative because fclose() would close the
socket. We could hash P_net_socket -> FILE *, but we wouldn't notice
if the fd was replaced meanwhile and the system will most probably
reuse fd numbers. (Or else all libporty calls that could affect what an fd
means would need to maintain the hash). */
#include <stdlib.h>
#include <stdarg.h>

#ifdef P_HAVE_VSNPRINTF
#	ifdef P_HAVE_VSNPRINTF_SAFE
#	define SAFETY 0
#	else
#	define SAFETY 1
#	endif
P_size_t P_net_printf(P_net_socket s, const char *format, ...)
{
	int size;
	char *buff;
	int ret;
	va_list ap;

	for(size = P_net_printf_minbuff; size <= P_net_printf_maxbuff; size = size * 2) {
		buff = malloc(size + SAFETY);
		if (buff == NULL)
			return 0;
		va_start(ap, format);
		ret = vsnprintf(buff, size, format, ap);
		va_end(ap);
		if (ret <= size && ret != -1)
			break;
	}
	if (ret <= size && ret >= 0)
		ret = P_net_write(s, buff, ret);

	free(buff);


	if (ret <= size)
		return ret;
	return -1;
}
#else
/* we don't have anything advanced so allocate a 'large enough' buffer and
check for overflow */
#include <stdlib.h>
#include <stdarg.h>
P_size_t P_net_printf(P_net_socket s, const char *format, ...)
{
	char *buff;
	int len;
	va_list ap;
	va_start(ap, format);

	buff = malloc(P_net_printf_maxbuff);
	len = vsprintf(buff, format, ap);
	if (len >= P_net_printf_maxbuff) {
		fprintf(stderr, "P_net_printf: buffer (%d) too small (need %d); please increase P_net_printf_maxbuff\n", P_net_printf_maxbuff, len);
		abort();
	}
	len = P_net_write(s, buff, len);

	free(buff);
	va_end(ap);
	return len;
}
#endif
#endif


#ifdef WIN32
int P_net_init(void)
{
	WSADATA w;
	return WSAStartup(MAKEWORD(2,2), &w);
}

int P_net_uninit(void)
{
	P_net_uninit_chain();
	return WSACleanup();
}
#else
#include <signal.h>
int P_net_init(void)
{
	signal(SIGPIPE, SIG_IGN);
	return 0;
}

int P_net_uninit(void)
{
	P_net_uninit_chain();
	return 0;
}
#endif

#ifdef WIN32
P_net_socket P_socket(int af, int protocol, int type)
{
	return WSASocket(af, protocol, type, NULL, 0, 0);
}
#endif
/*
    libporty - IPv4 TCP
    Copyright (C) 2005 gpmi-extension project
    Copyright (C) 2010..2012  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Project page: http://repo.hu/projects/libporty
    Author: libporty@igor2.repo.hu
*/

#include <stdio.h>
#include <string.h>

/**
 * Opens a listen-socket on TCP for IPv4.
 *
 * @param socket The socket that will get the listener
 * @param hostname The hostname (may be an IP) to bind to
 * @param port The bind-port
 * @param non_blocking Bitfield P_net_nonblock_net_nonblock_operation
 * @param backlog Size of backlog (see 'man listen')
 * @return see enum P_net_err
 */
int P_tcp4_listen(P_net_socket *sock, const char *hostname, int port, P_net_nonblock_t non_blocking, int backlog)
{
	P_ipv4_addr_t ip;
	struct sockaddr_in sin;
	int reuse = 1;


	if (P_dns4_name_to_IP(&ip, hostname) != 0)
		return P_net_err_dns;

	/* Create the socket */
	*sock = P_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*sock == P_NET_INVALID_SOCKET)
		return P_net_err_socket;

	{ /* Set the socket to reuse */
		/* The (const char*) cast is needed for windows!! */
		if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == -1) {
			P_net_close(*sock);
			return P_net_err_reuse;
		}
	}

	/* Switch to non-blocking, if needed */
	if (non_blocking & P_net_nonblock_socket) {
		if (P_net_set_nonblocking(*sock) != 0 && (non_blocking & P_net_nonblock_quit_on_error)) {
			P_net_close(*sock);
			return P_net_err_nonblock;
		}
	}

	/* Create the data for the bind */
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = ip;
	sin.sin_port = htons(port);

	/* Bind */
	if (bind(*sock, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
		P_net_close(*sock);
		return P_net_err_bind;
	}

	/* Start listener */
	if (listen(*sock, backlog) != 0) {
		P_net_close(*sock);
		return P_net_err_listen;
	}

	return 0;
}

/**
 * Connect to a hostname / port.
 *
 * @param sock The socket which will connect
 * @param hostname The host to connect to
 * @param port The port to connect to
 * @param non_blocking Bitfield, enum P_net_nonblock

  Bit 1: quit on error, bit 2: connect non-blocking (watch out with this!),
 *           bit 4: set non-blocking after connect
 * @return see enum P_net_err
 */
int P_tcp4_connect(P_net_socket *sock, const char *rem_hostname, int rem_port, const char *loc_hostname, int loc_port, P_net_nonblock_t non_blocking)
{
	P_ipv4_addr_t ip, loc_ip;
	struct sockaddr_in sin;

	if (P_dns4_name_to_IP(&ip, rem_hostname) != 0)
		return P_net_err_dns;

	*sock = P_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*sock == P_NET_INVALID_SOCKET)
		return P_net_err_socket;


	/* Switch to non-blocking, if needed */
	if (non_blocking & P_net_nonblock_connect) {
		if (P_net_set_nonblocking(*sock) != 0 && (non_blocking & P_net_nonblock_quit_on_error)) {
			P_net_close(*sock);
			return P_net_err_nonblock;
		}
	}

	/* set up sin for local and remote host/port */
	memset((char *) &sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;

	/* set local host if needed */
	if (loc_hostname != NULL) {
		if (P_dns4_name_to_IP(&loc_ip, loc_hostname) != 0)
			return P_net_err_dns;

		sin.sin_addr.s_addr = loc_ip;
	}

	/* set local port if needed */
	if (loc_port > 0)
		sin.sin_port = htons(loc_port);

	/* bind if anything local was set */
	if ((loc_hostname != NULL) || (loc_port > 0)) {
		if (bind(*sock, (struct sockaddr *) &sin, sizeof(sin)) != 0)
			return P_net_err_bind;
	}

	/* set up remote host and port */

	sin.sin_addr.s_addr = ip;
	sin.sin_port = htons(rem_port);

	if ((connect(*sock, (struct sockaddr*) &sin, sizeof(sin)) != 0) && (!P_net_inprogress)) {
		P_net_close(*sock);
		return P_net_err_connect;
	}

	/* Switch to non-blocking, if needed */
	if (non_blocking & P_net_nonblock_socket) {
		if (P_net_set_nonblocking(*sock) != 0 && (non_blocking & P_net_nonblock_quit_on_error)) {
			P_net_close(*sock);
			return P_net_err_nonblock;
		}
	}

	return 0;
}

int P_tcp4_accept(P_net_socket *new_socket, P_net_socket listen_sock, P_ipv4_addr_t *rem_ip, char **rem_host, int *rem_port, P_net_nonblock_t non_blocking)
{
	struct sockaddr_in sin;
	int ret;
	P_socklen_t len;

	len = sizeof(sin);
	ret = accept(listen_sock, (struct sockaddr *)&sin, &len);
	if (ret < 0)
		return P_net_err_accept;

	*new_socket = ret;
	if (rem_ip != NULL)
		*rem_ip = ntohl(sin.sin_addr.s_addr);
	if (rem_port != NULL)
		*rem_port = ntohs(sin.sin_port);
	if (rem_host != NULL) {
		P_ipv4_addr_t ip;
		ip = sin.sin_addr.s_addr;
		P_dns4_IP_to_name(rem_host, &ip);
	}

	/* Switch newly accepted socket to non-blocking, if needed */
	if (non_blocking & P_net_nonblock_socket) {
		if (P_net_set_nonblocking(*new_socket) != 0 && (non_blocking & P_net_nonblock_quit_on_error)) {
			P_net_close(*new_socket);
			return P_net_err_nonblock;
		}
	}

	return 0;
}

/*
    libporty - IPv4 dns
    Copyright (C) 2005 gpmi-extension project
    Copyright (C) 2010..2012  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Project page: http://repo.hu/projects/libporty
    Author: libporty@igor2.repo.hu
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#define P_NET_HOSTNAME_SIZE 256

#ifdef WIN32
/* this define is needed for getaddrinfo, freeaddrinfo and getnameinfo
 * because mingw enclose their declarations in #if (_WIN32_WINNT >= 0x0501)
 * but they are available on all windows version (accordiing msdn)
 */
#define _WIN32_WINNT 0x0501
#endif


#ifdef P_USE_PORTY_RESOLVER

/* include libporty's own resolver here */
#define P_NET_RESOLVER_LOCAL_USE
#undef P_NET_RESOLVER_LOCAL_USE


int P_dns4_name_to_IP(P_ipv4_addr_t *ip, const char *hostname)
{
	char *ips;
	int ret;

	/* P_net_resolv will eventually call us again with the IP of the name server;
	   we must handle IP numbers as special; otherwise this also speeds up
	   thing as if the original request was an IP address (from a connect for
	   example) we wouldn't be able to resolve it through DNS anyway */
	if (P_dns4_is_addr(hostname))
		return P_dns4_addr_to_IP(ip, hostname);

	if (hostname == NULL)
		return - 257;
	ips = NULL;
	ret = P_net_resolv((char **)&hostname,  &ips);
	if (ret != 0)
		return ret;

	ret = P_dns4_addr_to_IP(ip, ips);
	free(ips);
	return ret;
}

int P_dns4_IP_to_name(char **hostname, P_ipv4_addr_t *ip)
{
	char *ips;

	ips = malloc(256);

	if (P_dns4_IP_to_addr(ip, ips) != 0)
		return -257;

	*hostname = NULL;
	return P_net_resolv(hostname, &ips);
}


#else

/* If return is non-zero, something went wrong! */
int P_dns4_name_to_IP(P_ipv4_addr_t *ip, const char *hostname)
{
	struct addrinfo req, *ans;
	int code;

	memset(&req, 0, sizeof(req));
	req.ai_flags    = 0;
	req.ai_family   = PF_INET;
	req.ai_socktype = SOCK_STREAM;
	req.ai_protocol = IPPROTO_TCP;

	if ((code = getaddrinfo(hostname, NULL, &req, &ans)) != 0)
		return code;

	*ip = ((struct sockaddr_in *)ans->ai_addr)->sin_addr.s_addr;

	freeaddrinfo(ans);

	return 0;
}

int P_dns4_IP_to_name(char **hostname, P_ipv4_addr_t *ip)
{
	struct sockaddr_in sa;
	char *buf;

	memset(&sa, 0, sizeof(sa));
	sa.sin_family      = AF_INET;
	sa.sin_addr.s_addr = *ip;

	/* we expect getnameinfo to be succesfull most of the time, so we allocate in
	   any way and then free if we failed */
	buf = malloc(P_NET_HOSTNAME_SIZE);
	if (getnameinfo((struct sockaddr *)&sa, sizeof(sa), buf, P_NET_HOSTNAME_SIZE, NULL, 0, 0)) {
		free(buf);
		return P_net_err_dns;
	}

	*hostname = buf;

	return 0;
}

#endif

char *P_dns4_get_my_name(void)
{
	char *buf;

	buf = malloc(P_NET_HOSTNAME_SIZE);
	gethostname(buf, P_NET_HOSTNAME_SIZE);

	return buf;
}

P_ipv4_addr_t P_dns4_get_my_IP(void)
{
	P_ipv4_addr_t ip;
	char *name = P_dns4_get_my_name();

	ip = 0xFFFFFFFF;
	P_dns4_name_to_IP(&ip, name);
	free(name);

	return ip;
}

int P_dns4_is_addr(const char *name)
{
	char tmp[4];
	char *d;
	const char *s;
	int c;

	*tmp = '\0';
	tmp[3] = '\0';

	for(s = name, c = 3, d = tmp;; s++) {
		if (isdigit(*s)) {
			/* None of the tags may be longer than 3 characters */
			c--;
			if (c < 0)
				return 0;

			*d = *s;
			d++;
		}
		else if ((*s == '.') || (*s == '\0')) { /* A separator */
			*d = '\0';
			c = atoi(tmp);
			/* a tag is out of range */
			if ((*tmp == '\0') || (c < 0) || (c > 255))
				return 0;

			/* Check if end of the string, if so, return true */
			if (*s == '\0')
				return 1;

			c		= 3;
			d		= tmp;
			*d	= 0;
		}
		else /* Contains a char that is neither a digit nor a dot */
			return 0;
	}

	/* We can't get here */
	return 0;
}

int P_dns4_IP_to_addr(P_ipv4_addr_t *ip_in, char *addr_out)
{
	P_ipv4_addr_t ip_ = htonl(*ip_in);
	unsigned char *ip = (unsigned char *)&ip_;
	char *s;
	int n;

	s = addr_out;
	for(n = 0; n < 4; n++) {
		s += sprintf(s, "%d.", ip[n]);
	}
	if (s > addr_out)
		s--;
	*s = '\0';
	return 0;
}

int P_dns4_addr_to_IP(P_ipv4_addr_t *ip_out, const char *addr_in)
{
	const char *start;
	char *end;
	int token, n;
	unsigned char *ip = (unsigned char *)ip_out;

	start = addr_in;
	for(n = 0; n < 4; n++) {
		token = strtol(start, &end, 10);
		if ((*end != '.') && (*end != '\0'))
			return -1;
		if ((*end == '\0') && (n != 3))
			return -2;
		ip[3-n] = token;
		start = end + 1;
	}
	*ip_out = ntohl(*ip_out);
	return 0;
}
/*
    libporty - network uninit chains
    Copyright (C) 2012  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Project page: http://repo.hu/projects/libporty
    Author: libporty@igor2.repo.hu
*/

#include <stdlib.h>

P_net_uninit_t *P_net_uninits = NULL;

void P_net_uninit_chain(void)
{
	P_net_uninit_t *u, *next;

	/* walk through the list and call each item then free it */
	for(u = P_net_uninits; u != NULL; u = next) {
		next = u->next;
		u->cb();
		free(u);
	}
	P_net_uninits = NULL;
}

void P_net_uninit_reg(void (*cb)())
{
	P_net_uninit_t *u;

	u = malloc(sizeof(P_net_uninit_t));
	u->cb = cb;
	u->next = P_net_uninits;
	P_net_uninits = u;
}
