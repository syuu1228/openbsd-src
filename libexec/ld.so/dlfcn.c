/*	$OpenBSD: src/libexec/ld.so/dlfcn.c,v 1.7 2001/08/09 02:42:12 drahn Exp $ */

/*
 * Copyright (c) 1998 Per Fogelstrom, Opsycon AB
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed under OpenBSD by
 *	Per Fogelstrom, Opsycon AB, Sweden.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#define _DYN_LOADER

#include <sys/types.h>
#include <nlist.h>
#include <link.h>
#include <dlfcn.h>

#include "syscall.h"
#include "archdep.h"
#include "resolve.h"

int _dl_errno;

void _dl_show_objects(void);

static int _dl_real_close(void *handle);
static void _dl_unload_deps(elf_object_t *object);

void *
dlopen(const char *libname, int how)
{
	elf_object_t	*object;
	elf_object_t	*dynobj;
	Elf_Dyn	*dynp;

	DL_DEB(("loading: %s\n", libname));

	object = _dl_load_shlib(libname, _dl_objects, OBJTYPE_DLO);
	if (object == 0) {
		return((void *)0);
	}

	if (object->refcount > 1) {
		return((void *)object);	/* Already loaded */
	}

	/*
	 *	Check for 'needed' objects. For each 'needed' object we
	 *	create a 'shadow' object and add it to a list attached to
	 *	the object so we know our dependencies. This list should
	 *	also be used to determine the library search order when
	 *	resolving undefined symbols. This is not yet done. XXX
	 */
	dynobj = object;
	while (dynobj) {
		elf_object_t *tmpobj = dynobj;

                for (dynp = dynobj->load_dyn; dynp->d_tag; dynp++) {
			const char *libname;
			elf_object_t *depobj;

                        if (dynp->d_tag != DT_NEEDED) {
				continue;
			}
			libname = dynobj->dyn.strtab + dynp->d_un.d_val;
			depobj = _dl_load_shlib(libname, dynobj, OBJTYPE_DLO);
			if (!depobj) {
				_dl_exit(4);
			}
			tmpobj->dep_next = _dl_malloc(sizeof(elf_object_t));
			tmpobj->dep_next->next = depobj;
			tmpobj = tmpobj->dep_next;
                }
                dynobj = dynobj->next;
        }

	_dl_rtld(object);
	_dl_call_init(object);

#ifdef __mips__
	if(_dl_debug_map->r_brk) {
		_dl_debug_map->r_state = RT_ADD;
		(*((void (*)())_dl_debug_map->r_brk))();
		_dl_debug_map->r_state = RT_CONSISTENT;
		(*((void (*)())_dl_debug_map->r_brk))();
	}
#endif /* __mips__ */

	return((void *)object);
}

void *
dlsym(void *handle, const char *name)
{
	elf_object_t	*object;
	elf_object_t	*dynobj;
	void		*retval;
	const Elf_Sym	*sym = 0;

	object = (elf_object_t *)handle;
	dynobj = _dl_objects;
	while (dynobj && dynobj != object) {
		dynobj = dynobj->next;
	}
	if (!dynobj || object != dynobj) {
		_dl_errno = DL_INVALID_HANDLE;
		return(0);
	}

	retval = (void *)_dl_find_symbol(name, object, &sym, 1, 1);
	if (retval) {
		retval += sym->st_value;
	} else {
		_dl_errno = DL_NO_SYMBOL;
	}

	return (retval);
}

int
dlctl(void *handle, int command, void *data)
{
	switch (command) {

#ifdef __mips__
	case DL_DUMP_MAP:
		_dl_show_objects();
		return(0);
#endif /* __mips__ */

	default:
		_dl_errno = DL_INVALID_CTL;
		break;
	}
	return(-1);
}

int
dlclose(void *handle)
{
	int retval;

	retval = _dl_real_close(handle);

#ifdef __mips__
	if (_dl_debug_map->r_brk) {
		_dl_debug_map->r_state = RT_DELETE;
		(*((void (*)())_dl_debug_map->r_brk))();
		_dl_debug_map->r_state = RT_CONSISTENT;
		(*((void (*)())_dl_debug_map->r_brk))();
	}
#endif /* __mips__ */

	return (retval);
}

static int
_dl_real_close(void *handle)
{
	elf_object_t	*object;
	elf_object_t	*dynobj;

	object = (elf_object_t *)handle;
	dynobj = _dl_objects;
	while (dynobj && dynobj != object) {
		dynobj = dynobj->next;
	}
	if (!dynobj || object != dynobj) {
		_dl_errno = DL_INVALID_HANDLE;
		return (1);
	}

	if (object->refcount == 1) {
		if (dynobj->dep_next) {
			_dl_unload_deps(dynobj);
		}
	}

	_dl_unload_shlib(object);
	return (0);
}

/*
 *	Scan through the shadow dep list and 'unload' every library
 *	we depend upon. Shadow objects are removed when removing ourself.
 */
static void
_dl_unload_deps(elf_object_t *object)
{
	elf_object_t *depobj;

	depobj = object->dep_next;
	while (depobj) {
		if (depobj->next->refcount == 1) { /* This object will go away */
			if (depobj->next->dep_next) {
				_dl_unload_deps(depobj->next);
			}
			_dl_unload_shlib(depobj->next);
		}
		depobj = depobj->dep_next;
	}
}

/*
 *	dlerror()
 *
 *	Return a character string describing the last dl... error occured.
 */
const char *
dlerror()
{
	switch (_dl_errno) {
	case 0:	/* NO ERROR */
		return (NULL);
	case DL_NOT_FOUND:
		return ("File not found");
	case DL_CANT_OPEN:
		return ("Can't open file");
	case DL_NOT_ELF:
		return ("File not an ELF object");
	case DL_CANT_OPEN_REF:
		return ("Can't open referenced object");
	case DL_CANT_MMAP:
		return ("Can't map ELF object");
	case DL_INVALID_HANDLE:
		return ("Invalid handle");
	case DL_NO_SYMBOL:
		return ("Unable to resolve symbol");
	case DL_INVALID_CTL:
		return ("Invalid dlctl() command");
	default:
		return ("Unknown error");
	}
}

void
_dl_show_objects()
{
	elf_object_t *object;
	static char *otyp[] = {
		"none", "rtld", "exe ", "rlib", "dlib"
	};

	object = _dl_objects;

	_dl_printf("\tStart    End      Type Ref Name\n");

	while (object) {
		_dl_printf("\t%X %X %s  %d  %s\n", object->load_addr,
				object->load_size, otyp[object->obj_type],
				object->refcount, object->load_name);
		object = object->next;
	}
}
