/*	$OpenBSD: src/usr.bin/vim/proto/Attic/msdos.pro,v 1.1.1.1 1996/09/07 21:40:29 downsj Exp $	*/
/* msdos.c */
long mch_avail_mem __PARMS((int special));
void mch_delay __PARMS((long msec, int ignoreinput));
int vim_remove __PARMS((char_u *name));
void mch_write __PARMS((char_u *s, int len));
int mch_inchar __PARMS((char_u *buf, int maxlen, long time));
int mch_char_avail __PARMS((void));
void mch_suspend __PARMS((void));
void mch_windinit __PARMS((void));
int mch_check_win __PARMS((int argc, char **argv));
int mch_check_input __PARMS((void));
void mch_settitle __PARMS((char_u *title, char_u *icon));
void mch_restore_title __PARMS((int which));
int mch_can_restore_title __PARMS((void));
int mch_can_restore_icon __PARMS((void));
int mch_get_user_name __PARMS((char_u *s, int len));
void mch_get_host_name __PARMS((char_u *s, int len));
long mch_get_pid __PARMS((void));
int mch_dirname __PARMS((char_u *buf, int len));
int FullName __PARMS((char_u *fname, char_u *buf, int len, int force));
int isFullName __PARMS((char_u *fname));
long getperm __PARMS((char_u *name));
int setperm __PARMS((char_u *name, long perm));
int mch_isdir __PARMS((char_u *name));
void mch_windexit __PARMS((int r));
void interrupt catch_cbrk __PARMS((void));
void interrupt catch_cint __PARMS((unsigned bp, unsigned di, unsigned si, unsigned ds, unsigned es, unsigned dx, unsigned cx, unsigned bx, unsigned ax));
void mch_settmode __PARMS((int raw));
void mch_setmouse __PARMS((int on));
int mch_screenmode __PARMS((char_u *arg));
int mch_get_winsize __PARMS((void));
void set_window __PARMS((void));
void mch_set_winsize __PARMS((void));
int call_shell __PARMS((char_u *cmd, int options));
void mch_breakcheck __PARMS((void));
int mch_has_wildcard __PARMS((char_u *s));
int ExpandWildCards __PARMS((int num_pat, char_u **pat, int *num_file, char_u ***file, int files_only, int list_notfound));
int vim_chdir __PARMS((char *path));
char_u *vim_getenv __PARMS((char_u *var));
