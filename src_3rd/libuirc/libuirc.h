#include <genvector/gds_char.h>
#include <libporty/net/network.h>

#define UIRC_MAX_QUERIES 16

typedef enum {
	UIRC_UNUSED = 0,
	UIRC_SERVER,
	UIRC_CHAN,
	UIRC_PRIV
} uirc_query_type_t;

typedef struct {
	uirc_query_type_t type;
	char *name;

#if 0
	/* channel: */
	gds_t *nicks; /* , separated list of nicknames for a channel; first char is one of @, + or space */
	char *topic;
#endif
} uirc_query_t;

typedef enum { /* bitfield */
	UIRC_CONNECT          = 0x0001,
	UIRC_DISCONNECT       = 0x0002,
	UIRC_TOPIC            = 0x0004,
	UIRC_GOT_MISC         = 0x0008,
	UIRC_QUERY_BEGIN      = 0x0010,
	UIRC_QUERY_END        = 0x0020,
	UIRC_ME_QUIT          = 0x0040,
	UIRC_ME_JOIN          = 0x0080,
	UIRC_JOIN             = 0x0100,
	UIRC_ME_PART          = 0x0200,
	UIRC_PART             = 0x0400,
	UIRC_MSG              = 0x0800,
	UIRC_NOTICE           = 0x1000,
	UIRC_KICK             = 0x2000,
} uirc_event_t;

typedef struct uirc_s uirc_t;
struct uirc_s {
	char *nick;
	uirc_query_t query[UIRC_MAX_QUERIES]; /* query 0 is special: server "window" */
	int curr_query, last_new_query;

	void *user_data;
	void (*on_connect)(uirc_t *ctx);
	void (*on_disconnect)(uirc_t *ctx);
	void (*on_got_misc)(uirc_t *ctx, int query_to, const char *msg);
	void (*on_query_begin)(uirc_t *ctx, int query_to);
	void (*on_query_end)(uirc_t *ctx, int query_to); /* called on leaving the channel for any reason */
	void (*on_me_quit)(uirc_t *ctx);
	void (*on_me_join)(uirc_t *ctx, int query, char *chan);
	void (*on_join)(uirc_t *ctx, char *nick, int query, char *chan);
	void (*on_me_part)(uirc_t *ctx, int query, char *chan);
	void (*on_part)(uirc_t *ctx, char *nick, int query, char *chan, char *reason);
	void (*on_kick)(uirc_t *ctx, char *nick, int query, char *chan, char *victim, char *reason);
	void (*on_msg)(uirc_t *ctx, char *from, int query, char *to, char *text);
	void (*on_notice)(uirc_t *ctx, char *from, int query, char *to, char *text);
	void (*on_topic)(uirc_t *ctx, char *from, int query, char *to, char *text);

	/* internal */
	P_net_socket sk;
	gds_t ibuf, obuf;
	int dummy[2];
	unsigned connecting:1;
	unsigned alive:1;
};

int uirc_connect(uirc_t *ctx, const char *server, int port, char *user);
void uirc_disconnect(uirc_t *ctx);
uirc_event_t uirc_poll(uirc_t *ctx);

void uirc_join(uirc_t *ctx, const char *chan);
void uirc_close(uirc_t *ctx, int query);
void uirc_privmsg(uirc_t *ctx, const char *target, const char *msg);
void uirc_raw(uirc_t *ctx, const char *msg);

