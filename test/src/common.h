#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

/* using in TDEforPG test function */

#define ENCRYPT_BYTEA_TEST 0
#define ENCRYPT_TEXT_TEST 1
#define ENCRYPT_NUMERIC_TEST 2
#define ENCRYPT_TIMESTAMP_TEST 3
#define ENCRYPT_INTEGER_TEST 4

/* kind of data test */
#define BASIC_DATA 0
#define NULL_DATA 1
#define TOAST_DATA 2

/***********************************/


extern SQLHENV env;
extern SQLHDBC conn;

#define CHECK_STMT_RESULT(rc, msg, hstmt)	\
	if (!SQL_SUCCEEDED(rc)) \
	{ \
		print_diag(msg, SQL_HANDLE_STMT, hstmt);	\
		exit(1);									\
    }

#define CHECK_CONN_RESULT(rc, msg, hconn)	\
	if (!SQL_SUCCEEDED(rc)) \
	{ \
		print_diag(msg, SQL_HANDLE_DBC, hconn);	\
		exit(1);									\
    }

extern void print_diag(char *msg, SQLSMALLINT htype, SQLHANDLE handle);
extern const char *get_test_dsn(void);
extern int  IsAnsi(void);
extern void test_connect_ext(char *extraparams);
extern void test_connect(void);
extern void test_disconnect(void);
extern void print_result_meta_series(HSTMT hstmt,
									 SQLSMALLINT *colids,
									 SQLSMALLINT numcols);
extern void print_result_series(HSTMT hstmt,
								SQLSMALLINT *colids,
								SQLSMALLINT numcols,
								SQLINTEGER rowcount);
extern void print_result_meta(HSTMT hstmt);
extern void print_result(HSTMT hstmt);
extern const char *datatype_str(SQLSMALLINT datatype);
extern const char *nullable_str(SQLSMALLINT nullable);
void tdeforpg_crud(SQLSMALLINT datatype_patern, SQLSMALLINT data_kind);
void pgtde_begin_session(HSTMT hstmt);
void test_execute_sql_and_print(SQLCHAR* sql);
