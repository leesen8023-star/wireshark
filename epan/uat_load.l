%top {
/* Include this before everything else, for various large-file definitions */
#include "config.h"
#define WS_LOG_DOMAIN LOG_DOMAIN_UAT

#include <wireshark.h>

/* #define DEBUG_UAT_LOAD 1 */
}

/*
 * We want a reentrant scanner.
 */
%option reentrant

/*
 * We don't use input, so don't generate code for it.
 */
%option noinput

/*
 * We don't use unput, so don't generate code for it.
 */
%option nounput

/*
 * We don't read interactively from the terminal.
 */
%option never-interactive

/*
 * We want to stop processing when we get to the end of the input.
 */
%option noyywrap

/*
 * The type for the state we keep for a scanner.
 */
%option extra-type="uat_load_scanner_state_t *"

/*
 * We have to override the memory allocators so that we don't get
 * "unused argument" warnings from the yyscanner argument (which
 * we don't use, as we have a global memory allocator).
 *
 * We provide, as macros, our own versions of the routines generated by Flex,
 * which just call malloc()/realloc()/free() (as the Flex versions do),
 * discarding the extra argument.
 */
%option noyyalloc
%option noyyrealloc
%option noyyfree

/*
 * Prefix scanner routines with "uat_load_" rather than "yy", so this scanner
 * can coexist with other scanners.
 */
%option prefix="uat_load_"

%{
	/*
	 * uat_load.l
	 *
	 *  User Accessible Tables
	 *  Maintain an array of user accessible data strucures
	 *  One parser to fit them all
	 *
	 * (c) 2007, Luis E. Garcia Ontanon <luis@ontanon.org>
	 *
	 * Wireshark - Network traffic analyzer
	 * By Gerald Combs <gerald@wireshark.org>
	 * Copyright 2001 Gerald Combs
	 *
	 * SPDX-License-Identifier: GPL-2.0-or-later
	 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <glib.h>

#include "uat-int.h"
#include <wsutil/file_util.h>

/*
 * Disable diagnostics in the code generated by Flex.
 */
DIAG_OFF_FLEX()

typedef struct {
	uat_t* uat;
	char *parse_str;

	char* error;
	bool valid_record;
	unsigned colnum;
	char* ptrx;
	unsigned len;
	void* record;
	unsigned linenum;
	size_t parse_str_pos;
} uat_load_scanner_state_t;

/*
 * Signal a fatal error and stops parsing.
 * Since the record is internal to the parsing process, its contents must also
 * be cleared before terminating. Any values that are not handled yet (ptrx)
 * must also be freed.
 */
#define ERROR(fmtd) do { \
	char* fmt_str = ws_strdup_printf fmtd; \
	g_free(yyextra->error); \
	yyextra->error = ws_strdup_printf("%s:%d: %s",yyextra->uat->filename,yyextra->linenum,fmt_str); \
	g_free(fmt_str); \
	if (yyextra->uat->free_cb) { \
		yyextra->uat->free_cb(yyextra->record); \
	} \
	g_free(yyextra->ptrx); \
	yyterminate(); \
} while(0)

/*
 * Sets the field of the current (scanner-internal) record, using the parsed
 * value. If the field validation function exists and returns an error, then
 * the record is marked as invalid and an error message is stored such that it
 * can be shown after parsing. (If other errors occur after this issue, then
 * this message will be overwritten though.)
 */
