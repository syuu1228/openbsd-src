int ex __P((SCR **));
int ex_cmd __P((SCR *));
int ex_range __P((SCR *, EXCMD *, int *));
int ex_is_abbrev __P((char *, size_t));
int ex_is_unmap __P((char *, size_t));
void ex_badaddr
   __P((SCR *, EXCMDLIST const *, enum badaddr, enum nresult));
int ex_abbr __P((SCR *, EXCMD *));
int ex_unabbr __P((SCR *, EXCMD *));
int ex_append __P((SCR *, EXCMD *));
int ex_change __P((SCR *, EXCMD *));
int ex_insert __P((SCR *, EXCMD *));
int ex_next __P((SCR *, EXCMD *));
int ex_prev __P((SCR *, EXCMD *));
int ex_rew __P((SCR *, EXCMD *));
int ex_args __P((SCR *, EXCMD *));
char **ex_buildargv __P((SCR *, EXCMD *, char *));
int argv_init __P((SCR *, EXCMD *));
int argv_exp0 __P((SCR *, EXCMD *, char *, size_t));
int argv_exp1 __P((SCR *, EXCMD *, char *, size_t, int));
int argv_exp2 __P((SCR *, EXCMD *, char *, size_t));
int argv_exp3 __P((SCR *, EXCMD *, char *, size_t));
int argv_free __P((SCR *));
int ex_at __P((SCR *, EXCMD *));
int ex_bang __P((SCR *, EXCMD *));
int ex_cd __P((SCR *, EXCMD *));
int ex_cscope __P((SCR *, EXCMD *));
int cscope_display __P((SCR *));
int cscope_search __P((SCR *, TAGQ *, TAG *));
int ex_delete __P((SCR *, EXCMD *));
int ex_display __P((SCR *, EXCMD *));
int ex_edit __P((SCR *, EXCMD *));
int ex_equal __P((SCR *, EXCMD *));
int ex_file __P((SCR *, EXCMD *));
int ex_filter __P((SCR *, 
   EXCMD *, MARK *, MARK *, MARK *, char *, enum filtertype));
int ex_global __P((SCR *, EXCMD *));
int ex_v __P((SCR *, EXCMD *));
int ex_g_insdel __P((SCR *, lnop_t, recno_t));
int ex_screen_copy __P((SCR *, SCR *));
int ex_screen_end __P((SCR *));
int ex_optchange __P((SCR *, int, char *, u_long *));
int ex_exrc __P((SCR *));
int ex_run_str __P((SCR *, char *, char *, size_t, int, int));
int ex_join __P((SCR *, EXCMD *));
int ex_map __P((SCR *, EXCMD *));
int ex_unmap __P((SCR *, EXCMD *));
int ex_mark __P((SCR *, EXCMD *));
int ex_mkexrc __P((SCR *, EXCMD *));
int ex_copy __P((SCR *, EXCMD *));
int ex_move __P((SCR *, EXCMD *));
int ex_open __P((SCR *, EXCMD *));
int ex_perl __P((SCR*, EXCMD *));
int ex_preserve __P((SCR *, EXCMD *));
int ex_recover __P((SCR *, EXCMD *));
int ex_list __P((SCR *, EXCMD *));
int ex_number __P((SCR *, EXCMD *));
int ex_pr __P((SCR *, EXCMD *));
int ex_print __P((SCR *, EXCMD *, MARK *, MARK *, u_int32_t));
int ex_ldisplay __P((SCR *, const char *, size_t, size_t, u_int));
int ex_scprint __P((SCR *, MARK *, MARK *));
int ex_printf __P((SCR *, const char *, ...));
int ex_puts __P((SCR *, const char *));
int ex_fflush __P((SCR *sp));
int ex_put __P((SCR *, EXCMD *));
int ex_quit __P((SCR *, EXCMD *));
int ex_read __P((SCR *, EXCMD *));
int ex_readfp __P((SCR *, char *, FILE *, MARK *, recno_t *, int));
int ex_bg __P((SCR *, EXCMD *));
int ex_fg __P((SCR *, EXCMD *));
int ex_resize __P((SCR *, EXCMD *));
int ex_sdisplay __P((SCR *));
int ex_script __P((SCR *, EXCMD *));
int sscr_exec __P((SCR *, recno_t));
int sscr_input __P((SCR *));
int sscr_end __P((SCR *));
int ex_set __P((SCR *, EXCMD *));
int ex_shell __P((SCR *, EXCMD *));
int ex_exec_proc __P((SCR *, EXCMD *, char *, const char *, int));
int proc_wait __P((SCR *, long, const char *, int, int));
int ex_shiftl __P((SCR *, EXCMD *));
int ex_shiftr __P((SCR *, EXCMD *));
int ex_source __P((SCR *, EXCMD *));
int ex_stop __P((SCR *, EXCMD *));
int ex_s __P((SCR *, EXCMD *));
int ex_subagain __P((SCR *, EXCMD *));
int ex_subtilde __P((SCR *, EXCMD *));
int re_compile __P((SCR *,
    char *, size_t, char **, size_t *, regex_t *, u_int));
void re_error __P((SCR *, int, regex_t *));
int ex_tag_first __P((SCR *, char *));
int ex_tag_push __P((SCR *, EXCMD *));
int ex_tag_next __P((SCR *, EXCMD *));
int ex_tag_prev __P((SCR *, EXCMD *));
int ex_tag_nswitch __P((SCR *, TAG *, int));
int ex_tag_Nswitch __P((SCR *, TAG *, int));
int ex_tag_pop __P((SCR *, EXCMD *));
int ex_tag_top __P((SCR *, EXCMD *));
int ex_tag_display __P((SCR *));
int ex_tag_copy __P((SCR *, SCR *));
int tagq_free __P((SCR *, TAGQ *));
void tag_msg __P((SCR *, tagmsg_t, char *));
int ex_tagf_alloc __P((SCR *, char *));
int ex_tag_free __P((SCR *));
int ex_tcl __P((SCR*, EXCMD *));
int ex_txt __P((SCR *, TEXTH *, ARG_CHAR_T, u_int32_t));
int ex_undo __P((SCR *, EXCMD *));
int ex_help __P((SCR *, EXCMD *));
int ex_usage __P((SCR *, EXCMD *));
int ex_viusage __P((SCR *, EXCMD *));
void ex_cinit __P((EXCMD *,
   int, int, recno_t, recno_t, int, ARGS **));
void ex_cadd __P((EXCMD *, ARGS *, char *, size_t));
int ex_getline __P((SCR *, FILE *, size_t *));
int ex_ncheck __P((SCR *, int));
int ex_init __P((SCR *));
void ex_emsg __P((SCR *, char *, exm_t));
int ex_version __P((SCR *, EXCMD *));
int ex_visual __P((SCR *, EXCMD *));
int ex_wn __P((SCR *, EXCMD *));
int ex_wq __P((SCR *, EXCMD *));
int ex_write __P((SCR *, EXCMD *));
int ex_xit __P((SCR *, EXCMD *));
int ex_writefp __P((SCR *,
   char *, FILE *, MARK *, MARK *, u_long *, u_long *, int));
int ex_yank __P((SCR *, EXCMD *));
int ex_z __P((SCR *, EXCMD *));
