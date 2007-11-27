/*	$OpenBSD: src/sys/compat/svr4/svr4_syscall.h,v 1.35 2007/11/27 18:05:59 art Exp $	*/

/*
 * System call numbers.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	OpenBSD: syscalls.master,v 1.33 2003/01/30 03:29:49 millert Exp 
 */

/* syscall: "syscall" ret: "int" args: */
#define	SVR4_SYS_syscall	0

/* syscall: "exit" ret: "int" args: "int" */
#define	SVR4_SYS_exit	1

/* syscall: "fork" ret: "int" args: */
#define	SVR4_SYS_fork	2

/* syscall: "read" ret: "int" args: "int" "char *" "u_int" */
#define	SVR4_SYS_read	3

/* syscall: "write" ret: "int" args: "int" "char *" "u_int" */
#define	SVR4_SYS_write	4

/* syscall: "open" ret: "int" args: "char *" "int" "int" */
#define	SVR4_SYS_open	5

/* syscall: "close" ret: "int" args: "int" */
#define	SVR4_SYS_close	6

/* syscall: "wait" ret: "int" args: "int *" */
#define	SVR4_SYS_wait	7

/* syscall: "creat" ret: "int" args: "char *" "int" */
#define	SVR4_SYS_creat	8

/* syscall: "link" ret: "int" args: "char *" "char *" */
#define	SVR4_SYS_link	9

/* syscall: "unlink" ret: "int" args: "char *" */
#define	SVR4_SYS_unlink	10

/* syscall: "execv" ret: "int" args: "char *" "char **" */
#define	SVR4_SYS_execv	11

/* syscall: "chdir" ret: "int" args: "char *" */
#define	SVR4_SYS_chdir	12

/* syscall: "time" ret: "int" args: "svr4_time_t *" */
#define	SVR4_SYS_time	13

/* syscall: "mknod" ret: "int" args: "char *" "int" "int" */
#define	SVR4_SYS_mknod	14

/* syscall: "chmod" ret: "int" args: "char *" "int" */
#define	SVR4_SYS_chmod	15

/* syscall: "chown" ret: "int" args: "char *" "int" "int" */
#define	SVR4_SYS_chown	16

/* syscall: "break" ret: "int" args: "caddr_t" */
#define	SVR4_SYS_break	17

/* syscall: "stat" ret: "int" args: "char *" "struct svr4_stat *" */
#define	SVR4_SYS_stat	18

/* syscall: "lseek" ret: "long" args: "int" "long" "int" */
#define	SVR4_SYS_lseek	19

/* syscall: "getpid" ret: "pid_t" args: */
#define	SVR4_SYS_getpid	20

/* syscall: "setuid" ret: "int" args: "uid_t" */
#define	SVR4_SYS_setuid	23

/* syscall: "getuid" ret: "uid_t" args: */
#define	SVR4_SYS_getuid	24

/* syscall: "alarm" ret: "int" args: "unsigned" */
#define	SVR4_SYS_alarm	27

/* syscall: "fstat" ret: "int" args: "int" "struct svr4_stat *" */
#define	SVR4_SYS_fstat	28

/* syscall: "pause" ret: "int" args: */
#define	SVR4_SYS_pause	29

/* syscall: "utime" ret: "int" args: "char *" "struct svr4_utimbuf *" */
#define	SVR4_SYS_utime	30

/* syscall: "access" ret: "int" args: "char *" "int" */
#define	SVR4_SYS_access	33

/* syscall: "nice" ret: "int" args: "int" */
#define	SVR4_SYS_nice	34

/* syscall: "sync" ret: "int" args: */
#define	SVR4_SYS_sync	36

/* syscall: "kill" ret: "int" args: "int" "int" */
#define	SVR4_SYS_kill	37

/* syscall: "pgrpsys" ret: "int" args: "int" "int" "int" */
#define	SVR4_SYS_pgrpsys	39

/* syscall: "dup" ret: "int" args: "u_int" */
#define	SVR4_SYS_dup	41

