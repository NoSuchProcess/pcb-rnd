/*
    libuirc - minimal IRC protocol
    Copyright (C) 2020  Tibor 'Igor2' Palinkas

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

    Author: libporty (at) igor2.repo.hu
*/


#include "libuirc.h"
#include <librnd/core/compat_misc.h>

#include <stdio.h>

static int uirc_query_alloc(uirc_t *ctx)
{
	int n;
	for(n = 0; n < UIRC_MAX_QUERIES; n++)
		if (ctx->query[n].type == UIRC_UNUSED)
			return n;
	return -1;
}

static int uirc_query_search(uirc_t *ctx, char *name)
{
	int n;
	for(n = 0; n < UIRC_MAX_QUERIES; n++) {
		if ((ctx->query[n].type != UIRC_CHAN) && (ctx->query[n].type != UIRC_PRIV))
			continue;
		if (strcmp(name, ctx->query[n].name) == 0)
			return n;
	}
	return -1;
}


int uirc_connect(uirc_t *ctx, const char *server, int port, char *user)
{
	if (P_tcp4_connect(&ctx->sk, server, port, NULL, 0, P_net_nonblock_full) != 0)
		return -1;
	ctx->connecting = 1;
	ctx->alive = 1;
	gds_init(&ctx->ibuf);
	gds_init(&ctx->obuf);
	gds_append_str(&ctx->obuf, "user libuirc a b :");
	gds_append_str(&ctx->obuf, user);
	gds_append_str(&ctx->obuf, "\nnick ");
	gds_append_str(&ctx->obuf, ctx->nick);
	gds_append(&ctx->obuf, '\n');

	ctx->query[0].type = UIRC_SERVER;
	ctx->query[0].name = NULL;

	return 0;
}

void uirc_disconnect(uirc_t *ctx)
{
	P_net_close(ctx->sk);
	ctx->alive = 0;
}


static uirc_event_t uirc_parse_001(uirc_t *ctx, char *arg)
{
	if (ctx->on_connect != NULL)
		ctx->on_connect(ctx);
	return UIRC_CONNECT;
}

#define irc_strcasecmp rnd_strcasecmp

static uirc_event_t uirc_parse_join(uirc_t *ctx, char *nick, char *arg)
{
	int q;
/*:libuirc!~libuirc@78-131-56-146.static.hdsnet.hu JOIN :#dev*/

	if (*arg == ':') arg++;

	q = uirc_query_search(ctx, arg);
	if (irc_strcasecmp(nick, ctx->nick) == 0) {
		uirc_event_t res = 0;
		if (q < 0) {
			q = uirc_query_alloc(ctx);
			if (q < 0) {
				gds_append_str(&ctx->obuf, "part ");
				gds_append_str(&ctx->obuf, arg);
				gds_append_str(&ctx->obuf, " :I am on too many channels already.\n");
				return 0;
			}
			ctx->query[q].type = UIRC_CHAN;
			ctx->query[q].name = rnd_strdup(arg);
			ctx->last_new_query = q;
			res |= UIRC_QUERY_BEGIN;
		}

		if (ctx->on_me_join != NULL)
			ctx->on_me_join(ctx, q, arg);
		return res | UIRC_ME_JOIN;
	}

	if ((q >= 0) && (ctx->on_join != NULL)) {
		ctx->on_join(ctx, nick, q, arg);
		return UIRC_JOIN;
	}

	return 0;
}

#define payload(dst, src) \
do { \
	dst = strpbrk(src, " \t"); \
	if (dst != NULL) { \
		*dst++ = '\0'; \
		if (*dst == ':') dst++; \
	} \
} while(0)

static uirc_event_t uirc_parse_part(uirc_t *ctx, char *nick, char *arg)
{
	char *reason;
	int q;

	if (*arg == ':') arg++;
	payload(reason, arg);

	q = uirc_query_search(ctx, arg);
	if (q < 0)
		return 0;

	if (irc_strcasecmp(nick, ctx->nick) == 0) {
		if (ctx->on_me_part != NULL)
			ctx->on_me_part(ctx, q, arg);
		if (ctx->on_query_end != NULL)
			ctx->on_query_end(ctx, q);
		ctx->query[q].type = UIRC_UNUSED;
		free(ctx->query[q].name);
		ctx->query[q].name = NULL;
		return UIRC_ME_PART | UIRC_QUERY_END;
	}

	if (ctx->on_part != NULL) {
		ctx->on_part(ctx, nick, q, arg, reason);
		return UIRC_PART;
	}

	return 0;
}

