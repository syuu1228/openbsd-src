typedef union {long itype; tree ttype; char *strtype; enum tree_code code; flagged_type_tree ftype; } YYSTYPE;
#define	IDENTIFIER	258
#define	TYPENAME	259
#define	SELFNAME	260
#define	PFUNCNAME	261
#define	SCSPEC	262
#define	TYPESPEC	263
#define	CV_QUALIFIER	264
#define	CONSTANT	265
#define	STRING	266
#define	ELLIPSIS	267
#define	SIZEOF	268
#define	ENUM	269
#define	IF	270
#define	ELSE	271
#define	WHILE	272
#define	DO	273
#define	FOR	274
#define	SWITCH	275
#define	CASE	276
#define	DEFAULT	277
#define	BREAK	278
#define	CONTINUE	279
#define	RETURN	280
#define	GOTO	281
#define	ASM_KEYWORD	282
#define	GCC_ASM_KEYWORD	283
#define	TYPEOF	284
#define	ALIGNOF	285
#define	SIGOF	286
#define	ATTRIBUTE	287
#define	EXTENSION	288
#define	LABEL	289
#define	REALPART	290
#define	IMAGPART	291
#define	AGGR	292
#define	VISSPEC	293
#define	DELETE	294
#define	NEW	295
#define	THIS	296
#define	OPERATOR	297
#define	CXX_TRUE	298
#define	CXX_FALSE	299
#define	NAMESPACE	300
#define	TYPENAME_KEYWORD	301
#define	USING	302
#define	LEFT_RIGHT	303
#define	TEMPLATE	304
#define	TYPEID	305
#define	DYNAMIC_CAST	306
#define	STATIC_CAST	307
#define	REINTERPRET_CAST	308
#define	CONST_CAST	309
#define	SCOPE	310
#define	EMPTY	311
#define	PTYPENAME	312
#define	NSNAME	313
#define	THROW	314
#define	ASSIGN	315
#define	OROR	316
#define	ANDAND	317
#define	MIN_MAX	318
#define	EQCOMPARE	319
#define	ARITHCOMPARE	320
#define	LSHIFT	321
#define	RSHIFT	322
#define	POINTSAT_STAR	323
#define	DOT_STAR	324
#define	UNARY	325
#define	PLUSPLUS	326
#define	MINUSMINUS	327
#define	HYPERUNARY	328
#define	PAREN_STAR_PAREN	329
#define	POINTSAT	330
#define	TRY	331
#define	CATCH	332
#define	TYPENAME_ELLIPSIS	333
#define	PRE_PARSED_FUNCTION_DECL	334
#define	EXTERN_LANG_STRING	335
#define	ALL	336
#define	PRE_PARSED_CLASS_DECL	337
#define	DEFARG	338
#define	DEFARG_MARKER	339
#define	TYPENAME_DEFN	340
#define	IDENTIFIER_DEFN	341
#define	PTYPENAME_DEFN	342
#define	END_OF_LINE	343
#define	END_OF_SAVED_INPUT	344


extern YYSTYPE yylval;
#define YYEMPTY		-2