/* syscall: "opipe" ret: "int" args: */
#define	SVR4_SYS_opipe	42

/* syscall: "times" ret: "int" args: "struct tms *" */
#define	SVR4_SYS_times	43

/* syscall: "setgid" ret: "int" args: "gid_t" */
#define	SVR4_SYS_setgid	46

/* syscall: "getgid" ret: "gid_t" args: */
#define	SVR4_SYS_getgid	47

/* syscall: "signal" ret: "int" args: "int" "svr4_sig_t" */
#define	SVR4_SYS_signal	48

/* syscall: "msgsys" ret: "int" args: "int" "int" "int" "int" "int" */
#define	SVR4_SYS_msgsys	49

/* syscall: "sysarch" ret: "int" args: "int" "void *" */
#define	SVR4_SYS_sysarch	50

/* syscall: "shmsys" ret: "int" args: "int" "int" "int" "int" */
#define	SVR4_SYS_shmsys	52

/* syscall: "semsys" ret: "int" args: "int" "int" "int" "int" "int" */
#define	SVR4_SYS_semsys	53

/* syscall: "ioctl" ret: "int" args: "int" "u_long" "caddr_t" */
#define	SVR4_SYS_ioctl	54

/* syscall: "utssys" ret: "int" args: "void *" "void *" "int" "void *" */
#define	SVR4_SYS_utssys	57

/* syscall: "fsync" ret: "int" args: "int" */
#define	SVR4_SYS_fsync	58

/* syscall: "execve" ret: "int" args: "char *" "char **" "char **" */
#define	SVR4_SYS_execve	59

/* syscall: "umask" ret: "int" args: "int" */
#define	SVR4_SYS_umask	60

/* syscall: "chroot" ret: "int" args: "char *" */
#define	SVR4_SYS_chroot	61

/* syscall: "fcntl" ret: "int" args: "int" "int" "char *" */
#define	SVR4_SYS_fcntl	62

/* syscall: "ulimit" ret: "long" args: "int" "long" */
#define	SVR4_SYS_ulimit	63

				/* 70 is obsolete advfs */
				/* 71 is obsolete unadvfs */
				/* 72 is obsolete rmount */
				/* 73 is obsolete rumount */
				/* 74 is obsolete rfstart */
				/* 75 is obsolete sigret */
/* syscall: "rdebug" ret: "int" args: */
#define	SVR4_SYS_rdebug	76

				/* 77 is obsolete rfstop */
/* syscall: "rmdir" ret: "int" args: "char *" */
#define	SVR4_SYS_rmdir	79

/* syscall: "mkdir" ret: "int" args: "char *" "int" */
#define	SVR4_SYS_mkdir	80

/* syscall: "getdents" ret: "int" args: "int" "char *" "int" */
#define	SVR4_SYS_getdents	81

				/* 82 is obsolete libattach */
				/* 83 is obsolete libdetach */
/* syscall: "getmsg" ret: "int" args: "int" "struct svr4_strbuf *" "struct svr4_strbuf *" "int *" */
#define	SVR4_SYS_getmsg	85

/* syscall: "putmsg" ret: "int" args: "int" "struct svr4_strbuf *" "struct svr4_strbuf *" "int" */
#define	SVR4_SYS_putmsg	86

/* syscall: "poll" ret: "int" args: "struct pollfd *" "u_long" "int" */
#define	SVR4_SYS_poll	87

/* syscall: "lstat" ret: "int" args: "char *" "struct svr4_stat *" */
#define	SVR4_SYS_lstat	88

/* syscall: "symlink" ret: "int" args: "char *" "char *" */
#define	SVR4_SYS_symlink	89

/* syscall: "readlink" ret: "int" args: "char *" "char *" "int" */
#define	SVR4_SYS_readlink	90

/* syscall: "getgroups" ret: "int" args: "u_int" "gid_t *" */
#define	SVR4_SYS_getgroups	91

