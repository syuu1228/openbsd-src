/*	$OpenBSD: src/sys/kern/subr_userconf.c,v 1.4 1996/07/02 06:52:00 niklas Exp $	*/

/*
 * Copyright (c) 1996 Mats O Jansson <moj@stacken.kth.se>
 * All rights reserved.
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
 *	This product includes software developed by Mats O Jansson.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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
 */

#ifdef BOOT_CONFIG
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

extern char *locnames[];
extern short locnamp[];
extern struct cfdata cfdata[];
extern cfroots[];

int userconf_base = 16;
int userconf_maxdev = -1;
int userconf_maxlocnames = -1;
int userconf_cnt = -1;
int userconf_lines = 12;
char userconf_argbuf[40];
char userconf_cmdbuf[40];

#define UC_CHANGE 'c'
#define UC_DISABLE 'd'
#define UC_ENABLE 'e'
#define UC_FIND 'f'

char *userconf_cmds[] = {
/*	"add",		"a", */
        "base",         "b",
	"change",	"c",
	"disable",	"d",
	"enable",	"e",
	"exit",		"q",
	"find",		"f",
	"help",		"h",
	"list",		"l",
	"lines",	"L",
	"quit",		"q",
	"show",		"s",
	"?",		"h",
	"",		 "",
};

userconf_init()
{
	int i = 0;
	struct cfdata *cd;
	short *p;
	int   *l;
	int   ln;		

	while(cfdata[i].cf_attach != 0) {
		
		userconf_maxdev = i;

		cd = &cfdata[i];
		ln=cd->cf_locnames;
		while (locnamp[ln] != -1) {
			if (locnamp[ln] > userconf_maxlocnames) {
				userconf_maxlocnames = locnamp[ln];
			};
			ln++;
		}
		i++;
	}
}

int
userconf_more()
{
	int quit = 0;
	char c;

	if (userconf_cnt != -1) {
		if (userconf_cnt == userconf_lines) {
			printf("--- more ---");
			c = cngetc();
			userconf_cnt = 0;
			printf("\r            \r");
		}
		userconf_cnt++;
		if (c == 'q' || c == 'Q') quit = 1;
	}
	return(quit);
}

userconf_pnum(val)
int val;
{
	if (val > -2 & val < 16) {
		printf("%d",val);
	} else {
		switch(userconf_base) {
		case 8:
			printf("0%o",val);
			break;
		case 10:
			printf("%d",val);
			break;
		case 16:
		default:
			printf("0x%x",val);
			break;
		}
	}
}

userconf_pdevnam(dev)
short dev;
{
	struct cfdata *cd;

	cd = &cfdata[dev];
	printf(cd->cf_driver->cd_name);
	switch(cd->cf_fstate) {
	case FSTATE_NOTFOUND:
	case FSTATE_DNOTFOUND:
		printf("%d",cd->cf_unit);
		break;
	case FSTATE_FOUND:
	  	printf("*FOUND*");
		break;
	case FSTATE_STAR:
	case FSTATE_DSTAR:
		printf("*");
		break;
	default:
		printf("*UNKNOWN*");
		break;
	}
}

userconf_pdev(devno)
short devno;
{
	struct cfdata *cd;
	short *p;
	int   *l;
	int   ln;		
	char c;

	if (devno >  userconf_maxdev) {
		printf("Unknown devno (max is %d)\n",userconf_maxdev);
		return;
	}

	cd = &cfdata[devno];

	printf("%3d",devno);
	switch(cd->cf_fstate) {
	case FSTATE_NOTFOUND:
        case FSTATE_FOUND:
        case FSTATE_STAR:
		printf(" E ");
		break;
	case FSTATE_DNOTFOUND:
        case FSTATE_DSTAR:
		printf(" D ");
		break;
        default:
		printf(" ? ");
		break;
	}
		
	userconf_pdevnam(devno);
	printf(" at");
	c=' ';
	p=cd->cf_parents;
	if (*p == -1) printf(" root");
	while (*p != -1) {
		printf("%c",c);
		userconf_pdevnam(*p++);
		c='|';
	};
	l=cd->cf_loc;
	ln=cd->cf_locnames;
	while (locnamp[ln] != -1) {
		printf(" %s ",locnames[locnamp[ln]]);
		ln++;
		userconf_pnum(*l++);
	}
	printf("\n");

}

