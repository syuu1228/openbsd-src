/*	$OpenBSD: src/sys/compat/netbsd/Attic/netbsd_syscall.h,v 1.5 1999/09/15 20:42:27 kstailey Exp $	*/

/*
 * System call numbers.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from;	OpenBSD: syscalls.master,v 1.5 1999/09/15 20:41:16 kstailey Exp 
 */

/* syscall: "syscall" ret: "int" args: "int" "..." */
#define	NETBSD_SYS_syscall	0

/* syscall: "exit" ret: "void" args: "int" */
#define	NETBSD_SYS_exit	1

/* syscall: "fork" ret: "int" args: */
#define	NETBSD_SYS_fork	2

/* syscall: "read" ret: "ssize_t" args: "int" "void *" "size_t" */
#define	NETBSD_SYS_read	3

/* syscall: "write" ret: "ssize_t" args: "int" "const void *" "size_t" */
#define	NETBSD_SYS_write	4

/* syscall: "open" ret: "int" args: "const char *" "int" "..." */
#define	NETBSD_SYS_open	5

/* syscall: "close" ret: "int" args: "int" */
#define	NETBSD_SYS_close	6

/* syscall: "wait4" ret: "int" args: "int" "int *" "int" "struct rusage *" */
#define	NETBSD_SYS_wait4	7

/* syscall: "ocreat" ret: "int" args: "char *" "int" */
#define	NETBSD_SYS_ocreat	8

/* syscall: "link" ret: "int" args: "const char *" "const char *" */
#define	NETBSD_SYS_link	9

/* syscall: "unlink" ret: "int" args: "const char *" */
#define	NETBSD_SYS_unlink	10

				/* 11 is obsolete execv */
/* syscall: "chdir" ret: "int" args: "const char *" */
#define	NETBSD_SYS_chdir	12

/* syscall: "fchdir" ret: "int" args: "int" */
#define	NETBSD_SYS_fchdir	13

/* syscall: "mknod" ret: "int" args: "const char *" "int" "dev_t" */
#define	NETBSD_SYS_mknod	14

/* syscall: "chmod" ret: "int" args: "const char *" "int" */
#define	NETBSD_SYS_chmod	15

/* syscall: "chown" ret: "int" args: "const char *" "uid_t" "gid_t" */
#define	NETBSD_SYS_chown	16

/* syscall: "break" ret: "int" args: "char *" */
#define	NETBSD_SYS_break	17

/* syscall: "ogetfsstat" ret: "int" args: "struct statfs *" "long" "int" */
#define	NETBSD_SYS_ogetfsstat	18

/* syscall: "olseek" ret: "long" args: "int" "long" "int" */
#define	NETBSD_SYS_olseek	19

/* syscall: "getpid" ret: "pid_t" args: */
#define	NETBSD_SYS_getpid	20

/* syscall: "mount" ret: "int" args: "const char *" "const char *" "int" "void *" */
#define	NETBSD_SYS_mount	21

/* syscall: "unmount" ret: "int" args: "const char *" "int" */
#define	NETBSD_SYS_unmount	22

/* syscall: "setuid" ret: "int" args: "uid_t" */
#define	NETBSD_SYS_setuid	23

/* syscall: "getuid" ret: "uid_t" args: */
#define	NETBSD_SYS_getuid	24

/* syscall: "geteuid" ret: "uid_t" args: */
#define	NETBSD_SYS_geteuid	25

/* syscall: "ptrace" ret: "int" args: "int" "pid_t" "caddr_t" "int" */
#define	NETBSD_SYS_ptrace	26

/* syscall: "recvmsg" ret: "ssize_t" args: "int" "struct msghdr *" "int" */
#define	NETBSD_SYS_recvmsg	27

/* syscall: "sendmsg" ret: "ssize_t" args: "int" "const struct msghdr *" "int" */
#define	NETBSD_SYS_sendmsg	28

/* syscall: "recvfrom" ret: "ssize_t" args: "int" "void *" "size_t" "int" "struct sockaddr *" "socklen_t *" */
#define	NETBSD_SYS_recvfrom	29

/* syscall: "accept" ret: "int" args: "int" "struct sockaddr *" "socklen_t *" */
#define	NETBSD_SYS_accept	30

