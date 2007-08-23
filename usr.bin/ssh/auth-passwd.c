/* $OpenBSD: src/usr.bin/ssh/auth-passwd.c,v 1.42 2007/08/23 02:55:51 djm Exp $ */
/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Password authentication.  This file contains the functions to check whether
 * the password is valid for the user.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 *
 * Copyright (c) 1999 Dug Song.  All rights reserved.
 * Copyright (c) 2000 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>

#include <login_cap.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "packet.h"
#include "buffer.h"
#include "log.h"
#include "servconf.h"
#include "key.h"
#include "hostfile.h"
#include "auth.h"
#include "auth-options.h"

extern Buffer loginmsg;
extern ServerOptions options;
int sys_auth_passwd(Authctxt *, const char *);

extern login_cap_t *lc;

#define DAY		(24L * 60 * 60) /* 1 day in seconds */
#define TWO_WEEKS	(2L * 7 * DAY)	/* 2 weeks in seconds */

static void
disable_forwarding(void)
{
	no_port_forwarding_flag = 1;
	no_agent_forwarding_flag = 1;
	no_x11_forwarding_flag = 1;
}

/*
 * Tries to authenticate the user using password.  Returns true if
 * authentication succeeds.
 */
int
auth_password(Authctxt *authctxt, const char *password)
{
	struct passwd * pw = authctxt->pw;
	int ok = authctxt->valid;

	if (pw->pw_uid == 0 && options.permit_root_login != PERMIT_YES)
		ok = 0;
	if (*password == '\0' && options.permit_empty_passwd == 0)
		return 0;
#ifdef KRB5
	if (options.kerberos_authentication == 1) {
		int ret = auth_krb5_password(authctxt, password);
		if (ret == 1 || ret == 0)
			return ret && ok;
		/* Fall back to ordinary passwd authentication. */
	}
#endif
	return (sys_auth_passwd(authctxt, password) && ok);
}

#ifdef BSD_AUTH
static void
warn_expiry(Authctxt *authctxt, auth_session_t *as)
{
	char buf[256];
	quad_t pwtimeleft, actimeleft, daysleft, pwwarntime, acwarntime;

	pwwarntime = acwarntime = TWO_WEEKS;

	pwtimeleft = auth_check_change(as);
	actimeleft = auth_check_expire(as);
	if (authctxt->valid) {
		pwwarntime = login_getcaptime(lc, "password-warn", TWO_WEEKS,
		    TWO_WEEKS);
		acwarntime = login_getcaptime(lc, "expire-warn", TWO_WEEKS,
		    TWO_WEEKS);
	}
	if (pwtimeleft != 0 && pwtimeleft < pwwarntime) {
		daysleft = pwtimeleft / DAY + 1;
		snprintf(buf, sizeof(buf),
		    "Your password will expire in %lld day%s.\n",
		    daysleft, daysleft == 1 ? "" : "s");
		buffer_append(&loginmsg, buf, strlen(buf));
	}
	if (actimeleft != 0 && actimeleft < acwarntime) {
		daysleft = actimeleft / DAY + 1;
		snprintf(buf, sizeof(buf),
		    "Your account will expire in %lld day%s.\n",
		    daysleft, daysleft == 1 ? "" : "s");
		buffer_append(&loginmsg, buf, strlen(buf));
	}
}

int
sys_auth_passwd(Authctxt *authctxt, const char *password)
{
	struct passwd *pw = authctxt->pw;
	auth_session_t *as;
	static int expire_checked = 0;

	as = auth_usercheck(pw->pw_name, authctxt->style, "auth-ssh",
	    (char *)password);
	if (as == NULL)
		return (0);
	if (auth_getstate(as) & AUTH_PWEXPIRED) {
		auth_close(as);
		disable_forwarding();
		authctxt->force_pwchange = 1;
		return (1);
	} else {
		if (!expire_checked) {
			expire_checked = 1;
			warn_expiry(authctxt, as);
		}
		return (auth_close(as));
	}
}
#else
int
sys_auth_passwd(Authctxt *authctxt, const char *password)
{
	struct passwd *pw = authctxt->pw;
	char *encrypted_password;

	/* Check for users with no password. */
	if (strcmp(password, "") == 0 && strcmp(pw->pw_passwd, "") == 0)
		return (1);

	/* Encrypt the candidate password using the proper salt. */
	encrypted_password = crypt(password,
	    (pw->pw_passwd[0] && pw->pw_passwd[1]) ?
	    pw->pw_passwd : "xx");

	/*
	 * Authentication is accepted if the encrypted passwords
	 * are identical.
	 */
	return (strcmp(encrypted_password, pw->pw_passwd) == 0);
}
#endif
