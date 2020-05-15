
#include <unistd.h>
#include "os_includes.h"
#include "pnet_config.h"
#include "phost_types.h"


/* UNIX */
#if defined(UNIX)

/* get netdb.h to declare addrinfo on glibc */
#ifndef __USE_POSIX
# define __USE_POSIX
#endif

#	define P_net_socket int
#	define P_NET_INVALID_SOCKET -1
#	define GET_LAST_ERROR() (errno)

	typedef struct ifreq IFREQ;
#	include <errno.h>
#	include <netdb.h>

#	if defined(__GLIBC__) && (__GLIBC__ <= 2) && (__GLIBC_MINOR__ <= 1)
		typedef uint32_t in_addr_t;
#	endif
#endif
/* UNIX */


/* WIN32 */
#if defined(WIN32)

#	define P_net_socket SOCKET
#	define P_NET_INVALID_SOCKET -1

# include <stdio.h>

#	if !(defined(__MINGW32__) || defined(__CYGWIN__))
		typedef SSIZE_T ssize_t;
		typedef int socklen_t;
#	endif

#	define GET_LAST_ERROR() WSAGetLastError()
#	define EWOULDBLOCK WSAEWOULDBLOCK

	typedef unsigned long in_addr_t;
	typedef INTERFACE_INFO IFREQ;
#endif
/* WIN32 */

#ifndef P_HAVE_IOCTLSOCKET
#	define P_ioctl_socket ioctl
#else
#	define P_ioctl_socket ioctlsocket
#endif

/* os_net_inprogress: an expression to check for "in progress" after a call (so it's not returned as an error) */
#if defined(UNIX)
#	define P_net_inprogress	((errno == EINPROGRESS) || (errno == EAGAIN))
#elif defined(WIN32)
#	define P_net_inprogress	((WSAGetLastError() == WSAEINPROGRESS) || (WSAGetLastError() == WSAEWOULDBLOCK))
#else
#	error einprogress not ported
#endif


/* === P_poll() start === */
#ifdef P_NET_POLL

/* use native poll */
#define P_POLLIN     POLLIN
#define P_POLLPRI    POLLPRI
#define P_POLLOUT    POLLOUT
#define P_POLLERR    POLLERR
#define P_POLLHUP    POLLHUP
#define P_POLLNVAL   POLLNVAL

#define P_poll poll
#define P_pollfd pollfd

#else
/* define our own struct and constants */
struct P_pollfd {
	int fd;
	short events;
	short revents;
};

#define P_POLLIN     0x001
#define P_POLLPRI    0x002
#define P_POLLOUT    0x004
#define P_POLLERR    0x008
#define P_POLLHUP    0x010
#define P_POLLNVAL   0x020

typedef unsigned long int nfds_t;

# ifdef P_NET_SELECT
#  define P_poll P_poll_select
int P_poll_select(struct P_pollfd *fds, nfds_t nfds, int timeout);
# else
#  define P_poll P_poll_busyloop
int P_poll_busyloop(struct P_pollfd *fds, nfds_t nfds, int timeout);
# endif
#endif
/* === P_poll() end === */

#ifdef P_HAVE_SHUT
#	define P_SHUT_RD SHUT_RD
#	define P_SHUT_WR SHUT_WR
#	define P_SHUT_RDWR SHUT_RDWR
#else
#	ifdef P_HAVE_SD
#		define P_SHUT_RD 0
#		define P_SHUT_WR 1
#		define P_SHUT_RDWR 2
#	else
#		define P_SHUT_RD SD_RECEIVE
#		define P_SHUT_WR SD_SEND
#		define P_SHUT_RDWR SD_BOTH
#	endif
#endif

#ifdef WIN32
P_net_socket P_socket(int af, int protocol, int type);
#else
#define P_socket socket
#endif
#ifndef P_NETWORK_H
#define P_NETWORK_H

/* address type for IPv4 IP addresses */
typedef P_uint32_t P_ipv4_addr_t;

/* Must be called before any other network related call; returns  0 on success */
int P_net_init(void);

