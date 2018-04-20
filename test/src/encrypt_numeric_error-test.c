/*
* Test ERROR-disable_log ENCRYPT_NUMERIC_TEST.
 */
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

static unsigned char
hex_to_int(char c)
{
	char		result;

	if (c >= '0' && c <= '9')
		result = c - '0';
	else if (c >= 'a' && c <= 'f')
		result = c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		result = c - 'A' + 10;
	else
	{
		fprintf(stderr, "invalid hex-encoded numeric value\n");
		exit(1);
	}
	return (unsigned char) result;
}

static void
build_numeric_struct(SQL_NUMERIC_STRUCT *numericparam,
					 unsigned char sign, char *hexstr,
					 unsigned char precision, unsigned char scale)
{
	int			len;

	/* parse the hex-encoded value */
	memset(numericparam, 0, sizeof(SQL_NUMERIC_STRUCT));

	numericparam->sign = sign;
	numericparam->precision = precision;
	numericparam->scale = scale;

	len = 0;
	while (*hexstr)
	{
		if (*hexstr == ' ')
		{
			hexstr++;
			continue;
		}
		if (len >= SQL_MAX_NUMERIC_LEN)
		{
			fprintf(stderr, "hex-encoded numeric value too long\n");
			exit(1);
		}
		numericparam->val[len] =
			hex_to_int(*hexstr) << 4 | hex_to_int(*(hexstr + 1));
		hexstr += 2;
		len++;
	}
}