static uirc_event_t uirc_parse_kick(uirc_t *ctx, char *nick, char *arg)
{
	char *victim, *reason;
	int q;
	uirc_event_t res = 0;

	payload(victim, arg);
	payload(reason, victim);

	q = uirc_query_search(ctx, arg);
	if (q < 0)
		return 0;

	if (irc_strcasecmp(victim, ctx->nick) == 0) {
		if (ctx->on_me_part != NULL)
			ctx->on_me_part(ctx, q, arg);
		if (ctx->on_query_end != NULL)
			ctx->on_query_end(ctx, q);
		ctx->query[q].type = UIRC_UNUSED;
		free(ctx->query[q].name);
		ctx->query[q].name = NULL;
		res |= UIRC_ME_PART | UIRC_QUERY_END;
	}

	if (ctx->on_kick != NULL) {
		ctx->on_kick(ctx, nick, q, arg, victim, reason);
		res |= UIRC_KICK;
	}

	return res;
}

static uirc_event_t uirc_parse_msg(uirc_t *ctx, char *nick, char *arg)
{
	char *text;
	int q;

	if (*arg == ':') arg++;
	payload(text, arg);

	q = uirc_query_search(ctx, arg);

	if (ctx->on_msg != NULL) {
		ctx->on_msg(ctx, nick, q, arg, text);
		return UIRC_MSG;
	}

	return 0;
}

static uirc_event_t uirc_parse_notice(uirc_t *ctx, char *nick, char *arg)
{
	char *text;
	int q;

	if (*arg == ':') arg++;
	payload(text, arg);

	q = uirc_query_search(ctx, arg);

	if (ctx->on_notice != NULL) {
		ctx->on_notice(ctx, nick, q, arg, text);
		return UIRC_NOTICE;
	}

	return 0;
}

static uirc_event_t uirc_parse_topic(uirc_t *ctx, char *nick, char *arg, int numeric)
{
	char *text;
	int q;

	if (*arg == ':') arg++;
	payload(text, arg);

	q = uirc_query_search(ctx, arg);

	if (ctx->on_topic != NULL) {
		if (numeric)
			nick = NULL;
		ctx->on_topic(ctx, nick, q, arg, text);
		return UIRC_TOPIC;
	}

	return 0;
}

static void uirc_new_nick(uirc_t *ctx)
{
	int len = strlen(ctx->nick);
	if (len == 0) {
		free(ctx->nick);
		ctx->nick = rnd_strdup("libuirc");
		len = 7;
	}
	if (len < 9) {
		char *nick = malloc(len+2);
		memcpy(nick, ctx->nick, len);
		nick[len] = (rand() % ('a'-'z')) + 'a';
		nick[len+1] = '\0';
		free(ctx->nick);
		ctx->nick = nick;
	}
	else
		ctx->nick[rand() % len] = (rand() % ('a'-'z')) + 'a';
	gds_append_str(&ctx->obuf, "nick ");
	gds_append_str(&ctx->obuf, ctx->nick);
	gds_append(&ctx->obuf, '\n');
}

