/* $OpenBSD: src/usr.bin/tmux/cmd-list-sessions.c,v 1.11 2012/05/22 11:35:37 nicm Exp $ */

/*
 * Copyright (c) 2007 Nicholas Marriott <nicm@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <string.h>
#include <time.h>

#include "tmux.h"

/*
 * List all sessions.
 */

int	cmd_list_sessions_exec(struct cmd *, struct cmd_ctx *);

const struct cmd_entry cmd_list_sessions_entry = {
	"list-sessions", "ls",
	"F:", 0, 0,
	"[-F format]",
	0,
	NULL,
	NULL,
	cmd_list_sessions_exec
};

int
cmd_list_sessions_exec(struct cmd *self, struct cmd_ctx *ctx)
{
	struct args		*args = self->args;
	struct session		*s;
	u_int		 	 n;
	struct format_tree	*ft;
	const char		*template;
	char			*line;

	if ((template = args_get(args, 'F')) == NULL)
		template = DEFAULT_SESSION_TEMPLATE;

	n = 0;
	RB_FOREACH(s, sessions, &sessions) {
		ft = format_create();
		format_add(ft, "line", "%u", n);
		format_session(ft, s);

		line = format_expand(ft, template);
		ctx->print(ctx, "%s", line);
		xfree(line);

		format_free(ft);
		n++;
	}

	return (0);
}
