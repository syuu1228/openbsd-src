#name: s390x opcode
#objdump: -drw

.*: +file format .*

Disassembly of section .text:

.* <foo>:
.*:	e3 65 a0 00 80 08 [	 ]*ag	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 18 [	 ]*agf	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 7a [	 ]*ahy	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 98 [	 ]*alc	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 88 [	 ]*alcg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 0a [	 ]*alg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 1a [	 ]*algf	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 5e [	 ]*aly	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 5a [	 ]*ay	%r6,-524288\(%r5,%r10\)
.*:	e3 60 50 00 80 46 [	 ]*bctg	%r6,-524288\(%r5\)
.*:	eb 69 50 00 80 44 [	 ]*bxhg	%r6,%r9,-524288\(%r5\)
.*:	eb 69 50 00 80 45 [	 ]*bxleg	%r6,%r9,-524288\(%r5\)
.*:	eb 69 50 00 80 3e [	 ]*cdsg	%r6,%r9,-524288\(%r5\)
.*:	eb 69 50 00 80 31 [	 ]*cdsy	%r6,%r9,-524288\(%r5\)
.*:	e3 65 a0 00 80 20 [	 ]*cg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 30 [	 ]*cgf	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 79 [	 ]*chy	%r6,-524288\(%r5,%r10\)
.*:	eb 69 50 00 80 8f [	 ]*clclu	%r6,%r9,-524288\(%r5\)
.*:	e3 65 a0 00 80 21 [	 ]*clg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 31 [	 ]*clgf	%r6,-524288\(%r5,%r10\)
.*:	eb ff 50 00 80 55 [	 ]*cliy	-524288\(%r5\),255
.*:	eb 6f 50 00 80 20 [	 ]*clmh	%r6,15,-524288\(%r5\)
.*:	eb 6f 50 00 80 21 [	 ]*clmy	%r6,15,-524288\(%r5\)
.*:	e3 65 a0 00 80 55 [	 ]*cly	%r6,-524288\(%r5,%r10\)
.*:	eb 69 50 00 80 30 [	 ]*csg	%r6,%r9,-524288\(%r5\)
.*:	b9 8a 00 69 [	 ]*cspg	%r6,%r9
.*:	eb 69 50 00 80 14 [	 ]*csy	%r6,%r9,-524288\(%r5\)
.*:	e3 65 a0 00 80 0e [	 ]*cvbg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 06 [	 ]*cvby	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 2e [	 ]*cvdg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 26 [	 ]*cvdy	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 59 [	 ]*cy	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 97 [	 ]*dl	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 87 [	 ]*dlg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 0d [	 ]*dsg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 1d [	 ]*dsgf	%r6,-524288\(%r5,%r10\)
.*:	eb 6f 50 00 80 80 [	 ]*icmh	%r6,15,-524288\(%r5\)
.*:	eb 6f 50 00 80 81 [	 ]*icmy	%r6,15,-524288\(%r5\)
.*:	e3 65 a0 00 80 73 [	 ]*icy	%r6,-524288\(%r5,%r10\)
.*:	b9 8e 50 69 [	 ]*idte	%r6,%r9,%r5
.*:	eb 69 50 00 80 9a [	 ]*lamy	%a6,%a9,-524288\(%r5\)
.*:	e3 65 a0 00 80 71 [	 ]*lay	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 76 [	 ]*lb	%r6,-524288\(%r5,%r10\)
.*:	eb 69 50 00 80 2f [	 ]*lctlg	%c6,%c9,-524288\(%r5\)
.*:	ed 65 a0 00 80 65 [	 ]*ldy	%f6,-524288\(%r5,%r10\)
.*:	ed 65 a0 00 80 64 [	 ]*ley	%f6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 04 [	 ]*lg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 77 [	 ]*lgb	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 14 [	 ]*lgf	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 15 [	 ]*lgh	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 78 [	 ]*lhy	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 90 [	 ]*llgc	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 16 [	 ]*llgf	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 91 [	 ]*llgh	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 17 [	 ]*llgt	%r6,-524288\(%r5,%r10\)
.*:	eb 69 50 00 80 04 [	 ]*lmg	%r6,%r9,-524288\(%r5\)
.*:	eb 69 50 00 80 96 [	 ]*lmh	%r6,%r9,-524288\(%r5\)
.*:	eb 69 50 00 80 98 [	 ]*lmy	%r6,%r9,-524288\(%r5\)
.*:	e3 65 a0 00 80 8f [	 ]*lpq	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 03 [	 ]*lrag	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 13 [	 ]*lray	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 1e [	 ]*lrv	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 0f [	 ]*lrvg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 1f [	 ]*lrvh	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 58 [	 ]*ly	%r6,-524288\(%r5,%r10\)
.*:	ed 95 af ff 60 3e [	 ]*mad	%f6,%f9,4095\(%r5,%r10\)
.*:	b3 3e 60 95 [	 ]*madr	%f6,%f9,%f5
.*:	ed 95 af ff 60 2e [	 ]*mae	%f6,%f9,4095\(%r5,%r10\)
.*:	b3 2e 60 95 [	 ]*maer	%f6,%f9,%f5
.*:	e3 65 a0 00 80 96 [	 ]*ml	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 86 [	 ]*mlg	%r6,-524288\(%r5,%r10\)
.*:	ed 95 af ff 60 3f [	 ]*msd	%f6,%f9,4095\(%r5,%r10\)
.*:	b3 3f 60 95 [	 ]*msdr	%f6,%f9,%f5
.*:	ed 95 af ff 60 2f [	 ]*mse	%f6,%f9,4095\(%r5,%r10\)
.*:	b3 2f 60 95 [	 ]*mser	%f6,%f9,%f5
.*:	e3 65 a0 00 80 0c [	 ]*msg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 1c [	 ]*msgf	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 51 [	 ]*msy	%r6,-524288\(%r5,%r10\)
.*:	eb 69 50 00 80 8e [	 ]*mvclu	%r6,%r9,-524288\(%r5\)
.*:	eb ff 50 00 80 52 [	 ]*mviy	-524288\(%r5\),255
.*:	e3 65 a0 00 80 80 [	 ]*ng	%r6,-524288\(%r5,%r10\)
.*:	eb ff 50 00 80 54 [	 ]*niy	-524288\(%r5\),255
.*:	e3 65 a0 00 80 54 [	 ]*ny	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 81 [	 ]*og	%r6,-524288\(%r5,%r10\)
.*:	eb ff 50 00 80 56 [	 ]*oiy	-524288\(%r5\),255
.*:	e3 65 a0 00 80 56 [	 ]*oy	%r6,-524288\(%r5,%r10\)
.*:	eb 69 50 00 80 1d [	 ]*rll	%r6,%r9,-524288\(%r5\)
.*:	eb 69 50 00 80 1c [	 ]*rllg	%r6,%r9,-524288\(%r5\)
.*:	e3 65 a0 00 80 09 [	 ]*sg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 19 [	 ]*sgf	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 7b [	 ]*shy	%r6,-524288\(%r5,%r10\)
.*:	eb 69 50 00 80 0b [	 ]*slag	%r6,%r9,-524288\(%r5\)
.*:	e3 65 a0 00 80 99 [	 ]*slb	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 89 [	 ]*slbg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 0b [	 ]*slg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 1b [	 ]*slgf	%r6,-524288\(%r5,%r10\)
.*:	eb 69 50 00 80 0d [	 ]*sllg	%r6,%r9,-524288\(%r5\)
.*:	e3 65 a0 00 80 5f [	 ]*sly	%r6,-524288\(%r5,%r10\)
.*:	eb 69 50 00 80 0a [	 ]*srag	%r6,%r9,-524288\(%r5\)
.*:	eb 69 50 00 80 0c [	 ]*srlg	%r6,%r9,-524288\(%r5\)
.*:	eb 69 50 00 80 9b [	 ]*stamy	%a6,%a9,-524288\(%r5\)
.*:	eb 6f 50 00 80 2c [	 ]*stcmh	%r6,15,-524288\(%r5\)
.*:	eb 6f 50 00 80 2d [	 ]*stcmy	%r6,15,-524288\(%r5\)
.*:	eb 69 50 00 80 25 [	 ]*stctg	%c6,%c9,-524288\(%r5\)
.*:	e3 65 a0 00 80 72 [	 ]*stcy	%r6,-524288\(%r5,%r10\)
.*:	ed 65 a0 00 80 67 [	 ]*stdy	%f6,-524288\(%r5,%r10\)
.*:	ed 65 a0 00 80 66 [	 ]*stey	%f6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 24 [	 ]*stg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 70 [	 ]*sthy	%r6,-524288\(%r5,%r10\)
.*:	eb 69 50 00 80 24 [	 ]*stmg	%r6,%r9,-524288\(%r5\)
.*:	eb 69 50 00 80 26 [	 ]*stmh	%r6,%r9,-524288\(%r5\)
.*:	eb 69 50 00 80 90 [	 ]*stmy	%r6,%r9,-524288\(%r5\)
.*:	e3 65 a0 00 80 8e [	 ]*stpq	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 3e [	 ]*strv	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 2f [	 ]*strvg	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 3f [	 ]*strvh	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 50 [	 ]*sty	%r6,-524288\(%r5,%r10\)
.*:	e3 65 a0 00 80 5b [	 ]*sy	%r6,-524288\(%r5,%r10\)
.*:	eb ff 50 00 80 51 [	 ]*tmy	-524288\(%r5\),255
.*:	eb 69 50 00 80 0f [	 ]*tracg	%r6,%r9,-524288\(%r5\)
.*:	e3 65 a0 00 80 82 [	 ]*xg	%r6,-524288\(%r5,%r10\)
.*:	eb ff 50 00 80 57 [	 ]*xiy	-524288\(%r5\),255
.*:	e3 65 a0 00 80 57 [	 ]*xy	%r6,-524288\(%r5,%r10\)
.*:	07 07 [	 ]*bcr	0,%r7