#define SET_FIELD() \
	{ char* errx; \
	if (yyextra->uat->fields[yyextra->colnum].cb.chk) { \
		if ( ! yyextra->uat->fields[yyextra->colnum].cb.chk(yyextra->record, yyextra->ptrx, yyextra->len, yyextra->uat->fields[yyextra->colnum].cbdata.chk, yyextra->uat->fields[yyextra->colnum].fld_data, &errx) ) { \
			g_free(yyextra->error); \
			yyextra->error = ws_strdup_printf("%s:%d: %s",yyextra->uat->filename,yyextra->linenum,errx); \
			g_free(errx); \
			yyextra->valid_record = false; \
		}\
	}\
	yyextra->uat->fields[yyextra->colnum].cb.set(yyextra->record, yyextra->ptrx, yyextra->len, yyextra->uat->fields[yyextra->colnum].cbdata.set, yyextra->uat->fields[yyextra->colnum].fld_data);\
	g_free(yyextra->ptrx);\
	yyextra->ptrx = NULL;\
	yyextra->colnum++; \
	} while(0)

#ifdef DEBUG_UAT_LOAD
#define DUMP_FIELD(str) \
		{ unsigned i; printf("%s: %s='",str,yyextra->uat->fields[yyextra->colnum].name); for(i=0;i<yyextra->len;i++) if (yyextra->uat->fields[yyextra->colnum].mode == PT_TXTMOD_HEXBYTES) { printf("%.2x ",((uint8_t*)yyextra->ptrx)[i]); } else putc(yyextra->ptrx[i],stdout); printf("'[%d]\n",yyextra->len); }

#define DUMP(str) printf("%s\n",str)
#else
#define DUMP_FIELD(s)
#define DUMP(s)
#endif

/* Modified version of YY_INPUT generated by Flex 2.91 */
#define YY_INPUT(buf,result,max_size) \
	if ( yyextra->parse_str ) \
		{ \
		size_t n = 0; \
		size_t pslen = strlen(yyextra->parse_str); \
		if (yyextra->parse_str_pos < pslen) \
			{ \
			n = pslen - yyextra->parse_str_pos; \
			if (n > max_size) n = max_size; \
			memcpy(buf, yyextra->parse_str + yyextra->parse_str_pos, n); \
			yyextra->parse_str_pos += n; \
			} \
		result = n; \
		} \
	else \
		{ \
		errno=0; \
		while ( (result = fread(buf, 1, max_size, yyin))==0 && ferror(yyin)) \
			{ \
			if( errno != EINTR) \
				{ \
				YY_FATAL_ERROR( "input in flex scanner failed" ); \
				break; \
				} \
			errno=0; \
			clearerr(yyin); \
			} \
		}

		/*
		 * XXX
		 * quoted_string below fails badly on "...\\"
		 * workarround in uat_save(), using /x5c and /x22
		 */

#define YY_USER_INIT BEGIN START_OF_LINE;

/*
 * Sleazy hack to suppress compiler warnings in yy_fatal_error().
 */
#define YY_EXIT_FAILURE ((void)yyscanner, 2)

/*
 * Macros for the allocators, to discard the extra argument.
 */
#define uat_load_alloc(size, yyscanner)		(void *)malloc(size)
#define uat_load_realloc(ptr, size, yyscanner)	(void *)realloc((char *)(ptr), (size))
#define uat_load_free(ptr, yyscanner)		free((char *)ptr)

%}

quoted_string \042([^\042]|\134\042)*\042
extra_records_string (\042.*\042)
binstring ([0-9a-zA-Z][0-9a-zA-Z])*
separator [ \t]*,
newline [ \t]*[\r]?\n
ws [ \t]+
comment #[^\n]*\n

%x START_OF_LINE NEXT_FIELD SEPARATOR END_OF_RECORD ERRORED
%%
<START_OF_LINE,NEXT_FIELD>{ws} ;
<START_OF_LINE>{newline} yyextra->linenum++;
<START_OF_LINE>{comment} yyextra->linenum++;

<START_OF_LINE,NEXT_FIELD>{separator} {
	yyextra->ptrx = g_strdup("");
	yyextra->len = 0;

	DUMP_FIELD("empty->next");

	SET_FIELD();

	if ( yyextra->colnum >= yyextra->uat->ncols ) {
		ERROR(("more fields than required"));
	}

	BEGIN NEXT_FIELD;
}