/* syscall: "getpeername" ret: "int" args: "int" "struct sockaddr *" "int *" */
#define	NETBSD_SYS_getpeername	31

/* syscall: "getsockname" ret: "int" args: "int" "struct sockaddr *" "socklen_t *" */
#define	NETBSD_SYS_getsockname	32

/* syscall: "access" ret: "int" args: "const char *" "int" */
#define	NETBSD_SYS_access	33

/* syscall: "chflags" ret: "int" args: "const char *" "u_int" */
#define	NETBSD_SYS_chflags	34

/* syscall: "fchflags" ret: "int" args: "int" "u_int" */
#define	NETBSD_SYS_fchflags	35

/* syscall: "sync" ret: "void" args: */
#define	NETBSD_SYS_sync	36

/* syscall: "kill" ret: "int" args: "int" "int" */
#define	NETBSD_SYS_kill	37

/* syscall: "ostat" ret: "int" args: "const char *" "struct ostat *" */
#define	NETBSD_SYS_ostat	38

/* syscall: "getppid" ret: "pid_t" args: */
#define	NETBSD_SYS_getppid	39

/* syscall: "olstat" ret: "int" args: "char *" "struct ostat *" */
#define	NETBSD_SYS_olstat	40

/* syscall: "dup" ret: "int" args: "int" */
#define	NETBSD_SYS_dup	41

/* syscall: "opipe" ret: "int" args: */
#define	NETBSD_SYS_opipe	42

/* syscall: "getegid" ret: "gid_t" args: */
#define	NETBSD_SYS_getegid	43

/* syscall: "profil" ret: "int" args: "caddr_t" "size_t" "u_long" "u_int" */
#define	NETBSD_SYS_profil	44

/* syscall: "ktrace" ret: "int" args: "const char *" "int" "int" "pid_t" */
#define	NETBSD_SYS_ktrace	45

/* syscall: "sigaction" ret: "int" args: "int" "const struct sigaction *" "struct sigaction *" */
#define	NETBSD_SYS_sigaction	46

/* syscall: "getgid" ret: "gid_t" args: */
#define	NETBSD_SYS_getgid	47

/* syscall: "sigprocmask" ret: "int" args: "int" "sigset_t" */
#define	NETBSD_SYS_sigprocmask	48

/* syscall: "getlogin" ret: "int" args: "char *" "u_int" */
#define	NETBSD_SYS_getlogin	49

/* syscall: "setlogin" ret: "int" args: "const char *" */
#define	NETBSD_SYS_setlogin	50

/* syscall: "acct" ret: "int" args: "const char *" */
#define	NETBSD_SYS_acct	51

/* syscall: "sigpending" ret: "int" args: */
#define	NETBSD_SYS_sigpending	52

/* syscall: "sigaltstack" ret: "int" args: "const struct sigaltstack *" "struct sigaltstack *" */
#define	NETBSD_SYS_sigaltstack	53

/* syscall: "ioctl" ret: "int" args: "int" "u_long" "..." */
#define	NETBSD_SYS_ioctl	54

/* syscall: "reboot" ret: "int" args: "int" */
#define	NETBSD_SYS_reboot	55

/* syscall: "revoke" ret: "int" args: "const char *" */
#define	NETBSD_SYS_revoke	56

/* syscall: "symlink" ret: "int" args: "const char *" "const char *" */
#define	NETBSD_SYS_symlink	57

/* syscall: "readlink" ret: "int" args: "const char *" "char *" "size_t" */
#define	NETBSD_SYS_readlink	58

/* syscall: "execve" ret: "int" args: "const char *" "char *const *" "char *const *" */
#define	NETBSD_SYS_execve	59

/* syscall: "umask" ret: "int" args: "int" */
#define	NETBSD_SYS_umask	60

/* syscall: "chroot" ret: "int" args: "const char *" */
#define	NETBSD_SYS_chroot	61

/* syscall: "ofstat" ret: "int" args: "int" "struct ostat *" */
#define	NETBSD_SYS_ofstat	62

/* syscall: "ogetkerninfo" ret: "int" args: "int" "char *" "int *" "int" */
#define	NETBSD_SYS_ogetkerninfo	63

/* syscall: "ogetpagesize" ret: "int" args: */
#define	NETBSD_SYS_ogetpagesize	64