int
userconf_number(c,val)
char *c;
int *val;
{
	u_int num = 0;
	int neg = 0;
	int base = 10;

	if (*c == '-') {
		neg = 1;
		c++;
	}
	if (*c == '0') {
		base = 8;
		c++;
		if (*c == 'x' || *c == 'X') {
			base = 16;
			c++;
		}
	}
	while (*c != '\n' & *c != '\t' & *c != ' ' & *c != '\000') {
		u_char cc = *c;

		if (cc >= '0' && cc <= '9')
			cc = cc - '0';
		else if (cc >= 'a' && cc <= 'f')
			cc = cc - 'a' + 10;
		else if (cc >= 'A' && cc <= 'F')
			cc = cc - 'A' + 10;
		else
			return (-1);

		if (cc > base)
			return (-1);
		num = num * base + cc;
		c++;
	}
	
	if (neg && num > INT_MAX)       /* overflow */
		return (1);
	
	*val = neg ? - num : num;

	return(0);
}

int
userconf_device(cmd,len,unit,state)
char *cmd;
int *len;
short *unit, *state;
{
	short u = 0, s = FSTATE_FOUND;
	int l = 0;
	char *c;

	c = cmd;
	while(*c >= 'a' & *c <= 'z') { l++; c++; };
	if (*c == '*') {
		s = FSTATE_STAR;
		c++;
	} else {
		while(*c >= '0' & *c <= '9') {
			s = FSTATE_NOTFOUND;
			u=u*10 + *c - '0';
			c++;
		};
	}
	while(*c == ' ' || *c == '\t' || *c == '\n') c++;

	if (*c == '\000') {
		*len = l;
		*unit = u;
		*state = s;
		return(0);
	}

	return(-1);
}

userconf_modify(item, val)
char *item;
int  *val;
{
	int ok = 0;
	int a;
	char *c;
	int i;

	while(!ok) {
		printf("%s [",item);
		userconf_pnum(*val);
		printf("] ? ");

		i = getsn(userconf_argbuf,sizeof(userconf_argbuf));

		c = userconf_argbuf;
		while (*c == ' ' || *c == '\t' || *c == '\n') c++;

		if (*c != '\000') {
			if (userconf_number(c,&a) == 0) {
				*val = a;
				ok = 1;
			} else {
				printf("Unknown argument\n");
			}
		} else {
			ok = 1;
		}
	}
}

userconf_change(devno)
int devno;
{
	struct cfdata *cd;
	char c = '\000';
	int   *l;
	int   ln;		

	if (devno <=  userconf_maxdev) {

		userconf_pdev(devno);

		while(c != 'y' && c != 'Y' && c != 'n' && c != 'N') {
			printf("change (y/n) ?");
			c = cngetc();
			printf("\n");
		}

		if (c == 'y' || c == 'Y') {
			int share = 0, i, *lk;
			
			cd = &cfdata[devno];
			l=cd->cf_loc;
			ln=cd->cf_locnames;

			/*
			 * Search for some other driver sharing this
			 * locator table. if one does, we may need to
			 * replace the locators with a malloc'd copy.
			 */
			for (i = 0; cfdata[i].cf_driver; i++)
				if (i != devno && cfdata[i].cf_loc == l)
					share = 1;
			if (share) {
				for (i = 0; locnamp[ln+i] != -1 ; i++)
					;
				lk = l = (int *)malloc(sizeof(int) * i,
				    M_TEMP, M_NOWAIT);
				bcopy(cd->cf_loc, l, sizeof(int) * i);
			}
 
			while (locnamp[ln] != -1) {
				userconf_modify(locnames[locnamp[ln]],
						l);
				ln++;
				l++;
			}
			
			if (share) {
				if (bcmp(cd->cf_loc, lk, sizeof(int) * i))
					cd->cf_loc = lk;
				else
					free(lk, M_TEMP);
			}

			printf("%3d ",devno);
			userconf_pdevnam(devno);
			printf(" changed\n");
			userconf_pdev(devno);
			
		}

	} else {
		printf("Unknown devno (max is %d)\n",userconf_maxdev);
	}

}