<START_OF_LINE,NEXT_FIELD>{newline} {
	yyextra->ptrx = g_strdup("");
	yyextra->len = 0;

	BEGIN END_OF_RECORD;

	yyless((int) yyleng);
}

<START_OF_LINE,NEXT_FIELD>{quoted_string} {
	yyextra->ptrx = uat_undquote(yytext, (unsigned) yyleng, &yyextra->len);


	if (yyextra->colnum < yyextra->uat->ncols - 1) {
		DUMP("quoted_str->s");
		BEGIN SEPARATOR;
	} else {
		DUMP("quoted_str->eor");
		BEGIN END_OF_RECORD;
	}
}

<START_OF_LINE,NEXT_FIELD>{binstring} {
	yyextra->ptrx = uat_unbinstring(yytext,  (unsigned) yyleng, &yyextra->len);

	if (!yyextra->ptrx) {
		ERROR(("uneven hexstring for field %s",yyextra->uat->fields[yyextra->colnum].name));
	}

	if ( yyextra->colnum < yyextra->uat->ncols - 1 ) {
		DUMP("binstring->s");
		BEGIN SEPARATOR;
	} else {
		DUMP("binstring->eor");
		BEGIN END_OF_RECORD;
	}
}

<SEPARATOR>{separator} {

	DUMP_FIELD("separator->next");

	SET_FIELD();

	if ( yyextra->colnum >= yyextra->uat->ncols ) {
		ERROR(("more fields than required"));
	}

	BEGIN NEXT_FIELD;
}

<SEPARATOR>{newline} {
	DUMP("separator->newline");

	/*
	 * We've run out of fields. Check to see if we have any default
	 * values that we can fill in. The field for the current column
	 * will be filled in below in <END_OF_RECORD>{newline}.
	 */
	unsigned save_colnum = yyextra->colnum;
	yyextra->colnum++;
	while (yyextra->colnum < yyextra->uat->ncols) {
		if (!yyextra->uat->default_values) {
			break;
		}
		const char *default_value = yyextra->uat->default_values[yyextra->colnum];
		if (!default_value) {
			break;
		}
		yyextra->uat->fields[yyextra->colnum].cb.set(yyextra->record, default_value, (unsigned) strlen(default_value), yyextra->uat->fields[yyextra->colnum].cbdata.set, yyextra->uat->fields[yyextra->colnum].fld_data);
		ws_log(WS_LOG_DOMAIN, LOG_LEVEL_INFO, "%s:%d: Set %s to %s.",
			yyextra->uat->filename, yyextra->linenum, yyextra->uat->fields[yyextra->colnum].name, default_value);
		yyextra->colnum++;
	}

	if (yyextra->colnum < yyextra->uat->ncols) {
		ERROR(("expecting field %s", yyextra->uat->fields[yyextra->colnum].name));
	}

	yyextra->colnum = save_colnum;
	yyextra->linenum++;
	BEGIN END_OF_RECORD;
	yyless(0);
}

<SEPARATOR>. {
	ERROR(("unexpected char '%s' while looking for field %s",yytext,yyextra->uat->fields[yyextra->colnum].name));
}

<END_OF_RECORD>{separator}{extra_records_string} {
	/* If we wanted to be really fancy we could retain the extra data. */
	ws_log(WS_LOG_DOMAIN, LOG_LEVEL_WARNING, "%s:%d: More fields than required. Discarding '%s'.",
		yyextra->uat->filename, yyextra->linenum, yytext);
}

<END_OF_RECORD>{newline} {
	void* rec;
	char* err = NULL;

	yyextra->linenum++;

	DUMP_FIELD("newline->start");

	SET_FIELD();

	/* Last field was processed, try to store the full record in the UAT. */
	rec = uat_add_record(yyextra->uat, yyextra->record, yyextra->valid_record);

	if ((yyextra->uat->update_cb) && (rec != NULL)) {
		if (!yyextra->uat->update_cb(rec,&err)) {
			g_free(yyextra->error);
			yyextra->error = err;
			yyterminate();
		}
	}

	/* The record was duplicated to the UAT above, now free our fields. */
	if (yyextra->uat->free_cb) {
		yyextra->uat->free_cb(yyextra->record);
	}
	memset(yyextra->record, 0, yyextra->uat->record_size);

	yyextra->valid_record = true;
	yyextra->colnum = 0;
	yyextra->ptrx = NULL;
	yyextra->len = 0;

	BEGIN START_OF_LINE;
 }