/* syscall: "omsync" ret: "int" args: "caddr_t" "size_t" */
#define	NETBSD_SYS_omsync	65

/* syscall: "vfork" ret: "int" args: */
#define	NETBSD_SYS_vfork	66

				/* 67 is obsolete vread */
				/* 68 is obsolete vwrite */
/* syscall: "sbrk" ret: "int" args: "int" */
#define	NETBSD_SYS_sbrk	69

/* syscall: "sstk" ret: "int" args: "int" */
#define	NETBSD_SYS_sstk	70

/* syscall: "ommap" ret: "int" args: "caddr_t" "size_t" "int" "int" "int" "long" */
#define	NETBSD_SYS_ommap	71

/* syscall: "vadvise" ret: "int" args: "int" */
#define	NETBSD_SYS_vadvise	72

/* syscall: "munmap" ret: "int" args: "void *" "size_t" */
#define	NETBSD_SYS_munmap	73

/* syscall: "mprotect" ret: "int" args: "void *" "size_t" "int" */
#define	NETBSD_SYS_mprotect	74

/* syscall: "madvise" ret: "int" args: "void *" "size_t" "int" */
#define	NETBSD_SYS_madvise	75

				/* 76 is obsolete vhangup */
				/* 77 is obsolete vlimit */
/* syscall: "mincore" ret: "int" args: "void *" "size_t" "char *" */
#define	NETBSD_SYS_mincore	78

/* syscall: "getgroups" ret: "int" args: "int" "gid_t *" */
#define	NETBSD_SYS_getgroups	79

/* syscall: "setgroups" ret: "int" args: "int" "const gid_t *" */
#define	NETBSD_SYS_setgroups	80

/* syscall: "getpgrp" ret: "int" args: */
#define	NETBSD_SYS_getpgrp	81

/* syscall: "setpgid" ret: "int" args: "pid_t" "int" */
#define	NETBSD_SYS_setpgid	82

/* syscall: "setitimer" ret: "int" args: "int" "const struct itimerval *" "struct itimerval *" */
#define	NETBSD_SYS_setitimer	83

/* syscall: "owait" ret: "int" args: */
#define	NETBSD_SYS_owait	84

/* syscall: "swapon" ret: "int" args: "const char *" */
#define	NETBSD_SYS_swapon	85

/* syscall: "getitimer" ret: "int" args: "int" "struct itimerval *" */
#define	NETBSD_SYS_getitimer	86

/* syscall: "ogethostname" ret: "int" args: "char *" "u_int" */
#define	NETBSD_SYS_ogethostname	87

/* syscall: "osethostname" ret: "int" args: "char *" "u_int" */
#define	NETBSD_SYS_osethostname	88

/* syscall: "ogetdtablesize" ret: "int" args: */
#define	NETBSD_SYS_ogetdtablesize	89

/* syscall: "dup2" ret: "int" args: "int" "int" */
#define	NETBSD_SYS_dup2	90

/* syscall: "fcntl" ret: "int" args: "int" "int" "..." */
#define	NETBSD_SYS_fcntl	92

/* syscall: "select" ret: "int" args: "int" "fd_set *" "fd_set *" "fd_set *" "struct timeval *" */
#define	NETBSD_SYS_select	93

/* syscall: "fsync" ret: "int" args: "int" */
#define	NETBSD_SYS_fsync	95

/* syscall: "setpriority" ret: "int" args: "int" "int" "int" */
#define	NETBSD_SYS_setpriority	96

/* syscall: "socket" ret: "int" args: "int" "int" "int" */
#define	NETBSD_SYS_socket	97

/* syscall: "connect" ret: "int" args: "int" "const struct sockaddr *" "socklen_t" */
#define	NETBSD_SYS_connect	98

/* syscall: "oaccept" ret: "int" args: "int" "caddr_t" "int *" */
#define	NETBSD_SYS_oaccept	99

/* syscall: "getpriority" ret: "int" args: "int" "int" */
#define	NETBSD_SYS_getpriority	100

/* syscall: "osend" ret: "int" args: "int" "caddr_t" "int" "int" */
#define	NETBSD_SYS_osend	101

/* syscall: "orecv" ret: "int" args: "int" "caddr_t" "int" "int" */
#define	NETBSD_SYS_orecv	102

