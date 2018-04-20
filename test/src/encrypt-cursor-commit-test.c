/*
 * This test case tests for a bug in result set caching, with
 * UseDeclareFetch=1, that was fixed. The bug occurred when a cursor was
 * closed, due to transaction commit, before any rows were fetched from
 * it. That set the "base" of the internal cached rowset incorrectly,
 * off by one.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

static void
printFetchResult(HSTMT hstmt, int rc)
{
	if (SQL_SUCCEEDED(rc))
	{
		char buf[50],buf1[50],buf2[50],buf3[50],buf4[50],buf5[50],buf6[50],buf7[50];
		SQLLEN ind;

		if (rc == SQL_SUCCESS_WITH_INFO)
			print_diag("got SUCCESS_WITH_INFO", SQL_HANDLE_STMT, hstmt);

		rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT 'foo'::bytea,'foo'::encrypt_bytea,'foo'::text || g,'foo'::encrypt_text || g,11111.11111::numeric,11111.11111::encrypt_numeric,'2017-08-09 11:26:52.326167'::timestamp,'2017-08-09 11:26:52.326167'::encrypt_timestamp FROM generate_series(1, 3210) g", SQL_NTS);
		
		rc = SQLGetData(hstmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
		CHECK_STMT_RESULT(rc, "SQLGetData failed", hstmt);
		rc = SQLGetData(hstmt, 2, SQL_C_CHAR, buf1, sizeof(buf), &ind);
		CHECK_STMT_RESULT(rc, "SQLGetData failed", hstmt);
		rc = SQLGetData(hstmt, 3, SQL_C_CHAR, buf2, sizeof(buf), &ind);
		CHECK_STMT_RESULT(rc, "SQLGetData failed", hstmt);
		rc = SQLGetData(hstmt, 4, SQL_C_CHAR, buf3, sizeof(buf), &ind);
		CHECK_STMT_RESULT(rc, "SQLGetData failed", hstmt);
		rc = SQLGetData(hstmt, 5, SQL_C_CHAR, buf4, sizeof(buf), &ind);
		CHECK_STMT_RESULT(rc, "SQLGetData failed", hstmt);
		rc = SQLGetData(hstmt, 6, SQL_C_CHAR, buf5, sizeof(buf), &ind);
		CHECK_STMT_RESULT(rc, "SQLGetData failed", hstmt);
		rc = SQLGetData(hstmt, 7, SQL_C_CHAR, buf6, sizeof(buf), &ind);
		CHECK_STMT_RESULT(rc, "SQLGetData failed", hstmt);
		rc = SQLGetData(hstmt, 8, SQL_C_CHAR, buf7, sizeof(buf), &ind);
		CHECK_STMT_RESULT(rc, "SQLGetData failed", hstmt);
		printf("fetched: %s, %s, %s, %s, %s, %s, %s, %s\n", buf,buf1,buf2,buf3,buf4,buf5,buf6,buf7);
	}
	else if (rc == SQL_NO_DATA)
		printf("Fetch: no data found\n"); /* expected */
	else
		CHECK_STMT_RESULT(rc, "Fetch failed", hstmt);
}

static const char *
sql_commit_behavior_str(SQLUINTEGER info)
{
	static char buf[50];

	switch(info)
	{
		case SQL_CB_DELETE:
			return "SQL_CB_DELETE";

		case SQL_CB_CLOSE:
			return "SQL_CB_CLOSE";

		case SQL_CB_PRESERVE:
			return "SQL_CB_PRESERVE";

		default:
			sprintf(buf, "unknown (%u)", (unsigned int) info);
			return buf;
	}
}

int main(int argc, char **argv)
{
	int			rc;
	HSTMT		hstmt = SQL_NULL_HSTMT;
	SQLCHAR		charval[100];
	SQLLEN		len;
	int			row;

	test_connect();

	/* Start a transaction */
	rc = SQLSetConnectAttr(conn, SQL_ATTR_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);

	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("failed to allocate stmt handle", SQL_HANDLE_DBC, conn);
		exit(1);
	}
	/*
	 * Begin executing a query
	 */
	pgtde_begin_session(hstmt);

	rc = SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE,
						(SQLPOINTER) SQL_CURSOR_STATIC, SQL_IS_UINTEGER);
	CHECK_STMT_RESULT(rc, "SQLSetStmtAttr failed", hstmt);



	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT 'foo'::bytea,'foo'::encrypt_bytea,'foo'::text || g,'foo'::encrypt_text || g,11111.11111::numeric,11111.11111::encrypt_numeric,'2017-08-09 11:26:52.326167'::timestamp,'2017-08-09 11:26:52.326167'::encrypt_timestamp FROM generate_series(1, 3210) g", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed", hstmt);

	rc = SQLBindCol(hstmt, 1, SQL_C_CHAR, &charval, sizeof(charval), &len);
	CHECK_STMT_RESULT(rc, "SQLBindCol failed", hstmt);

	/* Commit. This implicitly closes the cursor in the server. */
	rc = SQLEndTran(SQL_HANDLE_DBC, conn, SQL_COMMIT);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("failed to commit", SQL_HANDLE_DBC, conn);
		exit(1);
	}

	rc = SQLFetchScroll(hstmt, SQL_FETCH_FIRST, 0);
	CHECK_STMT_RESULT(rc, "SQLFetchScroll(FIRST) failed", hstmt);

	if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
//		printf("first row: %s\n", charval);
		printf("\n");
		print_result(hstmt);
		printf("\n");

	row = 1;
	while (1)
	{
		rc = SQLFetchScroll(hstmt, SQL_FETCH_NEXT, 0);
		if (rc == SQL_NO_DATA)
			break;

		row++;

		if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
			printf("row %d: %s\n", row, charval);
		else
		{
			print_diag("SQLFetchScroll failed", SQL_HANDLE_STMT, hstmt);
			exit(1);
		}
	}

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* Clean up */
	test_disconnect();

	return 0;
}
