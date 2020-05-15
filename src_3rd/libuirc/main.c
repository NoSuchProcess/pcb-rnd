#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "libuirc.h"

static void on_me_part(uirc_t *ctx, int query, char *chan)
{
	printf("### left channel %s\n", chan);
}

static void on_msg(uirc_t *ctx, char *from, int query, char *to, char *text)
{
	printf("(msg:%s) <%s> %s\n", to, from, text);
}

static void on_notice(uirc_t *ctx, char *from, int query, char *to, char *text)
{
	printf("(msg:%s) -%s- %s\n", to, from, text);
}


static void on_topic(uirc_t *ctx, char *from, int query, char *to, char *text)
{
	printf("(by:%s) [%s] topic=%s\n", from, to, text);
}

static void on_kick(uirc_t *ctx, char *nick, int query, char *chan, char *victim, char *reason)
{
	printf("(kick: %s kick %s from %s reason=%s\n", nick, victim, chan, reason);
}


static void read_stdin(uirc_t *ctx)
{
	struct P_pollfd fds[1];

	fds[0].fd = 0;
	fds[0].events = P_POLLIN;

	if (P_poll(fds, 1, 0) == 1) {
		char line[600];
		fgets(line, sizeof(line), stdin);
		if (*line == '/') {
			uirc_raw(ctx, line+1);
		}
		else {
			if (ctx->query[ctx->curr_query].name != NULL)
				uirc_privmsg(ctx, ctx->query[ctx->curr_query].name, line);
		}
	}
}

int main()
{
	uirc_t irc;

	memset(&irc, 0, sizeof(irc));
	irc.on_me_part = on_me_part;
	irc.on_msg = on_msg;
	irc.on_notice = on_notice;
	irc.on_topic = on_topic;
	irc.on_kick = on_kick;

	irc.nick = strdup("libuirc");
	if (uirc_connect(&irc, "10.0.0.2", 6667, "libuirc test client") != 0) {
		printf("Failed to connect.\n");
		return 1;
	}

	for(;irc.alive;) {
		uirc_event_t ev = uirc_poll(&irc);
		if (ev & UIRC_CONNECT) {
			printf("joining\n");
			uirc_raw(&irc, "join :#dev");
		}
		if (ev & UIRC_QUERY_BEGIN) {
			irc.curr_query = irc.last_new_query;
			printf("### begin query with %s\n", irc.query[irc.curr_query].name);
		}

		read_stdin(&irc);
		usleep(100);
	}
	
	return 0;
}