/* Must be called at the end of the program; on some system this is required to
   shut down open connections. */
int P_net_uninit(void);



typedef enum P_net_err {
	P_net_err_dns       = -1,
	P_net_err_socket    = -2,
	P_net_err_reuse     = -3,
	P_net_err_nonblock  = -4,
	P_net_err_bind      = -5,
	P_net_err_listen    = -6,
	P_net_err_connect   = -7,
	P_net_err_broadcast = -8,
	P_net_err_accept    = -9,
	P_net_err_alloc     = -10
} P_net_err_t ;

typedef enum P_net_nonblock_e {
	P_net_nonblock_connect            = 1,    /* switch to non-blocking for connect */
	P_net_nonblock_socket             = 2,    /* switch to non-blocking after connect */
	P_net_nonblock_full               = 3,    /* do everything in non-blocking */
	P_net_nonblock_quit_on_error      = 4     /* quit if can't be set to nonblock */
} P_net_nonblock_t;

typedef enum P_net_broadcast {
	P_net_broadcast_socket            = 1,    /* switch the socket to broadcast */
	P_net_broadcast_quit_on_error     = 2     /* quit if can't be set to broadcast */
} P_net_broadcast_t;

int P_net_close(P_net_socket s);

/* use the following two calls exclusively for reading and writing sockets */
P_size_t P_net_read(P_net_socket s, void *buf, P_size_t count);
P_size_t P_net_write(P_net_socket s, void *buf, P_size_t count);
extern int P_net_printf_minbuff, P_net_printf_maxbuff; /* buffer size control (writable); irrelevant on systems supporting vdprintf() */
P_size_t P_net_printf(P_net_socket s, const char *format, ...);

/* set socket properties */
int P_net_set_nonblocking(int sock);
int P_net_set_blocking(int sock);
int P_net_enable_broadcast(int sock);

/* shutdown() compatibility */
#define P_net_shutdown(s, how) shutdown(s, how)

#endif /* P_NETWORK_H */
#ifndef P_NET_TCP4_H
#define P_NET_TCP4_H


int P_tcp4_listen(P_net_socket *sock, const char *loc_hostname, int loc_port, P_net_nonblock_t non_blocking, int backlog);
int P_tcp4_connect(P_net_socket *sock, const char *rem_hostname, int rem_port, const char *loc_hostname, int loc_port, P_net_nonblock_t non_blocking);

int P_tcp4_accept(P_net_socket *new_socket, P_net_socket listen_sock, P_ipv4_addr_t *rem_ip, char **rem_host, int *rem_port, P_net_nonblock_t non_blocking);

#endif /* P_NET_TCP4_H */
#ifndef P_NET_DNS4_H
#define P_NET_DNS4_H


/* Resolve hostname to IP; returns 0 on success. */
int P_dns4_name_to_IP(P_ipv4_addr_t *ip, const char *hostname);

/* Resolve IP to hostname; returns 0 on success. */
int P_dns4_IP_to_name(char **hostname, P_ipv4_addr_t *ip);

/* Returns host's hostname */
char *P_dns4_get_my_name(void);

/* Returns host's IP address */
P_ipv4_addr_t P_dns4_get_my_IP(void);

/* Returns non-zero if name is a valid IP address */
int P_dns4_is_addr(const char *name);


/* Convert ipv4 address to string; string must be long enough (>=16 characers)
   Return 0 on success. */
int P_dns4_IP_to_addr(P_ipv4_addr_t *ip_in, char *addr_out);

/* Convert ipv4 string to address
   Return 0 on success. */
int P_dns4_addr_to_IP(P_ipv4_addr_t *ip_out, const char *addr_in);

#endif /* P_NET_DNS4_H */
/* register a callback for P_net_uninit() */
void P_net_uninit_reg(void (*cb)());


/* interanls */

typedef struct P_net_uninit_s P_net_uninit_t;

struct P_net_uninit_s {
	void (*cb)(void);
	P_net_uninit_t *next;
};

extern P_net_uninit_t *P_net_uninits;


/* call all pending uninit callbacks and free resources */
void P_net_uninit_chain(void);