<END_OF_RECORD>. {
	ERROR(("unexpected char %s while looking for end of line", yytext));
}

<ERRORED>{newline} { yyextra->linenum++; BEGIN START_OF_LINE; }
<ERRORED>. ;

{newline} { yyextra->linenum++; ERROR(("incomplete record")); }
. { ERROR(("unexpected input")); }

%%

/*
 * Turn diagnostics back on, so we check the code that we've written.
 */
DIAG_ON_FLEX()

bool
uat_load(uat_t *uat, const char *filename, char **errx)
{
	char *fname;
	FILE *in;
	yyscan_t scanner;
	uat_load_scanner_state_t state;

	if (filename) {
		fname = g_strdup(filename);
	} else {
		fname = uat_get_actual_filename(uat, false);
	}

	if (!fname) {
		UAT_UPDATE(uat);

		if (uat->post_update_cb)
			uat->post_update_cb();

		return true;
	}


	if (!(in = ws_fopen(fname,"r"))) {
		*errx = g_strdup(g_strerror(errno));
		g_free(fname);
		return false;
	}

	if (uat_load_lex_init(&scanner) != 0) {
		*errx = g_strdup(g_strerror(errno));
		fclose(in);
		g_free(fname);
		return false;
	}

	uat_load_set_in(in, scanner);

	state.uat = uat;
	state.parse_str = NULL;	/* we're reading from a file */

	state.error = NULL;
	state.valid_record = true;
	state.colnum = 0;
	state.ptrx = NULL;
	state.len = 0;
	state.record = g_malloc0(uat->record_size);
	state.linenum = 1;
	state.parse_str_pos = 0;

	DUMP(fname);
	g_free(fname);	/* we're done with the file name now */

	/* Associate the state with the scanner */
	uat_load_set_extra(&state, scanner);

	uat_load_lex(scanner);

	uat_load_lex_destroy(scanner);
	g_free(state.record);
	fclose(in);

	uat->changed = false;
	uat->loaded = true;
	UAT_UPDATE(uat);

	if (state.error) {
		*errx = state.error;
		return false;
	}

	if (uat->post_update_cb)
		uat->post_update_cb();

	*errx = NULL;
	return true;
}

bool
uat_load_str(uat_t *uat, const char *entry, char **err)
{
	yyscan_t scanner;
	uat_load_scanner_state_t state;

	state.uat = uat;
	state.parse_str = ws_strdup_printf("%s\n", entry); /* Records must end with a newline */

	state.error = NULL;
	state.valid_record = true;
	state.colnum = 0;
	state.ptrx = NULL;
	state.len = 0;
	state.record = g_malloc0(uat->record_size);
	state.linenum = 1;
	state.parse_str_pos = 0;

	if (uat_load_lex_init(&scanner) != 0) {
		*err = g_strdup(g_strerror(errno));
		g_free(state.parse_str);
		g_free(state.record);
		return false;
	}

	DUMP(entry);

	/* Associate the state with the scanner */
	uat_load_set_extra(&state, scanner);

	uat_load_lex(scanner);

	uat_load_lex_destroy(scanner);
	g_free(state.record);
	g_free(state.parse_str);

	uat->changed = true;
	uat->loaded = true;
	UAT_UPDATE(uat);

	if (state.error) {
		*err = state.error;
		return false;
	}

	if (uat->post_update_cb)
		uat->post_update_cb();

	*err = NULL;
	return true;
}
