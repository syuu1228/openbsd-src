@echo off
echo Generating x86 for MASM assember

echo Bignum
cd crypto\bn\asm
perl x86.pl win32 > bn-win32.asm
cd ..\..\..

echo DES
cd crypto\des\asm
perl des-586.pl win32 > d-win32.asm
cd ..\..\..

echo "crypt(3)"

cd crypto\des\asm
perl crypt586.pl win32 > y-win32.asm
cd ..\..\..

echo Blowfish

cd crypto\bf\asm
perl bf-586.pl win32 > b-win32.asm
cd ..\..\..

echo CAST5
cd crypto\cast\asm
perl cast-586.pl win32 > c-win32.asm
cd ..\..\..

echo RC4
cd crypto\rc4\asm
perl rc4-586.pl win32 > r4-win32.asm
cd ..\..\..

echo MD5
cd crypto\md5\asm
perl md5-586.pl win32 > m5-win32.asm
cd ..\..\..

echo SHA1
cd crypto\sha\asm
perl sha1-586.pl win32 > s1-win32.asm
cd ..\..\..

echo RIPEMD160
cd crypto\ripemd\asm
perl rmd-586.pl win32 > rm-win32.asm
cd ..\..\..

echo RC5\32
cd crypto\rc5\asm
perl rc5-586.pl win32 > r5-win32.asm
cd ..\..\..

echo on

perl util\mkfiles.pl >MINFO
rem perl util\mk1mf.pl VC-MSDOS no-sock >ms\msdos.mak
rem perl util\mk1mf.pl VC-W31-32 >ms\w31.mak
perl util\mk1mf.pl dll VC-W31-32 >ms\w31dll.mak
perl util\mk1mf.pl VC-WIN32 >ms\nt.mak
perl util\mk1mf.pl dll VC-WIN32 >ms\ntdll.mak

perl util\mkdef.pl 16 libeay > ms\libeay16.def
perl util\mkdef.pl 32 libeay > ms\libeay32.def
perl util\mkdef.pl 16 ssleay > ms\ssleay16.def
perl util\mkdef.pl 32 ssleay > ms\ssleay32.def