/* syscall: "setgroups" ret: "int" args: "u_int" "gid_t *" */
#define	SVR4_SYS_setgroups	92

/* syscall: "fchmod" ret: "int" args: "int" "int" */
#define	SVR4_SYS_fchmod	93

/* syscall: "fchown" ret: "int" args: "int" "int" "int" */
#define	SVR4_SYS_fchown	94

/* syscall: "sigprocmask" ret: "int" args: "int" "svr4_sigset_t *" "svr4_sigset_t *" */
#define	SVR4_SYS_sigprocmask	95

/* syscall: "sigsuspend" ret: "int" args: "svr4_sigset_t *" */
#define	SVR4_SYS_sigsuspend	96

/* syscall: "sigaltstack" ret: "int" args: "struct svr4_sigaltstack *" "struct svr4_sigaltstack *" */
#define	SVR4_SYS_sigaltstack	97

/* syscall: "sigaction" ret: "int" args: "int" "struct svr4_sigaction *" "struct svr4_sigaction *" */
#define	SVR4_SYS_sigaction	98

/* syscall: "sigpending" ret: "int" args: "int" "svr4_sigset_t *" */
#define	SVR4_SYS_sigpending	99

/* syscall: "context" ret: "int" args: "int" "struct svr4_ucontext *" */
#define	SVR4_SYS_context	100

/* syscall: "statvfs" ret: "int" args: "char *" "struct svr4_statvfs *" */
#define	SVR4_SYS_statvfs	103

/* syscall: "fstatvfs" ret: "int" args: "int" "struct svr4_statvfs *" */
#define	SVR4_SYS_fstatvfs	104

/* syscall: "waitsys" ret: "int" args: "int" "int" "union svr4_siginfo *" "int" */
#define	SVR4_SYS_waitsys	107

/* syscall: "hrtsys" ret: "int" args: "int" "int" "int" "void *" "void *" */
#define	SVR4_SYS_hrtsys	109

/* syscall: "pathconf" ret: "int" args: "char *" "int" */
#define	SVR4_SYS_pathconf	113

/* syscall: "mincore" ret: "int" args: "caddr_t" "int" "char *" */
#define	SVR4_SYS_mincore	114

/* syscall: "mmap" ret: "int" args: "svr4_caddr_t" "svr4_size_t" "int" "int" "int" "svr4_off_t" */
#define	SVR4_SYS_mmap	115

/* syscall: "mprotect" ret: "int" args: "caddr_t" "int" "int" */
#define	SVR4_SYS_mprotect	116

/* syscall: "munmap" ret: "int" args: "caddr_t" "int" */
#define	SVR4_SYS_munmap	117

/* syscall: "fpathconf" ret: "int" args: "int" "int" */
#define	SVR4_SYS_fpathconf	118

/* syscall: "vfork" ret: "int" args: */
#define	SVR4_SYS_vfork	119

/* syscall: "fchdir" ret: "int" args: "int" */
#define	SVR4_SYS_fchdir	120

/* syscall: "readv" ret: "int" args: "int" "struct iovec *" "u_int" */
#define	SVR4_SYS_readv	121

/* syscall: "writev" ret: "int" args: "int" "struct iovec *" "u_int" */
#define	SVR4_SYS_writev	122

/* syscall: "xstat" ret: "int" args: "int" "char *" "struct svr4_xstat *" */
#define	SVR4_SYS_xstat	123

/* syscall: "lxstat" ret: "int" args: "int" "char *" "struct svr4_xstat *" */
#define	SVR4_SYS_lxstat	124

/* syscall: "fxstat" ret: "int" args: "int" "int" "struct svr4_xstat *" */
#define	SVR4_SYS_fxstat	125

/* syscall: "xmknod" ret: "int" args: "int" "char *" "svr4_mode_t" "svr4_dev_t" */
#define	SVR4_SYS_xmknod	126

/* syscall: "setrlimit" ret: "int" args: "int" "struct ogetrlimit *" */
#define	SVR4_SYS_setrlimit	128

