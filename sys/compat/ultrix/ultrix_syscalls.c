/*	$OpenBSD: src/sys/compat/ultrix/Attic/ultrix_syscalls.c,v 1.13 2004/07/09 23:56:46 millert Exp $	*/

/*
 * System call names.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	OpenBSD: syscalls.master,v 1.11 2004/07/09 23:52:02 millert Exp 
 */

char *ultrix_syscallnames[] = {
	"syscall",			/* 0 = syscall */
	"exit",			/* 1 = exit */
	"fork",			/* 2 = fork */
	"read",			/* 3 = read */
	"write",			/* 4 = write */
	"open",			/* 5 = open */
	"close",			/* 6 = close */
	"owait",			/* 7 = owait */
	"creat",			/* 8 = creat */
	"link",			/* 9 = link */
	"unlink",			/* 10 = unlink */
	"execv",			/* 11 = execv */
	"chdir",			/* 12 = chdir */
	"#13 (obsolete time)",		/* 13 = obsolete time */
	"mknod",			/* 14 = mknod */
	"chmod",			/* 15 = chmod */
	"lchown",			/* 16 = lchown */
	"break",			/* 17 = break */
	"#18 (obsolete stat)",		/* 18 = obsolete stat */
	"lseek",			/* 19 = lseek */
	"getpid",			/* 20 = getpid */
	"mount",			/* 21 = mount */
	"#22 (obsolete sysV_unmount)",		/* 22 = obsolete sysV_unmount */
	"setuid",			/* 23 = setuid */
	"getuid",			/* 24 = getuid */
	"#25 (obsolete v7 stime)",		/* 25 = obsolete v7 stime */
	"#26 (obsolete v7 ptrace)",		/* 26 = obsolete v7 ptrace */
	"#27 (obsolete v7 alarm)",		/* 27 = obsolete v7 alarm */
	"#28 (obsolete v7 fstat)",		/* 28 = obsolete v7 fstat */
	"#29 (obsolete v7 pause)",		/* 29 = obsolete v7 pause */
	"#30 (obsolete v7 utime)",		/* 30 = obsolete v7 utime */
	"#31 (obsolete v7 stty)",		/* 31 = obsolete v7 stty */
	"#32 (obsolete v7 gtty)",		/* 32 = obsolete v7 gtty */
	"access",			/* 33 = access */
	"#34 (obsolete v7 nice)",		/* 34 = obsolete v7 nice */
	"#35 (obsolete v7 ftime)",		/* 35 = obsolete v7 ftime */
	"sync",			/* 36 = sync */
	"kill",			/* 37 = kill */
	"stat43",			/* 38 = stat43 */
	"#39 (obsolete v7 setpgrp)",		/* 39 = obsolete v7 setpgrp */
	"olstat",			/* 40 = olstat */
	"dup",			/* 41 = dup */
	"opipe",			/* 42 = opipe */
	"#43 (obsolete v7 times)",		/* 43 = obsolete v7 times */
	"profil",			/* 44 = profil */
	"#45 (unimplemented)",		/* 45 = unimplemented */
	"#46 (obsolete v7 setgid)",		/* 46 = obsolete v7 setgid */
	"getgid",			/* 47 = getgid */
	"#48 (unimplemented ssig)",		/* 48 = unimplemented ssig */
	"#49 (unimplemented reserved for USG)",		/* 49 = unimplemented reserved for USG */
	"#50 (unimplemented reserved for USG)",		/* 50 = unimplemented reserved for USG */
#ifdef ACCOUNTING
	"acct",			/* 51 = acct */
#else
	"#51 (unimplemented acct)",		/* 51 = unimplemented acct */
#endif
	"#52 (unimplemented)",		/* 52 = unimplemented */
	"#53 (unimplemented syslock)",		/* 53 = unimplemented syslock */
	"ioctl",			/* 54 = ioctl */
	"reboot",			/* 55 = reboot */
	"#56 (unimplemented v7 mpxchan)",		/* 56 = unimplemented v7 mpxchan */
	"symlink",			/* 57 = symlink */
	"readlink",			/* 58 = readlink */
	"execve",			/* 59 = execve */
	"umask",			/* 60 = umask */
	"chroot",			/* 61 = chroot */
	"fstat",			/* 62 = fstat */
	"#63 (unimplemented)",		/* 63 = unimplemented */
	"getpagesize",			/* 64 = getpagesize */
	"#65 (unimplemented mremap)",		/* 65 = unimplemented mremap */
	"vfork",			/* 66 = vfork */
	"#67 (obsolete vread)",		/* 67 = obsolete vread */
	"#68 (obsolete vwrite)",		/* 68 = obsolete vwrite */
	"sbrk",			/* 69 = sbrk */
	"sstk",			/* 70 = sstk */
	"mmap",			/* 71 = mmap */
	"vadvise",			/* 72 = vadvise */
	"munmap",			/* 73 = munmap */
	"mprotect",			/* 74 = mprotect */
	"madvise",			/* 75 = madvise */
	"vhangup",			/* 76 = vhangup */
	"#77 (unimplemented old vlimit)",		/* 77 = unimplemented old vlimit */
	"mincore",			/* 78 = mincore */
	"getgroups",			/* 79 = getgroups */
	"setgroups",			/* 80 = setgroups */
	"getpgrp",			/* 81 = getpgrp */
	"setpgrp",			/* 82 = setpgrp */
	"setitimer",			/* 83 = setitimer */
	"wait3",			/* 84 = wait3 */
	"swapon",			/* 85 = swapon */
	"getitimer",			/* 86 = getitimer */
	"gethostname",			/* 87 = gethostname */
	"sethostname",			/* 88 = sethostname */
	"getdtablesize",			/* 89 = getdtablesize */
	"dup2",			/* 90 = dup2 */
	"#91 (unimplemented getdopt)",		/* 91 = unimplemented getdopt */
	"fcntl",			/* 92 = fcntl */
	"select",			/* 93 = select */
	"#94 (unimplemented setdopt)",		/* 94 = unimplemented setdopt */
	"fsync",			/* 95 = fsync */
	"setpriority",			/* 96 = setpriority */
	"socket",			/* 97 = socket */
	"connect",			/* 98 = connect */
	"accept",			/* 99 = accept */
	"getpriority",			/* 100 = getpriority */
	"send",			/* 101 = send */
	"recv",			/* 102 = recv */
	"sigreturn",			/* 103 = sigreturn */
	"bind",			/* 104 = bind */
	"setsockopt",			/* 105 = setsockopt */
	"listen",			/* 106 = listen */
	"#107 (unimplemented vtimes)",		/* 107 = unimplemented vtimes */
	"sigvec",			/* 108 = sigvec */
	"sigblock",			/* 109 = sigblock */
	"sigsetmask",			/* 110 = sigsetmask */
	"sigsuspend",			/* 111 = sigsuspend */
	"sigstack",			/* 112 = sigstack */
	"recvmsg",			/* 113 = recvmsg */
	"sendmsg",			/* 114 = sendmsg */
	"#115 (obsolete vtrace)",		/* 115 = obsolete vtrace */
	"gettimeofday",			/* 116 = gettimeofday */
	"getrusage",			/* 117 = getrusage */
	"getsockopt",			/* 118 = getsockopt */
	"#119 (unimplemented resuba)",		/* 119 = unimplemented resuba */
	"readv",			/* 120 = readv */
	"writev",			/* 121 = writev */
	"settimeofday",			/* 122 = settimeofday */
	"fchown",			/* 123 = fchown */
	"fchmod",			/* 124 = fchmod */
	"recvfrom",			/* 125 = recvfrom */
	"setreuid",			/* 126 = setreuid */
	"setregid",			/* 127 = setregid */
	"rename",			/* 128 = rename */
	"truncate",			/* 129 = truncate */
	"ftruncate",			/* 130 = ftruncate */
	"flock",			/* 131 = flock */
	"#132 (unimplemented)",		/* 132 = unimplemented */
	"sendto",			/* 133 = sendto */
	"shutdown",			/* 134 = shutdown */
	"socketpair",			/* 135 = socketpair */
	"mkdir",			/* 136 = mkdir */
	"rmdir",			/* 137 = rmdir */
	"utimes",			/* 138 = utimes */
	"sigcleanup",			/* 139 = sigcleanup */
	"adjtime",			/* 140 = adjtime */
	"getpeername",			/* 141 = getpeername */
	"gethostid",			/* 142 = gethostid */
	"#143 (unimplemented old sethostid)",		/* 143 = unimplemented old sethostid */
	"getrlimit",			/* 144 = getrlimit */
	"setrlimit",			/* 145 = setrlimit */
	"killpg",			/* 146 = killpg */
	"#147 (unimplemented)",		/* 147 = unimplemented */
	"#148 (unimplemented setquota)",		/* 148 = unimplemented setquota */
	"#149 (unimplemented quota / * needs to be nullop to boot on Ultrix root partition * /)",		/* 149 = unimplemented quota / * needs to be nullop to boot on Ultrix root partition * / */
	"getsockname",			/* 150 = getsockname */
	"#151 (unimplemented sysmips / * 4 args * /)",		/* 151 = unimplemented sysmips / * 4 args * / */
	"#152 (unimplemented cacheflush / * 4 args * /)",		/* 152 = unimplemented cacheflush / * 4 args * / */
	"#153 (unimplemented cachectl / * 3 args * /)",		/* 153 = unimplemented cachectl / * 3 args * / */
	"#154 (unimplemented)",		/* 154 = unimplemented */
	"#155 (unimplemented atomic_op)",		/* 155 = unimplemented atomic_op */
	"#156 (unimplemented)",		/* 156 = unimplemented */
	"#157 (unimplemented)",		/* 157 = unimplemented */
#ifdef NFSSERVER
	"nfssvc",			/* 158 = nfssvc */
#else
	"#158 (unimplemented)",		/* 158 = unimplemented */
#endif
	"getdirentries",			/* 159 = getdirentries */
	"statfs",			/* 160 = statfs */
	"fstatfs",			/* 161 = fstatfs */
	"#162 (unimplemented umount)",		/* 162 = unimplemented umount */
#ifdef NFSCLIENT
	"async_daemon",			/* 163 = async_daemon */
	"getfh",			/* 164 = getfh */
#else
	"#163 (unimplemented async_daemon)",		/* 163 = unimplemented async_daemon */
	"#164 (unimplemented getfh)",		/* 164 = unimplemented getfh */
#endif
	"getdomainname",			/* 165 = getdomainname */
	"setdomainname",			/* 166 = setdomainname */
	"#167 (unimplemented)",		/* 167 = unimplemented */
	"quotactl",			/* 168 = quotactl */
	"exportfs",			/* 169 = exportfs */
	"#170 (unimplemented { int ultrix_sys_mount ( char * special , char * dir , int rdonly , int type , caddr_t data ) ; })",		/* 170 = unimplemented { int ultrix_sys_mount ( char * special , char * dir , int rdonly , int type , caddr_t data ) ; } */
	"#171 (unimplemented 4 hdwconf)",		/* 171 = unimplemented 4 hdwconf */
	"#172 (unimplemented msgctl)",		/* 172 = unimplemented msgctl */
	"#173 (unimplemented msgget)",		/* 173 = unimplemented msgget */
	"#174 (unimplemented msgrcv)",		/* 174 = unimplemented msgrcv */
	"#175 (unimplemented msgsnd)",		/* 175 = unimplemented msgsnd */
	"#176 (unimplemented semctl)",		/* 176 = unimplemented semctl */
	"#177 (unimplemented semget)",		/* 177 = unimplemented semget */
	"#178 (unimplemented semop)",		/* 178 = unimplemented semop */
	"uname",			/* 179 = uname */
	"#180 (unimplemented shmsys)",		/* 180 = unimplemented shmsys */
	"#181 (unimplemented 0 plock)",		/* 181 = unimplemented 0 plock */
	"#182 (unimplemented 0 lockf)",		/* 182 = unimplemented 0 lockf */
	"ustat",			/* 183 = ustat */
	"getmnt",			/* 184 = getmnt */
	"#185 (unimplemented notdef)",		/* 185 = unimplemented notdef */
	"#186 (unimplemented notdef)",		/* 186 = unimplemented notdef */
	"sigpending",			/* 187 = sigpending */
	"setsid",			/* 188 = setsid */
	"waitpid",			/* 189 = waitpid */
	"#190 (unimplemented)",		/* 190 = unimplemented */
	"#191 (unimplemented)",		/* 191 = unimplemented */
	"#192 (unimplemented)",		/* 192 = unimplemented */
	"#193 (unimplemented)",		/* 193 = unimplemented */
	"#194 (unimplemented)",		/* 194 = unimplemented */
	"#195 (unimplemented)",		/* 195 = unimplemented */
	"#196 (unimplemented)",		/* 196 = unimplemented */
	"#197 (unimplemented)",		/* 197 = unimplemented */
	"#198 (unimplemented)",		/* 198 = unimplemented */
	"#199 (unimplemented)",		/* 199 = unimplemented */
	"#200 (unimplemented)",		/* 200 = unimplemented */
	"#201 (unimplemented)",		/* 201 = unimplemented */
	"#202 (unimplemented)",		/* 202 = unimplemented */
	"#203 (unimplemented)",		/* 203 = unimplemented */
	"#204 (unimplemented)",		/* 204 = unimplemented */
	"#205 (unimplemented)",		/* 205 = unimplemented */
	"#206 (unimplemented)",		/* 206 = unimplemented */
	"#207 (unimplemented)",		/* 207 = unimplemented */
	"#208 (unimplemented)",		/* 208 = unimplemented */
	"#209 (unimplemented)",		/* 209 = unimplemented */
	"#210 (unimplemented)",		/* 210 = unimplemented */
	"#211 (unimplemented)",		/* 211 = unimplemented */
	"#212 (unimplemented)",		/* 212 = unimplemented */
	"#213 (unimplemented)",		/* 213 = unimplemented */
	"#214 (unimplemented)",		/* 214 = unimplemented */
	"#215 (unimplemented)",		/* 215 = unimplemented */
	"#216 (unimplemented)",		/* 216 = unimplemented */
	"#217 (unimplemented)",		/* 217 = unimplemented */
	"#218 (unimplemented)",		/* 218 = unimplemented */
	"#219 (unimplemented)",		/* 219 = unimplemented */
	"#220 (unimplemented)",		/* 220 = unimplemented */
	"#221 (unimplemented)",		/* 221 = unimplemented */
	"#222 (unimplemented)",		/* 222 = unimplemented */
	"#223 (unimplemented)",		/* 223 = unimplemented */
	"#224 (unimplemented)",		/* 224 = unimplemented */
	"#225 (unimplemented)",		/* 225 = unimplemented */
	"#226 (unimplemented)",		/* 226 = unimplemented */
	"#227 (unimplemented)",		/* 227 = unimplemented */
	"#228 (unimplemented)",		/* 228 = unimplemented */
	"#229 (unimplemented)",		/* 229 = unimplemented */
	"#230 (unimplemented)",		/* 230 = unimplemented */
	"#231 (unimplemented)",		/* 231 = unimplemented */
	"#232 (unimplemented)",		/* 232 = unimplemented */
	"#233 (unimplemented 1 utc_gettime)",		/* 233 = unimplemented 1 utc_gettime */
	"#234 (unimplemented 2 utc_adjtime)",		/* 234 = unimplemented 2 utc_adjtime */
	"#235 (unimplemented)",		/* 235 = unimplemented */
	"#236 (unimplemented)",		/* 236 = unimplemented */
	"#237 (unimplemented)",		/* 237 = unimplemented */
	"#238 (unimplemented)",		/* 238 = unimplemented */
	"#239 (unimplemented)",		/* 239 = unimplemented */
	"#240 (unimplemented)",		/* 240 = unimplemented */
	"#241 (unimplemented)",		/* 241 = unimplemented */
	"#242 (unimplemented)",		/* 242 = unimplemented */
	"#243 (unimplemented)",		/* 243 = unimplemented */
	"#244 (unimplemented)",		/* 244 = unimplemented */
	"#245 (unimplemented)",		/* 245 = unimplemented */
	"#246 (unimplemented)",		/* 246 = unimplemented */
	"#247 (unimplemented)",		/* 247 = unimplemented */
	"#248 (unimplemented)",		/* 248 = unimplemented */
	"#249 (unimplemented)",		/* 249 = unimplemented */
	"#250 (unimplemented)",		/* 250 = unimplemented */
	"#251 (unimplemented)",		/* 251 = unimplemented */
	"#252 (unimplemented audctl / * Make no-op for installation on Ultrix rootpartition? * /)",		/* 252 = unimplemented audctl / * Make no-op for installation on Ultrix rootpartition? * / */
	"#253 (unimplemented audgen / * Make no-op for installation on Ultrix rootpartition? * /)",		/* 253 = unimplemented audgen / * Make no-op for installation on Ultrix rootpartition? * / */
	"#254 (unimplemented startcpu)",		/* 254 = unimplemented startcpu */
	"#255 (unimplemented stopcpu)",		/* 255 = unimplemented stopcpu */
	"getsysinfo",			/* 256 = getsysinfo */
	"setsysinfo",			/* 257 = setsysinfo */
};