/* syscall: "sigreturn" ret: "int" args: "struct sigcontext *" */
#define	NETBSD_SYS_sigreturn	103

/* syscall: "bind" ret: "int" args: "int" "const struct sockaddr *" "socklen_t" */
#define	NETBSD_SYS_bind	104

/* syscall: "setsockopt" ret: "int" args: "int" "int" "int" "const void *" "socklen_t" */
#define	NETBSD_SYS_setsockopt	105

/* syscall: "listen" ret: "int" args: "int" "int" */
#define	NETBSD_SYS_listen	106

				/* 107 is obsolete vtimes */
/* syscall: "osigvec" ret: "int" args: "int" "struct sigvec *" "struct sigvec *" */
#define	NETBSD_SYS_osigvec	108

/* syscall: "osigblock" ret: "int" args: "int" */
#define	NETBSD_SYS_osigblock	109

/* syscall: "osigsetmask" ret: "int" args: "int" */
#define	NETBSD_SYS_osigsetmask	110

/* syscall: "sigsuspend" ret: "int" args: "int" */
#define	NETBSD_SYS_sigsuspend	111

/* syscall: "osigstack" ret: "int" args: "struct sigstack *" "struct sigstack *" */
#define	NETBSD_SYS_osigstack	112

/* syscall: "orecvmsg" ret: "int" args: "int" "struct omsghdr *" "int" */
#define	NETBSD_SYS_orecvmsg	113

/* syscall: "osendmsg" ret: "int" args: "int" "caddr_t" "int" */
#define	NETBSD_SYS_osendmsg	114

/* syscall: "vtrace" ret: "int" args: "int" "int" */
#define	NETBSD_SYS_vtrace	115

				/* 115 is obsolete vtrace */
/* syscall: "gettimeofday" ret: "int" args: "struct timeval *" "struct timezone *" */
#define	NETBSD_SYS_gettimeofday	116

/* syscall: "getrusage" ret: "int" args: "int" "struct rusage *" */
#define	NETBSD_SYS_getrusage	117

/* syscall: "getsockopt" ret: "int" args: "int" "int" "int" "void *" "socklen_t *" */
#define	NETBSD_SYS_getsockopt	118

				/* 119 is obsolete resuba */
/* syscall: "readv" ret: "ssize_t" args: "int" "const struct iovec *" "int" */
#define	NETBSD_SYS_readv	120

/* syscall: "writev" ret: "ssize_t" args: "int" "const struct iovec *" "int" */
#define	NETBSD_SYS_writev	121

/* syscall: "settimeofday" ret: "int" args: "const struct timeval *" "const struct timezone *" */
#define	NETBSD_SYS_settimeofday	122

/* syscall: "fchown" ret: "int" args: "int" "uid_t" "gid_t" */
#define	NETBSD_SYS_fchown	123

/* syscall: "fchmod" ret: "int" args: "int" "int" */
#define	NETBSD_SYS_fchmod	124

/* syscall: "orecvfrom" ret: "int" args: "int" "caddr_t" "size_t" "int" "caddr_t" "int *" */
#define	NETBSD_SYS_orecvfrom	125

/* syscall: "osetreuid" ret: "int" args: "int" "int" */
#define	NETBSD_SYS_osetreuid	126

/* syscall: "osetregid" ret: "int" args: "int" "int" */
#define	NETBSD_SYS_osetregid	127

/* syscall: "rename" ret: "int" args: "const char *" "const char *" */
#define	NETBSD_SYS_rename	128

/* syscall: "otruncate" ret: "int" args: "char *" "long" */
#define	NETBSD_SYS_otruncate	129

/* syscall: "oftruncate" ret: "int" args: "int" "long" */
#define	NETBSD_SYS_oftruncate	130

/* syscall: "flock" ret: "int" args: "int" "int" */
#define	NETBSD_SYS_flock	131

/* syscall: "mkfifo" ret: "int" args: "const char *" "int" */
#define	NETBSD_SYS_mkfifo	132

/* syscall: "sendto" ret: "ssize_t" args: "int" "const void *" "size_t" "int" "const struct sockaddr *" "socklen_t" */
#define	NETBSD_SYS_sendto	133

/* syscall: "shutdown" ret: "int" args: "int" "int" */
#define	NETBSD_SYS_shutdown	134

