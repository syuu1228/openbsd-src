#objdump: -dr
#name: @OC@

# Test the @OC@ insn.


.*:[ 	]+file format .*-cris

Disassembly of section \.text:
0+ <notstart>:
[ 	]+0:	0000[ ]+	bcc ( 0x2|\.\+2)
	\.\.\.

0+4 <start>:
[	 ]+4:[	 ]+@IR+0354@[ 	]+@OC@\.b \$?r3,\$?r5
[	 ]+6:[	 ]+@IR+15d4@[ 	]+@OC@\.w \$?r5,\$?r13
[	 ]+8:[	 ]+@IM+0058@[ 	]+@OC@\.b \[\$?r0\],\$?r5
[	 ]+a:[	 ]+@IM+15d8@[ 	]+@OC@\.w \[\$?r5\],\$?r13
[	 ]+c:[	 ]+@IM+005c@[ 	]+@OC@\.b \[\$?r0\+\],\$?r5
[	 ]+e:[	 ]+@IM+15dc@[ 	]+@OC@\.w \[\$?r5\+\],\$?r13
[	 ]+10:[	 ]+@IM+0f5c@ 0000[ 	]+@OC@\.b (0x|)0,\$?r5
[	 ]+14:[	 ]+@IM+0f5c@ 0100[ 	]+@OC@\.b (0x|)1,\$?r5
[	 ]+18:[	 ]+@IM+0f5c@ 7f00[ 	]+@OC@\.b (0x7f|127),\$?r5
[	 ]+1c:[	 ]+@IM+0f5c@ 8000[ 	]+@OC@\.b (0x80|128),\$?r5
[	 ]+20:[	 ]+@IM+0f5c@ ffff[ 	]+@OC@\.b (0xffff|-1),\$?r5
[	 ]+24:[	 ]+@IM+0f5c@ 81ff[ 	]+@OC@\.b (0xff81|-127),\$?r5
[	 ]+28:[	 ]+@IM+0f5c@ 80ff[ 	]+@OC@\.b (0xff80|-128),\$?r5
[	 ]+2c:[	 ]+@IM+0f5c@ ff00[ 	]+@OC@\.b (0xff|255),\$?r5
[	 ]+30:[	 ]+@IM+0f5c@ 2a00[ 	]+@OC@\.b (0x2a|42),\$?r5
[	 ]+34:[	 ]+@IM+0f5c@ d6ff[ 	]+@OC@\.b (0xffd6|-42),\$?r5
[	 ]+38:[	 ]+@IM+0f5c@ 2a00[ 	]+@OC@\.b (0x2a|42),\$?r5
[	 ]+3c:[	 ]+@IM+0f5c@ d6ff[ 	]+@OC@\.b (0xffd6|-42),\$?r5
[	 ]+40:[	 ]+@IM+0f5c@ d6ff[ 	]+@OC@\.b (0xffd6|-42),\$?r5
[	 ]+44:[	 ]+@IM+0f5c@ 2a00[ 	]+@OC@\.b (0x2a|42),\$?r5
[	 ]+48:[	 ]+@IM+0f5c@ 0000[ 	]+@OC@\.b (0x|)0,\$?r5
[ 	]+4a:[ 	]+(R_CRIS_)?16[ 	]+externalsym
[	 ]+4c:[	 ]+@IM+1fdc@ 0000[ 	]+@OC@\.w (0x|)0,\$?r13
[	 ]+50:[	 ]+@IM+1fdc@ 0100[ 	]+@OC@\.w (0x|)1,\$?r13
[	 ]+54:[	 ]+@IM+1fdc@ 7f00[ 	]+@OC@\.w (0x7f|127),\$?r13
[	 ]+58:[	 ]+@IM+1fdc@ 8000[ 	]+@OC@\.w (0x80|128),\$?r13
[	 ]+5c:[	 ]+@IM+1fdc@ ffff[ 	]+@OC@\.w (0xffff|-1),\$?r13
[	 ]+60:[	 ]+@IM+1fdc@ 81ff[ 	]+@OC@\.w (0xff81|-127),\$?r13
[	 ]+64:[	 ]+@IM+1fdc@ 80ff[ 	]+@OC@\.w (0xff80|-128),\$?r13
[	 ]+68:[	 ]+@IM+1fdc@ 7fff[ 	]+@OC@\.w (0xff7f|-129),\$?r13
[	 ]+6c:[	 ]+@IM+1fdc@ ff00[ 	]+@OC@\.w (0xff|255),\$?r13
[	 ]+70:[	 ]+@IM+1fdc@ 01ff[ 	]+@OC@\.w (0xff01|-255),\$?r13
[	 ]+74:[	 ]+@IM+1fdc@ 0001[ 	]+@OC@\.w (0x[0]?100|256),\$?r13
[	 ]+78:[	 ]+@IM+1fdc@ 68dd[ 	]+@OC@\.w (0xdd68|-8856),\$?r13
[	 ]+7c:[	 ]+@IM+1fdc@ 9822[ 	]+@OC@\.w (0x2298|8856),\$?r13
[	 ]+80:[	 ]+@IM+1fdc@ 2a00[ 	]+@OC@\.w (0x2a|42),\$?r13
[	 ]+84:[	 ]+@IM+1fdc@ d6ff[ 	]+@OC@\.w (0xffd6|-42),\$?r13
[	 ]+88:[	 ]+@IM+1fdc@ 2a00[ 	]+@OC@\.w (0x2a|42),\$?r13
[	 ]+8c:[	 ]+@IM+1fdc@ d6ff[ 	]+@OC@\.w (0xffd6|-42),\$?r13
[	 ]+90:[	 ]+@IM+1f5c@ d6ff[ 	]+@OC@\.w (0xffd6|-42),\$?r5
[	 ]+94:[	 ]+@IM+1f5c@ 2a00[ 	]+@OC@\.w (0x2a|42),\$?r5
[	 ]+98:[	 ]+@IM+1f5c@ ff7f[ 	]+@OC@\.w (0x7fff|32767),\$?r5
[	 ]+9c:[	 ]+@IM+1f5c@ 0080[ 	]+@OC@\.w (0x8000|-32768),\$?r5
[	 ]+a0:[	 ]+@IM+1fdc@ 0180[ 	]+@OC@\.w (0x8001|-32767),\$?r13
[	 ]+a4:[	 ]+@IM+1fdc@ 0180[ 	]+@OC@\.w (0x8001|-32767),\$?r13
[	 ]+a8:[	 ]+@IM+1fdc@ 0080[ 	]+@OC@\.w (0x8000|-32768),\$?r13
[	 ]+ac:[	 ]+@IM+1f5c@ ffff[ 	]+@OC@\.w (0xffff|-1),\$?r5
[	 ]+b0:[	 ]+@IM+1f5c@ 0000[ 	]+@OC@\.w (0x|)0,\$?r5
[ 	]+b2:[ 	]+(R_CRIS_)?16[ 	]+externalsym
[	 ]+b4:[	 ]+4205 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+\$?r0\.b\],\$?r5
[	 ]+b8:[	 ]+4255 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+\$?r5\.b\],\$?r13
[	 ]+bc:[	 ]+4029 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+\[\$?r0\]\.b\],\$?r5
[	 ]+c0:[	 ]+4529 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\]\.b\],\$?r13
[	 ]+c4:[	 ]+402d @IM+0558@[ 	]+@OC@\.b \[\$?r2\+\[\$?r0\+\]\.b\],\$?r5
[	 ]+c8:[	 ]+452d @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\+\]\.b\],\$?r13
[	 ]+cc:[	 ]+452d @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\+\]\.b\],\$?r13
[	 ]+d0:[	 ]+5205 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+\$?r0\.w\],\$?r5
[	 ]+d4:[	 ]+5255 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+\$?r5\.w\],\$?r13
[	 ]+d8:[	 ]+5029 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+\[\$?r0\]\.w\],\$?r5
[	 ]+dc:[	 ]+5529 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\]\.w\],\$?r13
[	 ]+e0:[	 ]+502d @IM+0558@[ 	]+@OC@\.b \[\$?r2\+\[\$?r0\+\]\.w\],\$?r5
[	 ]+e4:[	 ]+552d @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\+\]\.w\],\$?r13
[	 ]+e8:[	 ]+552d @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\+\]\.w\],\$?r13
[	 ]+ec:[	 ]+6205 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+\$?r0\.d\],\$?r5
[	 ]+f0:[	 ]+6255 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+\$?r5\.d\],\$?r13
[	 ]+f4:[	 ]+6029 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+\[\$?r0\]\.d\],\$?r5
[	 ]+f8:[	 ]+6529 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\]\.d\],\$?r13
[	 ]+fc:[	 ]+602d @IM+0558@[ 	]+@OC@\.b \[\$?r2\+\[\$?r0\+\]\.d\],\$?r5
[	 ]+100:[	 ]+652d @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\+\]\.d\],\$?r13
[	 ]+104:[	 ]+652d @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\+\]\.d\],\$?r13
[	 ]+108:[	 ]+0021 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+0\],\$?r5
[	 ]+10c:[	 ]+0121 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+1\],\$?r5
[	 ]+110:[	 ]+7f21 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+127\],\$?r5
[	 ]+114:[	 ]+5f2d 8000 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+128\],\$?r5
[	 ]+11a:[	 ]+ff21 @IM+0558@[ 	]+@OC@\.b \[\$?r2-1\],\$?r5
[	 ]+11e:[	 ]+8121 @IM+0558@[ 	]+@OC@\.b \[\$?r2-127\],\$?r5
[	 ]+122:[	 ]+8021 @IM+0558@[ 	]+@OC@\.b \[\$?r2-128\],\$?r5
[	 ]+126:[	 ]+5f2d ff00 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+255\],\$?r5
[	 ]+12c:[	 ]+2a21 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+42\],\$?r5
[	 ]+130:[	 ]+d621 @IM+0558@[ 	]+@OC@\.b \[\$?r2-42\],\$?r5
[	 ]+134:[	 ]+d621 @IM+0558@[ 	]+@OC@\.b \[\$?r2-42\],\$?r5
[	 ]+138:[	 ]+2a21 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+42\],\$?r5
[	 ]+13c:[	 ]+d621 @IM+0558@[ 	]+@OC@\.b \[\$?r2-42\],\$?r5
[	 ]+140:[	 ]+d621 @IM+0558@[ 	]+@OC@\.b \[\$?r2-42\],\$?r5
[	 ]+144:[	 ]+2a21 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+42\],\$?r5
[	 ]+148:[	 ]+d621 @IM+0558@[ 	]+@OC@\.b \[\$?r2-42\],\$?r5
[	 ]+14c:[	 ]+2a21 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+42\],\$?r5
[	 ]+150:[	 ]+6f2d 0000 0000 @IM+0558@[ 	]+@OC@\.b \[\$?r2\+0( <notstart>)?\],\$?r5
[ 	]+152:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[	 ]+158:[	 ]+0021 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+0\],\$?r13
[	 ]+15c:[	 ]+0121 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+1\],\$?r13
[	 ]+160:[	 ]+7f21 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+127\],\$?r13
[	 ]+164:[	 ]+5f2d 8000 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+128\],\$?r13
[	 ]+16a:[	 ]+ff21 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-1\],\$?r13
[	 ]+16e:[	 ]+ff21 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-1\],\$?r13
[	 ]+172:[	 ]+8121 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-127\],\$?r13
[	 ]+176:[	 ]+8021 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-128\],\$?r13
[	 ]+17a:[	 ]+5f2d 7fff @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-129\],\$?r13
[	 ]+180:[	 ]+8121 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-127\],\$?r13
[	 ]+184:[	 ]+8021 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-128\],\$?r13
[	 ]+188:[	 ]+5f2d 7fff @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-129\],\$?r13
[	 ]+18e:[	 ]+5f2d ff00 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+255\],\$?r13
[	 ]+194:[	 ]+5f2d 01ff @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-255\],\$?r13
[	 ]+19a:[	 ]+5f2d 01ff @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-255\],\$?r13
[	 ]+1a0:[	 ]+5f2d 0001 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+256\],\$?r13
[	 ]+1a6:[	 ]+5f2d 00ff @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-256\],\$?r13
[	 ]+1ac:[	 ]+5f2d 68dd @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-8856\],\$?r13
[	 ]+1b2:[	 ]+5f2d 68dd @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-8856\],\$?r13
[	 ]+1b8:[	 ]+5f2d 9822 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+8856\],\$?r13
[	 ]+1be:[	 ]+2a21 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+42\],\$?r13
[	 ]+1c2:[	 ]+d621 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-42\],\$?r13
[	 ]+1c6:[	 ]+d621 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-42\],\$?r13
[	 ]+1ca:[	 ]+2a21 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+42\],\$?r13
[	 ]+1ce:[	 ]+d621 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-42\],\$?r13
[	 ]+1d2:[	 ]+d621 @IM+1558@[ 	]+@OC@\.w \[\$?r2-42\],\$?r5
[	 ]+1d6:[	 ]+d621 @IM+1558@[ 	]+@OC@\.w \[\$?r2-42\],\$?r5
[	 ]+1da:[	 ]+2a21 @IM+1558@[ 	]+@OC@\.w \[\$?r2\+42\],\$?r5
[	 ]+1de:[	 ]+5f2d ff7f @IM+1558@[ 	]+@OC@\.w \[\$?r2\+32767\],\$?r5
[	 ]+1e4:[	 ]+6f2d 0080 0000 @IM+1558@[ 	]+@OC@\.w \[\$?r2\+(32768|8000 <three2767\+0x1>)\],\$?r5
[	 ]+1ec:[	 ]+6f2d 0180 0000 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2\+(32769|8001 <three2767\+0x2>)\],\$?r13
[	 ]+1f4:[	 ]+5f2d 0180 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-32767\],\$?r13
[	 ]+1fa:[	 ]+5f2d 0080 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-32768\],\$?r13
[	 ]+200:[	 ]+6f2d ff7f ffff @IM+1558@[ 	]+@OC@\.w \[\$?r2\+[^]]+\],\$?r5
[	 ]+208:[	 ]+5f2d 0180 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-32767\],\$?r13
[	 ]+20e:[	 ]+5f2d 0080 @IM+1dd8@[ 	]+@OC@\.w \[\$?r2-32768\],\$?r13
[	 ]+214:[	 ]+6f2d ff7f ffff @IM+1558@[ 	]+@OC@\.w \[\$?r2\+[^]]+\],\$?r5
[	 ]+21c:[	 ]+6f2d ffff 0000 @IM+1558@[ 	]+@OC@\.w \[\$?r2\+(65535|ffff <six5535>)\],\$?r5
[	 ]+224:[	 ]+6f2d 0000 0000 @IM+1558@[ 	]+@OC@\.w \[\$?r2\+0( <notstart>)?\],\$?r5
[ 	]+226:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[	 ]+22c:[	 ]+4205 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+\$?r0\.b\],\$?r5,\$?r8
[	 ]+230:[	 ]+4255 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+\$?r5\.b\],\$?r13,\$?r8
[	 ]+234:[	 ]+4029 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+\[\$?r0\]\.b\],\$?r5,\$?r8
[	 ]+238:[	 ]+4529 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\]\.b\],\$?r13,\$?r8
[	 ]+23c:[	 ]+402d @IM+0858@[ 	]+@OC@\.b \[\$?r2\+\[\$?r0\+\]\.b\],\$?r5,\$?r8
[	 ]+240:[	 ]+452d @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\+\]\.b\],\$?r13,\$?r8
[	 ]+244:[	 ]+452d @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\+\]\.b\],\$?r13,\$?r8
[	 ]+248:[	 ]+5205 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+\$?r0\.w\],\$?r5,\$?r8
[	 ]+24c:[	 ]+5255 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+\$?r5\.w\],\$?r13,\$?r8
[	 ]+250:[	 ]+5029 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+\[\$?r0\]\.w\],\$?r5,\$?r8
[	 ]+254:[	 ]+5529 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\]\.w\],\$?r13,\$?r8
[	 ]+258:[	 ]+502d @IM+0858@[ 	]+@OC@\.b \[\$?r2\+\[\$?r0\+\]\.w\],\$?r5,\$?r8
[	 ]+25c:[	 ]+552d @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\+\]\.w\],\$?r13,\$?r8
[	 ]+260:[	 ]+552d @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\+\]\.w\],\$?r13,\$?r8
[	 ]+264:[	 ]+6205 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+\$?r0\.d\],\$?r5,\$?r8
[	 ]+268:[	 ]+6255 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+\$?r5\.d\],\$?r13,\$?r8
[	 ]+26c:[	 ]+6029 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+\[\$?r0\]\.d\],\$?r5,\$?r8
[	 ]+270:[	 ]+6529 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\]\.d\],\$?r13,\$?r8
[	 ]+274:[	 ]+602d @IM+0858@[ 	]+@OC@\.b \[\$?r2\+\[\$?r0\+\]\.d\],\$?r5,\$?r8
[	 ]+278:[	 ]+652d @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\+\]\.d\],\$?r13,\$?r8
[	 ]+27c:[	 ]+652d @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+\[\$?r5\+\]\.d\],\$?r13,\$?r8
[	 ]+280:[	 ]+0021 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+0\],\$?r5,\$?r8
[	 ]+284:[	 ]+0121 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+1\],\$?r5,\$?r8
[	 ]+288:[	 ]+7f21 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+127\],\$?r5,\$?r8
[	 ]+28c:[	 ]+5f2d 8000 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+128\],\$?r5,\$?r8
[	 ]+292:[	 ]+ff21 @IM+0858@[ 	]+@OC@\.b \[\$?r2-1\],\$?r5,\$?r8
[	 ]+296:[	 ]+8121 @IM+0858@[ 	]+@OC@\.b \[\$?r2-127\],\$?r5,\$?r8
[	 ]+29a:[	 ]+8021 @IM+0858@[ 	]+@OC@\.b \[\$?r2-128\],\$?r5,\$?r8
[	 ]+29e:[	 ]+5f2d ff00 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+255\],\$?r5,\$?r8
[	 ]+2a4:[	 ]+2a21 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+42\],\$?r5,\$?r8
[	 ]+2a8:[	 ]+d621 @IM+0858@[ 	]+@OC@\.b \[\$?r2-42\],\$?r5,\$?r8
[	 ]+2ac:[	 ]+d621 @IM+0858@[ 	]+@OC@\.b \[\$?r2-42\],\$?r5,\$?r8
[	 ]+2b0:[	 ]+2a21 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+42\],\$?r5,\$?r8
[	 ]+2b4:[	 ]+d621 @IM+0858@[ 	]+@OC@\.b \[\$?r2-42\],\$?r5,\$?r8
[	 ]+2b8:[	 ]+d621 @IM+0858@[ 	]+@OC@\.b \[\$?r2-42\],\$?r5,\$?r8
[	 ]+2bc:[	 ]+2a21 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+42\],\$?r5,\$?r8
[	 ]+2c0:[	 ]+d621 @IM+0858@[ 	]+@OC@\.b \[\$?r2-42\],\$?r5,\$?r8
[	 ]+2c4:[	 ]+2a21 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+42\],\$?r5,\$?r8
[	 ]+2c8:[	 ]+6f2d 0000 0000 @IM+0858@[ 	]+@OC@\.b \[\$?r2\+0( <notstart>)?\],\$?r5,\$?r8
[ 	]+2ca:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[	 ]+2d0:[	 ]+0021 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+0\],\$?r13,\$?r8
[	 ]+2d4:[	 ]+0121 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+1\],\$?r13,\$?r8
[	 ]+2d8:[	 ]+7f21 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+127\],\$?r13,\$?r8
[	 ]+2dc:[	 ]+5f2d 8000 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+128\],\$?r13,\$?r8
[	 ]+2e2:[	 ]+ff21 @IM+18d8@[ 	]+@OC@\.w \[\$?r2-1\],\$?r13,\$?r8
[	 ]+2e6:[	 ]+ff21 @IM+18d8@[ 	]+@OC@\.w \[\$?r2-1\],\$?r13,\$?r8
[	 ]+2ea:[	 ]+8121 @IM+18d8@[ 	]+@OC@\.w \[\$?r2-127\],\$?r13,\$?r8
[	 ]+2ee:[	 ]+8021 @IM+18d8@[ 	]+@OC@\.w \[\$?r2-128\],\$?r13,\$?r8
[	 ]+2f2:[	 ]+5f2d 7fff @IM+18d8@[ 	]+@OC@\.w \[\$?r2-129\],\$?r13,\$?r8
[	 ]+2f8:[	 ]+8121 @IM+18d8@[ 	]+@OC@\.w \[\$?r2-127\],\$?r13,\$?r8
[	 ]+2fc:[	 ]+8021 @IM+18d8@[ 	]+@OC@\.w \[\$?r2-128\],\$?r13,\$?r8
[	 ]+300:[	 ]+5f2d 7fff @IM+18d8@[ 	]+@OC@\.w \[\$?r2-129\],\$?r13,\$?r8
[	 ]+306:[	 ]+5f2d ff00 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+255\],\$?r13,\$?r8
[	 ]+30c:[	 ]+5f2d 01ff @IM+18d8@[ 	]+@OC@\.w \[\$?r2-255\],\$?r13,\$?r8
[	 ]+312:[	 ]+5f2d 01ff @IM+18d8@[ 	]+@OC@\.w \[\$?r2-255\],\$?r13,\$?r8
[	 ]+318:[	 ]+5f2d 0001 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+256\],\$?r13,\$?r8
[	 ]+31e:[	 ]+5f2d 00ff @IM+18d8@[ 	]+@OC@\.w \[\$?r2-256\],\$?r13,\$?r8
[	 ]+324:[	 ]+5f2d 68dd @IM+18d8@[ 	]+@OC@\.w \[\$?r2-8856\],\$?r13,\$?r8
[	 ]+32a:[	 ]+5f2d 68dd @IM+18d8@[ 	]+@OC@\.w \[\$?r2-8856\],\$?r13,\$?r8
[	 ]+330:[	 ]+5f2d 9822 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+8856\],\$?r13,\$?r8
[	 ]+336:[	 ]+2a21 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+42\],\$?r13,\$?r8
[	 ]+33a:[	 ]+d621 @IM+18d8@[ 	]+@OC@\.w \[\$?r2-42\],\$?r13,\$?r8
[	 ]+33e:[	 ]+d621 @IM+18d8@[ 	]+@OC@\.w \[\$?r2-42\],\$?r13,\$?r8
[	 ]+342:[	 ]+2a21 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+42\],\$?r13,\$?r8
[	 ]+346:[	 ]+d621 @IM+18d8@[ 	]+@OC@\.w \[\$?r2-42\],\$?r13,\$?r8
[	 ]+34a:[	 ]+d621 @IM+1858@[ 	]+@OC@\.w \[\$?r2-42\],\$?r5,\$?r8
[	 ]+34e:[	 ]+d621 @IM+1858@[ 	]+@OC@\.w \[\$?r2-42\],\$?r5,\$?r8
[	 ]+352:[	 ]+2a21 @IM+1858@[ 	]+@OC@\.w \[\$?r2\+42\],\$?r5,\$?r8
[	 ]+356:[	 ]+5f2d ff7f @IM+1858@[ 	]+@OC@\.w \[\$?r2\+32767\],\$?r5,\$?r8
[	 ]+35c:[	 ]+6f2d 0080 0000 @IM+1858@[ 	]+@OC@\.w \[\$?r2\+(32768|8000 <three2767\+0x1>)\],\$?r5,\$?r8
[	 ]+364:[	 ]+6f2d 0180 0000 @IM+18d8@[ 	]+@OC@\.w \[\$?r2\+(32769|8001 <three2767\+0x2>)\],\$?r13,\$?r8
[	 ]+36c:[	 ]+5f2d 0180 @IM+18d8@[ 	]+@OC@\.w \[\$?r2-32767\],\$?r13,\$?r8
[	 ]+372:[	 ]+5f2d 0080 @IM+18d8@[ 	]+@OC@\.w \[\$?r2-32768\],\$?r13,\$?r8
[	 ]+378:[	 ]+6f2d ff7f ffff @IM+1858@[ 	]+@OC@\.w \[\$?r2\+[^]]+\],\$?r5,\$?r8
[	 ]+380:[	 ]+5f2d 0180 @IM+18d8@[ 	]+@OC@\.w \[\$?r2-32767\],\$?r13,\$?r8
[	 ]+386:[	 ]+5f2d 0080 @IM+18d8@[ 	]+@OC@\.w \[\$?r2-32768\],\$?r13,\$?r8
[	 ]+38c:[	 ]+6f2d ff7f ffff @IM+1858@[ 	]+@OC@\.w \[\$?r2\+[^]]+\],\$?r5,\$?r8
[	 ]+394:[	 ]+6f2d ffff 0000 @IM+1858@[ 	]+@OC@\.w \[\$?r2\+(65535|ffff <six5535>)\],\$?r5,\$?r8
[	 ]+39c:[	 ]+6f2d 0000 0000 @IM+1858@[ 	]+@OC@\.w \[\$?r2\+0( <notstart>)?\],\$?r5,\$?r8
[ 	]+39e:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[	 ]+3a4:[	 ]+4205 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+\$?r0\.b\],\$?r5
[	 ]+3a8:[	 ]+4255 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+\$?r5\.b\],\$?r13
[	 ]+3ac:[	 ]+4029 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+\[\$?r0\]\.b\],\$?r5
[	 ]+3b0:[	 ]+4529 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+\[\$?r5\]\.b\],\$?r13
[	 ]+3b4:[	 ]+402d @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+\[\$?r0\+\]\.b\],\$?r5
[	 ]+3b8:[	 ]+452d @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+\[\$?r5\+\]\.b\],\$?r13
[	 ]+3bc:[	 ]+452d @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+\[\$?r5\+\]\.b\],\$?r13
[	 ]+3c0:[	 ]+5205 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+\$?r0\.w\],\$?r5
[	 ]+3c4:[	 ]+5255 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+\$?r5\.w\],\$?r13
[	 ]+3c8:[	 ]+5029 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+\[\$?r0\]\.w\],\$?r5
[	 ]+3cc:[	 ]+5529 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+\[\$?r5\]\.w\],\$?r13
[	 ]+3d0:[	 ]+502d @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+\[\$?r0\+\]\.w\],\$?r5
[	 ]+3d4:[	 ]+552d @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+\[\$?r5\+\]\.w\],\$?r13
[	 ]+3d8:[	 ]+552d @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+\[\$?r5\+\]\.w\],\$?r13
[	 ]+3dc:[	 ]+6205 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+\$?r0\.d\],\$?r5
[	 ]+3e0:[	 ]+6255 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+\$?r5\.d\],\$?r13
[	 ]+3e4:[	 ]+6029 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+\[\$?r0\]\.d\],\$?r5
[	 ]+3e8:[	 ]+6529 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+\[\$?r5\]\.d\],\$?r13
[	 ]+3ec:[	 ]+602d @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+\[\$?r0\+\]\.d\],\$?r5
[	 ]+3f0:[	 ]+652d @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+\[\$?r5\+\]\.d\],\$?r13
[	 ]+3f4:[	 ]+652d @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+\[\$?r5\+\]\.d\],\$?r13
[	 ]+3f8:[	 ]+0021 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+0\],\$?r5
[	 ]+3fc:[	 ]+0121 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+1\],\$?r5
[	 ]+400:[	 ]+7f21 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+127\],\$?r5
[	 ]+404:[	 ]+5f2d 8000 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+128\],\$?r5
[	 ]+40a:[	 ]+ff21 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2-1\],\$?r5
[	 ]+40e:[	 ]+8121 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2-127\],\$?r5
[	 ]+412:[	 ]+8021 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2-128\],\$?r5
[	 ]+416:[	 ]+5f2d ff00 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+255\],\$?r5
[	 ]+41c:[	 ]+2a21 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+42\],\$?r5
[	 ]+420:[	 ]+d621 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2-42\],\$?r5
[	 ]+424:[	 ]+d621 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2-42\],\$?r5
[	 ]+428:[	 ]+2a21 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+42\],\$?r5
[	 ]+42c:[	 ]+d621 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2-42\],\$?r5
[	 ]+430:[	 ]+d621 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2-42\],\$?r5
[	 ]+434:[	 ]+2a21 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+42\],\$?r5
[	 ]+438:[	 ]+d621 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2-42\],\$?r5
[	 ]+43c:[	 ]+2a21 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+42\],\$?r5
[	 ]+440:[	 ]+6f2d 0000 0000 @IM+0c5c@[ 	]+@OC@\.b \[\$?r12=\$?r2\+0( <notstart>)?\],\$?r5
[ 	]+442:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[	 ]+448:[	 ]+0021 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+0\],\$?r13
[	 ]+44c:[	 ]+0121 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+1\],\$?r13
[	 ]+450:[	 ]+7f21 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+127\],\$?r13
[	 ]+454:[	 ]+5f2d 8000 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+128\],\$?r13
[	 ]+45a:[	 ]+ff21 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-1\],\$?r13
[	 ]+45e:[	 ]+ff21 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-1\],\$?r13
[	 ]+462:[	 ]+8121 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-127\],\$?r13
[	 ]+466:[	 ]+8021 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-128\],\$?r13
[	 ]+46a:[	 ]+5f2d 7fff @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-129\],\$?r13
[	 ]+470:[	 ]+8121 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-127\],\$?r13
[	 ]+474:[	 ]+8021 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-128\],\$?r13
[	 ]+478:[	 ]+5f2d 7fff @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-129\],\$?r13
[	 ]+47e:[	 ]+5f2d ff00 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+255\],\$?r13
[	 ]+484:[	 ]+5f2d 01ff @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-255\],\$?r13
[	 ]+48a:[	 ]+5f2d 01ff @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-255\],\$?r13
[	 ]+490:[	 ]+5f2d 0001 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+256\],\$?r13
[	 ]+496:[	 ]+5f2d 00ff @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-256\],\$?r13
[	 ]+49c:[	 ]+5f2d 68dd @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-8856\],\$?r13
[	 ]+4a2:[	 ]+5f2d 68dd @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-8856\],\$?r13
[	 ]+4a8:[	 ]+5f2d 9822 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+8856\],\$?r13
[	 ]+4ae:[	 ]+2a21 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+42\],\$?r13
[	 ]+4b2:[	 ]+d621 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-42\],\$?r13
[	 ]+4b6:[	 ]+d621 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-42\],\$?r13
[	 ]+4ba:[	 ]+2a21 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+42\],\$?r13
[	 ]+4be:[	 ]+d621 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-42\],\$?r13
[	 ]+4c2:[	 ]+d621 @IM+1c5c@[ 	]+@OC@\.w \[\$?r12=\$?r2-42\],\$?r5
[	 ]+4c6:[	 ]+d621 @IM+1c5c@[ 	]+@OC@\.w \[\$?r12=\$?r2-42\],\$?r5
[	 ]+4ca:[	 ]+2a21 @IM+1c5c@[ 	]+@OC@\.w \[\$?r12=\$?r2\+42\],\$?r5
[	 ]+4ce:[	 ]+5f2d ff7f @IM+1c5c@[ 	]+@OC@\.w \[\$?r12=\$?r2\+32767\],\$?r5
[	 ]+4d4:[	 ]+6f2d 0080 0000 @IM+1c5c@[ 	]+@OC@\.w \[\$?r12=\$?r2\+(32768|8000 <three2767\+0x1>)\],\$?r5
[	 ]+4dc:[	 ]+6f2d 0180 0000 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2\+(32769|8001 <three2767\+0x2>)\],\$?r13
[	 ]+4e4:[	 ]+5f2d 0180 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-32767\],\$?r13
[	 ]+4ea:[	 ]+5f2d 0080 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-32768\],\$?r13
[	 ]+4f0:[	 ]+6f2d ff7f ffff @IM+1c5c@[ 	]+@OC@\.w \[\$?r12=\$?r2\+[^]]+\],\$?r5
[	 ]+4f8:[	 ]+5f2d 0180 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-32767\],\$?r13
[	 ]+4fe:[	 ]+5f2d 0080 @IM+1cdc@[ 	]+@OC@\.w \[\$?r12=\$?r2-32768\],\$?r13
[	 ]+504:[	 ]+6f2d ff7f ffff @IM+1c5c@[ 	]+@OC@\.w \[\$?r12=\$?r2\+[^]]+\],\$?r5
[	 ]+50c:[	 ]+6f2d ffff 0000 @IM+1c5c@[ 	]+@OC@\.w \[\$?r12=\$?r2\+(65535|ffff <six5535>)\],\$?r5
[	 ]+514:[	 ]+6f2d 0000 0000 @IM+1c5c@[ 	]+@OC@\.w \[\$?r12=\$?r2\+0( <notstart>)?\],\$?r5
[ 	]+516:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[	 ]+51c:[	 ]+7309 @IM+0558@[ 	]+@OC@\.b \[\[\$?r3\]\],\$?r5
[	 ]+520:[	 ]+7209 @IM+1448@[ 	]+@OC@\.w \[\[\$?r2\]\],\$?r4
[	 ]+524:[	 ]+790d @IM+0778@[ 	]+@OC@\.b \[\[\$?r9\+\]\],\$?r7
[	 ]+528:[	 ]+730d @IM+1558@[ 	]+@OC@\.w \[\[\$?r3\+\]\],\$?r5
[	 ]+52c:[	 ]+7f0d 0000 0000 @IM+0558@[ 	]+@OC@\.b \[(0x0|0 <notstart>)\],\$?r5
[ 	]+52e:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[	 ]+534:[	 ]+7f0d 0000 0000 @IM+1448@[ 	]+@OC@\.w \[(0x0|0 <notstart>)\],\$?r4
[ 	]+536:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[	 ]+53c:[	 ]+7f0d 0000 0000 @IM+0558@[ 	]+@OC@\.b \[(0x0|0 <notstart>)\],\$?r5
[ 	]+53e:[ 	]+(R_CRIS_)?32[ 	]+\.text
[	 ]+544:[	 ]+7f0d 0000 0000 @IM+1448@[ 	]+@OC@\.w \[(0x0|0 <notstart>)\],\$?r4
[ 	]+546:[ 	]+(R_CRIS_)?32[ 	]+\.text
[	 ]+54c:[	 ]+7309 @IM+0c58@[ 	]+@OC@\.b \[\[\$?r3\]\],\$?r5,\$?r12
[	 ]+550:[	 ]+7209 @IM+1948@[ 	]+@OC@\.w \[\[\$?r2\]\],\$?r4,\$?r9
[	 ]+554:[	 ]+790d @IM+0a78@[ 	]+@OC@\.b \[\[\$?r9\+\]\],\$?r7,\$?r10
[	 ]+558:[	 ]+730d @IM+1958@[ 	]+@OC@\.w \[\[\$?r3\+\]\],\$?r5,\$?r9
[	 ]+55c:[	 ]+7f0d 0000 0000 @IM+0758@[ 	]+@OC@\.b \[(0x0|0 <notstart>)\],\$?r5,\$?r7
[ 	]+55e:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[	 ]+564:[	 ]+7f0d 0000 0000 @IM+1948@[ 	]+@OC@\.w \[(0x0|0 <notstart>)\],\$?r4,\$?r9
[ 	]+566:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[	 ]+56c:[	 ]+7f0d 0000 0000 @IM+0958@[ 	]+@OC@\.b \[(0x0|0 <notstart>)\],\$?r5,\$?r9
[ 	]+56e:[ 	]+(R_CRIS_)?32[ 	]+\.text
[	 ]+574:[	 ]+7f0d 0000 0000 @IM+1c48@[ 	]+@OC@\.w \[(0x0|0 <notstart>)\],\$?r4,\$?r12
[ 	]+576:[ 	]+(R_CRIS_)?32[ 	]+\.text