userconf_disable(devno)
int devno;
{
	int done = 0;
	
	if (devno <= userconf_maxdev) {

		switch(cfdata[devno].cf_fstate) { 
		case FSTATE_NOTFOUND:
			cfdata[devno].cf_fstate = FSTATE_DNOTFOUND;
			break;
		case FSTATE_STAR:
			cfdata[devno].cf_fstate = FSTATE_DSTAR;
			break;
		case FSTATE_DNOTFOUND:
		case FSTATE_DSTAR:
			done = 1;
			break;
		default:
			printf("Error unknown state\n");
			break;
		}

		printf("%3d ",devno);
		userconf_pdevnam(devno);
		if (done) printf(" already");
		printf(" disabled\n");
		
	} else {
		printf("Unknown devno (max is %d)\n",userconf_maxdev);
	}

}

userconf_enable(devno)
int devno;
{
	int done = 0;
	
	if (devno <= userconf_maxdev) {

		switch(cfdata[devno].cf_fstate) { 
		case FSTATE_DNOTFOUND:
			cfdata[devno].cf_fstate = FSTATE_NOTFOUND;
			break;
		case FSTATE_DSTAR:
			cfdata[devno].cf_fstate = FSTATE_STAR;
			break;
		case FSTATE_NOTFOUND:
		case FSTATE_STAR:
			done = 1;
			break;
		default:
			printf("Error unknown state\n");
			break;
		}

		printf("%3d ",devno);
		userconf_pdevnam(devno);
		if (done) printf(" already");
		printf(" enabled\n");
		
	} else {
		printf("Unknown devno (max is %d)\n",userconf_maxdev);
	}

}

userconf_help()
{
	int j = 0,k;

	printf("command   args          description\n");
	while(*userconf_cmds[j] != '\000') {
		printf(userconf_cmds[j]);
		k=strlen(userconf_cmds[j]);
		while (k < 10) { printf(" "); k++; }
		switch(*userconf_cmds[j+1]) {
		case 'L':
			printf("[count]       number of lines before more");
			break;
		case 'b':
			printf("8|10|16       base on large numbers");
			break;
		case 'c':
			printf("devno|dev     %s devices","change");
			break;
		case 'd':
			printf("devno|dev     %s devices","disable");
			break;
		case 'e':
			printf("devno|dev     %s devices","enable");
			break;
		case 'f':
			printf("devno|dev     %s devices","find");
			break;
		case 'h':
			printf("              %s","this message");
			break;
		case 'l':
			printf("              %s","list configuration");
			break;
		case 'q':
			printf("              %s","leave UKC");
			break;
		case 's':
			printf("[attr [val]]  %s",
			   "show attributes (or devices with an attribute)");
			break;
		default:
			printf("              %s","don't know");
			break;
		}
		printf("\n");
		j++; j++;
	}
	
}

userconf_list(count)
int count;
{
	int i = 0;

	userconf_cnt = 0;

	while(cfdata[i].cf_attach != 0) {
		if (userconf_more()) break;
		userconf_pdev(i++);
	};

	userconf_cnt = -1;
}