/* syscall: "socketpair" ret: "int" args: "int" "int" "int" "int *" */
#define	NETBSD_SYS_socketpair	135

/* syscall: "mkdir" ret: "int" args: "const char *" "int" */
#define	NETBSD_SYS_mkdir	136

/* syscall: "rmdir" ret: "int" args: "const char *" */
#define	NETBSD_SYS_rmdir	137

/* syscall: "utimes" ret: "int" args: "const char *" "const struct timeval *" */
#define	NETBSD_SYS_utimes	138

				/* 139 is obsolete 4.2 sigreturn */
/* syscall: "adjtime" ret: "int" args: "const struct timeval *" "struct timeval *" */
#define	NETBSD_SYS_adjtime	140

/* syscall: "ogetpeername" ret: "int" args: "int" "caddr_t" "int *" */
#define	NETBSD_SYS_ogetpeername	141

/* syscall: "ogethostid" ret: "int32_t" args: */
#define	NETBSD_SYS_ogethostid	142

/* syscall: "osethostid" ret: "int" args: "int32_t" */
#define	NETBSD_SYS_osethostid	143

/* syscall: "ogetrlimit" ret: "int" args: "u_int" "struct ogetrlimit *" */
#define	NETBSD_SYS_ogetrlimit	144

/* syscall: "osetrlimit" ret: "int" args: "u_int" "struct ogetrlimit *" */
#define	NETBSD_SYS_osetrlimit	145

/* syscall: "okillpg" ret: "int" args: "int" "int" */
#define	NETBSD_SYS_okillpg	146

/* syscall: "setsid" ret: "int" args: */
#define	NETBSD_SYS_setsid	147

/* syscall: "quotactl" ret: "int" args: "const char *" "int" "int" "char *" */
#define	NETBSD_SYS_quotactl	148

/* syscall: "oquota" ret: "int" args: */
#define	NETBSD_SYS_oquota	149

/* syscall: "ogetsockname" ret: "int" args: "int" "caddr_t" "int *" */
#define	NETBSD_SYS_ogetsockname	150

/* syscall: "nfssvc" ret: "int" args: "int" "void *" */
#define	NETBSD_SYS_nfssvc	155

/* syscall: "ogetdirentries" ret: "int" args: "int" "char *" "u_int" "long *" */
#define	NETBSD_SYS_ogetdirentries	156

/* syscall: "ostatfs" ret: "int" args: "char *" "struct ostatfs *" */
#define	NETBSD_SYS_ostatfs	157

/* syscall: "ofstatfs" ret: "int" args: "int" "struct ostatfs *" */
#define	NETBSD_SYS_ofstatfs	158

/* syscall: "getfh" ret: "int" args: "const char *" "fhandle_t *" */
#define	NETBSD_SYS_getfh	161

/* syscall: "ogetdomainname" ret: "int" args: "char *" "int" */
#define	NETBSD_SYS_ogetdomainname	162

/* syscall: "osetdomainname" ret: "int" args: "char *" "int" */
#define	NETBSD_SYS_osetdomainname	163

/* syscall: "ouname" ret: "int" args: "struct outsname *" */
#define	NETBSD_SYS_ouname	164

/* syscall: "sysarch" ret: "int" args: "int" "char *" */
#define	NETBSD_SYS_sysarch	165

/* syscall: "osemsys" ret: "int" args: "int" "int" "int" "int" "int" */
#define	NETBSD_SYS_osemsys	169

/* syscall: "omsgsys" ret: "int" args: "int" "int" "int" "int" "int" "int" */
#define	NETBSD_SYS_omsgsys	170

/* syscall: "shmsys" ret: "int" args: "int" "int" "int" "int" */
#define	NETBSD_SYS_shmsys	171

/* syscall: "ntp_gettime" ret: "int" args: "struct ntptimeval *" */
#define	NETBSD_SYS_ntp_gettime	175

/* syscall: "ntp_adjtime" ret: "int" args: "struct timex *" */
#define	NETBSD_SYS_ntp_adjtime	176

/* syscall: "setgid" ret: "int" args: "gid_t" */
#define	NETBSD_SYS_setgid	181

/* syscall: "setegid" ret: "int" args: "gid_t" */
#define	NETBSD_SYS_setegid	182

