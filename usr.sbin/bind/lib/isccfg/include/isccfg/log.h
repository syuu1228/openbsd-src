/*
 * Copyright (C) 2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* $ISC: log.h,v 1.3.2.1 2001/10/16 05:58:29 marka Exp $ */

#ifndef ISCCFG_LOG_H
#define ISCCFG_LOG_H 1

#include <isc/lang.h>
#include <isc/log.h>

LIBISCCFG_EXTERNAL_DATA extern isc_logcategory_t cfg_categories[];
LIBISCCFG_EXTERNAL_DATA extern isc_logmodule_t cfg_modules[];

#define CFG_LOGCATEGORY_CONFIG	(&cfg_categories[0])

#define CFG_LOGMODULE_PARSER		(&cfg_modules[0])

ISC_LANG_BEGINDECLS

void
cfg_log_init(isc_log_t *lctx);
/*
 * Make the libisccfg categories and modules available for use with the
 * ISC logging library.
 *
 * Requires:
 *	lctx is a valid logging context.
 *
 *	cfg_log_init() is called only once.
 *
 * Ensures:
 * 	The catgories and modules defined above are available for
 * 	use by isc_log_usechannnel() and isc_log_write().
 */

ISC_LANG_ENDDECLS

#endif /* ISCCFG_LOG_H */
