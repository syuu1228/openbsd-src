* Date: Sat, 16 Mar 1996 19:58:37 -0500 (EST)
* From: Kate Hedstrom <kate@ahab.Rutgers.EDU>
* To: burley@gnu.ai.mit.edu
* Subject: g77 bug in assign
* 
* I found some files in the NCAR graphics source code which used to
* compile with g77 and now don't.  All contain the following combination
* of "save" and "assign".  It fails on a Sun running SunOS 4.1.3 and a
* Sun running SunOS 5.5 (slightly older g77), but compiles on an
* IBM/RS6000:
* 
C
      SUBROUTINE QUICK
      SAVE
C
      ASSIGN 101 TO JUMP
  101 Continue
C
      RETURN
      END
* 
* Everything else in the NCAR distribution compiled, including quite a
* few C routines.
* 
* Kate
* 
* 
* nemo% g77 -v -c quick.f
*  gcc -v -c -xf77 quick.f
* Reading specs from /usr/local/lib/gcc-lib/sparc-sun-sunos4.1.3/2.7.2/specs
* gcc version 2.7.2
*  /usr/local/lib/gcc-lib/sparc-sun-sunos4.1.3/2.7.2/f771 quick.f -fset-g77-defaults -quiet -dumpbase quick.f -version -fversion -o /usr/tmp/cca24166.s
* GNU F77 version 2.7.2 (sparc) compiled by GNU C version 2.7.1.
* GNU Fortran Front End version 0.5.18-960314 compiled: Mar 16 1996 14:28:11
* gcc: Internal compiler error: program f771 got fatal signal 11
* 
* 
* nemo% gdb /usr/local/lib/gcc-lib/*/*/f771 core
* GDB is free software and you are welcome to distribute copies of it
*  under certain conditions; type "show copying" to see the conditions.
* There is absolutely no warranty for GDB; type "show warranty" for details.
* GDB 4.14 (sparc-sun-sunos4.1.3), 
* Copyright 1995 Free Software Foundation, Inc...
* Core was generated by `f771'.
* Program terminated with signal 11, Segmentation fault.
* Couldn't read input and local registers from core file
* find_solib: Can't read pathname for load map: I/O error
* 
* Couldn't read input and local registers from core file
* #0  0x21aa4 in ffecom_sym_transform_assign_ (s=???) at f/com.c:7881
* 7881          if ((ffesymbol_save (s) || ffe_is_saveall ())
* (gdb) where
* #0  0x21aa4 in ffecom_sym_transform_assign_ (s=???) at f/com.c:7881
* Error accessing memory address 0xefffefcc: Invalid argument.
* (gdb) 
* 
* 
* ahab% g77 -v -c quick.f
*  gcc -v -c -xf77 quick.f
* Reading specs from /usr/local/lib/gcc-lib/sparc-sun-solaris2.5/2.7.2/specs
* gcc version 2.7.2
*  /usr/local/lib/gcc-lib/sparc-sun-solaris2.5/2.7.2/f771 quick.f -quiet -dumpbase quick.f -version -fversion -o /var/tmp/cca003D2.s
* GNU F77 version 2.7.2 (sparc) compiled by GNU C version 2.7.2.
* GNU Fortran Front End version 0.5.18-960304 compiled: Mar  5 1996 16:12:46
* gcc: Internal compiler error: program f771 got fatal signal 11
* 
* 
* ahab% !gdb
* gdb /usr/local/lib/gcc-lib/*/*/f771 core
* GDB is free software and you are welcome to distribute copies of it
*  under certain conditions; type "show copying" to see the conditions.
* There is absolutely no warranty for GDB; type "show warranty" for details.
* GDB 4.15.1 (sparc-sun-solaris2.4), 
* Copyright 1995 Free Software Foundation, Inc...
* Core was generated by
* `/usr/local/lib/gcc-lib/sparc-sun-solaris2.5/2.7.2/f771 quick.f -quiet -dumpbase'.
* Program terminated with signal 11, Segmentation fault.
* Reading symbols from /usr/lib/libc.so.1...done.
* Reading symbols from /usr/lib/libdl.so.1...done.
* #0  0x43e04 in ffecom_sym_transform_assign_ (s=0x3a22f8) at f/com.c:7963
* Source file is more recent than executable.
* 7963      assert (st != NULL);
* (gdb) where
* #0  0x43e04 in ffecom_sym_transform_assign_ (s=0x3a22f8) at f/com.c:7963
* #1  0x38044 in ffecom_expr_ (expr=0x3a23c0, dest_tree=0x0, dest=0x0, dest_used=0x0, assignp=true) at f/com.c:2100
* #2  0x489c8 in ffecom_expr_assign_w (expr=0x3a23c0) at f/com.c:10238
* #3  0xe9228 in ffeste_R838 (label=0x3a1ba8, target=0x3a23c0) at f/ste.c:2769
* #4  0xdae60 in ffestd_stmt_pass_ () at f/std.c:840
* #5  0xdc090 in ffestd_exec_end () at f/std.c:1405
* #6  0xcb534 in ffestc_shriek_subroutine_ (ok=true) at f/stc.c:4849
* #7  0xd8f00 in ffestc_R1225 (name=0x0) at f/stc.c:12307
* #8  0xcc808 in ffestc_end () at f/stc.c:5572
* #9  0x9fa84 in ffestb_end3_ (t=0x3a19c8) at f/stb.c:3216
* #10 0x9f30c in ffestb_end (t=0x3a19c8) at f/stb.c:2995
* #11 0x98414 in ffesta_save_ (t=0x3a19c8) at f/sta.c:453
* #12 0x997ec in ffesta_second_ (t=0x3a19c8) at f/sta.c:1178
* #13 0x8ed84 in ffelex_send_token_ () at f/lex.c:1614
* #14 0x8cab8 in ffelex_finish_statement_ () at f/lex.c:946
* #15 0x91684 in ffelex_file_fixed (wf=0x397780, f=0x37a560) at f/lex.c:2946
* #16 0x107a94 in ffe_file (wf=0x397780, f=0x37a560) at f/top.c:456
* #17 0x96218 in yyparse () at f/parse.c:77
* #18 0x10beac in compile_file (name=0xdffffaf7 "quick.f") at toplev.c:2239
* #19 0x110dc0 in main (argc=9, argv=0xdffff994, envp=0xdffff9bc) at toplev.c:3927