/* syscall: "seteuid" ret: "int" args: "uid_t" */
#define	NETBSD_SYS_seteuid	183

/* syscall: "lfs_bmapv" ret: "int" args: "fsid_t *" "struct block_info *" "int" */
#define	NETBSD_SYS_lfs_bmapv	184

/* syscall: "lfs_markv" ret: "int" args: "fsid_t *" "struct block_info *" "int" */
#define	NETBSD_SYS_lfs_markv	185

/* syscall: "lfs_segclean" ret: "int" args: "fsid_t *" "u_long" */
#define	NETBSD_SYS_lfs_segclean	186

/* syscall: "lfs_segwait" ret: "int" args: "fsid_t *" "struct timeval *" */
#define	NETBSD_SYS_lfs_segwait	187

/* syscall: "stat" ret: "int" args: "const char *" "struct stat *" */
#define	NETBSD_SYS_stat	188

/* syscall: "fstat" ret: "int" args: "int" "struct stat12 *" */
#define	NETBSD_SYS_fstat	189

/* syscall: "lstat" ret: "int" args: "const char *" "struct stat *" */
#define	NETBSD_SYS_lstat	190

/* syscall: "pathconf" ret: "long" args: "const char *" "int" */
#define	NETBSD_SYS_pathconf	191

/* syscall: "fpathconf" ret: "long" args: "int" "int" */
#define	NETBSD_SYS_fpathconf	192

/* syscall: "swapctl" ret: "int" args: "int" "const void *" "int" */
#define	NETBSD_SYS_swapctl	193

/* syscall: "getrlimit" ret: "int" args: "int" "struct rlimit *" */
#define	NETBSD_SYS_getrlimit	194

/* syscall: "setrlimit" ret: "int" args: "int" "const struct rlimit *" */
#define	NETBSD_SYS_setrlimit	195

/* syscall: "getdirentries" ret: "int" args: "int" "char *" "int" "long *" */
#define	NETBSD_SYS_getdirentries	196

/* syscall: "mmap" ret: "void *" args: "void *" "size_t" "int" "int" "int" "long" "off_t" */
#define	NETBSD_SYS_mmap	197

/* syscall: "__syscall" ret: "quad_t" args: "quad_t" "..." */
#define	NETBSD_SYS___syscall	198

/* syscall: "lseek" ret: "off_t" args: "int" "int" "off_t" "int" */
#define	NETBSD_SYS_lseek	199

/* syscall: "truncate" ret: "int" args: "const char *" "int" "off_t" */
#define	NETBSD_SYS_truncate	200

/* syscall: "ftruncate" ret: "int" args: "int" "int" "off_t" */
#define	NETBSD_SYS_ftruncate	201

/* syscall: "__sysctl" ret: "int" args: "int *" "u_int" "void *" "size_t *" "void *" "size_t" */
#define	NETBSD_SYS___sysctl	202

/* syscall: "mlock" ret: "int" args: "const void *" "size_t" */
#define	NETBSD_SYS_mlock	203

/* syscall: "munlock" ret: "int" args: "const void *" "size_t" */
#define	NETBSD_SYS_munlock	204

/* syscall: "undelete" ret: "int" args: "const char *" */
#define	NETBSD_SYS_undelete	205

/* syscall: "futimes" ret: "int" args: "int" "const struct timeval *" */
#define	NETBSD_SYS_futimes	206

/* syscall: "getpgid" ret: "int" args: "pid_t" */
#define	NETBSD_SYS_getpgid	207

/* syscall: "xfspioctl" ret: "int" args: "int" "char *" "int" "struct ViceIoctl *" "int" */
#define	NETBSD_SYS_xfspioctl	208

/* syscall: "poll" ret: "int" args: "struct pollfd *" "unsigned long" "int" */
#define	NETBSD_SYS_poll	209

/* syscall: "__osemctl" ret: "int" args: "int" "int" "int" "union semun *" */
#define	NETBSD_SYS___osemctl	220

/* syscall: "semget" ret: "int" args: "key_t" "int" "int" */
#define	NETBSD_SYS_semget	221

/* syscall: "semop" ret: "int" args: "int" "struct sembuf *" "u_int" */
#define	NETBSD_SYS_semop	222

