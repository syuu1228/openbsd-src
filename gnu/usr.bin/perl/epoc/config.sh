#!/bin/sh
#
# This file is manually maintained.
#
# It is NOT produced by running the Configure script.
#

# Package name      : perl5
# Source directory  : .
# Configuration time: 
# Configured by     : Olaf Flebbe
# Target system     : EPOC

Author=''
Date='$Date'
Header=''
Id='$Id'
Locker=''
Log='$Log'
Mcc=''
RCSfile='$RCSfile'
Revision='$Revision'
Source=''
State=''
_a='.a'
_exe='.exe'
_o='.o'
afs='false'
alignbytes='8'
ansi2knr=''
aphostname=''
apirevision=''
apisubversion=''
apiversion=''
ar='arm-pe-ar'
archlib='/perl/lib/5.6.0/epoc'
archlibexp='/perl/lib/5.6.0/epoc'
archname64=''
archname='epoc'
archobjs='epoc.o epocish.o epoc_stubs.o'
awk='awk'
baserev='5.0'
bash=''
bin=''
bincompat5005='false'
binexp=''
bison='bison'
byacc=''
byteorder='1234'
c=''
castflags='0'
cat='cat'
cc='arm-pe-gcc -B/usr/local/lib/gcc-lib/arm-pe/cygnus-2.7.2-960323/ -nostdinc -D__SYMBIAN32__ -D__PSISOFT32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__EXE__ -I/usr/local/epoc/include/ -I/usr/local/epoc/include/libc -DEPOC'
cccdlflags=''
ccdlflags=''
ccflags='-Wno-ctor-dtor-privacy -mcpu-arm710 -mapcs-32 -mshort-load-bytes -msoft-float -fcheck-new -fvtable-thunks'
ccsymbols=''
cf_by='olaf'
cf_email='o.flebbe@gmx.de'
cf_time='Dec 1999'
chgrp=''
chmod=''
chown=''
clocktype=''
comm=''
compress=''
contains='grep'
cp='cp'
cpio=''
cpp='arm-pe-cpp'
cpp_stuff='42'
cppccsymbols='EPOC=1'
cppflags=' -nostdinc -D__SYMBIAN32__ -D__PSISOFT32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__EXE__ -I/usr/local/epoc/include/ -I/usr/local/epoc/include/libc'
cpplast='-'
cppminus='-'
cpprun='arm-pe-gcc -E  -B/usr/local/lib/gcc-lib/arm-pe/cygnus-2.7.2-960323/'
cppstdin='arm-pe-gcc -E -B/usr/local/lib/gcc-lib/arm-pe/cygnus-2.7.2-960323/'
cppsymbols=''
crosscompile='define'
cryptlib=''
csh='csh'
d_Gconvert='sprintf((b),"%.*g",(n),(x))'
d_PRIEldbl='undef'
d_PRIFldbl='undef'
d_PRIGldbl='undef'
d_PRIX64='undef'
d_PRId64='undef'
d_PRIeldbl='undef'
d_PRIfldbl='define'
d_PRIgldbl='define'
d_PRIi64='undef'
d_PRIo64='undef'
d_PRIu64='undef'
d_PRIx64='undef'
d_access='undef'
d_accessx='undef'
d_alarm='undef'
d_archlib='define'
d_atolf='undef'
d_atoll='undef'
d_attribut='undef'
d_bcmp='define'
d_bcopy='define'
d_bincompat5005='undef'
d_bsd='undef'
d_bsdgetpgrp='undef'
d_bsdsetpgrp='undef'
d_bzero='define'
d_casti32='undef'
d_castneg='undef'
d_charvspr='undef'
d_chown='undef'
d_chroot='undef'
d_chsize='undef'
d_closedir='undef'
d_cmsghdr_s='undef'
d_const='define'
d_crypt='undef'
d_csh='undef'
d_cuserid='undef'
d_dbl_dig='undef'
d_difftime='define'
d_dirnamlen='undef'
d_dlerror='undef'
d_dlopen='undef'
d_dlsymun='undef'
d_dosuid='undef'
d_drand48proto='define'
d_dup2='undef'
d_eaccess='undef'
d_endgrent='undef'
d_endhent='undef'
d_endnent='undef'
d_endpent='undef'
d_endpwent='undef'
d_endsent='undef'
d_endspent='undef'
d_eofnblk='define'
d_eunice='undef'
d_fchmod='undef'
d_fchown='undef'
d_fcntl='undef'
d_fd_macros='undef'
d_fd_set='define'
d_fds_bits='undef'
d_fgetpos='define'
d_flexfnam='define'
d_flock='undef'
d_fork='undef'
d_fpathconf='undef'
d_fpos64_t='undef'
d_fseeko='undef'
d_fsetpos='define'
d_fstatfs='define'
d_fstatvfs='undef'
d_ftello='undef'
d_ftime='undef'
d_getfsstat='undef'
d_getgrent='undef'
d_getgrps='undef'
d_gethbyaddr='define'
d_gethbyname='define'
d_gethent='undef'
d_gethname='undef'
d_gethostprotos='define'
d_getlogin='undef'
d_getmntent='undef'
d_getnbyaddr='undef'
d_getnbyname='undef'
d_getnent='undef'
d_getnetprotos='define'
d_getpbyname='define'
d_getpbynumber='define'
d_getpent='undef'
d_getpgid='undef'
d_getpgrp2='undef'
d_getpgrp='undef'
d_getppid='undef'
d_getprior='undef'
d_getprotoprotos='define'
d_getpwent='undef'
d_getsbyname='undef'
d_getsbyport='undef'
d_getsent='undef'
d_getservprotos='define'
d_getspent='undef'
d_getspnam='undef'
d_gettimeod='define'
d_gnulibc='undef'
d_grpasswd='undef'
d_hasmntopt='undef'
d_htonl='define'
d_iconv='undef'
d_index='undef'
d_inetaton='define'
d_int64t='undef'
d_iovec_s='undef'
d_isascii='define'
d_killpg='undef'
d_lchown='undef'
d_ldbl_dig='undef'
d_link='undef'
d_llseek='undef'
d_locconv='undef'
d_lockf='undef'
d_longdbl='undef'
d_longlong='define'
d_lseekproto='define'
d_lstat='undef'
d_madvise='undef'
d_mblen='undef'
d_mbstowcs='undef'
d_mbtowc='undef'
d_memchr='define'
d_memcmp='define'
d_memcpy='define'
d_memmove='define'
d_memset='define'
d_mkdir='define'
d_mkfifo='undef'
d_mktime='define'
d_mmap='undef'
d_mprotect='undef'
d_msg='undef'
d_msg_ctrunc='undef'
d_msg_dontroute='undef'
d_msg_oob='undef'
d_msg_peek='undef'
d_msg_proxy='undef'
d_msgctl='undef'
d_msgget='undef'
d_msghdr_s='undef'
d_msgrcv='undef'
d_msgsnd='undef'
d_msync='undef'
d_munmap='undef'
d_mymalloc='undef'
d_nice='undef'
d_off64_t='undef'
d_old_pthread_create_joinable='undef'
d_oldpthreads='undef'
d_oldsock='undef'
d_open3='define'
d_pathconf='undef'
d_pause='undef'
d_phostname='undef'
d_pipe='undef'
d_poll='undef'
d_portable='undef'
d_pthread_yield='undef'
d_pwage='undef'
d_pwchange='undef'
d_pwclass='undef'
d_pwcomment='undef'
d_pwexpire='undef'
d_pwgecos='undef'
d_pwpasswd='undef'
d_pwquota='undef'
d_qgcvt='undef'
d_readdir='define'
d_readlink='undef'
d_readv='undef'
d_recvmsg='undef'
d_rename='define'
d_rewinddir='define'
d_rmdir='define'
d_safebcpy='undef'
d_safemcpy='undef'
d_sanemcmp='define'
d_sched_yield='undef'
d_scm_rights='undef'
d_seekdir='define'
d_select='undef'
d_sem='undef'
d_semctl='undef'
d_semctl_semid_ds='define'
d_semctl_semun='define'
d_semget='undef'
d_semop='undef'
d_sendmsg='undef'
d_setegid='undef'
d_seteuid='undef'
d_setgrent='undef'
d_setgrps='undef'
d_sethent='undef'
d_setlinebuf='undef'
d_setlocale='undef'
d_setnent='undef'
d_setpent='undef'
d_setpgid='undef'
d_setpgrp2='undef'
d_setpgrp='undef'
d_setprior='undef'
d_setpwent='undef'
d_setregid='undef'
d_setresgid='undef'
d_setresuid='undef'
d_setreuid='undef'
d_setrgid='undef'
d_setruid='undef'
d_setsent='undef'
d_setsid='undef'
d_setspent='undef'
d_setvbuf='undef'
d_sfio='undef'
d_shm='undef'
d_shmat='undef'
d_shmatprototype='undef'
d_shmctl='undef'
d_shmdt='undef'
d_shmget='undef'
d_sigaction='undef'
d_sigsetjmp='undef'
d_socket='define'
d_sockpair='undef'
d_statblks='define'
d_statfs='undef'
d_statfsflags='define'
d_statvfs='undef'
d_stdio_cnt_lval='define'
d_stdio_ptr_lval='define'
d_stdio_stream_array='undef'
d_stdiobase='undef'
d_stdstdio='undef'
d_strchr='define'
d_strcoll='define'
d_strctcpy='define'
d_strerrm='strerror(e)'
d_strerror='define'
d_strtod='define'
d_strtol='define'
d_strtoul='define'
d_strtoull='undef'
d_strxfrm='define'
d_suidsafe='undef'
d_symlink='undef'
d_syscall='undef'
d_sysconf='define'
d_sysernlst='undef'
d_syserrlst='undef'
d_system='define'
d_tcgetpgrp='undef'
d_tcsetpgrp='undef'
d_telldir='define'
d_telldirproto='define'
d_time='undef'
d_times='undef'
d_truncate='undef'
d_tzname='undef'
d_umask='undef'
d_uname='undef'
d_union_semun='undef'
d_vendorlib='undef'
d_vfork='undef'
d_void_closedir='undef'
d_voidsig='undef'
d_voidtty='undef'
d_volatile='define'
d_vprintf='define'
d_wait4='undef'
d_waitpid='undef'
d_wcstombs='undef'
d_wctomb='undef'
d_writev='undef'
d_xenix='undef'
date='date'
db_hashtype='undef'
db_prefixtype='undef'
defvoidused='15'
direntrytype='struct dirent'
dlext='none'
dlsrc='dl_none.xs'
doublesize='8'
drand01='(rand()/(double)(1U<<RANDBITS))'
dynamic_ext=''
eagain='EAGAIN'
ebcdic='undef'
echo='echo'
egrep='egrep'
emacs=''
eunicefix=':'
exe_ext=''
expr='expr'
extensions='Data/Dumper File/Glob IO Socket'
fflushNULL='undef'
fflushall='define'
find=''
firstmakefile='makefile'
flex=''
fpostype='fpos_t'
freetype='void'
full_ar='/usr/local/bin/arm-pe-ar'
full_csh=''
full_sed='/usr/bin/sed'
gccversion=''
gidtype='gid_t'
glibpth=''
grep='grep'
groupcat=''
groupstype='gid_t'
gzip='gzip'
h_fcntl=''
h_sysfile=''
hint=''
hostcat=''
huge=''
i_arpainet='define'
i_bsdioctl='undef'
i_db='undef'
i_dbm='undef'
i_dirent='define'
i_dld='undef'
i_dlfcn='undef'
i_fcntl='define'
i_float='undef'
i_gdbm='undef'
i_grp='undef'
i_iconv='undef'
i_ieeefp='undef'
i_inttypes='undef'
i_limits='define'
i_locale='undef'
i_machcthr='undef'
i_malloc='undef'
i_math='define'
i_memory='undef'
i_mntent='undef'
i_ndbm='undef'
i_netdb='define'
i_neterrno='undef'
i_netinettcp='define'
i_niin='define'
i_poll='undef'
i_pthread='undef'
i_pwd='undef'
i_rpcsvcdbm='undef'
i_sfio='undef'
i_sgtty='undef'
i_shadow='undef'
i_socks='undef'
i_stdarg='define'
i_stddef='define'
i_stdlib='define'
i_string='define'
i_sunmath='undef'
i_sysaccess='undef'
i_sysdir='undef'
i_sysfile='undef'
i_sysfilio='undef'
i_sysin='undef'
i_sysioctl='define'
i_sysmman='undef'
i_sysmount='undef'
i_sysndir='undef'
i_sysparam='define'
i_sysresrc='define'
i_syssecrt='undef'
i_sysselct='undef'
i_syssockio='undef'
i_sysstat='define'
i_sysstatvfs='undef'
i_systime='define'
i_systimek='undef'
i_systimes='define'
i_systypes='define'
i_sysuio='undef'
i_sysun='undef'
i_syswait='define'
i_termio='undef'
i_termios='undef'
i_time='define'
i_unistd='define'
i_utime='undef'
i_values='undef'
i_varargs='undef'
i_varhdr='undef'
i_vfork='undef'
ignore_versioned_solibs=''
incpath=''
inews=''
installarchlib='/home/olaf/E/lib'
installbin='/home/olaf/E/bin'
installman1dir=''
installman3dir=''
installprefix='/home/olaf/'
installprefixexp=''
installprivlib=''
installscript=''
installsitearch='/home/olaf/E/site/'
installsitelib='/home/olaf/E/site/lib'
installstyle=''
installusrbinperl='undef'
installvendorlib=''
intsize='4'
known_extensions='Data/Dumper File/Glob IO Socket'
ksh=''
large=''
ld='echo'
lddlflags=''
ldflags=''
ldlibpthname=''
less=''
lib_ext=''
libc=''
libperl='perl.a'
libpth=''
libs=''
libswanted=''
line=''
lint=''
lkflags=''
ln='ln'
lns='/bin/ln -s'
locincpth=''
loclibpth=''
longdblsize='8'
longlongsize='8'
longsize='4'
lp=''
lpr=''
ls='ls'
lseeksize='8'
lseektype='off_t'
mail=''
mailx=''
make='make'
make_set_make='#'
mallocobj=''
mallocsrc=''
malloctype='void *'
man1dir=''
man1direxp=''
man1ext=''
man3dir=''
man3direxp=''
man3ext=''
medium=''
mips=''
mips_type=''
mkdir='mkdir'
mmaptype=''
models='none'
modetype='mode_t'
more='more'
multiarch='define'
mv=''
myarchname='epoc'
mydomain='.gmx.de'
myhostname='dragon'
myuname=''
n='-n'
netdb_hlen_type='int'
netdb_host_type='const char *'
netdb_name_type='const char *'
netdb_net_type='int'
nm='arm-pe-nm'
nm_opt=''
nm_so_opt=''
nonxs_ext=''
nroff='nroff'
o_nonblock='O_NONBLOCK'
obj_ext=''
old_pthread_create_joinable=''
optimize='-fomit-frame-pointer -DNDEBUG -O'
orderlib=''
osname='epoc'
osvers=''
package=''
pager=''
passcat=''
patchlevel=''
path_sep=''
perl=''
perladmin=''
perlpath=''
pg=''
phostname=''
pidtype='pid_t'
plibpth=''
pmake=''
pr=''
prefix=''
prefixexp=''
privlib='/perl/lib/5.6.0'
privlibexp='/perl/lib/5.6.0'
prototype='define'
ptrsize='4'
randbits='31'
randfunc=''
randseedtype='unsigned'
ranlib='arm-pe-ranlib'
rd_nodata='-1'
rm='rm'
rmail=''
runnm='false'
sPRIEldbl=''
sPRIFldbl=''
sPRIGldbl=''
sPRIX64=''
sPRId64=''
sPRIeldbl=''
sPRIfldbl='"f"'
sPRIgldbl='"g"'
sPRIi64=''
sPRIo64=''
sPRIu64=''
sPRIx64=''
sched_yield=''
scriptdir=''
scriptdirexp=''
sed='sed'
seedfunc='srand'
selectminbits='32'
selecttype=''
sendmail=''
sh='/bin/sh'
shar=''
sharpbang='#!'
shmattype=''
shortsize='2'
shrpenv=''
shsharp=''
sig_count='ZERO '
sig_name=''
sig_name_init='"ZERO", 0'
sig_num='0'
sig_num_init='0, 0'
signal_t='void'
sitearch='/perl/lib/site_perl/5.6.0/epoc'
sitearchexp='/perl/lib/site_perl/5.6.0/epoc'
sitelib='/perl/lib/site_perl/5.6.0/'
sitelib_stem='/perl/lib/site_perl'
sitelibexp='/perl/lib/site_perl/5.6.0/'
siteprefix=''
siteprefixexp=''
sizesize='4'
sizetype='size_t'
sleep=''
smail=''
small=''
so=''
socksizetype='int'
sockethdr=''
socketlib=''
sort='sort'
spackage=''
spitshell='cat'
split=''
src='.'
ssizetype='long'
startperl=''
startsh='#!/bin/sh'
static_ext='Data/Dumper File/Glob IO Socket'
stdchar='char'
stdio_base=''
stdio_bufsiz=''
stdio_cnt=''
stdio_filbuf=''
stdio_ptr=''
stdio_stream_array=''
strings=''
submit=''
subversion=''
sysman=''
tail=''
tar=''
tbl=''
tee='tee'
test='test'
timeincl=''
timetype='time_t'
touch='touch'
tr='tr'
trnl='\n'
troff=''
uidsign='1'
uidtype='uid_t'
uname='uname'
uniq='uniq'
use64bitall='undef'
use64bitint='undef'
usedl='undef'
uselargefiles='undef'
uselongdouble='undef'
usemorebits='undef'
usemultiplicity='undef'
usemymalloc='n'
usenm=''
useopcode=''
useperlio='undef'
useposix=''
usesfio=''
useshrplib=''
usesocks='undef'
usethreads='undef'
usevendorprefix=''
usevfork=''
usrinc=''
uuname=''
vendorlib=''
vendorlib_stem=''
vendorlibexp=''
vendorprefix=''
vendorprefixexp=''
version='5.6.0'
vi=''
voidflags='15'
xlibpth=''
zcat=''
zip=''
# Configure command line arguments.
config_arg0=''
config_args=''
config_argc=11
config_arg1=''
config_arg2=''
config_arg3=''
config_arg4=''
config_arg5=''
config_arg6=''
config_arg7=''
config_arg8=''
config_arg9=''
config_arg10=''
config_arg11=''
PERL_REVISION=5
PERL_VERSION=6
PERL_SUBVERSION=0
PERL_API_REVISION=5
PERL_API_VERSION=6
PERL_API_SUBVERSION=0
CONFIGDOTSH=true
# Variables propagated from previous config.sh file.
pp_sys_cflags=''
epocish_cflags='ccflags="$cflags -xc++"'
ivtype='int'
uvtype='unsigned int'
i8type='char'
u8type='unsigned char'
i16type='short'
u16type='unsigned short'
i32type='int'
u32type='unsigned int'
i64type='long long'
u64type='unsigned long long'
d_quad='define'
quadtype='long long'
quadtype='unsigned long long'
quadkind='QUAD_IS_LONG_LONG'
nvtype='double'
ivsize='4'
uvsize='4'
i8size='1'
u8size='1'
i16size='2'
u16size='2'
i32size='4'
u32size='4'
i64size='8'
u64size='8'
d_fs_data_s='undef'
d_fseeko='undef'
d_ldbl_dig='undef'
d_sqrtl='undef'
d_getmnt='undef'
d_statfs_f_flags='undef'
d_statfs_s='undef'
d_ustat='undef'
i_sysstatfs='undef'
i_sysvfs='undef'
i_ustat='undef'
uidsize='2'
uidsign='1'
gidsize='2'
gidsign='1'
ivdformat='"ld"'
uvuformat='"lu"'
uvoformat='"lo"'
uvxformat='"lx"'
uidformat='"hu"'
gidformat='"hu"'
d_strtold='undef'
d_strtoll='undef'
d_strtouq='undef'
d_nv_preserves_uv='define'
use5005threads='undef'
useithreads='undef'
inc_version_list=' '
inc_version_list_init='0'
