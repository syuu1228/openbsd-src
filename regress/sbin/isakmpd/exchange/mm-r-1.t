# $OpenBSD: src/regress/sbin/isakmpd/exchange/mm-r-1.t,v 1.1 2005/04/08 17:12:49 cloder Exp $
# $EOM: mm-r-1.t,v 1.1 1999/08/05 15:07:38 niklas Exp $

# The seed to isakmpd was 19990805

# Respond to this MM
999999	recv H*	ad9de636 f12460bb 00000000 00000000 01100200 00000000 \
		00000050 00000034 00000001 00000001 00000028 01010001 \
		00000020 00010000 80010005 80020002 80030001 80040002 \
		800b0001 800c0258

40	send H*	ad9de636 f12460bb 2aa5a583 ab2f3980 01100200 00000000 \
		00000050 00000034 00000001 00000001 00000028 01010001 \
		00000020 00010000 80010005 80020002 80030001 80040002 \
		800b0001 800c0258

1100	recv H*	ad9de636 f12460bb 2aa5a583 ab2f3980 04100200 00000000 \
		000000b4 0a000084 60a8c102 ce97687e 45e3fdd9 6fd586b4 \
		f3a91167 559dd214 a78d678e 2772b7b2 83267487 15ec02a9 \
		419b77ee 0f2add09 e9e09b7d ad40c883 ef2039c9 c59b67ff \
		56e4d6f8 c99d47cb d4a565bc 8d192f76 f695d243 09121df5 \
		524884a7 3f702630 7f4fad44 e222c4b1 242fd1cd ca3a161d \
		bcdf6706 025cc90d c4b00ef9 bee5ada2 00000014 ff7c493c \
		88e68a10 4ab19a6a 7e75c771

80	send H*	ad9de636 f12460bb 2aa5a583 ab2f3980 04100200 00000000 \
		000000b4 0a000084 681b9859 7680a3ff 894bb982 ef924bc8 \
		4d9c7ebf 3a92db7b bcfe68f7 6e1de327 a975293f f5c550b1 \
		9c69d6ed 64f201ec 514f4f44 0e6242b9 df4917e6 4418212d \
		66a66eb1 f3b70c2d 4e14e382 d42ebe04 1027957c 5dadcaf1 \
		a531c085 6cee739f 433c185c 12a8a946 88622f66 f211783c \
		277e134d 22d8e809 f1d38bab 6ca2a35f 00000014 6a917048 \
		a406fd47 b3d16554 cd6f0967

1400	recv H*	ad9de636 f12460bb 2aa5a583 ab2f3980 05100201 00000000 \
		0000005c d6571dec a8b55acb 1069210c 7d195417 1c2738e9 \
		42f1d9a3 8561d0ec 0697cd06 ac1beb19 1dc8acf5 904ec1d5 \
		5b2b154e 38b0de90 4f2e1f71 083047ca 10cab3d5

90	send H*	ad9de636 f12460bb 2aa5a583 ab2f3980 05100201 00000000 \
		00000044 b46b1db4 9ecfbfa6 a5e9baa2 8eb3cb68 be3a857c \
		b039fa72 d595e69f 03669dbd 350781e2 56c36dce