userconf_show()
{
	int i = 0;

	userconf_cnt = 0;

	while(i <= userconf_maxlocnames) {
		if (userconf_more()) break;
		printf("%s\n",locnames[i++]);
	}

	userconf_cnt = -1;
}

userconf_show_attr_val(attr, val)
short attr;
int   *val;
{
	struct cfdata *cd;
	int   *l;
	int   ln;		
	int i = 0, quit = 0;
	
	userconf_cnt = 0;

	while(i <= userconf_maxdev) {
		cd = &cfdata[i];
		l=cd->cf_loc;
		ln=cd->cf_locnames;
		while (locnamp[ln] != -1) {
			if (locnamp[ln] == attr) {
				if (val == NULL) {
					quit = userconf_more();
					userconf_pdev(i);
				} else {
					if (*val == *l) {
						quit = userconf_more();
						userconf_pdev(i);
					}
				}
			}
			if (quit) break;
			ln++;
			l++;
		}
		if (quit) break;
		i++;
	}

	userconf_cnt = -1;
}

userconf_show_attr(cmd)
char *cmd;
{
	char *c;
	short attr = -1, i = 0, l = 0;
	int a;
	
	c = cmd;
	while (*c != ' ' & *c != '\t' & *c != '\n' & *c != '\000') {
		c++; l++;
	}
	while (*c == ' ' || *c == '\t' || *c == '\n') {
		c++;
	}
	
	while(i <= userconf_maxlocnames) {
		if (strlen(locnames[i]) == l) {
			if (strncasecmp(cmd,locnames[i],l) == 0) {
				attr = i;
			}
		}
		i++;
	}

	if (attr == -1) {
		printf("Unknown attribute\n");
		return;
	}

	if (*c == '\000') {
		userconf_show_attr_val(attr,NULL);
	} else {
		if (userconf_number(c,&a) == 0) {
			userconf_show_attr_val(attr,&a);
		} else {
			printf("Unknown argument\n");
		}
	}
}

userconf_common_dev(dev,len,unit,state,routine)
char *dev;
int len;
short unit, state;
char routine;
{
	int i = 0;

	switch(routine) {
	case UC_CHANGE:
		break;
	default:
		userconf_cnt = 0;
		break;
	}

	while(cfdata[i].cf_attach != 0) {

		if (strlen(cfdata[i].cf_driver->cd_name) == len) {

			/*
			 * Ok, if device name is correct 
			 *  If state == FSTATE_FOUND, look for "dev"
			 *  If state == FSTATE_STAR, look for "dev*"
			 *  If state == FSTATE_NOTFOUND, look for "dev0"
			 */
			if (strncasecmp(dev,cfdata[i].cf_driver->cd_name,
					len) == 0 &&
			    (state == FSTATE_FOUND ||
			     (state == FSTATE_STAR &&
			      (cfdata[i].cf_fstate == FSTATE_STAR ||
			       cfdata[i].cf_fstate == FSTATE_DSTAR)) ||
			     (state == FSTATE_NOTFOUND &&
			      cfdata[i].cf_unit == unit &&
			      (cfdata[i].cf_fstate == FSTATE_NOTFOUND ||
			       cfdata[i].cf_fstate == FSTATE_DNOTFOUND)))) {
			  	if (userconf_more()) break;
				switch(routine) {
				case UC_CHANGE:
					userconf_change(i);
					break;
				case UC_ENABLE:
					userconf_enable(i);
					break;
				case UC_DISABLE:
					userconf_disable(i);
					break;
				case UC_FIND:
					userconf_pdev(i);
					break;
				default:
					printf("Unknown routine /%c/\n",
					       routine);
					break;
				}
			}
		}
		i++;

	};

	switch(routine) {
	case UC_CHANGE:
		break;
	default:
		userconf_cnt = -1;
		break;
	}

}

