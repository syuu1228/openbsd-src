/*	$OpenBSD: src/distrib/crunch/crunchide/Attic/elf_hide.c,v 1.1 1997/01/09 04:16:52 rahnds Exp $ */

/*
 * Copyright (c) 1997 Dale Rahn. All rights reserved.
 *
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
 *	This product includes software developed by Dale Rahn.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <sys/exec_elf.h>

void load_strtab(Elf32_Ehdr *pehdr, char *pexe);
void dump_strtab();
char *get_str(int indx);

void load_symtab(Elf32_Ehdr *pehdr, char *pexe);
void dump_symtab();

void load_shstr_tab(Elf32_Ehdr *pehdr, char *pexe);
char *get_shstr(int indx);
void fprint_shstr(FILE * channel, int indx);

void hide_sym();

void reorder_syms(Elf32_Ehdr *ehdr, Elf32_Shdr *symsect,
	Elf32_Sym *symtab, int symtabsize, int symtabsecnum);

typedef int Symmap;

void renum_reloc_syms(Elf32_Ehdr *ehdr, Symmap *symmap, int symtabsecnum);

char * pexe;
elf_hide(int pfile)
{
	int i;

	Elf32_Ehdr *pehdr;
	Elf32_Shdr *pshdr;
	Elf32_Phdr *pphdr;
	struct stat sb;

	fstat(pfile, &sb);
	pexe = mmap(0, sb.st_size, PROT_READ|PROT_WRITE, 
		MAP_FILE | MAP_SHARED, pfile, 0);

	pehdr = (Elf32_Ehdr *)pexe;

#ifdef DEBUG
	printf("elf header\n");
	printf("e_type %x\n", pehdr->e_type);
	printf("e_machine %x\n", pehdr->e_machine);
	printf("e_version %x\n", pehdr->e_version);
	printf("e_entry %x\n", pehdr->e_entry);
	printf("e_phoff %x\n", pehdr->e_phoff);
	printf("e_shoff %x\n", pehdr->e_shoff);
	printf("e_flags %x\n", pehdr->e_flags);
	printf("e_ehsize %x\n", pehdr->e_ehsize);
	printf("e_phentsize %x\n", pehdr->e_phentsize);
	printf("e_phnum %x\n", pehdr->e_phnum);
	printf("e_shentsize %x\n", pehdr->e_shentsize);
	printf("e_shnum %x\n", pehdr->e_shnum);
	printf("e_shstrndx %x\n", pehdr->e_shstrndx);
#endif
	
	load_shstr_tab(pehdr, pexe);
#ifdef DEBUG
	for (i = 0; i < pehdr->e_shnum; i++) {
		pshdr = (Elf32_Phdr *) (pexe + pehdr->e_shoff +
			(i * pehdr->e_shentsize));

		printf("section header %d\n",i);
		printf("sh_name %x ", pshdr->sh_name);
		fprint_shstr(stdout, pshdr->sh_name);
		printf("\n");
		printf("sh_type %x\n", pshdr->sh_type);
		printf("sh_flags %x\n", pshdr->sh_flags);
		printf("sh_addr %x\n", pshdr->sh_addr);
		printf("sh_offset %x\n", pshdr->sh_offset);
		printf("sh_size %x\n", pshdr->sh_size);
		printf("sh_link %x\n", pshdr->sh_link);
		printf("sh_info %x\n", pshdr->sh_info);
		printf("sh_addralign %x\n", pshdr->sh_addralign);
		printf("sh_entsize %x\n", pshdr->sh_entsize);
	}
#endif /* DEBUG */