int main(int argc, char **argv)
{
	SQLRETURN	rc;
	HSTMT		hstmt = SQL_NULL_HSTMT;
	SQL_NUMERIC_STRUCT numericparam;
	SQLLEN		cbParam1, cbParam2;
	SQLCHAR *sql;

	test_connect();

	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("failed to allocate stmt handle", SQL_HANDLE_DBC, conn);
		exit(1);
	}

	printf("\n====== diff_DATA ==============");

	/* INSERT_VARCHAR */
	printf("\nINSERT_VARCHAR\n");

	/* cipher_key disable */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT cipher_key_disable_log()", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while cipher_key disable", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* pgtde begin session */
	sql = "SELECT pgtde_begin_session('aofiafoaeiaofijabeoptuiotaoi')";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while pgtde begin session", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* INSERT INTO pre_enc_num */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_num(c1) VALUES(?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);
	
		/* 24197857161011715162171839636988778104 */
		build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
		
		cbParam1 = sizeof(numericparam);
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_CHAR,	/* value type */
							  SQL_CHAR,	/* param type */
							  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
							  0,			/* dec digits */
							  &numericparam, /* param value ptr */
							  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
							  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
		CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* INSERT INTO pre_enc_num */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_num(c11) VALUES(?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);
	
		/* 24197857161011715162171839636988778104 */
		build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
		
		cbParam1 = sizeof(numericparam);
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_CHAR,	/* value type */
							  SQL_CHAR,	/* param type */
							  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
							  0,			/* dec digits */
							  &numericparam, /* param value ptr */
							  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
							  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
		CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM pre_enc_num */
	sql = "SELECT * FROM pre_enc_num";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, sql, hstmt);
	printf("\n");
	printf(sql);
	printf("\n");
	print_result(hstmt);
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	printf("\n----------------------------------------");

	/* INSERT_TIMESTAMP */
	printf("\nINSERT_TIMESTAMP\n");

	/* INSERT INTO pre_enc_num */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_num(c1) VALUES(?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);
	
		/* 24197857161011715162171839636988778104 */
		build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
		
		cbParam1 = sizeof(numericparam);
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_TIMESTAMP,	/* value type */
							  SQL_TIMESTAMP,	/* param type */
							  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
							  0,			/* dec digits */
							  &numericparam, /* param value ptr */
							  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
							  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
		CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* INSERT INTO pre_enc_num */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_num(c11) VALUES(?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);
	
		/* 24197857161011715162171839636988778104 */
		build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
		
		cbParam1 = sizeof(numericparam);
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_TIMESTAMP,	/* value type */
							  SQL_TIMESTAMP,	/* param type */
							  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
							  0,			/* dec digits */
							  &numericparam, /* param value ptr */
							  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
							  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
		CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_enc_num */
	sql = "SELECT * FROM pre_enc_num";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, sql, hstmt);
	printf("\n");
	printf(sql);
	printf("\n");
	print_result(hstmt);
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	printf("\n----------------------------------------");

	/* INSERT_BINARY */
	printf("\nINSERT_BINARY\n");

	/* INSERT INTO pre_enc_num */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_num(c1) VALUES(?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);
	
		/* 24197857161011715162171839636988778104 */
		build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
		
		cbParam1 = sizeof(numericparam);
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_BINARY,	/* value type */
							  SQL_BINARY,	/* param type */
							  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
							  0,			/* dec digits */
							  &numericparam, /* param value ptr */
							  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
							  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
		CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* INSERT INTO pre_enc_num */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_num(c11) VALUES(?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);
	
		/* 24197857161011715162171839636988778104 */
		build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
		
		cbParam1 = sizeof(numericparam);
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_BINARY,	/* value type */
							  SQL_BINARY,	/* param type */
							  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
							  0,			/* dec digits */
							  &numericparam, /* param value ptr */
							  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
							  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
		CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_enc_num */
	sql = "SELECT * FROM pre_enc_num";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, sql, hstmt);
	printf("\n");
	printf(sql);
	printf("\n");
	print_result(hstmt);
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	printf("\n----------------------------------------");

	printf("\nSELECT_VARCHAR\n");

	/* SELECT * FROM pre_enc_num WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_num WHERE c1=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_num WHERE c1 failed", hstmt);
	
		/* 24197857161011715162171839636988778104 */
		build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
		
		cbParam1 = sizeof(numericparam);
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_CHAR,	/* value type */
							  SQL_CHAR,	/* param type */
							  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
							  0,			/* dec digits */
							  &numericparam, /* param value ptr */
							  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
							  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
		CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

	rc = SQLExecute(hstmt);
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);
	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_enc_num WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_num WHERE c11=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_num WHERE c11 failed", hstmt);
	
		/* 24197857161011715162171839636988778104 */
		build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
		
		cbParam1 = sizeof(numericparam);
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_CHAR,	/* value type */
							  SQL_CHAR,	/* param type */
							  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
							  0,			/* dec digits */
							  &numericparam, /* param value ptr */
							  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
							  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
		CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

	rc = SQLExecute(hstmt);
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);
	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	printf("\n----------------------------------------");

	printf("\nSELECT_TIMESTAMP\n");

	/* SELECT * FROM pre_enc_num WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_num WHERE c1=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_num WHERE c1 failed", hstmt);
	
		/* 24197857161011715162171839636988778104 */
		build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
		
		cbParam1 = sizeof(numericparam);
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_TIMESTAMP,	/* value type */
							  SQL_TIMESTAMP,	/* param type */
							  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
							  0,			/* dec digits */
							  &numericparam, /* param value ptr */
							  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
							  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
		CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

	rc = SQLExecute(hstmt);
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);
	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM pre_enc_num WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_num WHERE c11=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_num WHERE c11 failed", hstmt);
	
		/* 24197857161011715162171839636988778104 */
		build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
		
		cbParam1 = sizeof(numericparam);
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_TIMESTAMP,	/* value type */
							  SQL_TIMESTAMP,	/* param type */
							  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
							  0,			/* dec digits */
							  &numericparam, /* param value ptr */
							  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
							  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
		CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

	rc = SQLExecute(hstmt);
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);
	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	printf("\n----------------------------------------");

	printf("\nSELECT_BINARY\n");

	/* SELECT * FROM pre_enc_num WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_num WHERE c1=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_num WHERE c1 failed", hstmt);
	
		/* 24197857161011715162171839636988778104 */
		build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
		
		cbParam1 = sizeof(numericparam);
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_BINARY,	/* value type */
							  SQL_BINARY,	/* param type */
							  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
							  0,			/* dec digits */
							  &numericparam, /* param value ptr */
							  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
							  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
		CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

		rc = SQLExecute(hstmt);
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_enc_num WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_num WHERE c11=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_num WHERE c11 failed", hstmt);
	
		/* 24197857161011715162171839636988778104 */
		build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
		
		cbParam1 = sizeof(numericparam);
		rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_BINARY,	/* value type */
							  SQL_BINARY,	/* param type */
							  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
							  0,			/* dec digits */
							  &numericparam, /* param value ptr */
							  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
							  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
		CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

		rc = SQLExecute(hstmt);
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* Clean up */
	test_disconnect();

	return 0;
}
