/*	$OpenBSD: src/sys/arch/loongson/include/autoconf.h,v 1.8 2010/08/31 10:24:46 pirofti Exp $ */

/*
 * Copyright (c) 2001-2003 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Definitions used by autoconfiguration.
 */

#ifndef _MACHINE_AUTOCONF_H_
#define _MACHINE_AUTOCONF_H_

#include <machine/bus.h>

struct bonito_config;
struct mips_isa_chipset;

/*
 * List of legacy I/O ranges.
 */
struct legacy_io_range {
	bus_addr_t	start;
	bus_size_t	end;	/* inclusive */
};

/*
 * Per platform information.
 */
struct platform {
	int				 system_type;
	char				*vendor;
	char				*product;

	const struct bonito_config	*bonito_config;
	struct mips_isa_chipset		*isa_chipset;
	const struct legacy_io_range	*legacy_io_ranges;

	void				(*setup)(void);
	void				(*device_register)(struct device *,
					    void *);

	void				(*powerdown)(void);
	void				(*reset)(void);
	int				(*suspend)(void);
	int				(*resume)(void);
};

extern const struct platform *sys_platform;
extern uint loongson_ver;

struct mainbus_attach_args {
	const char	*maa_name;
};

extern struct device *bootdv;
extern char bootdev[];
extern enum devclass bootdev_class;

#include <mips64/autoconf.h>

#endif /* _MACHINE_AUTOCONF_H_ */
