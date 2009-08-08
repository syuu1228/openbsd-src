/* $OpenBSD: src/usr.bin/tmux/cmd-set-environment.c,v 1.1 2009/08/08 21:52:43 nicm Exp $ */

/*
 * Copyright (c) 2009 Nicholas Marriott <nicm@users.sourceforge.net>
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

#include <stdlib.h>
#include <string.h>

#include "tmux.h"

/*
 * Set an environment variable.
 */

int	cmd_set_environment_exec(struct cmd *, struct cmd_ctx *);

const struct cmd_entry cmd_set_environment_entry = {
	"set-environment", "setenv",
	"[-gru] " CMD_OPTION_SESSION_USAGE,
	0, CMD_CHFLAG('g')|CMD_CHFLAG('r')|CMD_CHFLAG('u'),
	NULL,
	cmd_option_parse,
	cmd_set_environment_exec,
	cmd_option_free,
	cmd_option_print
};

int
cmd_set_environment_exec(struct cmd *self, struct cmd_ctx *ctx)
{
	struct cmd_option_data	*data = self->data;
	struct session		*s;
	struct environ		*env;

	if (*data->option == '\0') {
		ctx->error(ctx, "empty variable name");
		return (-1);
	}
	if (strchr(data->option, '=') != NULL) {
		ctx->error(ctx, "variable name contains =");
		return (-1);
	}

	if (data->chflags & CMD_CHFLAG('g'))
		env = &global_environ;
	else {
		if ((s = cmd_find_session(ctx, data->target)) == NULL)
			return (-1);
		env = &s->environ;
	}

	if (data->chflags & CMD_CHFLAG('u')) {
		if (data->value != NULL) {
			ctx->error(ctx, "can't specify a value with -u");
			return (-1);
		}
		environ_unset(env, data->option);
	} else if (data->chflags & CMD_CHFLAG('r')) {
		if (data->value != NULL) {
			ctx->error(ctx, "can't specify a value with -r");
			return (-1);
		}
		environ_set(env, data->option, NULL);
	} else {
		if (data->value == NULL) {
			ctx->error(ctx, "no value specified");
			return (-1);
		}
		environ_set(env, data->option, data->value);
	}

	return (0);
}