int
userconf_parse(cmd)
char *cmd;
{
	char *c,*v;
	int i = 0, j = 0, k, a;
	short unit, state;

	c = cmd;
	while (*c == ' ' || *c == '\t') c++;
	v = c;
	while (*c != ' ' && *c != '\t' && *c != '\n' && *c != '\000') {
	  c++; i++;
	}

	k = -1;
	while(*userconf_cmds[j] != '\000') {
		if (strlen(userconf_cmds[j]) == i) {
			if (strncasecmp(v,userconf_cmds[j],i) == 0) {
				k = j;
			}
		}
		j++; j++;
	}

	while (*c == ' ' || *c == '\t' || *c == '\n') {
		c++;
	}
	
	if (k == -1) {
		if (*v != '\n') {
			printf("Unknown command, try help\n");
		};
	} else {
		switch(*userconf_cmds[k+1]) {
		case 'L':
			if (*c == '\000') {
				printf("Argument expected\n");
			} else if (userconf_number(c,&a) == 0) {
				userconf_lines = a;
			} else {
				printf("Unknown argument\n");
			}
			break;
		case 'b':
			if (*c == '\000') {
				printf("8|10|16 expected\n");
			} else if (userconf_number(c,&a) == 0) {
				if (a == 8 || a == 10 || a == 16) {
					userconf_base = a;
				} else {
					printf("8|10|16 expected\n");
				}
			} else {
				printf("Unknown argument\n");
			}
			break;
		case 'c':
			if (*c == '\000') {
				printf("DevNo or Dev expected\n");
			} else if (userconf_number(c,&a) == 0) {
				userconf_change(a);
			} else if (userconf_device(c,&a,&unit,&state) == 0) {
				userconf_common_dev(c,a,unit,state,UC_CHANGE);
			} else {
				printf("Unknown argument\n");
			}
			break;
		case 'd':
			if (*c == '\000') {
				printf("DevNo or Dev expected\n");
			} else if (userconf_number(c,&a) == 0) {
				userconf_disable(a);
			} else if (userconf_device(c,&a,&unit,&state) == 0) {
				userconf_common_dev(c,a,unit,state,UC_DISABLE);
			} else {
				printf("Unknown argument\n");
			}
			break;
		case 'e':
			if (*c == '\000') {
				printf("DevNo or Dev expected\n");
			} else if (userconf_number(c,&a) == 0) {
				userconf_enable(a);
			} else if (userconf_device(c,&a,&unit,&state) == 0) {
				userconf_common_dev(c,a,unit,state,UC_ENABLE);
			} else {
				printf("Unknown argument\n");
			}
			break;
		case 'f':
			if (*c == '\000') {
				printf("DevNo or Dev expected\n");
			} else if (userconf_number(c,&a) == 0) {
				userconf_pdev(a);
			} else if (userconf_device(c,&a,&unit,&state) == 0) {
				userconf_common_dev(c,a,unit,state,UC_FIND);
			} else {
				printf("Unknown argument\n");
			}
			break;
		case 'h':
			userconf_help();
			break;
		case 'l':
			if (*c == '\000') {
				userconf_list();
			} else {
				printf("Unknown argument\n");
			}
			break;
		case 'q':
			return(-1);
			break;
		case 's':
			if (*c == '\000') {
				userconf_show();
			} else {	
				userconf_show_attr(c);
			}
			break;
		default:
			printf("Unknown command\n");
			break;
		}
	}
	return(0);
}

void
user_config()
{
	char prompt[] = "UKC> ";
	int  i;
	
	userconf_init();
	printf("User Kernel Config\n");
	
	printf(prompt);
	while (i = getsn(userconf_cmdbuf,sizeof(userconf_cmdbuf)) != 0) {
		if (userconf_parse(&userconf_cmdbuf)) break;
		printf(prompt);
	}
	printf("Continuing...\n");
}
#else BOOT_CONFIG
void
user_config()
{
	printf("User Kernel Config isn't supported in this kernel\n");
}
#endif
