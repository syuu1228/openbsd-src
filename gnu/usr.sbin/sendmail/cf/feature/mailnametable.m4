divert(-1)
#
# Simple feature to do username rewriting (converse of an alias)
#

divert(0)
VERSIONID(`$OpenBSD: src/gnu/usr.sbin/sendmail/cf/feature/Attic/mailnametable.m4,v 1.2 2001/01/15 21:08:55 millert Exp $')
divert(-1)

define(`MAILNAME_TABLE', ifelse(_ARG_, `', DATABASE_MAP_TYPE `-o' MAIL_SETTINGS_DIR`mailnames', `_ARG_'))
LOCAL_CONFIG
ifdef(`MAILNAME_TABLE', `Kmailnames' MAILNAME_TABLE)
PUSHDIVERT(1)
LOCAL_RULE_1
ifdef(`MAILNAME_TABLE',
R$-				$: $>3 $(mailnames $1 $)
R$- < @ $=w . >			$: $>3 $(mailnames $1 $)
)
POPDIVERT