/* syscall: "getrlimit" ret: "int" args: "int" "struct ogetrlimit *" */
#define	SVR4_SYS_getrlimit	129

/* syscall: "lchown" ret: "int" args: "char *" "int" "int" */
#define	SVR4_SYS_lchown	130

/* syscall: "memcntl" ret: "int" args: "svr4_caddr_t" "svr4_size_t" "int" "svr4_caddr_t" "int" "int" */
#define	SVR4_SYS_memcntl	131

/* syscall: "rename" ret: "int" args: "char *" "char *" */
#define	SVR4_SYS_rename	134

/* syscall: "uname" ret: "int" args: "struct svr4_utsname *" "int" */
#define	SVR4_SYS_uname	135

/* syscall: "setegid" ret: "int" args: "gid_t" */
#define	SVR4_SYS_setegid	136

/* syscall: "sysconfig" ret: "int" args: "int" */
#define	SVR4_SYS_sysconfig	137

/* syscall: "adjtime" ret: "int" args: "struct timeval *" "struct timeval *" */
#define	SVR4_SYS_adjtime	138

/* syscall: "systeminfo" ret: "long" args: "int" "char *" "long" */
#define	SVR4_SYS_systeminfo	139

/* syscall: "seteuid" ret: "int" args: "uid_t" */
#define	SVR4_SYS_seteuid	141

/* syscall: "fchroot" ret: "int" args: "int" */
#define	SVR4_SYS_fchroot	153

/* syscall: "utimes" ret: "int" args: "char *" "struct timeval *" */
#define	SVR4_SYS_utimes	154

/* syscall: "vhangup" ret: "int" args: */
#define	SVR4_SYS_vhangup	155

/* syscall: "gettimeofday" ret: "int" args: "struct timeval *" */
#define	SVR4_SYS_gettimeofday	156

/* syscall: "getitimer" ret: "int" args: "u_int" "struct itimerval *" */
#define	SVR4_SYS_getitimer	157

/* syscall: "setitimer" ret: "int" args: "u_int" "struct itimerval *" "struct itimerval *" */
#define	SVR4_SYS_setitimer	158

/* syscall: "pread" ret: "ssize_t" args: "int" "void *" "size_t" "svr4_off_t" */
#define	SVR4_SYS_pread	173

/* syscall: "pwrite" ret: "ssize_t" args: "int" "const void *" "size_t" "svr4_off_t" */
#define	SVR4_SYS_pwrite	174

/* syscall: "llseek" ret: "svr4_off64_t" args: "int" "long" "long" "int" */
#define	SVR4_SYS_llseek	175

/* syscall: "acl" ret: "int" args: "char *" "int" "int" "struct svr4_aclent *" */
#define	SVR4_SYS_acl	185

/* syscall: "auditsys" ret: "int" args: "int" "int" "int" "int" "int" "int" */
#define	SVR4_SYS_auditsys	186

/* syscall: "clock_gettime" ret: "int" args: "clockid_t" "struct timespec *" */
#define	SVR4_SYS_clock_gettime	191

/* syscall: "clock_settime" ret: "int" args: "clockid_t" "const struct timespec *" */
#define	SVR4_SYS_clock_settime	192

/* syscall: "clock_getres" ret: "int" args: "clockid_t" "struct timespec *" */
#define	SVR4_SYS_clock_getres	193

/* syscall: "nanosleep" ret: "int" args: "const struct timespec *" "struct timespec *" */
#define	SVR4_SYS_nanosleep	199

/* syscall: "facl" ret: "int" args: "int" "int" "int" "struct svr4_aclent *" */
#define	SVR4_SYS_facl	200

/* syscall: "setreuid" ret: "int" args: "uid_t" "uid_t" */
#define	SVR4_SYS_setreuid	202

/* syscall: "setregid" ret: "int" args: "gid_t" "gid_t" */
#define	SVR4_SYS_setregid	203

