/*
* Test ERROR-disable_log ENCRYPT_BYTEA_TEST.
 */
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int main(int argc, char **argv)
{
	SQLRETURN rc;
	HSTMT hstmt = SQL_NULL_HSTMT;
	char param1[20] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	char param2[20] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	SQLLEN cbParam;
	SQLLEN cbParam1;
	SQLCHAR *sql;

	test_connect();

	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("failed to allocate stmt handle", SQL_HANDLE_DBC, conn);
		exit(1);
	}

	printf("\n====== No_disable_log==============");

	/* PREPARE_INSERT TEST */
	printf("\nPREPARE_INSERT TEST");

	/* pgtde begin session */
	sql = "SELECT pgtde_begin_session('aofiafoaeiaofijabeoptuiotaoi')";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);

	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* INSERT INTO pre_enc_bytea */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_bytea(c1) VALUES (?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

				cbParam = 8;
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_BINARY,	/* value type */
							  SQL_BINARY,	/* param type */
							  20,			/* column size */
							  0,			/* dec digits */
							  param1,		/* param value ptr */
							  0,			/* buffer len */
							  &cbParam		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	

	/* PREPARE SELECT TEST */
	printf("\nPREPARE_SELECT_TEST");

	/* pgtde begin session */
	sql = "SELECT pgtde_begin_session('aofiafoaeiaofijabeoptuiotaoi')";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);

	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM pre_enc_bytea WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_bytea WHERE c1=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_bytea WHERE c1 failed", hstmt);
	
			/* bind param  */
			cbParam1 = 8;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_BINARY,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param1,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			
			/* Print error, it is expected */
			if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	printf("\n----------------------------------------");

	/* DIRECT_INSERT TEST */
	printf("\nDIRECT_INSERT TEST");

	/* pgtde begin session */
	sql = "SELECT pgtde_begin_session('aofiafoaeiaofijabeoptuiotaoi')";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);

	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* INSERT INTO dir_enc_bytea */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "INSERT INTO dir_enc_bytea(c1) VALUES ( '\\x0102030405060708')", SQL_NTS);

	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
	
	/* DIRECT_SELECT TEST */
	printf("\nDIRECT_SELECT TEST");

	/* pgtde begin session */
	sql = "SELECT pgtde_begin_session('aofiafoaeiaofijabeoptuiotaoi')";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM dir_enc_bytea */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT * FROM dir_enc_bytea", SQL_NTS);

	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	
	printf("\n====== No_begin_session==============");

	/* PREPARE_INSERT TEST */
	printf("\nPREPARE_INSERT TEST");

	/* cipher_key disable */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT cipher_key_disable_log()", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while cipher_key disable", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* INSERT INTO pre_enc_bytea */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_bytea(c1) VALUES (?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

				cbParam = 8;
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_BINARY,	/* value type */
							  SQL_BINARY,	/* param type */
							  20,			/* column size */
							  0,			/* dec digits */
							  param1,		/* param value ptr */
							  0,			/* buffer len */
							  &cbParam		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	

	/* PREPARE SELECT TEST */
	printf("\nPREPARE_SELECT_TEST");

	/* cipher_key disable */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT cipher_key_disable_log()", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while cipher_key disable", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM pre_enc_bytea WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_bytea WHERE c1=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_bytea WHERE c1 failed", hstmt);

			/* bind param  */
			cbParam1 = 8;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_BINARY,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param1,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			
			/* Print error, it is expected */
			if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	printf("\n----------------------------------------");

	/* DIRECT_INSERT TEST */
	printf("\nDIRECT_INSERT TEST");

	/* cipher_key disable */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT cipher_key_disable_log()", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while cipher_key disable", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* INSERT INTO dir_enc_bytea */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "INSERT INTO dir_enc_bytea(c1) VALUES ( '\\x0102030405060708')", SQL_NTS);

	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
	
	/* DIRECT_SELECT TEST */
	printf("\nDIRECT_SELECT TEST");

	/* cipher_key disable */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT cipher_key_disable_log()", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while cipher_key disable", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM dir_enc_bytea */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT * FROM dir_enc_bytea", SQL_NTS);

	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
	
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
	
	/* INSERT INTO pre_enc_bytea */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_bytea VALUES (2, ?, ?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

				cbParam = 8;
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_CHAR,	/* value type */
									  SQL_CHAR,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  param1,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

				cbParam1 = 8;
				rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
									  SQL_C_CHAR,	/* value type */
									  SQL_CHAR,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  param2,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam1		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_enc_bytea */
	sql = "SELECT * FROM pre_enc_bytea";
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

	/* INSERT_NUMERIC */
	printf("\nINSERT_NUMERIC\n");

	/* INSERT INTO pre_enc_bytea */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_bytea VALUES (3, ?, ?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

				cbParam = 8;
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_NUMERIC,	/* value type */
									  SQL_NUMERIC,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  param1,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

				cbParam1 = 8;
				rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
									  SQL_C_NUMERIC,	/* value type */
									  SQL_NUMERIC,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  param2,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam1		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_enc_bytea */
	sql = "SELECT * FROM pre_enc_bytea";
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

	/* INSERT INTO pre_enc_bytea */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_bytea VALUES (4, ?, ?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

				cbParam = 8;
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_TIMESTAMP ,	/* value type */
									  SQL_TIMESTAMP ,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  param1,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

				cbParam1 = 8;
				rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
									  SQL_C_TIMESTAMP ,	/* value type */
									  SQL_TIMESTAMP ,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  param2,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam1		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_enc_bytea */
	sql = "SELECT * FROM pre_enc_bytea";
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

	/* SELECT * FROM pre_enc_bytea WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_bytea WHERE c1=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_bytea WHERE c1 failed", hstmt);
	
			/* bind param  */
			cbParam = 8;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_CHAR,	/* value type */
								  SQL_CHAR,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param1,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			printf("\n");
			printf(sql);
			printf("\n");
			print_result(hstmt);
			
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM pre_enc_bytea WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_bytea WHERE c11=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_bytea WHERE c1 failed", hstmt);
	
			/* bind param  */
			cbParam = 8;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_CHAR,	/* value type */
								  SQL_CHAR,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param1,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			printf("\n");
			printf(sql);
			printf("\n");
			print_result(hstmt);
			
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	printf("\n----------------------------------------");

	printf("\nSELECT_NUMERIC\n");

	/* SELECT * FROM pre_enc_bytea WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_bytea WHERE c1=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_bytea WHERE c1 failed", hstmt);
	
			/* bind param  */
				cbParam = 8;
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_NUMERIC,	/* value type */
									  SQL_NUMERIC,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  param1,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			printf("\n");
			printf(sql);
			printf("\n");
			print_result(hstmt);
			
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM pre_enc_bytea WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_bytea WHERE c11=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_bytea WHERE c1 failed", hstmt);
	
			/* bind param  */
				cbParam = 8;
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_NUMERIC,	/* value type */
									  SQL_NUMERIC,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  param1,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			printf("\n");
			printf(sql);
			printf("\n");
			print_result(hstmt);
			
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	printf("\n----------------------------------------");

	printf("\nSELECT_TIMESTAMP\n");

	/* SELECT * FROM pre_enc_bytea WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_bytea WHERE c1=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_bytea WHERE c1 failed", hstmt);
	
			/* bind param  */
				cbParam = 8;
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_TIMESTAMP ,	/* value type */
									  SQL_TIMESTAMP ,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  param1,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			if (!SQL_SUCCEEDED(rc))
				print_diag("", SQL_HANDLE_STMT, hstmt);
			
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_enc_bytea WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_bytea WHERE c11=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_bytea WHERE c1 failed", hstmt);
	
			/* bind param  */
				cbParam = 8;
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_TIMESTAMP ,	/* value type */
									  SQL_TIMESTAMP ,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  param1,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			if (!SQL_SUCCEEDED(rc))
				print_diag("", SQL_HANDLE_STMT, hstmt);
			
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* Clean up */
	test_disconnect();

	return 0;
}
