@echo off
echo Configuring GCC for go32
rem This batch file assumes a unix-type "sed" program

if not exist config\msdos\configure.bat chdir ..\..

update config\i386\xm-dos.h config.h
update config\i386\xm-dos.h hconfig.h
update config\i386\xm-dos.h tconfig.h
update config\i386\go32.h tm.h
update config\i386\i386.md md
update config\i386\i386.c aux-output.c

echo # Makefile generated by "configure.bat"> Makefile
echo all.dos: cccp cc1 cc1obj xgcc libgcc.a s-objlist >> Makefile
sed -f config/msdos/top.sed Makefile.in >> Makefile

set LANG=

if not exist cp\make-lang.in goto no_cp
sed -f config/msdos/top.sed cp\make-lang.in >> Makefile
sed -f config/msdos/top.sed cp\makefile.in > cp\Makefile
set LANG=%LANG% c++.&	
:no_cp

echo lang.mostlyclean: %LANG% | sed "s/&/mostlyclean/g" >> Makefile
echo lang.clean: %LANG% | sed "s/&/clean/g" >> Makefile
echo lang.distclean: %LANG% | sed "s/&/distclean/g" >> Makefile
echo lang.maintainer-clean: %LANG% | sed "s/&/maintainer-clean/g" >> Makefile
echo /* options.h */ > options.h
if exist cp\lang-options.h echo #include "cp/lang-options.h" >> options.h
if exist ada\lang-options.h echo #include "ada/lang-options.h" >> options.h
if exist f\lang-options.h echo #include "f/lang-options.h" >> options.h
echo /* specs.h */ > specs.h
if exist cp\lang-specs.h echo #include "cp/lang-specs.h" >> specs.h
if exist ada\lang-specs.h echo #include "ada/lang-specs.h" >> specs.h
if exist f\lang-specs.h echo #include "f/lang-specs.h" >> specs.h

echo #define MULTILIB_SELECT ". ;" > multilib.h1
update multilib.h1 multilib.h
del multilib.h1
