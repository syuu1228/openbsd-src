/*
 * Public Domain 2003 Dale Rahn
 *
 * $OpenBSD: src/regress/libexec/ld.so/dlopen/libab/ab.h,v 1.1.1.1 2005/09/13 20:51:39 drahn Exp $
 */

class BB {
public:
        BB(char *);
        ~BB();
private:
	char *_name;
};