#ifdef DEBUG
	for (i = 0; i < pehdr->e_phnum; i++) {
		pshdr = (Elf32_Phdr *) (pexe + pehdr->e_phoff +
			(i * pehdr->e_phentsize);

		printf("program header %d\n", i);
		printf("p_type %x\n", pphdr->p_type);
		printf("p_offset %x\n", pphdr->p_offset);
		printf("p_vaddr %x\n", pphdr->p_vaddr);
		printf("p_paddr %x\n", pphdr->p_paddr);
		printf("p_filesz %x\n", pphdr->p_filesz);
		printf("p_memsz %x\n", pphdr->p_memsz);
		printf("p_flags %x\n", pphdr->p_flags);
		printf("p_align %x\n", pphdr->p_align);
	}
#endif /* DEBUG */
#if 0
	for (i = 0; i < pehdr->e_shnum; i++) {
		pshdr = (Elf32_Phdr *) (pexe + pehdr->e_shoff +
			(i * pehdr->e_shentsize);
		if (0 == strcmp(".strtab", get_shstr(pshdr->sh_name))) {
			break;
		}
	}
	fprint_shstr(stdout, pshdr->sh_name);
	printf("\n");
#endif

	load_strtab(pehdr, pexe);
	load_symtab(pehdr, pexe);

	munmap(pexe, sb.st_size);
	close (pfile);
}
char *shstrtab;

void
load_shstr_tab(Elf32_Ehdr *pehdr, char *pexe)
{
	Elf32_Shdr *pshdr;
	shstrtab = NULL;
	if (pehdr->e_shstrndx == 0) {
		return;
	}
	pshdr = (Elf32_Shdr *)(pexe + pehdr->e_shoff +
		(pehdr->e_shstrndx * pehdr->e_shentsize));

	shstrtab = (char *)(pexe + pshdr->sh_offset);
}

void
fprint_shstr(FILE * channel, int indx)
{
	if (shstrtab != NULL) {
		fprintf(channel, "\"%s\"", &(shstrtab[indx]));
	}
	return;
}

char *
get_shstr(int indx)
{
	return &(shstrtab[indx]);
}

void
load_symtab(Elf32_Ehdr *pehdr, char *pexe)
{
	Elf32_Sym *symtab;
	Elf32_Shdr *symsect;
	int        symtabsize;
	Elf32_Shdr *pshdr;
	Elf32_Shdr *psymshdr;
	char *shname;
	int i;
	symtab = NULL;
	for (i = 0; i < pehdr->e_shnum; i++) {
		pshdr = (Elf32_Shdr *)(pexe + pehdr->e_shoff +
		(i * pehdr->e_shentsize) );
		if (SHT_REL != pshdr->sh_type && SHT_RELA != pshdr->sh_type){
			continue;
		}
		psymshdr = (Elf32_Shdr *)(pexe + pehdr->e_shoff +
			(pshdr->sh_link * pehdr->e_shentsize) );
#ifdef DEBUG
		fprint_shstr(stdout, pshdr->sh_name);
		printf("\n");
#endif
		symtab = (Elf32_Sym *)(pexe + psymshdr->sh_offset);
		symsect = psymshdr;
		symtabsize = psymshdr->sh_size;

#ifdef DEBUG
		dump_symtab(symsect, symtab, symtabsize);
#endif
		hide_sym(pehdr, symsect, symtab, symtabsize, pshdr->sh_link);
	}

}

void
dump_symtab (Elf32_Shdr *symsect, Elf32_Sym *symtab, int symtabsize)
{
	int i;
	Elf32_Sym *psymtab;

	for (i = 0 ; i < (symtabsize/sizeof(Elf32_Sym)); i++) {
		psymtab = &(symtab[i]);
		if ((psymtab->st_info & 0xf0) == 0x10 &&
			(psymtab->st_shndx != SHN_UNDEF)) {
		printf("symbol %d:\n", i);
		printf("st_name %x \"%s\"\n", psymtab->st_name,
			get_str(psymtab->st_name));
		printf("st_value %x\n", psymtab->st_value);
		printf("st_size %x\n", psymtab->st_size);
		printf("st_info %x\n", psymtab->st_info);
		printf("st_other %x\n", psymtab->st_other);
		printf("st_shndx %x\n", psymtab->st_shndx);
		}
	}
}

char * strtab;
int strtabsize;
void
load_strtab(Elf32_Ehdr *pehdr, char *pexe)
{
	Elf32_Shdr *pshdr;
	char *shname;
	int i;
	strtab = NULL;
	for (i = 0; i < pehdr->e_shnum; i++) {
		pshdr = (Elf32_Shdr *) (pexe + pehdr->e_shoff +
			(i * pehdr->e_shentsize));

		shname = get_shstr(pshdr->sh_name);
		if (0 == strcmp(".strtab", shname)) {
			break;
		}
	}
#ifdef DEBUG
	fprint_shstr(stdout, pshdr->sh_name);
	printf("\n");
#endif

	strtab = (char *) (pexe + pshdr->sh_offset);

	strtabsize = pshdr->sh_size;

#ifdef DEBUG
	dump_strtab();
#endif
}

void
dump_strtab()
{
	int index;
	char *pstr;
	char *pnstr;
	int i = 0;
	index = 0;
	pstr = strtab;
	while (index < strtabsize) {
		printf("string %x: \"%s\"\n",i, pstr);
		pnstr = (char *) ((int)strchr(pstr, '\0') + 1);
		index = pnstr - strtab;
		pstr = pnstr;
		i++;
	}
	
}
fprint_str(FILE * channel, int indx)
{
	if (strtab != NULL) {
		fprintf(channel, "\"%s\"", &(strtab[indx]));
	}
	return;
}
char *
get_str(int indx)
{
	return &(strtab[indx]);
}

int in_keep_list(char *symbol);

void
hide_sym(Elf32_Ehdr *ehdr, Elf32_Shdr *symsect,
	Elf32_Sym *symtab, int symtabsize, int symtabsecnum)
{
	int i;
	unsigned char info;
	Elf32_Sym *psymtab;

	for (i = 0 ; i < (symtabsize/sizeof(Elf32_Sym)); i++) {
		psymtab = &(symtab[i]);
		if ((psymtab->st_info & 0xf0) == 0x10 &&
			(psymtab->st_shndx != SHN_UNDEF)) {
			if (in_keep_list(get_str(psymtab->st_name))) {
				continue;
			}
#ifdef DEBUG
			printf("symbol %d:\n", i);
			printf("st_name %x \"%s\"\n", psymtab->st_name,
				get_str(psymtab->st_name));
			printf("st_info %x\n", psymtab->st_info);
#endif
			info = psymtab->st_info;
			info = info & 0xf;
			psymtab->st_info = info;
#ifdef DEBUG
			printf("st_info %x\n", psymtab->st_info);
#endif
		}
	}
#ifdef DEBUG
#endif
	reorder_syms(ehdr, symsect, symtab, symtabsize, symtabsecnum);
}
void
reorder_syms(Elf32_Ehdr *ehdr, Elf32_Shdr *symsect,
	Elf32_Sym *symtab, int symtabsize, int symtabsecnum)
{
	int i;
	int nsyms;
	int cursym;
	Elf32_Sym *tmpsymtab;
	Symmap *symmap;


	nsyms = symtabsize / sizeof(Elf32_Sym);

	tmpsymtab = (Elf32_Sym *)malloc(symtabsize);
	symmap = (Symmap *)malloc( sizeof(Symmap) * (nsyms));

	assert (NULL != tmpsymtab);

	bcopy (symtab, tmpsymtab, symtabsize);

	cursym = 1;
	for (i = 1; i < nsyms; i++) {
		if ((tmpsymtab[i].st_info & 0xf0) == 0x00) {
#ifdef DEBUG
			printf("copying  l o%d n%d <%s>\n",i, cursym,
				get_str(tmpsymtab[i].st_name));
#endif
			bcopy (&(tmpsymtab[i]),
				&(symtab[cursym]), sizeof(Elf32_Sym));
			symmap[i] = cursym;
			cursym++;
		}
	}
	symsect->sh_info = cursym;
	for (i = 1; i < nsyms; i++) {
		if ((tmpsymtab[i].st_info & 0xf0) != 0x00) {
#ifdef DEBUG
			printf("copying nl o%d n%d <%s>\n",i, cursym,
				get_str(tmpsymtab[i].st_name));
#endif
			bcopy (&(tmpsymtab[i]),
				&(symtab[cursym]), sizeof(Elf32_Sym));
			symmap[i] = cursym;
			cursym++;
		}
	}
	if (cursym != nsyms) {
		printf("miscounted symbols somewhere c %d n %d \n",
			cursym, nsyms);
		exit (5);
	}
	renum_reloc_syms(ehdr, symmap, symtabsecnum);
	free (tmpsymtab);
	free (symmap);
}
void
renum_reloc_syms(Elf32_Ehdr *ehdr, Symmap *symmap, int symtabsecnum)
{
	Elf32_Shdr *pshdr;
	int i, j;
	int num_reloc;
	Elf32_Rel  *prel;
	Elf32_Rela *prela;
	int symnum;

	for (i = 0; i < ehdr->e_shnum; i++) {
		pshdr = (Elf32_Shdr *)(pexe + ehdr->e_shoff +
		(i * ehdr->e_shentsize) );
		if ((pshdr->sh_type == SHT_RELA) &&
			pshdr->sh_link == symtabsecnum)
		{
			
#ifdef DEBUG
			printf ("section %d has rela relocations in symtab\n", i);
#endif
			prela = (Elf32_Rela *)(pexe + pshdr->sh_offset);
			num_reloc = pshdr->sh_size /sizeof (Elf32_Rela);
			for (j = 0; j < num_reloc; j++) {
				symnum = ELF32_R_SYM(prela[j].r_info);
#ifdef DEBUG
				printf("sym num o %d n %d\n", symnum,
					symmap[symnum]);
#endif
				prela[j].r_info = ELF32_R_INFO (symmap[symnum],
					ELF32_R_TYPE(prela[j].r_info));
			}

		}
		if ((pshdr->sh_type == SHT_REL) &&
			pshdr->sh_link == symtabsecnum)
		{
#ifdef DEBUG
			printf ("section %d has rel relocations in symtab\n", i);
#endif
			prel = (Elf32_Rel *)(pexe + pshdr->sh_offset);
			num_reloc = pshdr->sh_size /sizeof (Elf32_Rel);
			for (j = 0; j < num_reloc; j++) {
				symnum = ELF32_R_SYM(prel[j].r_info);
#ifdef DEBUG
				printf("sym num o %d n %d\n", symnum,
					symmap[symnum]);
#endif
				prel[j].r_info = ELF32_R_INFO (symmap[symnum],
					ELF32_R_TYPE(prel[j].r_info));
			}
		}
	}
	
}