/* syscall: "semconfig" ret: "int" args: "int" */
#define	NETBSD_SYS_semconfig	223

/* syscall: "omsgctl" ret: "int" args: "int" "int" "struct omsqid_ds *" */
#define	NETBSD_SYS_omsgctl	224

/* syscall: "msgget" ret: "int" args: "key_t" "int" */
#define	NETBSD_SYS_msgget	225

/* syscall: "msgsnd" ret: "int" args: "int" "const void *" "size_t" "int" */
#define	NETBSD_SYS_msgsnd	226

/* syscall: "msgrcv" ret: "int" args: "int" "void *" "size_t" "long" "int" */
#define	NETBSD_SYS_msgrcv	227

/* syscall: "shmat" ret: "void *" args: "int" "const void *" "int" */
#define	NETBSD_SYS_shmat	228

/* syscall: "oshmctl" ret: "int" args: "int" "int" "struct oshmid_ds *" */
#define	NETBSD_SYS_oshmctl	229

/* syscall: "shmdt" ret: "int" args: "const void *" */
#define	NETBSD_SYS_shmdt	230

/* syscall: "shmget" ret: "int" args: "key_t" "int" "int" */
#define	NETBSD_SYS_shmget	231

/* syscall: "clock_gettime" ret: "int" args: "clockid_t" "struct timespec *" */
#define	NETBSD_SYS_clock_gettime	232

/* syscall: "clock_settime" ret: "int" args: "clockid_t" "const struct timespec *" */
#define	NETBSD_SYS_clock_settime	233

/* syscall: "clock_getres" ret: "int" args: "clockid_t" "struct timespec *" */
#define	NETBSD_SYS_clock_getres	234

/* syscall: "nanosleep" ret: "int" args: "const struct timespec *" "struct timespec *" */
#define	NETBSD_SYS_nanosleep	240

/* syscall: "getdents" ret: "int" args: "int" "char *" "size_t" */
#define	NETBSD_SYS_getdents	272

/* syscall: "minherit" ret: "int" args: "void *" "size_t" "int" */
#define	NETBSD_SYS_minherit	273

/* syscall: "lchown" ret: "int" args: "const char *" "uid_t" "gid_t" */
#define	NETBSD_SYS_lchown	275

/* syscall: "msync" ret: "int" args: "void *" "size_t" "int" */
#define	NETBSD_SYS_msync	277

/* syscall: "__stat13" ret: "int" args: "const char *" "struct netbsd_stat *" */
#define	NETBSD_SYS___stat13	278

/* syscall: "__fstat13" ret: "int" args: "int" "struct netbsd_stat *" */
#define	NETBSD_SYS___fstat13	279

/* syscall: "__lstat13" ret: "int" args: "const char *" "struct netbsd_stat *" */
#define	NETBSD_SYS___lstat13	280

/* syscall: "__sigaltstack14" ret: "int" args: "const struct netbsd_sigaltstack *" "struct netbsd_sigaltstack *" */
#define	NETBSD_SYS___sigaltstack14	281

/* syscall: "__vfork14" ret: "int" args: */
#define	NETBSD_SYS___vfork14	282

/* syscall: "getsid" ret: "int" args: "pid_t" */
#define	NETBSD_SYS_getsid	286

/* syscall: "__sigaction14" ret: "int" args: "int" "const struct netbsd_sigaction *" "struct netbsd_sigaction *" */
#define	NETBSD_SYS___sigaction14	291

/* syscall: "__sigpending14" ret: "int" args: "netbsd_sigset_t *" */
#define	NETBSD_SYS___sigpending14	292

/* syscall: "__sigprocmask14" ret: "int" args: "int" "const netbsd_sigset_t *" "netbsd_sigset_t *" */
#define	NETBSD_SYS___sigprocmask14	293

/* syscall: "__sigsuspend14" ret: "int" args: "const netbsd_sigset_t *" */
#define	NETBSD_SYS___sigsuspend14	294

/* syscall: "__sigreturn14" ret: "int" args: "struct netbsd_sigcontext *" */
#define	NETBSD_SYS___sigreturn14	295

/* syscall: "__getcwd" ret: "int" args: "char *" "size_t" */
#define	NETBSD_SYS___getcwd	296

#define	NETBSD_SYS_MAXSYSCALL	298
