#source: start.s
#source: a.s
#source: bza.s
#as: -no-expand
#ld: -m mmo
#objdump: -dr

.*:     file format mmo

Disassembly of section \.text:

0+ <(Main|_start)>:
   0+:	e3fd0001 	setl \$253,0x1

0+4 <a>:
   4:	e3fd0004 	setl \$253,0x4

0+8 <bza>:
   8:	e3fd0002 	setl \$253,0x2
   c:	43eafffe 	bz \$234,4 <a>
  10:	e3fd0003 	setl \$253,0x3