/* syscall: "getdents64" ret: "int" args: "int" "struct svr4_dirent64 *" "int" */
#define	SVR4_SYS_getdents64	213

/* syscall: "mmap64" ret: "int" args: "svr4_caddr_t" "svr4_size_t" "int" "int" "int" "svr4_off64_t" */
#define	SVR4_SYS_mmap64	214

/* syscall: "stat64" ret: "int" args: "const char *" "struct svr4_stat64 *" */
#define	SVR4_SYS_stat64	215

/* syscall: "lstat64" ret: "int" args: "const char *" "struct svr4_stat64 *" */
#define	SVR4_SYS_lstat64	216

/* syscall: "fstat64" ret: "int" args: "int" "struct svr4_stat64 *" */
#define	SVR4_SYS_fstat64	217

/* syscall: "fstatvfs64" ret: "int" args: "int" "struct svr4_statvfs64 *" */
#define	SVR4_SYS_fstatvfs64	219

/* syscall: "pread64" ret: "ssize_t" args: "int" "void *" "size_t" "svr4_off64_t" */
#define	SVR4_SYS_pread64	222

/* syscall: "pwrite64" ret: "ssize_t" args: "int" "const void *" "size_t" "svr4_off64_t" */
#define	SVR4_SYS_pwrite64	223

/* syscall: "creat64" ret: "int" args: "char *" "int" */
#define	SVR4_SYS_creat64	224

/* syscall: "open64" ret: "int" args: "char *" "int" "int" */
#define	SVR4_SYS_open64	225

/* syscall: "socket" ret: "int" args: "int" "int" "int" */
#define	SVR4_SYS_socket	230

/* syscall: "socketpair" ret: "int" args: "int" "int" "int" "int *" */
#define	SVR4_SYS_socketpair	231

/* syscall: "bind" ret: "int" args: "int" "const struct sockaddr *" "int" */
#define	SVR4_SYS_bind	232

/* syscall: "listen" ret: "int" args: "int" "int" */
#define	SVR4_SYS_listen	233

/* syscall: "accept" ret: "int" args: "int" "struct sockaddr *" "int *" */
#define	SVR4_SYS_accept	234

/* syscall: "connect" ret: "int" args: "int" "const struct sockaddr *" "int" */
#define	SVR4_SYS_connect	235

/* syscall: "shutdown" ret: "int" args: "int" "int" */
#define	SVR4_SYS_shutdown	236

/* syscall: "recv" ret: "int" args: "int" "caddr_t" "int" "int" */
#define	SVR4_SYS_recv	237

/* syscall: "recvfrom" ret: "ssize_t" args: "int" "void *" "size_t" "int" "struct sockaddr *" "int *" */
#define	SVR4_SYS_recvfrom	238

/* syscall: "recvmsg" ret: "ssize_t" args: "int" "struct msghdr *" "int" */
#define	SVR4_SYS_recvmsg	239

/* syscall: "send" ret: "int" args: "int" "caddr_t" "int" "int" */
#define	SVR4_SYS_send	240

/* syscall: "sendmsg" ret: "ssize_t" args: "int" "const struct msghdr *" "int" */
#define	SVR4_SYS_sendmsg	241

/* syscall: "sendto" ret: "ssize_t" args: "int" "const void *" "size_t" "int" "const struct sockaddr *" "int" */
#define	SVR4_SYS_sendto	242

/* syscall: "getpeername" ret: "int" args: "int" "struct sockaddr *" "int *" */
#define	SVR4_SYS_getpeername	243

/* syscall: "getsockname" ret: "int" args: "int" "struct sockaddr *" "int *" */
#define	SVR4_SYS_getsockname	244

/* syscall: "getsockopt" ret: "int" args: "int" "int" "int" "void *" "int *" */
#define	SVR4_SYS_getsockopt	245

/* syscall: "setsockopt" ret: "int" args: "int" "int" "int" "const void *" "int" */
#define	SVR4_SYS_setsockopt	246

#define	SVR4_SYS_MAXSYSCALL	250
