#name: R_MIPS16_HI16 and R_MIPS16_LO16 relocs
#source: ../../../gas/testsuite/gas/mips/mips16-hilo.s
#source: mips16-hilo.s
#objdump: -mmips:16 --prefix-addresses -tdr --show-raw-insn
#ld: -Tmips16-hilo.ld -e 0x500000 -N

.*:     file format elf.*mips.*

#...

Disassembly of section .text:
0+500000 <[^>]*> 6c00      	li	a0,0
0+500002 <[^>]*> f400 3480 	sll	a0,16
0+500006 <[^>]*> 4c00      	addiu	a0,0
0+500008 <[^>]*> f060 6c05 	li	a0,101
0+50000c <[^>]*> f400 3480 	sll	a0,16
0+500010 <[^>]*> f328 4c00 	addiu	a0,17184
0+500014 <[^>]*> f060 6c05 	li	a0,101
0+500018 <[^>]*> f400 3480 	sll	a0,16
0+50001c <[^>]*> f328 4c04 	addiu	a0,17188
0+500020 <[^>]*> f060 6c05 	li	a0,101
0+500024 <[^>]*> f400 3480 	sll	a0,16
0+500028 <[^>]*> f328 4c10 	addiu	a0,17200
0+50002c <[^>]*> f060 6c05 	li	a0,101
0+500030 <[^>]*> f400 3480 	sll	a0,16
0+500034 <[^>]*> f728 4c00 	addiu	a0,18208
0+500038 <[^>]*> f060 6c16 	li	a0,118
0+50003c <[^>]*> f400 3480 	sll	a0,16
0+500040 <[^>]*> f02b 4c00 	addiu	a0,22560
0+500044 <[^>]*> f060 6c16 	li	a0,118
0+500048 <[^>]*> f400 3480 	sll	a0,16
0+50004c <[^>]*> f40b 4c09 	addiu	a0,23561
0+500050 <[^>]*> f060 6c16 	li	a0,118
0+500054 <[^>]*> f400 3480 	sll	a0,16
0+500058 <[^>]*> f42a 4c10 	addiu	a0,21552
0+50005c <[^>]*> f060 6c16 	li	a0,118
0+500060 <[^>]*> f400 3480 	sll	a0,16
0+500064 <[^>]*> f40b 4c08 	addiu	a0,23560
0+500068 <[^>]*> 6c00      	li	a0,0
0+50006a <[^>]*> f400 3480 	sll	a0,16
0+50006e <[^>]*> 4c01      	addiu	a0,1
0+500070 <[^>]*> f060 6c05 	li	a0,101
0+500074 <[^>]*> f400 3480 	sll	a0,16
0+500078 <[^>]*> f328 4c01 	addiu	a0,17185
0+50007c <[^>]*> f060 6c05 	li	a0,101
0+500080 <[^>]*> f400 3480 	sll	a0,16
0+500084 <[^>]*> f328 4c05 	addiu	a0,17189
0+500088 <[^>]*> f060 6c05 	li	a0,101
0+50008c <[^>]*> f400 3480 	sll	a0,16
0+500090 <[^>]*> f328 4c11 	addiu	a0,17201
0+500094 <[^>]*> f060 6c05 	li	a0,101
0+500098 <[^>]*> f400 3480 	sll	a0,16
0+50009c <[^>]*> f728 4c01 	addiu	a0,18209
0+5000a0 <[^>]*> f060 6c16 	li	a0,118
0+5000a4 <[^>]*> f400 3480 	sll	a0,16
0+5000a8 <[^>]*> f02b 4c01 	addiu	a0,22561
0+5000ac <[^>]*> f060 6c16 	li	a0,118
0+5000b0 <[^>]*> f400 3480 	sll	a0,16
0+5000b4 <[^>]*> f40b 4c0a 	addiu	a0,23562
0+5000b8 <[^>]*> f060 6c16 	li	a0,118
0+5000bc <[^>]*> f400 3480 	sll	a0,16
0+5000c0 <[^>]*> f42a 4c11 	addiu	a0,21553
0+5000c4 <[^>]*> f060 6c16 	li	a0,118
0+5000c8 <[^>]*> f400 3480 	sll	a0,16
0+5000cc <[^>]*> f40b 4c09 	addiu	a0,23561
0+5000d0 <[^>]*> 6c01      	li	a0,1
0+5000d2 <[^>]*> f400 3480 	sll	a0,16
0+5000d6 <[^>]*> f010 4c00 	addiu	a0,-32768
0+5000da <[^>]*> f060 6c06 	li	a0,102
0+5000de <[^>]*> f400 3480 	sll	a0,16
0+5000e2 <[^>]*> f338 4c00 	addiu	a0,-15584
0+5000e6 <[^>]*> f060 6c06 	li	a0,102
0+5000ea <[^>]*> f400 3480 	sll	a0,16
0+5000ee <[^>]*> f338 4c04 	addiu	a0,-15580
0+5000f2 <[^>]*> f060 6c06 	li	a0,102
0+5000f6 <[^>]*> f400 3480 	sll	a0,16
0+5000fa <[^>]*> f338 4c10 	addiu	a0,-15568
0+5000fe <[^>]*> f060 6c06 	li	a0,102
0+500102 <[^>]*> f400 3480 	sll	a0,16
0+500106 <[^>]*> f738 4c00 	addiu	a0,-14560
0+50010a <[^>]*> f060 6c17 	li	a0,119
0+50010e <[^>]*> f400 3480 	sll	a0,16
0+500112 <[^>]*> f03b 4c00 	addiu	a0,-10208
0+500116 <[^>]*> f060 6c17 	li	a0,119
0+50011a <[^>]*> f400 3480 	sll	a0,16
0+50011e <[^>]*> f41b 4c09 	addiu	a0,-9207
0+500122 <[^>]*> f060 6c17 	li	a0,119
0+500126 <[^>]*> f400 3480 	sll	a0,16
0+50012a <[^>]*> f43a 4c10 	addiu	a0,-11216
0+50012e <[^>]*> f060 6c17 	li	a0,119
0+500132 <[^>]*> f400 3480 	sll	a0,16
0+500136 <[^>]*> f41b 4c08 	addiu	a0,-9208
0+50013a <[^>]*> 6c00      	li	a0,0
0+50013c <[^>]*> f400 3480 	sll	a0,16
0+500140 <[^>]*> f010 4c00 	addiu	a0,-32768
0+500144 <[^>]*> f060 6c05 	li	a0,101
0+500148 <[^>]*> f400 3480 	sll	a0,16
0+50014c <[^>]*> f338 4c00 	addiu	a0,-15584
0+500150 <[^>]*> f060 6c05 	li	a0,101
0+500154 <[^>]*> f400 3480 	sll	a0,16
0+500158 <[^>]*> f338 4c04 	addiu	a0,-15580
0+50015c <[^>]*> f060 6c05 	li	a0,101
0+500160 <[^>]*> f400 3480 	sll	a0,16
0+500164 <[^>]*> f338 4c10 	addiu	a0,-15568
0+500168 <[^>]*> f060 6c05 	li	a0,101
0+50016c <[^>]*> f400 3480 	sll	a0,16
0+500170 <[^>]*> f738 4c00 	addiu	a0,-14560
0+500174 <[^>]*> f060 6c16 	li	a0,118
0+500178 <[^>]*> f400 3480 	sll	a0,16
0+50017c <[^>]*> f03b 4c00 	addiu	a0,-10208
0+500180 <[^>]*> f060 6c16 	li	a0,118
0+500184 <[^>]*> f400 3480 	sll	a0,16
0+500188 <[^>]*> f41b 4c09 	addiu	a0,-9207
0+50018c <[^>]*> f060 6c16 	li	a0,118
0+500190 <[^>]*> f400 3480 	sll	a0,16
0+500194 <[^>]*> f43a 4c10 	addiu	a0,-11216
0+500198 <[^>]*> f060 6c16 	li	a0,118
0+50019c <[^>]*> f400 3480 	sll	a0,16
0+5001a0 <[^>]*> f41b 4c08 	addiu	a0,-9208
0+5001a4 <[^>]*> 6c01      	li	a0,1
0+5001a6 <[^>]*> f400 3480 	sll	a0,16
0+5001aa <[^>]*> 4c00      	addiu	a0,0
0+5001ac <[^>]*> f060 6c06 	li	a0,102
0+5001b0 <[^>]*> f400 3480 	sll	a0,16
0+5001b4 <[^>]*> f328 4c00 	addiu	a0,17184
0+5001b8 <[^>]*> f060 6c06 	li	a0,102
0+5001bc <[^>]*> f400 3480 	sll	a0,16
0+5001c0 <[^>]*> f328 4c04 	addiu	a0,17188
0+5001c4 <[^>]*> f060 6c06 	li	a0,102
0+5001c8 <[^>]*> f400 3480 	sll	a0,16
0+5001cc <[^>]*> f328 4c10 	addiu	a0,17200
0+5001d0 <[^>]*> f060 6c06 	li	a0,102
0+5001d4 <[^>]*> f400 3480 	sll	a0,16
0+5001d8 <[^>]*> f728 4c00 	addiu	a0,18208
0+5001dc <[^>]*> f060 6c17 	li	a0,119
0+5001e0 <[^>]*> f400 3480 	sll	a0,16
0+5001e4 <[^>]*> f02b 4c00 	addiu	a0,22560
0+5001e8 <[^>]*> f060 6c17 	li	a0,119
0+5001ec <[^>]*> f400 3480 	sll	a0,16
0+5001f0 <[^>]*> f40b 4c09 	addiu	a0,23561
0+5001f4 <[^>]*> f060 6c17 	li	a0,119
0+5001f8 <[^>]*> f400 3480 	sll	a0,16
0+5001fc <[^>]*> f42a 4c10 	addiu	a0,21552
0+500200 <[^>]*> f060 6c17 	li	a0,119
0+500204 <[^>]*> f400 3480 	sll	a0,16
0+500208 <[^>]*> f40b 4c08 	addiu	a0,23560
0+50020c <[^>]*> 6c02      	li	a0,2
0+50020e <[^>]*> f400 3480 	sll	a0,16
0+500212 <[^>]*> f5b4 4c05 	addiu	a0,-23131
0+500216 <[^>]*> f060 6c07 	li	a0,103
0+50021a <[^>]*> f400 3480 	sll	a0,16
0+50021e <[^>]*> f0dd 4c05 	addiu	a0,-5947
0+500222 <[^>]*> f060 6c07 	li	a0,103
0+500226 <[^>]*> f400 3480 	sll	a0,16
0+50022a <[^>]*> f0dd 4c09 	addiu	a0,-5943
0+50022e <[^>]*> f060 6c07 	li	a0,103
0+500232 <[^>]*> f400 3480 	sll	a0,16
0+500236 <[^>]*> f0dd 4c15 	addiu	a0,-5931
0+50023a <[^>]*> f060 6c07 	li	a0,103
0+50023e <[^>]*> f400 3480 	sll	a0,16
0+500242 <[^>]*> f4dd 4c05 	addiu	a0,-4923
0+500246 <[^>]*> f060 6c18 	li	a0,120
0+50024a <[^>]*> f400 3480 	sll	a0,16
0+50024e <[^>]*> f5df 4c05 	addiu	a0,-571
0+500252 <[^>]*> f060 6c18 	li	a0,120
0+500256 <[^>]*> f400 3480 	sll	a0,16
0+50025a <[^>]*> f1a0 4c0e 	addiu	a0,430
0+50025e <[^>]*> f060 6c18 	li	a0,120
0+500262 <[^>]*> f400 3480 	sll	a0,16
0+500266 <[^>]*> f1df 4c15 	addiu	a0,-1579
0+50026a <[^>]*> f060 6c18 	li	a0,120
0+50026e <[^>]*> f400 3480 	sll	a0,16
0+500272 <[^>]*> f1a0 4c0d 	addiu	a0,429
0+500276 <[^>]*> 6d00      	li	a1,0
0+500278 <[^>]*> f400 35a0 	sll	a1,16
0+50027c <[^>]*> 9d80      	lw	a0,0\(a1\)
0+50027e <[^>]*> f060 6d05 	li	a1,101
0+500282 <[^>]*> f400 35a0 	sll	a1,16
0+500286 <[^>]*> f060 9d85 	lw	a0,101\(a1\)
0+50028a <[^>]*> f060 6d05 	li	a1,101
0+50028e <[^>]*> f400 35a0 	sll	a1,16
0+500292 <[^>]*> f060 9d85 	lw	a0,101\(a1\)
0+500296 <[^>]*> f060 6d05 	li	a1,101
0+50029a <[^>]*> f400 35a0 	sll	a1,16
0+50029e <[^>]*> f328 9d90 	lw	a0,17200\(a1\)
0+5002a2 <[^>]*> f060 6d05 	li	a1,101
0+5002a6 <[^>]*> f400 35a0 	sll	a1,16
0+5002aa <[^>]*> f728 9d80 	lw	a0,18208\(a1\)
0+5002ae <[^>]*> f060 6d16 	li	a1,118
0+5002b2 <[^>]*> f400 35a0 	sll	a1,16
0+5002b6 <[^>]*> f02b 9d80 	lw	a0,22560\(a1\)
0+5002ba <[^>]*> f060 6d16 	li	a1,118
0+5002be <[^>]*> f400 35a0 	sll	a1,16
0+5002c2 <[^>]*> f40b 9d89 	lw	a0,23561\(a1\)
0+5002c6 <[^>]*> f060 6d16 	li	a1,118
0+5002ca <[^>]*> f400 35a0 	sll	a1,16
0+5002ce <[^>]*> f42a 9d90 	lw	a0,21552\(a1\)
0+5002d2 <[^>]*> f060 6d16 	li	a1,118
0+5002d6 <[^>]*> f400 35a0 	sll	a1,16
0+5002da <[^>]*> f40b 9d88 	lw	a0,23560\(a1\)
0+5002de <[^>]*> 6d00      	li	a1,0
0+5002e0 <[^>]*> f400 35a0 	sll	a1,16
0+5002e4 <[^>]*> f000 9d81 	lw	a0,1\(a1\)
0+5002e8 <[^>]*> f060 6d05 	li	a1,101
0+5002ec <[^>]*> f400 35a0 	sll	a1,16
0+5002f0 <[^>]*> f328 9d81 	lw	a0,17185\(a1\)
0+5002f4 <[^>]*> f060 6d05 	li	a1,101
0+5002f8 <[^>]*> f400 35a0 	sll	a1,16
0+5002fc <[^>]*> f328 9d85 	lw	a0,17189\(a1\)
0+500300 <[^>]*> f060 6d05 	li	a1,101
0+500304 <[^>]*> f400 35a0 	sll	a1,16
0+500308 <[^>]*> f328 9d91 	lw	a0,17201\(a1\)
0+50030c <[^>]*> f060 6d05 	li	a1,101
0+500310 <[^>]*> f400 35a0 	sll	a1,16
0+500314 <[^>]*> f728 9d81 	lw	a0,18209\(a1\)
0+500318 <[^>]*> f060 6d16 	li	a1,118
0+50031c <[^>]*> f400 35a0 	sll	a1,16
0+500320 <[^>]*> f02b 9d81 	lw	a0,22561\(a1\)
0+500324 <[^>]*> f060 6d16 	li	a1,118
0+500328 <[^>]*> f400 35a0 	sll	a1,16
0+50032c <[^>]*> f40b 9d8a 	lw	a0,23562\(a1\)
0+500330 <[^>]*> f060 6d16 	li	a1,118
0+500334 <[^>]*> f400 35a0 	sll	a1,16
0+500338 <[^>]*> f42a 9d91 	lw	a0,21553\(a1\)
0+50033c <[^>]*> f060 6d16 	li	a1,118
0+500340 <[^>]*> f400 35a0 	sll	a1,16
0+500344 <[^>]*> f40b 9d89 	lw	a0,23561\(a1\)
0+500348 <[^>]*> 6d01      	li	a1,1
0+50034a <[^>]*> f400 35a0 	sll	a1,16
0+50034e <[^>]*> f010 9d80 	lw	a0,-32768\(a1\)
0+500352 <[^>]*> f060 6d06 	li	a1,102
0+500356 <[^>]*> f400 35a0 	sll	a1,16
0+50035a <[^>]*> f338 9d80 	lw	a0,-15584\(a1\)
0+50035e <[^>]*> f060 6d06 	li	a1,102
0+500362 <[^>]*> f400 35a0 	sll	a1,16
0+500366 <[^>]*> f338 9d84 	lw	a0,-15580\(a1\)
0+50036a <[^>]*> f060 6d06 	li	a1,102
0+50036e <[^>]*> f400 35a0 	sll	a1,16
0+500372 <[^>]*> f338 9d90 	lw	a0,-15568\(a1\)
0+500376 <[^>]*> f060 6d06 	li	a1,102
0+50037a <[^>]*> f400 35a0 	sll	a1,16
0+50037e <[^>]*> f738 9d80 	lw	a0,-14560\(a1\)
0+500382 <[^>]*> f060 6d17 	li	a1,119
0+500386 <[^>]*> f400 35a0 	sll	a1,16
0+50038a <[^>]*> f03b 9d80 	lw	a0,-10208\(a1\)
0+50038e <[^>]*> f060 6d17 	li	a1,119
0+500392 <[^>]*> f400 35a0 	sll	a1,16
0+500396 <[^>]*> f41b 9d89 	lw	a0,-9207\(a1\)
0+50039a <[^>]*> f060 6d17 	li	a1,119
0+50039e <[^>]*> f400 35a0 	sll	a1,16
0+5003a2 <[^>]*> f43a 9d90 	lw	a0,-11216\(a1\)
0+5003a6 <[^>]*> f060 6d17 	li	a1,119
0+5003aa <[^>]*> f400 35a0 	sll	a1,16
0+5003ae <[^>]*> f41b 9d88 	lw	a0,-9208\(a1\)
0+5003b2 <[^>]*> 6d00      	li	a1,0
0+5003b4 <[^>]*> f400 35a0 	sll	a1,16
0+5003b8 <[^>]*> f010 9d80 	lw	a0,-32768\(a1\)
0+5003bc <[^>]*> f060 6d05 	li	a1,101
0+5003c0 <[^>]*> f400 35a0 	sll	a1,16
0+5003c4 <[^>]*> f338 9d80 	lw	a0,-15584\(a1\)
0+5003c8 <[^>]*> f060 6d05 	li	a1,101
0+5003cc <[^>]*> f400 35a0 	sll	a1,16
0+5003d0 <[^>]*> f338 9d84 	lw	a0,-15580\(a1\)
0+5003d4 <[^>]*> f060 6d05 	li	a1,101
0+5003d8 <[^>]*> f400 35a0 	sll	a1,16
0+5003dc <[^>]*> f338 9d90 	lw	a0,-15568\(a1\)
0+5003e0 <[^>]*> f060 6d05 	li	a1,101
0+5003e4 <[^>]*> f400 35a0 	sll	a1,16
0+5003e8 <[^>]*> f738 9d80 	lw	a0,-14560\(a1\)
0+5003ec <[^>]*> f060 6d16 	li	a1,118
0+5003f0 <[^>]*> f400 35a0 	sll	a1,16
0+5003f4 <[^>]*> f03b 9d80 	lw	a0,-10208\(a1\)
0+5003f8 <[^>]*> f060 6d16 	li	a1,118
0+5003fc <[^>]*> f400 35a0 	sll	a1,16
0+500400 <[^>]*> f41b 9d89 	lw	a0,-9207\(a1\)
0+500404 <[^>]*> f060 6d16 	li	a1,118
0+500408 <[^>]*> f400 35a0 	sll	a1,16
0+50040c <[^>]*> f43a 9d90 	lw	a0,-11216\(a1\)
0+500410 <[^>]*> f060 6d16 	li	a1,118
0+500414 <[^>]*> f400 35a0 	sll	a1,16
0+500418 <[^>]*> f41b 9d88 	lw	a0,-9208\(a1\)
0+50041c <[^>]*> 6d01      	li	a1,1
0+50041e <[^>]*> f400 35a0 	sll	a1,16
0+500422 <[^>]*> 9d80      	lw	a0,0\(a1\)
0+500424 <[^>]*> f060 6d06 	li	a1,102
0+500428 <[^>]*> f400 35a0 	sll	a1,16
0+50042c <[^>]*> f328 9d80 	lw	a0,17184\(a1\)
0+500430 <[^>]*> f060 6d06 	li	a1,102
0+500434 <[^>]*> f400 35a0 	sll	a1,16
0+500438 <[^>]*> f328 9d84 	lw	a0,17188\(a1\)
0+50043c <[^>]*> f060 6d06 	li	a1,102
0+500440 <[^>]*> f400 35a0 	sll	a1,16
0+500444 <[^>]*> f328 9d90 	lw	a0,17200\(a1\)
0+500448 <[^>]*> f060 6d06 	li	a1,102
0+50044c <[^>]*> f400 35a0 	sll	a1,16
0+500450 <[^>]*> f728 9d80 	lw	a0,18208\(a1\)
0+500454 <[^>]*> f060 6d17 	li	a1,119
0+500458 <[^>]*> f400 35a0 	sll	a1,16
0+50045c <[^>]*> f02b 9d80 	lw	a0,22560\(a1\)
0+500460 <[^>]*> f060 6d17 	li	a1,119
0+500464 <[^>]*> f400 35a0 	sll	a1,16
0+500468 <[^>]*> f40b 9d89 	lw	a0,23561\(a1\)
0+50046c <[^>]*> f060 6d17 	li	a1,119
0+500470 <[^>]*> f400 35a0 	sll	a1,16
0+500474 <[^>]*> f42a 9d90 	lw	a0,21552\(a1\)
0+500478 <[^>]*> f060 6d17 	li	a1,119
0+50047c <[^>]*> f400 35a0 	sll	a1,16
0+500480 <[^>]*> f40b 9d88 	lw	a0,23560\(a1\)
0+500484 <[^>]*> 6d02      	li	a1,2
0+500486 <[^>]*> f400 35a0 	sll	a1,16
0+50048a <[^>]*> f5b4 9d85 	lw	a0,-23131\(a1\)
0+50048e <[^>]*> f060 6d07 	li	a1,103
0+500492 <[^>]*> f400 35a0 	sll	a1,16
0+500496 <[^>]*> f0dd 9d85 	lw	a0,-5947\(a1\)
0+50049a <[^>]*> f060 6d07 	li	a1,103
0+50049e <[^>]*> f400 35a0 	sll	a1,16
0+5004a2 <[^>]*> f0dd 9d89 	lw	a0,-5943\(a1\)
0+5004a6 <[^>]*> f060 6d07 	li	a1,103
0+5004aa <[^>]*> f400 35a0 	sll	a1,16
0+5004ae <[^>]*> f0dd 9d95 	lw	a0,-5931\(a1\)
0+5004b2 <[^>]*> f060 6d07 	li	a1,103
0+5004b6 <[^>]*> f400 35a0 	sll	a1,16
0+5004ba <[^>]*> f4dd 9d85 	lw	a0,-4923\(a1\)
0+5004be <[^>]*> f060 6d18 	li	a1,120
0+5004c2 <[^>]*> f400 35a0 	sll	a1,16
0+5004c6 <[^>]*> f5df 9d85 	lw	a0,-571\(a1\)
0+5004ca <[^>]*> f060 6d18 	li	a1,120
0+5004ce <[^>]*> f400 35a0 	sll	a1,16
0+5004d2 <[^>]*> f1a0 9d8e 	lw	a0,430\(a1\)
0+5004d6 <[^>]*> f060 6d18 	li	a1,120
0+5004da <[^>]*> f400 35a0 	sll	a1,16
0+5004de <[^>]*> f1df 9d95 	lw	a0,-1579\(a1\)
0+5004e2 <[^>]*> f060 6d18 	li	a1,120
0+5004e6 <[^>]*> f400 35a0 	sll	a1,16
0+5004ea <[^>]*> f1a0 9d8d 	lw	a0,429\(a1\)
0+5004ee <[^>]*> 6500      	nop
#pass