static uirc_event_t uirc_parse(uirc_t *ctx, char *line)
{
	char *end, *arg, *cmd, *from = line;
	uirc_event_t res = 0;

#ifdef LIBUIRC_TRACE
printf("line='%s'\n", line);
#endif

	if (strncmp(line, "PING", 4) == 0) {
		from[1] = 'O';
		uirc_raw(ctx, line);
		return 0;
	}

	cmd = strpbrk(line, " \t\r\n");
	if (cmd == NULL)
		return 0;
	*cmd = '\0';
	cmd++;

	if (strchr(from, '@') == 0) {
		if (ctx->on_got_misc != NULL)
			ctx->on_got_misc(ctx, 0, cmd);
		res |= UIRC_GOT_MISC;
	}

	end = strchr(from, '!');
	if (end != NULL) {
		if (*from == ':')
			from++;
		*end = 0;
	}

	arg = strpbrk(cmd, " \t\r\n");
	if (arg != NULL) {
		*arg = '\0';
		arg++;
	}

	end = strpbrk(arg, "\r\n");
	if (end != NULL)
		*end = '\0';

	if (strcmp(cmd, "001") == 0) return res | uirc_parse_001(ctx, arg);
	if (irc_strcasecmp(cmd, "join") == 0) return res | uirc_parse_join(ctx, from, arg);
	if (irc_strcasecmp(cmd, "part") == 0) return res | uirc_parse_part(ctx, from, arg);
	if (irc_strcasecmp(cmd, "kick") == 0) return res | uirc_parse_kick(ctx, from, arg);
	if (irc_strcasecmp(cmd, "privmsg") == 0) return res | uirc_parse_msg(ctx, from, arg);
	if (irc_strcasecmp(cmd, "notice") == 0) return res | uirc_parse_notice(ctx, from, arg);
	if (irc_strcasecmp(cmd, "topic") == 0) return res | uirc_parse_topic(ctx, from, arg, 0);
	if (irc_strcasecmp(cmd, "332") == 0) return res | uirc_parse_topic(ctx, from, arg, 1);
	if (irc_strcasecmp(cmd, "433") == 0) uirc_new_nick(ctx);

	return res;
}

uirc_event_t uirc_poll(uirc_t *ctx)
{
	struct P_pollfd fds[1];
	int new_data = 0;
	char *nl;
	uirc_event_t res = 0;

	fds[0].fd = ctx->sk;

	for(;;) {
		fds[0].events = P_POLLIN;
		if (ctx->obuf.used > 0)
			fds[0].events |= P_POLLOUT;
		
		if (P_poll(fds, 1, 0) < 1)
			break;

		if (fds[0].revents & P_POLLIN) {
			P_size_t oused = ctx->ibuf.used;
			gds_enlarge(&ctx->ibuf, ctx->ibuf.used + 600);
			ctx->ibuf.used = oused;
			P_size_t len = P_net_read(ctx->sk, ctx->ibuf.array + ctx->ibuf.used, 600);
			if (len < 0) {
				if (P_net_inprogress)
					continue;
				uirc_disconnect(ctx);
				return res | UIRC_DISCONNECT;
			}
			if (len == 0) {
				uirc_disconnect(ctx);
				return res | UIRC_DISCONNECT;
			}
			if (len > 0) {
				ctx->ibuf.used += len;
				new_data = 1;
			}
		}
		if ((fds[0].revents & P_POLLOUT) && (ctx->obuf.used > 0)) {
			P_size_t len = P_net_write(ctx->sk, ctx->obuf.array, ctx->obuf.used);
			ctx->connecting = 0;
			if (len < 0) {
				if (P_net_inprogress)
					continue;
				uirc_disconnect(ctx);
				return res | UIRC_DISCONNECT;
			}
			if (len == 0) {
				uirc_disconnect(ctx);
				return res | UIRC_DISCONNECT;
			}
			if (len > 0)
				gds_remove(&ctx->obuf, 0, len);
		}
		if (fds[0].revents & (~(P_POLLIN | P_POLLOUT))) {
			uirc_disconnect(ctx);
			return res | UIRC_DISCONNECT;
		}
	}

	/* call the protocol parse */
	if (!new_data)
		return res;

	for(;;) {
		nl = strchr(ctx->ibuf.array, '\n');
		if (nl == NULL)
			break;
		*nl = '\0';
		res |= uirc_parse(ctx, ctx->ibuf.array);
		gds_remove(&ctx->ibuf, 0, nl - ctx->ibuf.array+1);
	}
	return res;
}


void uirc_join(uirc_t *ctx, const char *chan)
{

}

void uirc_close(uirc_t *ctx, int query)
{

}

void uirc_privmsg(uirc_t *ctx, const char *target, const char *msg)
{
	gds_append_str(&ctx->obuf, "PRIVMSG ");
	gds_append_str(&ctx->obuf, target);
	gds_append_str(&ctx->obuf, " :");
	gds_append_str(&ctx->obuf, (char *)msg);
	gds_append(&ctx->obuf, '\n');
}

void uirc_raw(uirc_t *ctx, const char *msg)
{
	gds_append_str(&ctx->obuf, (char *)msg);
	gds_append(&ctx->obuf, '\n');
}


