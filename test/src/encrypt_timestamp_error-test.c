/*
* Test ERROR-ENCRYPT_TIMESTAMP_TEST.
 */
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int main(int argc, char **argv)
{
	SQLRETURN rc;
	HSTMT hstmt = SQL_NULL_HSTMT;
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

	/* INSERT INTO pre_enc_timestamp */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_timestamp(c1) VALUES (?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

	/* bind param  */
	SQLLEN cbParam1 = 0;
	TIMESTAMP_STRUCT param1;
	param1.year = 2017;
	param1.month = 8;
	param1.day = 18;
	param1.hour = 5;
	param1.minute = 50;
	param1.second = 30;
	param1.fraction = 10;

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
					 SQL_C_CHAR,
					 SQL_CHAR,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 0,
					 &param1,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 &cbParam1);
	CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* INSERT INTO pre_enc_timestamp */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_timestamp(c11) VALUES (?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
					 SQL_C_CHAR,
					 SQL_CHAR,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 0,
					 &param1,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 &cbParam1);
	CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_enc_timestamp */
	sql = "SELECT * FROM pre_enc_timestamp";
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

	/* INSERT INTO pre_enc_timestamp */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_timestamp(c1) VALUES (?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

	/* bind param  */
	param1.year = 2017;
	param1.month = 8;
	param1.day = 18;
	param1.hour = 5;
	param1.minute = 50;
	param1.second = 30;
	param1.fraction = 10;

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
					 SQL_C_NUMERIC,
					 SQL_NUMERIC,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 0,
					 &param1,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 &cbParam1);
	CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* INSERT INTO pre_enc_timestamp */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_timestamp(c11) VALUES (?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
					 SQL_C_NUMERIC,
					 SQL_NUMERIC,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 0,
					 &param1,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 &cbParam1);
	CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_enc_timestamp */
	sql = "SELECT * FROM pre_enc_timestamp";
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

	/* INSERT INTO pre_enc_timestamp */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_timestamp(c1) VALUES (?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

	/* bind param  */
	param1.year = 2017;
	param1.month = 8;
	param1.day = 18;
	param1.hour = 5;
	param1.minute = 50;
	param1.second = 30;
	param1.fraction = 10;

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
					 SQL_C_BINARY,
					 SQL_BINARY,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 0,
					 &param1,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 &cbParam1);
	CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
			
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* INSERT INTO pre_enc_timestamp */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_timestamp(c11) VALUES (?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
					 SQL_C_BINARY,
					 SQL_BINARY,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 0,
					 &param1,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 &cbParam1);
	CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
			
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_enc_timestamp */
	sql = "SELECT * FROM pre_enc_timestamp";
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

	/* SELECT * FROM pre_enc_timestamp WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_timestamp WHERE c1=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_timestamp WHERE c1 failed", hstmt);
	
	/* bind param  */
	param1.year = 2017;
	param1.month = 7;
	param1.day = 7;
	param1.hour = 5;
	param1.minute = 50;
	param1.second = 30;
	param1.fraction = 10;

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
					 SQL_C_CHAR,
					 SQL_CHAR,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 0,
					 &param1,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 &cbParam1);
	CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
	rc = SQLExecute(hstmt);
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);
	
		/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_enc_timestamp WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_timestamp WHERE c11=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_timestamp WHERE c11 failed", hstmt);
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
					 SQL_C_CHAR,
					 SQL_CHAR,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 0,
					 &param1,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 &cbParam1);
	CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
	rc = SQLExecute(hstmt);
	
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);
	
		/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	printf("\n----------------------------------------");

	printf("\nSELECT_NUMERIC\n");
	/* SELECT * FROM pre_enc_timestamp WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_timestamp WHERE c1=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_timestamp WHERE c1 failed", hstmt);
	
	/* bind param  */
	param1.year = 2017;
	param1.month = 7;
	param1.day = 7;
	param1.hour = 5;
	param1.minute = 50;
	param1.second = 30;
	param1.fraction = 10;

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
					 SQL_C_NUMERIC,
					 SQL_NUMERIC,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 0,
					 &param1,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 &cbParam1);
	CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
	rc = SQLExecute(hstmt);
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))print_diag("", SQL_HANDLE_STMT, hstmt);
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	
	/* SELECT * FROM pre_enc_timestamp WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_timestamp WHERE c11=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_timestamp WHERE c11 failed", hstmt);

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
					 SQL_C_NUMERIC,
					 SQL_NUMERIC,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 0,
					 &param1,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 &cbParam1);
	CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
	rc = SQLExecute(hstmt);
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))
		print_diag("", SQL_HANDLE_STMT, hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	printf("\n----------------------------------------");

	printf("\nSELECT_BINARY\n");

	/* SELECT * FROM pre_enc_timestamp WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_timestamp WHERE c1=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_timestamp WHERE c1 failed", hstmt);
	
	/* bind param  */
	param1.year = 2017;
	param1.month = 7;
	param1.day = 7;
	param1.hour = 5;
	param1.minute = 50;
	param1.second = 30;
	param1.fraction = 10;

	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
					 SQL_C_BINARY,
					 SQL_BINARY,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 0,
					 &param1,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 &cbParam1);
	CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

	rc = SQLExecute(hstmt);
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))
		print_diag("", SQL_HANDLE_STMT, hstmt);
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
		/* SELECT * FROM pre_enc_timestamp WHERE */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_timestamp WHERE c11=?", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SELECT * FROM pre_enc_timestamp WHERE c11 failed", hstmt);
	
	SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
					 SQL_C_BINARY,
					 SQL_BINARY,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 0,
					 &param1,
					 sizeof(SQL_TIMESTAMP_STRUCT),
					 &cbParam1);
	CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

	rc = SQLExecute(hstmt);
	/* Print error, it is expected */
	if (!SQL_SUCCEEDED(rc))
		print_diag("", SQL_HANDLE_STMT, hstmt);
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* Clean up */
	test_disconnect();

	printf("\nlog mask confirm test\n");fflush(stdout);
	printf("\n---------- LOG_CHECK_START ----------\n");fflush(stdout);
	system("C:/MinGW/msys/1.0/bin/grep -ir aofiafoaeiaofijabeoptuiotaoi D:/SWF/Jenkins/workspace/psqlodbc_log_dont_delete/*");
	printf("\n---------- LOG_CHECK_END ----------\n");fflush(stdout);
	printf("\n----------LOG_DELETE_START----------\n");
	system("C:/MinGW/msys/1.0/bin/rm -f D:/SWF/Jenkins/workspace/psqlodbc_log_dont_delete/*");
	printf("\n----------LOG_DELETE_END----------\n");

	return 0;

}
