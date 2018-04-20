#include "common.h"

SQLHENV env;
SQLHDBC conn;

/* use for TDEforPG test */
static void encrypt_bytea_crud(SQLSMALLINT data_kind);
static void encrypt_text_crud(SQLSMALLINT data_kind);
static void encrypt_numeric_crud(SQLSMALLINT data_kind);
static void encrypt_timestamp_crud(SQLSMALLINT data_kind);

void
print_diag(char *msg, SQLSMALLINT htype, SQLHANDLE handle)
{
	char		sqlstate[32];
	char		message[1000];
	SQLINTEGER	nativeerror;
	SQLSMALLINT textlen;
	SQLRETURN	ret;
	SQLSMALLINT	recno = 0;

	if (msg)
		printf("%s\n", msg);

	do
	{
		recno++;
		ret = SQLGetDiagRec(htype, handle, recno, sqlstate, &nativeerror,
							message, sizeof(message), &textlen);
		if (ret == SQL_INVALID_HANDLE)
			printf("Invalid handle\n");
		else if (SQL_SUCCEEDED(ret))
			printf("%s=%s\n", sqlstate, message);
	} while (ret == SQL_SUCCESS);

	if (ret == SQL_NO_DATA && recno == 1)
		printf("No error information\n");
}

const char * const default_dsn = "psqlodbc_test_dsn";
const char * const test_dsn_env = "PSQLODBC_TEST_DSN";
const char * const test_dsn_ansi = "psqlodbc_test_dsn_ansi";

const char *get_test_dsn(void)
{
	char	*env = getenv(test_dsn_env);

	if (NULL != env && env[0])
		return env;
	return default_dsn;
}

int IsAnsi(void)
{
	return (strcmp(get_test_dsn(), test_dsn_ansi) == 0);
}

void
test_connect_ext(char *extraparams)
{
	SQLRETURN ret;
	SQLCHAR str[1024];
	SQLSMALLINT strl;
	SQLCHAR dsn[1024];
	const char * const test_dsn = get_test_dsn();
	char *envvar;

	/*
	 *	Use an environment variable to switch settings of connection
	 *	strings throughout the regression test. Note that extraparams
	 *	parameters have precedence over the environment variable.
	 *	ODBC spec says
	 *		If any keywords are repeated in the connection string,
	 *		the driver uses the value associated with the first
	 *		occurrence of the keyword.
	 *	But the current psqlodbc driver uses the value associated with
	 *	the last occurrence of the keyword. Here we place extraparams
	 *	both before and after the value of the environment variable
	 *	so as to protect the priority order whichever way we take.
	 */
	if ((envvar = getenv("COMMON_CONNECTION_STRING_FOR_REGRESSION_TEST")) != NULL && envvar[0] != '\0')
	{
		if (NULL == extraparams)
			snprintf(dsn, sizeof(dsn), "DSN=%s;%s", test_dsn, envvar);
		else
			snprintf(dsn, sizeof(dsn), "DSN=%s;%s;%s;%s",
			 test_dsn, extraparams, envvar, extraparams);
	}
	else
		snprintf(dsn, sizeof(dsn), "DSN=%s;%s",
			 test_dsn, extraparams ? extraparams : "");

	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);

	SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);

	SQLAllocHandle(SQL_HANDLE_DBC, env, &conn);
	ret = SQLDriverConnect(conn, NULL, dsn, SQL_NTS,
						   str, sizeof(str), &strl,
						   SQL_DRIVER_COMPLETE);
	if (SQL_SUCCEEDED(ret)) {
		printf("connected\n");
	} else {
		print_diag("SQLDriverConnect failed.", SQL_HANDLE_DBC, conn);
		exit(1);
	}
}

void
test_connect(void)
{
	test_connect_ext(NULL);
}

void
test_disconnect(void)
{
	SQLRETURN rc;

	printf("disconnecting\n");
	rc = SQLDisconnect(conn);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("SQLDisconnect failed", SQL_HANDLE_DBC, conn);
		exit(1);
	}

	rc = SQLFreeHandle(SQL_HANDLE_DBC, conn);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("SQLFreeHandle failed", SQL_HANDLE_DBC, conn);
		exit(1);
	}
	conn = NULL;

	rc = SQLFreeHandle(SQL_HANDLE_ENV, env);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("SQLFreeHandle failed", SQL_HANDLE_ENV, env);
		exit(1);
	}
	env = NULL;
}

const char *
datatype_str(SQLSMALLINT datatype)
{
	static char buf[100];

	switch (datatype)
	{
		case SQL_CHAR:
			return "CHAR";
		case SQL_VARCHAR:
			return "VARCHAR";
		case SQL_LONGVARCHAR:
			return "LONGVARCHAR";
		case SQL_WCHAR:
			return "WCHAR";
		case SQL_WVARCHAR:
			return "WVARCHAR";
		case SQL_WLONGVARCHAR:
			return "WLONGVARCHAR";
		case SQL_DECIMAL:
			return "DECIMAL";
		case SQL_NUMERIC:
			return "NUMERIC";
		case SQL_SMALLINT:
			return "SMALLINT";
		case SQL_INTEGER:
			return "INTEGER";
		case SQL_REAL:
			return "REAL";
		case SQL_FLOAT:
			return "FLOAT";
		case SQL_DOUBLE:
			return "DOUBLE";
		case SQL_BIT:
			return "BIT";
		case SQL_TINYINT:
			return "TINYINT";
		case SQL_BIGINT:
			return "BIGINT";
		case SQL_BINARY:
			return "BINARY";
		case SQL_VARBINARY:
			return "VARBINARY";
		case SQL_LONGVARBINARY:
			return "LONGVARBINARY";
		case SQL_TYPE_DATE:
			return "TYPE_DATE";
		case SQL_TYPE_TIME:
			return "TYPE_TIME";
		case SQL_TYPE_TIMESTAMP:
			return "TYPE_TIMESTAMP";
		case SQL_GUID:
			return "GUID";
		default:
			snprintf(buf, sizeof(buf), "unknown sql type %d", datatype);
			return buf;
	}
}

const char *nullable_str(SQLSMALLINT nullable)
{
	static char buf[100];

	switch(nullable)
	{
		case SQL_NO_NULLS:
			return "not nullable";
		case SQL_NULLABLE:
			return "nullable";
		case SQL_NULLABLE_UNKNOWN:
			return "nullable_unknown";
		default:
			snprintf(buf, sizeof(buf), "unknown nullable value %d", nullable);
			return buf;
	}
}

void
print_result_meta_series(HSTMT hstmt,
						 SQLSMALLINT *colids,
						 SQLSMALLINT numcols)
{
	int i;

	printf("Result set metadata:\n");

	for (i = 0; i < numcols; i++)
	{
		SQLRETURN rc;
		SQLCHAR colname[50];
		SQLSMALLINT colnamelen;
		SQLSMALLINT datatype;
		SQLULEN colsize;
		SQLSMALLINT decdigits;
		SQLSMALLINT nullable;

		rc = SQLDescribeCol(hstmt, colids[i],
							colname, sizeof(colname),
							&colnamelen,
							&datatype,
							&colsize,
							&decdigits,
							&nullable);
		if (!SQL_SUCCEEDED(rc))
		{
			print_diag("SQLDescribeCol failed", SQL_HANDLE_STMT, hstmt);
			return;
		}
		printf("%s: %s(%u) digits: %d, %s\n",
			   colname, datatype_str(datatype), (unsigned int) colsize,
			   decdigits, nullable_str(nullable));
	}
}

void
print_result_meta(HSTMT hstmt)
{
	SQLRETURN rc;
	SQLSMALLINT numcols, i;
	SQLSMALLINT *colids;

	rc = SQLNumResultCols(hstmt, &numcols);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("SQLNumResultCols failed", SQL_HANDLE_STMT, hstmt);
		return;
	}

	colids = (SQLSMALLINT *) malloc(numcols * sizeof(SQLSMALLINT));
	for (i = 0; i < numcols; i++)
		colids[i] = i + 1;
	print_result_meta_series(hstmt, colids, numcols);
	free(colids);
}

/*
 * Initialize a buffer with "XxXxXx..." to indicate an uninitialized value.
 */
static void
invalidate_buf(char *buf, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
	{
		if (i % 2 == 0)
			buf[i] = 'X';
		else
			buf[i] = 'x';
	}
	buf[len - 1] = '\0';
}

/*
 * Print result only for the selected columns.
 */
void
print_result_series(HSTMT hstmt, SQLSMALLINT *colids, SQLSMALLINT numcols, SQLINTEGER rowcount)
{
	SQLRETURN rc;
	SQLINTEGER	rowc = 0;

	printf("Result set:\n");
	while (rowcount <0 || rowc < rowcount)
	{
		rc = SQLFetch(hstmt);
		if (rc == SQL_NO_DATA)
			break;
		if (rc == SQL_SUCCESS)
		{
			char buf[40];
			int i;
			SQLLEN ind;

			rowc++;
			for (i = 0; i < numcols; i++)
			{
				/*
				 * Initialize the buffer with garbage, so that we see readily
				 * if SQLGetData fails to set the value properly or forgets
				 * to null-terminate it.
				 */
				invalidate_buf(buf, sizeof(buf));
				rc = SQLGetData(hstmt, colids[i], SQL_C_CHAR, buf, sizeof(buf), &ind);
				if (!SQL_SUCCEEDED(rc))
				{
					print_diag("SQLGetData failed", SQL_HANDLE_STMT, hstmt);
					return;
				}
				if (ind == SQL_NULL_DATA)
					strcpy(buf, "NULL");
				printf("%s%s", (i > 0) ? "\t" : "", buf);
			}
			printf("\n");
		}
		else
		{
			print_diag("SQLFetch failed", SQL_HANDLE_STMT, hstmt);
			exit(1);
		}
	}
}

/*
 * Print result on all the columns
 */
void
print_result(HSTMT hstmt)
{
	SQLRETURN rc;
	SQLSMALLINT numcols, i;
	SQLSMALLINT *colids;

	rc = SQLNumResultCols(hstmt, &numcols);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("SQLNumResultCols failed", SQL_HANDLE_STMT, hstmt);
		return;
	}

	colids = (SQLSMALLINT *) malloc(numcols * sizeof(SQLSMALLINT));
	for (i = 0; i < numcols; i++)
		colids[i] = i + 1;
	print_result_series(hstmt, colids, numcols, -1);
	free(colids);
}

/*
 * Test TDEforPG for CRUD.
 */
void tdeforpg_crud(SQLSMALLINT datatype_patern, SQLSMALLINT data_kind){	

	switch(datatype_patern)
	{
		case ENCRYPT_BYTEA_TEST:
			/* ENCRYPT_BYTEA test */
			encrypt_bytea_crud(data_kind);
			break;
		case ENCRYPT_TEXT_TEST:
			/* ENCRYPT_TEXT_TEST test */
			encrypt_text_crud(data_kind);
			break;
		case ENCRYPT_NUMERIC_TEST:
			/* ENCRYPT_NUMERIC_TEST test */
			encrypt_numeric_crud(data_kind);
			break;
		case ENCRYPT_TIMESTAMP_TEST:
			/* ENCRYPT_TIMESTAMP_TEST test */
			encrypt_timestamp_crud(data_kind);
			break;

		default:
			print_diag("Patern does not exit",  SQL_HANDLE_DBC, conn);
			exit(1);
	}

}

/*
 * Test TDEforPG for ENCRYPT_BYTEA_TEST.
 */
static void encrypt_bytea_crud(SQLSMALLINT data_kind){
	
	SQLRETURN rc;
	HSTMT hstmt = SQL_NULL_HSTMT;
	char param1[20] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	char param2[20] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	SQLLEN cbParam, paramsize,i;
	SQLLEN cbParam1,cbParam2;
	SQLCHAR *sql;
	SQLINTEGER longparam;
	SQL_INTERVAL_STRUCT intervalparam;
	SQLSMALLINT colcount;
	char		byteaParam1[5000];
	char		byteaParam2[6000];
	int			j;
	
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("failed to allocate stmt handle", SQL_HANDLE_DBC, conn);
		exit(1);
	}

	/* drop table  pre_enc_bytea */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "DROP TABLE IF EXISTS pre_enc_bytea,pre_bytea_tbl,dir_enc_bytea,dir_bytea_tbl", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while dropping temp table", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* create pre_enc_bytea */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "CREATE TABLE pre_enc_bytea (id int4, c1 encrypt_bytea, c11 bytea)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while creating temp table", hstmt);

	/* pgtde_begin_session */
	pgtde_begin_session(hstmt);
	/***********************/
	
	/* PREPARE TEST */
	printf("\nSQLPrepare_TEST_START\n");
	
	/* INSERT TEST */
	printf("\nINSERT_TEST");

	/* INSERT INTO pre_enc_bytea */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_bytea VALUES (?, ?, ?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

	/* bind param  */
	switch (data_kind)
	{
		case BASIC_DATA: 
			for (i=0;i<3;i++)
			{
				cbParam = 7+i;
				paramsize = sizeof(i);
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_SLONG,	/* value type */
							  SQL_INTEGER,	/* param type */
							  0,			/* column size */
							  0,			/* dec digits */
							  &i,		/* param value ptr */
							  paramsize,/* buffer len */
							  &paramsize/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
				rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
							  SQL_C_BINARY,	/* value type */
							  SQL_BINARY,	/* param type */
							  20,			/* column size */
							  0,			/* dec digits */
							  param1,		/* param value ptr */
							  0,			/* buffer len */
							  &cbParam		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

				rc = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT,
							  SQL_C_BINARY,	/* value type */
							  SQL_BINARY,	/* param type */
							  20,			/* column size */
							  0,			/* dec digits */
							  param2,		/* param value ptr */
							  0,			/* buffer len */
							  &cbParam		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
				/* Execute */
				rc = SQLExecute(hstmt);
				CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
				
			}
	
			break;
			
		case NULL_DATA:
			for (i=0;i<3;i++)
			{
				/* bind param_NULL  */
				cbParam = SQL_NULL_DATA;
				
				paramsize = sizeof(i);
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_SLONG,	/* value type */
									  SQL_INTEGER,	/* param type */
									  0,			/* column size */
									  0,			/* dec digits */
									  &i,		/* param value ptr */
									  paramsize,/* buffer len */
									  &paramsize/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
				rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
									  SQL_C_BINARY,	/* value type */
									  SQL_BINARY,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  NULL,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

				rc = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT,
									  SQL_C_BINARY,	/* value type */
									  SQL_BINARY,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  NULL,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
				/* Execute */
				rc = SQLExecute(hstmt);
				CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			}
			break;
		
		case TOAST_DATA:
			for (i=0;i<3;i++)
			{
				cbParam = 7+i;
				paramsize = sizeof(i);
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							  SQL_C_SLONG,	/* value type */
							  SQL_INTEGER,	/* param type */
							  0,			/* column size */
							  0,			/* dec digits */
							  &i,		/* param value ptr */
							  paramsize,/* buffer len */
							  &paramsize/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
				
				/* fill in test data */
				for (j = 0; j < sizeof(byteaParam1); j++)
				byteaParam1[j] = (char) j;

				cbParam1 = j;
				rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
									  SQL_C_BINARY,	/* value type */
									  SQL_VARBINARY,	/* param type */
									  sizeof(byteaParam1), /* column size */
									  0,			/* dec digits */
									  byteaParam1,	/* param value ptr */
									  sizeof(byteaParam1), /* buffer len */
									  &cbParam1		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);

				cbParam2 = j;
				rc = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT,
									  SQL_C_BINARY,	/* value type */
									  SQL_VARBINARY,	/* param type */
									  sizeof(byteaParam1), /* column size */
									  0,			/* dec digits */
									  byteaParam1,	/* param value ptr */
									  sizeof(byteaParam1), /* buffer len */
									  &cbParam2		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);
					
				/* Execute */
				rc = SQLExecute(hstmt);
				CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
				
			}
			break;
			
		default:
			break;
			
	}

	/* SELECT * FROM pre_enc_bytea */
	sql = "SELECT * FROM pre_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT (c1=c11)::text */
	sql = "SELECT (c1=c11)::text FROM pre_enc_bytea";
	test_execute_sql_and_print(sql);
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);


	/* UPDATE_TEST */
	/* UPDATE pre_enc_bytea SET c1 */
	printf("\nUPDATE_TEST\n");
	
	switch (data_kind)
	{
		case BASIC_DATA:
			rc = SQLPrepare(hstmt, (SQLCHAR *) "UPDATE pre_enc_bytea SET c1=? WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "UPDATE pre_enc_bytea SET c1 failed", hstmt);

			/* bind param  */
			cbParam1 = 6;
			
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_BINARY,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param1,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			cbParam2 = 7;
			rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_BINARY,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param2,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam2		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);

			/* UPDATE pre_enc_bytea SET c11 */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "UPDATE pre_enc_bytea SET c11=? WHERE c11=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "UPDATE pre_enc_bytea SET c11 failed", hstmt);

			cbParam1 = 6;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_BINARY,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param1,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			cbParam2 = 7;
			rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_BINARY,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param2,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam2		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;

	case TOAST_DATA:
			rc = SQLPrepare(hstmt, (SQLCHAR *) "UPDATE pre_enc_bytea SET c1=? WHERE c1=? and id=2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "UPDATE pre_enc_bytea SET c1 failed", hstmt);

			/* fill in test data */
			for (j = 0; j < sizeof(byteaParam2); j++)
			byteaParam2[j] = (char) j;

			cbParam = j;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_VARBINARY,	/* param type */
								  sizeof(byteaParam2), /* column size */
								  0,			/* dec digits */
								  byteaParam2,	/* param value ptr */
								  sizeof(byteaParam2), /* buffer len */
								  &cbParam		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);
			
			/* fill in test data */
			for (j = 0; j < sizeof(byteaParam1); j++)
			byteaParam1[j] = (char) j;

			cbParam1 = j;
			rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_VARBINARY,	/* param type */
								  sizeof(byteaParam1), /* column size */
								  0,			/* dec digits */
								  byteaParam1,	/* param value ptr */
								  sizeof(byteaParam1), /* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			
			/* UPDATE pre_enc_bytea SET c11 */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "UPDATE pre_enc_bytea SET c11=? WHERE c11=? and id=2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "UPDATE pre_enc_bytea SET c11 failed", hstmt);

			/* fill in test data */
			for (j = 0; j < sizeof(byteaParam2); j++)
			byteaParam2[j] = (char) j;

			cbParam = j;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_VARBINARY,	/* param type */
								  sizeof(byteaParam2), /* column size */
								  0,			/* dec digits */
								  byteaParam2,	/* param value ptr */
								  sizeof(byteaParam2), /* buffer len */
								  &cbParam		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);
			
			/* fill in test data */
			for (j = 0; j < sizeof(byteaParam1); j++)
			byteaParam1[j] = (char) j;

			cbParam1 = j;
			rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_VARBINARY,	/* param type */
								  sizeof(byteaParam1), /* column size */
								  0,			/* dec digits */
								  byteaParam1,	/* param value ptr */
								  sizeof(byteaParam1), /* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
				
		default:
			break;
	
	}

	/* SELECT * FROM pre_enc_bytea */
	sql = "SELECT * FROM pre_enc_bytea";
	test_execute_sql_and_print(sql);
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT (c1=c11)::text */
	sql = "SELECT id, (c1=c11)::text FROM pre_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);


	/* DELETE_TEST */
	printf("\nDELETE_TEST\n");

	switch (data_kind)
	{
		case BASIC_DATA:
			/* DELETE FROM pre_enc_bytea WHERE c1 */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "DELETE FROM pre_enc_bytea WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "DELETE FROM pre_enc_bytea WHERE c1 failed", hstmt);

			/* bind param  */
			cbParam1 = 6;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_BINARY,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param1,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
			break;
	
			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			break;

		case TOAST_DATA:
			/* DELETE FROM pre_enc_bytea WHERE c1 */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "DELETE FROM pre_enc_bytea WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "DELETE FROM pre_enc_bytea WHERE c1 failed", hstmt);

			/* fill in test data */
			for (j = 0; j < sizeof(byteaParam2); j++)
			byteaParam2[j] = (char) j;

			cbParam = j;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_VARBINARY,	/* param type */
								  sizeof(byteaParam2), /* column size */
								  0,			/* dec digits */
								  byteaParam2,	/* param value ptr */
								  sizeof(byteaParam2), /* buffer len */
								  &cbParam		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			break;
				
			default:
				break;
	}

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM pre_enc_bytea */
	sql = "SELECT * FROM pre_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT (c1=c11)::text */
	sql = "SELECT (c1=c11)::text FROM pre_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT_TEST */
	printf("\nSELECT_TEST\n");

	switch (data_kind)
	{
		case BASIC_DATA:
			/* SELECT c1=c11 FROM pre_enc_bytea */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT (c1=c11)::text FROM pre_enc_bytea WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SELECT c1=c11 FROM pre_enc_bytea WHERE c1 failed", hstmt);

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
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			printf("SELECT c1=c11 FROM pre_enc_bytea WHERE c1");
			printf("\n");
			print_result(hstmt);
			
			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* SELECT * FROM pre_enc_bytea */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_bytea WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SELECT c1=c11 FROM pre_enc_bytea WHERE c1 failed", hstmt);

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
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			printf("\n");
			printf("SELECT * FROM pre_enc_bytea WHERE c1");
			printf("\n");
			print_result(hstmt);
			break;
			
	case TOAST_DATA:
			/* SELECT c1=c11 FROM pre_enc_bytea */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT (c1=c11)::text FROM pre_enc_bytea WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SELECT c1=c11 FROM pre_enc_bytea WHERE c1 failed", hstmt);

			/* fill in test data */
			for (j = 0; j < sizeof(byteaParam1); j++)
			byteaParam1[j] = (char) j;

			cbParam1 = j;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_VARBINARY,	/* param type */
								  sizeof(byteaParam1), /* column size */
								  0,			/* dec digits */
								  byteaParam1,	/* param value ptr */
								  sizeof(byteaParam1), /* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			printf("SELECT c1=c11 FROM pre_enc_bytea WHERE c1");
			printf("\n");
			print_result(hstmt);
			
			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* SELECT * FROM pre_enc_bytea */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM pre_enc_bytea WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SELECT c1=c11 FROM pre_enc_bytea WHERE c1 failed", hstmt);

			/* fill in test data */
			for (j = 0; j < sizeof(byteaParam1); j++)
			byteaParam1[j] = (char) j;

			cbParam1 = j;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_BINARY,	/* value type */
								  SQL_VARBINARY,	/* param type */
								  sizeof(byteaParam1), /* column size */
								  0,			/* dec digits */
								  byteaParam1,	/* param value ptr */
								  sizeof(byteaParam1), /* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			printf("\n");
			printf("SELECT * FROM pre_enc_bytea WHERE c1");
			printf("\n");
			print_result(hstmt);
			break;
						
			default:
				break;
	}

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM pre_enc_bytea */
	sql = "SELECT * FROM pre_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT (c1=c11)::text */
	sql = "SELECT (c1=c11)::text,c1=c11 FROM pre_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	
	/* create pre_bytea_tbl */
	sql = "CREATE TABLE pre_bytea_tbl (id int4, c1 bytea)";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while create pre_bytea_tbl table", hstmt);	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* insert pre_bytea_tbl */
	sql = "INSERT INTO pre_bytea_tbl(id, c1) SELECT id, c1 FROM pre_enc_bytea";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while insert pre_bytea_tbl", hstmt);	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_bytea_tbl */
	sql = "SELECT * FROM pre_bytea_tbl UNION ALL SELECT id,c1 FROM pre_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* select e.c1=t.c1 */
	sql = "SELECT (e.c1=t.c1)::text FROM pre_enc_bytea e NATURAL JOIN pre_bytea_tbl t";
	test_execute_sql_and_print(sql);
	
	printf("\nPREPARE_TEST_COMPLETE\n");
	
	printf("\n----------------------------------------");

	/* DIRECT TEST */
	printf("\nSQLDirect_TEST_START\n");

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* create dir_enc_bytea */
	sql = "CREATE TABLE dir_enc_bytea (id int4, c1 encrypt_bytea, c11 bytea)";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while create dir_enc_bytea table", hstmt);	

	
	/* INSERT INTO_TEST */
	printf("\nINSERT_TEST");

	/* INSERT INTO dir_enc_bytea */
	switch (data_kind)
	{
		case BASIC_DATA:	
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "INSERT INTO dir_enc_bytea VALUES (1, '\\x0102030405060708', '\\x0102030405060708')", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_bytea table", hstmt);	
			
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* INSERT INTO dir_enc_bytea */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "INSERT INTO dir_enc_bytea VALUES (2, '\\x01020304050607', '\\x01020304050607')", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_bytea table", hstmt);	
			break;
			
		case NULL_DATA:	
			/* INSERT INTO dir_enc_bytea */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "INSERT INTO dir_enc_bytea VALUES (1, '\\x0102030405060708', '\\x0102030405060708')", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_bytea table", hstmt);	
			
			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* INSERT INTO dir_enc_bytea */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "INSERT INTO dir_enc_bytea VALUES (2, NULL, NULL)", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_bytea table", hstmt);	

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* INSERT INTO dir_enc_bytea */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "INSERT INTO dir_enc_bytea VALUES (3, NULL, NULL)", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_bytea table", hstmt);	
			break;

		case TOAST_DATA:
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "INSERT INTO dir_enc_bytea SELECT * FROM pre_enc_bytea", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_bytea table", hstmt);	
			break;
			
		default:
			break;

	}			
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* SELECT * FROM dir_enc_bytea */
	sql = "SELECT * FROM dir_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT (c1=c11)::text */
	sql = "SELECT (c1=c11)::text FROM dir_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	
	/* UPDATE_TEST */
	printf("\nUPDATE_TEST");

	switch (data_kind)
	{
		case BASIC_DATA:
			/* UPDATE dir_enc_bytea SET c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_bytea SET c1='\\x010203040506' WHERE c1='\\x01020304050607'", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* UPDATE dir_enc_bytea SET c11 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_bytea SET c11='\\x010203040506' WHERE c11='\\x01020304050607'", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c11", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
			break;

	case NULL_DATA:
			/* UPDATE dir_enc_bytea SET c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_bytea SET c1='\\x010203040506' WHERE c1 IS NULL AND id = 2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* UPDATE dir_enc_bytea SET c11 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_bytea SET c11='\\x010203040506' WHERE c11 IS NULL AND id = 2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c11", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* UPDATE dir_enc_bytea SET c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_bytea SET c1 = NULL WHERE c1='\\x010203040506' AND id = 2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* UPDATE dir_enc_bytea SET c11 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_bytea SET c1 = NULL WHERE c11='\\x010203040506' AND id = 2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c11", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
			break;

	case TOAST_DATA:
			/* UPDATE dir_enc_bytea SET c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_bytea SET c1=(SELECT repeat('1',10000))::bytea WHERE id=1", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* UPDATE dir_enc_bytea SET c11 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_bytea SET c11=(SELECT repeat('1',10000))::bytea WHERE id=1", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c11", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
			break;
		
		default:
			break;
	}

	/* SELECT * FROM dir_enc_bytea */
	sql = "SELECT * FROM dir_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT (c1=c11)::text */
	sql = "SELECT (c1=c11)::text FROM dir_enc_bytea";
	test_execute_sql_and_print(sql);
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	
	/* DELETE_TEST */
	printf("\nDELETE_TEST");

	switch (data_kind)
	{
		case BASIC_DATA:
			/* DELETE FROM dir_enc_bytea WHERE c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "DELETE FROM dir_enc_bytea WHERE c1='\\x010203040506'", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while DELETE FROM dir_enc_bytea WHERE c1", hstmt);
			break;
			
		case NULL_DATA:
			/* DELETE FROM dir_enc_bytea WHERE c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "DELETE FROM dir_enc_bytea WHERE c1 IS NULL and id = 3", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while DELETE FROM dir_enc_bytea WHERE c1", hstmt);
			break;
		
	case TOAST_DATA:
			/* DELETE FROM dir_enc_bytea WHERE c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "DELETE FROM dir_enc_bytea WHERE id=1", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while DELETE FROM dir_enc_bytea WHERE c1", hstmt);
			break;
			
		default:
			break;		
	}
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
	
	/* SELECT * FROM dir_enc_bytea */
	sql = "SELECT * FROM dir_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM dir_enc_bytea */
	sql = "SELECT * FROM dir_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT (c1=c11)::text */
	sql = "SELECT (c1=c11)::text FROM dir_enc_bytea";
	test_execute_sql_and_print(sql);
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);


	/* SELECT_TEST */
	printf("\nSELECT_TEST");

	switch (data_kind)
	{
		case BASIC_DATA:
			/* SELECT * FROM dir_enc_bytea */
			sql = "SELECT * FROM dir_enc_bytea WHERE c1='\\x0102030405060708'";
			test_execute_sql_and_print(sql);
			break;
			
		case NULL_DATA:
			/* SELECT * FROM dir_enc_bytea WHERE c1 IS NULL */
			sql = "SELECT * FROM dir_enc_bytea WHERE c1 IS NULL";
			test_execute_sql_and_print(sql);
			break;
		
	case TOAST_DATA:
			/* SELECT * FROM dir_enc_bytea */
			sql = "SELECT * FROM dir_enc_bytea where id=0";
			test_execute_sql_and_print(sql);
			break;
			
		default:
			break;		
	}
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
	
	/* SELECT * FROM dir_enc_bytea */
	sql = "SELECT * FROM pre_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM dir_enc_bytea */
	sql = "SELECT * FROM dir_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT (c1=c11)::text */
	sql = "SELECT (c1=c11)::text FROM dir_enc_bytea";
	test_execute_sql_and_print(sql);
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* create dir_bytea_tbl */
	sql = "CREATE TABLE dir_bytea_tbl (id int4, c1 bytea)";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while create dir_bytea_tbl table", hstmt);	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* INSERT INTO dir_bytea_tbl */
	sql = "INSERT INTO dir_bytea_tbl(id, c1) SELECT id, c1 FROM dir_enc_bytea";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_bytea_tbl table", hstmt);	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* select e.c1=t.c1 */
	sql = "SELECT * FROM dir_bytea_tbl UNION ALL SELECT id,c1 FROM dir_enc_bytea";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* select e.c1=t.c1 */
	sql = "SELECT (e.c1=t.c1)::text FROM dir_enc_bytea e NATURAL JOIN dir_bytea_tbl t";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	printf("\nDIRECTE_TEST_COMPLETE\n");
	printf("----------------------------------------\n");

	rc = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("SQLFreeHandle failed", SQL_HANDLE_STMT, hstmt);
		exit(1);
	}
}	

/*
 * Test TDEforPG for ENCRYPT_TEXT_TEST.
 */
static void encrypt_text_crud(SQLSMALLINT data_kind){
	SQLRETURN rc;
	HSTMT hstmt = SQL_NULL_HSTMT;
	char *param1;
	char *param2;
	SQLLEN cbParam;
	SQLLEN paramsize;
	SQLLEN cbParam1;
	SQLLEN cbParam2;
	SQLCHAR *sql;
	int i;
	SQLINTEGER longparam;
	SQL_INTERVAL_STRUCT intervalparam;
	SQLSMALLINT colcount;
	char		byteaParam1[5000];
	char		byteaParam2[5000];
	
	const char* onebyte = "a";
	char 		*texttest_ext;
	int			j;

	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("failed to allocate stmt handle", SQL_HANDLE_DBC, conn);
		exit(1);
	}

	/* PREPARE TEST */
	printf("\nSQLPrepare_TEST_START\n");

	/* drop table  pre_enc_bytea */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "DROP TABLE IF EXISTS pre_enc_text,pre_text_tbl,dir_enc_text,dir_text_tbl", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while dropping temp table", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* create enc_text */
	sql = "CREATE TABLE pre_enc_text (id int4, c1 encrypt_text, c11 text)";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while create pre_enc_text table", hstmt);	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/*pgtde_begin_session*/
	pgtde_begin_session(hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* INSERT_TEST */
	printf("\nINSERT_TEST");

	/* INSERT INTO pre_enc_text */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_text VALUES (?, ?, ?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

	/* bind param  */
	switch (data_kind)
	{
		case BASIC_DATA: 

			/* INSERT INTO pre_enc_text */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_text VALUES (0, ?, ?)", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

			param1 = "foo";
			cbParam1 = 8;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_CHAR,	/* value type */
								  SQL_CHAR,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param1,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			param2 = "foo";
			cbParam2 = 8;
			rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
								  SQL_C_CHAR,	/* value type */
								  SQL_CHAR,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param2,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam2		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* INSERT INTO pre_enc_text */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_text VALUES (1, ?, ?)", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

			param1 = "bar";
			cbParam1 = 8;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_CHAR,	/* value type */
								  SQL_CHAR,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param1,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			param2 = "bar";
			cbParam2 = 8;
			rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
								  SQL_C_CHAR,	/* value type */
								  SQL_CHAR,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param2,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam2		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);


			/* INSERT INTO pre_enc_text */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_text VALUES (2, ?, ?)", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

			param1 = "tde";
			cbParam1 = 8;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_CHAR,	/* value type */
								  SQL_CHAR,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param1,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			param2 = "tde";
			cbParam2 = 8;
			rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
								  SQL_C_CHAR,	/* value type */
								  SQL_CHAR,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param2,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam2		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);

			break;
		
		case NULL_DATA:
			/* INSERT INTO pre_enc_text */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_text VALUES (?, ?, ?)", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

			for (i=0;i<2;i++)
				{
					cbParam = SQL_NULL_DATA;
					
					paramsize = sizeof(i);
					rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
										  SQL_C_SLONG,	/* value type */
										  SQL_INTEGER,	/* param type */
										  0,			/* column size */
										  0,			/* dec digits */
										  &i,		/* param value ptr */
										  paramsize,/* buffer len */
										  &paramsize/* StrLen_or_IndPtr */);
					CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
					
					rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
										  SQL_C_CHAR,	/* value type */
										  SQL_CHAR,	/* param type */
										  20,			/* column size */
										  0,			/* dec digits */
										  NULL,		/* param value ptr */
										  0,			/* buffer len */
										  &cbParam		/* StrLen_or_IndPtr */);
					CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

					rc = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT,
										  SQL_C_CHAR,	/* value type */
										  SQL_CHAR,	/* param type */
										  20,			/* column size */
										  0,			/* dec digits */
										  NULL,		/* param value ptr */
										  0,			/* buffer len */
										  &cbParam		/* StrLen_or_IndPtr */);
					CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
					
					/* Execute */
					rc = SQLExecute(hstmt);
					CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
				}		
			break;

		case TOAST_DATA:
			/* INSERT INTO pre_enc_text */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_text VALUES (?, ?, ?)", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

			texttest_ext = malloc(9001*sizeof(char));
			strcpy(texttest_ext,onebyte);
			for (i=1; i<9000;i++)
				strcat(texttest_ext,onebyte);
			cbParam1 = 8999;
		
			for (i=0;i<4;i++)
				{
					cbParam = 7+i;
					paramsize = sizeof(i);
					rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_SLONG,	/* value type */
								  SQL_INTEGER,	/* param type */
								  0,			/* column size */
								  0,			/* dec digits */
								  &i,		/* param value ptr */
								  paramsize,/* buffer len */
								  &paramsize/* StrLen_or_IndPtr */);
					CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);
					
					rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
										  SQL_C_CHAR,	/* value type */
										  SQL_CHAR,	/* param type */
										  sizeof(texttest_ext), /* column size */
										  0,			/* dec digits */
										  texttest_ext,	/* param value ptr */
										  sizeof(texttest_ext), /* buffer len */
										  &cbParam1		/* StrLen_or_IndPtr */);
					CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);

					rc = SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT,
										  SQL_C_CHAR,	/* value type */
										  SQL_CHAR,	/* param type */
										  sizeof(texttest_ext), /* column size */
										  0,			/* dec digits */
										  texttest_ext,	/* param value ptr */
										  sizeof(texttest_ext), /* buffer len */
										  &cbParam1		/* StrLen_or_IndPtr */);
					CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);
						
					/* Execute */
					rc = SQLExecute(hstmt);
					CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
				}
				break;
			
		default:
			break;		
	}
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* SELECT * FROM pre_enc_text */
	sql = "SELECT * FROM pre_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* select c1=c11 */
	sql = "SELECT (c1=c11)::text FROM pre_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* UPDATE pre_enc_text */
	printf("\nUPDATE_TEST");

	switch (data_kind)
	{
		case BASIC_DATA: 
			/* UPDATE pre_enc_text SET c1 */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "UPDATE pre_enc_text SET c1=? WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "UPDATE pre_enc_text SET c1 failed", hstmt);

			/* bind param  */
			param1 = "enc";
			cbParam1 = 8;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_CHAR,	/* value type */
								  SQL_CHAR,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param1,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			param2 = "tde";
			cbParam2 = 8;
			rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
								  SQL_C_CHAR,	/* value type */
								  SQL_CHAR,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param2,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam2		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* UPDATE pre_enc_text SET c1 */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "UPDATE pre_enc_text SET c11=? WHERE c11=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "UPDATE pre_enc_text SET c11 failed", hstmt);

			/* bind param  */
			param1 = "enc";
			cbParam1 = 8;
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_CHAR,	/* value type */
								  SQL_CHAR,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param1,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam1		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			param2 = "tde";
			cbParam2 = 8;
			rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
								  SQL_C_CHAR,	/* value type */
								  SQL_CHAR,	/* param type */
								  20,			/* column size */
								  0,			/* dec digits */
								  param2,		/* param value ptr */
								  0,			/* buffer len */
								  &cbParam2		/* StrLen_or_IndPtr */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			break;
			
		case NULL_DATA: 
			break;
			
		case TOAST_DATA: 
			/* UPDATE pre_enc_text SET c1 */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "UPDATE pre_enc_text SET c1=? WHERE id=2 OR id=3", SQL_NTS);
			CHECK_STMT_RESULT(rc, "UPDATE pre_enc_text SET c1 failed", hstmt);

			/* bind param  */
				const char* onebyte = "b";
				texttest_ext = malloc(9001*sizeof(char));
				strcpy(texttest_ext,onebyte);
				for (i=1; i<9000;i++)
					strcat(texttest_ext,onebyte);
				cbParam1 = 8999;

				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_CHAR,	/* value type */
									  SQL_CHAR,	/* param type */
									  sizeof(texttest_ext), /* column size */
									  0,			/* dec digits */
									  texttest_ext,	/* param value ptr */
									  sizeof(texttest_ext), /* buffer len */
									  &cbParam1		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* UPDATE pre_enc_text SET c1 */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "UPDATE pre_enc_text SET c11=? WHERE id=2 OR id=3", SQL_NTS);
			CHECK_STMT_RESULT(rc, "UPDATE pre_enc_text SET c1 failed", hstmt);

			/* bind param  */
				texttest_ext = malloc(9001*sizeof(char));
				strcpy(texttest_ext,onebyte);
				for (i=1; i<9000;i++)
					strcat(texttest_ext,onebyte);
				cbParam1 = 8999;

				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_CHAR,	/* value type */
									  SQL_CHAR,	/* param type */
									  sizeof(texttest_ext), /* column size */
									  0,			/* dec digits */
									  texttest_ext,	/* param value ptr */
									  sizeof(texttest_ext), /* buffer len */
									  &cbParam1		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);

				/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			break;
	}

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* SELECT * FROM pre_enc_text */
	sql = "SELECT * FROM pre_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* select c1=c11 */
	sql = "SELECT (c1=c11)::text FROM pre_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	
	/* DELETE pre_enc_text */
	printf("\nDELETE_TEST");

	switch (data_kind)
	{
		case BASIC_DATA: 
			/* DELETE FROM pre_enc_text */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "DELETE FROM pre_enc_text WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

				/* bind param  */
				param1 = "enc";
				cbParam1 = 8;
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_CHAR,	/* value type */
									  SQL_CHAR,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  param1,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam1		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			break;
			
		case TOAST_DATA: 
			/* DELETE FROM pre_enc_text */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "DELETE FROM pre_enc_text WHERE c1=? AND id=3", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

			/* bind param  */
				const char* onebyte = "b";
				texttest_ext = malloc(9001*sizeof(char));
				strcpy(texttest_ext,onebyte);
				for (i=1; i<9000;i++)
					strcat(texttest_ext,onebyte);
				cbParam1 = 8999;

				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_CHAR,	/* value type */
									  SQL_CHAR,	/* param type */
									  sizeof(texttest_ext), /* column size */
									  0,			/* dec digits */
									  texttest_ext,	/* param value ptr */
									  sizeof(texttest_ext), /* buffer len */
									  &cbParam1		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			break;			
	}

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* SELECT * FROM pre_enc_text */
	sql = "SELECT * FROM pre_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* select c1=c11 */
	sql = "SELECT (c1=c11)::text FROM pre_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	
	/* SELECT_TEST */
	printf("\nSELECT_TEST");

	switch (data_kind)
	{
		case BASIC_DATA: 
			/* SELECT c1=c11 FROM pre_enc_text */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT (c1=c11)::text FROM pre_enc_text WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

				/* bind param  */
				param1 = "foo";
				cbParam1 = 8;
				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_CHAR,	/* value type */
									  SQL_CHAR,	/* param type */
									  20,			/* column size */
									  0,			/* dec digits */
									  param1,		/* param value ptr */
									  0,			/* buffer len */
									  &cbParam1		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			printf("\n");
			printf("SELECT (c1=c11)::text FROM pre_enc_text WHERE c1");
			printf("\n");
			print_result(hstmt);
			break;
			
		case TOAST_DATA: 
			/* SELECT c1=c11 FROM pre_enc_text */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT (c1=c11)::text FROM pre_enc_text WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

				const char* onebyte = "a";
				texttest_ext = malloc(9001*sizeof(char));
				strcpy(texttest_ext,onebyte);
				for (i=1; i<9000;i++)
					strcat(texttest_ext,onebyte);
				cbParam1 = 8999;

				rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
									  SQL_C_CHAR,	/* value type */
									  SQL_CHAR,	/* param type */
									  sizeof(texttest_ext), /* column size */
									  0,			/* dec digits */
									  texttest_ext,	/* param value ptr */
									  sizeof(texttest_ext), /* buffer len */
									  &cbParam1		/* StrLen_or_IndPtr */);
				CHECK_STMT_RESULT(rc, "\nSQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			printf("\n");
			printf("SELECT (c1=c11)::text FROM pre_enc_text WHERE c1");
			printf("\n");
			print_result(hstmt);
			break;

		default:
			break;		
			}

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* SELECT * FROM pre_enc_text */
	sql = "SELECT * FROM pre_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* select c1=c11 */
	sql = "SELECT (c1=c11)::text FROM pre_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* select c1=c11 */
	sql = "SELECT (c1=c11)::text,c1=c11 FROM pre_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* create pre_text_tbl */
	sql = "CREATE TABLE pre_text_tbl (id int4, c1 text)";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while create pre_text_tbl table", hstmt);	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* insert text_tbl */
	sql = "INSERT INTO pre_text_tbl(id, c1) SELECT id, c1 FROM pre_enc_text";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while insert text_tbl", hstmt);	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* SELECT * FROM pre_text_tbl */
	sql = "SELECT * FROM pre_text_tbl";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* select e.c1=t.c1 */
	sql = "SELECT (e.c1=t.c1)::text FROM pre_enc_text e NATURAL JOIN pre_text_tbl t";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while select e.c1=t.t1", hstmt);
	printf("\n");
	printf(sql);
	printf("\n");
	print_result(hstmt);
	
	printf("\nPREPARE_TEST_COMPLETE\n");
	printf("\n----------------------------------------");

	/* DIRECT TEST */
	printf("\nSQLDirect_TEST_START\n");

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* create dir_enc_text */
	sql = "CREATE TABLE dir_enc_text (id int4, c1 encrypt_text, c11 text)";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while create dir_enc_text table", hstmt);	
	
	/*pgtde_begin_session*/
	pgtde_begin_session(hstmt);


	/* INSERT_TEST */
	printf("\nINSERT_TEST");

	/* INSERT INTO dir_enc_text */
	switch (data_kind)
	{
		case BASIC_DATA: 
			sql = "INSERT INTO dir_enc_text VALUES (0, 'foo', 'foo'), (1, 'bar', 'bar')";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_text", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
		
		case NULL_DATA: 
			sql = "INSERT INTO dir_enc_text VALUES (0, 'foo', 'foo'), (1, NULL, NULL), (2, NULL, NULL)";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_text", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;

		case TOAST_DATA:
			sql = "INSERT INTO dir_enc_text SELECT * FROM pre_enc_text where id=0 OR id=1";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_text", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
			
		default:
			break;		
	}
	/* SELECT * FROM dir_enc_text */
	sql = "SELECT * FROM dir_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* select c1=c11 */
	sql = "SELECT (c1=c11)::text FROM dir_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	
	/* UPDATE_TEST */
	printf("\nUPDATE_TEST");

	/* UPDATE dir_enc_text SET c1 */
	switch (data_kind)
	{
		case BASIC_DATA: 
			sql = "UPDATE dir_enc_text SET c1='enc' WHERE c1='bar'";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_text SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* UPDATE dir_enc_text SET c11 */
			sql = "UPDATE dir_enc_text SET c11='enc' WHERE c11='bar'";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_text SET c11", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
	
		case NULL_DATA: 
			sql = "UPDATE dir_enc_text SET c1='enc' WHERE c1 is NULL AND id=2";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_text SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* UPDATE dir_enc_text SET c11 */
			sql = "UPDATE dir_enc_text SET c11='enc' WHERE c11 Is NULL AND id=2";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_text SET c11", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* SELECT * FROM dir_enc_text */
			sql = "SELECT * FROM dir_enc_text";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* select c1=c11 */
			sql = "SELECT (c1=c11)::text FROM dir_enc_text";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			sql = "UPDATE dir_enc_text SET c1=NULL WHERE c1='enc' AND id=2";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_text SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* UPDATE dir_enc_text SET c11 */
			sql = "UPDATE dir_enc_text SET c11=NULL WHERE c11='enc' AND id=2";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_text SET c11", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			/* SELECT * FROM dir_enc_text */
			sql = "SELECT * FROM dir_enc_text";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* select c1=c11 */
			sql = "SELECT (c1=c11)::text FROM dir_enc_text";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;

		case TOAST_DATA:
			sql = "UPDATE dir_enc_text SET c1=(SELECT c1 FROM pre_enc_text WHERE id=2) WHERE id=1";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_text", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			sql = "UPDATE dir_enc_text SET c11=(SELECT c1 FROM pre_enc_text WHERE id=2) WHERE id=1";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_text SET c11", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
			
		default:
			break;		
			
	}

	/* SELECT * FROM dir_enc_text */
	sql = "SELECT * FROM dir_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* select c1=c11 */
	sql = "SELECT (c1=c11)::text FROM dir_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);


	/* DELETE_TEST */
	printf("\nDELETE_TEST");

	/* DELETE FROM dir_enc_text WHERE c1 */
	switch (data_kind)
	{
		case BASIC_DATA: 
			sql = "DELETE FROM dir_enc_text WHERE c1 = 'enc'";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while DELETE FROM dir_enc_text WHERE c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;

		case NULL_DATA: 
			sql = "DELETE FROM dir_enc_text WHERE c1 IS NULL and id = 2";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while DELETE FROM dir_enc_text WHERE c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;

		case TOAST_DATA:
			sql = "DELETE FROM dir_enc_text WHERE id=1";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while DELETE FROM dir_enc_text WHERE c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
			
		default:
			break;
	}	

	/* SELECT * FROM dir_enc_text */
	sql = "SELECT * FROM dir_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* select c1=c11 */
	sql = "SELECT (c1=c11)::text FROM dir_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT_TEST */
	printf("\nSELECT_TEST\n");

	switch (data_kind)
	{
		case BASIC_DATA: 
			sql = "SELECT * FROM dir_enc_text";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;

		case NULL_DATA: 
			sql = "SELECT * FROM dir_enc_text WHERE c1 IS NULL";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;

		case TOAST_DATA:
			sql = "SELECT * FROM dir_enc_text";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
			
		default:
			break;
	}	

	/* SELECT * FROM dir_enc_text */
	sql = "SELECT * FROM dir_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* select c1=c11 */
	sql = "SELECT (c1=c11)::text FROM dir_enc_text";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* create dir_text_tbl */
	sql = "CREATE TABLE dir_text_tbl (id int4, c1 text)";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while create dir_text_tbl table", hstmt);	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* insert dir_enc_text */
	sql = "INSERT INTO dir_text_tbl(id, c1) SELECT id, c1 FROM dir_enc_text";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while insert dir_enc_text", hstmt);	

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM dir_text_tbl */
	sql = "SELECT * FROM dir_text_tbl";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* select d.c1=t.c1 */
	sql = "SELECT (d.c1=t.c1)::text FROM dir_enc_text d NATURAL JOIN dir_text_tbl t";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* select p.c1=d.c1 */
	sql = "SELECT (p.c1=d.c1)::text FROM dir_enc_text d NATURAL JOIN pre_enc_text p";
	test_execute_sql_and_print(sql);

	printf("\nDIRECT_TEST_COMPLETE\n");
	printf("----------------------------------------\n");

	rc = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("SQLFreeHandle failed", SQL_HANDLE_STMT, hstmt);
		exit(1);
	}
}	

/*
 * Test TDEforPG for ENCRYPT_NUMERIC_TEST.
 */
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

static void encrypt_numeric_crud(SQLSMALLINT data_kind){
	SQLRETURN	rc;
	HSTMT		hstmt = SQL_NULL_HSTMT;
	SQL_NUMERIC_STRUCT numericparam;
	SQLLEN		cbParam1, cbParam2;
	SQLCHAR *sql;
	
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("failed to allocate stmt handle", SQL_HANDLE_DBC, conn);
		exit(1);
	}

	/**** Test PREPARE_encrypt_numeric (SQL_C_NUMERIC) ****/

	/* PREPARE TEST */
	printf("\nSQLPrepare_TEST_START\n");

	/*pgtde_begin_session*/
	pgtde_begin_session(hstmt);

	/* drop table  pre_enc_bytea */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "DROP TABLE IF EXISTS pre_enc_num,pre_num_tbl,dir_enc_num,dir_num_tbl", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while dropping temp table", hstmt);

	/* CREATE TABLE pre_enc_num */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "CREATE TABLE pre_enc_num (id int4, c1 encrypt_numeric, c11 numeric)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while CREATE TABLE num_tbl", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* INSERT INTO pre_enc_num */
	printf("\nINSERT_TEST");

	/* INSERT INTO pre_enc_num */
	rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_num VALUES(1, ?, ?)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);
	
	switch (data_kind)
	{
		case BASIC_DATA:	
			/* 24197857161011715162171839636988778104 */
			build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
			cbParam1 = sizeof(numericparam);
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_NUMERIC,	/* value type */
								  SQL_NUMERIC,	/* param type */
								  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
								  0,			/* dec digits */
								  &numericparam, /* param value ptr */
								  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
								  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* 24197857161011715162171839636988778104 */
			build_numeric_struct(&numericparam, 2, "78563412 78563412 78563412 78563412", 41, 0);

			cbParam2 = sizeof(numericparam);
			rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
								  SQL_C_NUMERIC,	/* value type */
								  SQL_NUMERIC,	/* param type */
								  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
								  0,			/* dec digits */
								  &numericparam, /* param value ptr */
								  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
								  &cbParam2 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			if (!SQL_SUCCEEDED(rc))
			{
				print_diag("SQLExecute failed", SQL_HANDLE_STMT, hstmt);
			}
			
			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_num VALUES(2, ?, ?)", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);
		
			/* 12345678901234567890123456789012345678 */
			build_numeric_struct(&numericparam, 1, "4EF338DE509049C4133302F0F6B04909", 38, 0);
			
			cbParam1 = sizeof(numericparam);
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_NUMERIC,	/* value type */
								  SQL_NUMERIC,	/* param type */
								  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
								  0,			/* dec digits */
								  &numericparam, /* param value ptr */
								  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
								  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* 12345678901234567890123456789012345678 */
			build_numeric_struct(&numericparam, 2, "4EF338DE509049C4133302F0F6B04909", 38, 0);

			cbParam2 = sizeof(numericparam);
			rc = SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT,
								  SQL_C_NUMERIC,	/* value type */
								  SQL_NUMERIC,	/* param type */
								  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
								  0,			/* dec digits */
								  &numericparam, /* param value ptr */
								  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
								  &cbParam2 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			if (!SQL_SUCCEEDED(rc))
			{
				print_diag("SQLExecute failed", SQL_HANDLE_STMT, hstmt);
			}
			
			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);		
			break;
			
		default:
			break;
			
	}

	/* SELECT * FROM pre_enc_num */
	sql = "SELECT * FROM pre_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* SELECT *c1=c11 */
	sql = "SELECT (c1=c11)::text FROM pre_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	
	/* UPDATE_TEST */
	printf("\nUPDATE_TEST");

	/* UPDATE pre_enc_num SET c1 */
	switch (data_kind)
	{
		case BASIC_DATA:
			rc = SQLPrepare(hstmt, (SQLCHAR *) "UPDATE pre_enc_num SET c1=? WHERE c1=12345678901234567890123456789012345678", SQL_NTS);
			CHECK_STMT_RESULT(rc, "UPDATE pre_enc_num SET c1 failed", hstmt);

			/* 25.212 (per Microsoft KB 22831) */
			build_numeric_struct(&numericparam, 1, "7C62", 5, 3);
			
			cbParam1 = sizeof(numericparam);
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_NUMERIC,	/* value type */
								  SQL_NUMERIC,	/* param type */
								  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
								  0,			/* dec digits */
								  &numericparam, /* param value ptr */
								  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
								  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			if (!SQL_SUCCEEDED(rc))
			{
				print_diag("SQLExecute failed", SQL_HANDLE_STMT, hstmt);
		    }
			
			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* UPDATE pre_enc_num SET c11 */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "UPDATE pre_enc_num SET c11=? WHERE c11=12345678901234567890123456789012345678", SQL_NTS);
			CHECK_STMT_RESULT(rc, "UPDATE pre_enc_num SET c1 failed", hstmt);

			/* 25.212 (per Microsoft KB 22831) */
			build_numeric_struct(&numericparam, 1, "7C62", 5, 3);
			
			cbParam1 = sizeof(numericparam);
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_NUMERIC,	/* value type */
								  SQL_NUMERIC,	/* param type */
								  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
								  0,			/* dec digits */
								  &numericparam, /* param value ptr */
								  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
								  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			if (!SQL_SUCCEEDED(rc))
			{
				print_diag("SQLExecute failed", SQL_HANDLE_STMT, hstmt);
		    }
			
			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;

		default:
			break;			
	}

	/* SELECT * FROM pre_enc_num */
	sql = "SELECT * FROM pre_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* SELECT *c1=c11 */
	sql = "SELECT (c1=c11)::text FROM pre_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	
	/* DELETE_TEST */
	printf("\nDELETE_TEST");

	/* DELETE FROM pre_enc_num WHERE c1 */
	switch (data_kind)
	{
		case BASIC_DATA: 
			rc = SQLPrepare(hstmt, (SQLCHAR *) "DELETE FROM pre_enc_num WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "DELETE FROM pre_enc_num WHERE c1 failed", hstmt);
			
			/* 25.212 (per Microsoft KB 22831) */
			build_numeric_struct(&numericparam, 1, "7C62", 5, 3);
			
			cbParam1 = sizeof(numericparam);
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_NUMERIC,	/* value type */
								  SQL_NUMERIC,	/* param type */
								  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
								  0,			/* dec digits */
								  &numericparam, /* param value ptr */
								  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
								  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			if (!SQL_SUCCEEDED(rc))
			{
				print_diag("SQLExecute failed", SQL_HANDLE_STMT, hstmt);
		    }
			
			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;

		default:
			break;
			

	}
	/* SELECT * FROM pre_enc_num */
	sql = "SELECT * FROM pre_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* SELECT *c1=c11 */
	sql = "SELECT (c1=c11)::text FROM pre_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	
	/* SELECT_TEST */
	printf("\nSELECT_TEST");

	switch (data_kind)
	{
		case BASIC_DATA:
			/* SELECT c1=c11 FROM pre_enc_num */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT (c1=c11)::text FROM pre_enc_num WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SELECT c1=c11 FROM pre_enc_num WHERE c1 failed", hstmt);
			
			/* 24197857161011715162171839636988778104 */
			build_numeric_struct(&numericparam, 1, "78563412 78563412 78563412 78563412", 41, 0);
			
			cbParam1 = sizeof(numericparam);
			rc = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								  SQL_C_NUMERIC,	/* value type */
								  SQL_NUMERIC,	/* param type */
								  0,			/* column size (ignored for SQL_INTERVAL_SECOND) */
								  0,			/* dec digits */
								  &numericparam, /* param value ptr */
								  sizeof(numericparam), /* buffer len (ignored for SQL_C_INTERVAL_SECOND) */
								  &cbParam1 /* StrLen_or_IndPtr (ignored for SQL_C_INTERVAL_SECOND) */);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			if (!SQL_SUCCEEDED(rc))
			{
				print_diag("SQLExecute failed", SQL_HANDLE_STMT, hstmt);
		    }
			printf("\n");
			printf(sql);
			printf("\n");
			print_result(hstmt);
			
			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
			
		default:
			break;
	}	

	/* SELECT * FROM pre_enc_num */
	sql = "SELECT * FROM pre_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* SELECT *c1=c11 */
	sql = "SELECT c1=c11 FROM pre_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

	/* select c1=c11 */
	sql = "SELECT (c1=c11)::text,c1=c11 FROM pre_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* CREATE TABLE pre_enc_num */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "CREATE TABLE pre_num_tbl (id int4, c1 numeric)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while CREATE TABLE num_tbl", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* insert pre_num_tbl */
	sql = "INSERT INTO pre_num_tbl(id, c1) SELECT id, c1 FROM pre_enc_num";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while insert pre_num_tbl", hstmt);	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM pre_num_tbl */
	sql = "SELECT * FROM pre_num_tbl";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* select e.c1=t.c1 */
	sql = "SELECT (e.c1=t.c1)::text FROM pre_enc_num e NATURAL JOIN pre_num_tbl t";
	test_execute_sql_and_print(sql);


	printf("\nPREPARE_TEST_COMPLETE\n");
	printf("\n----------------------------------------");
	
	/* DIRECT TEST */
	printf("\nDIRECT_TEST_START\n");

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* CREATE TABLE pre_enc_num */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "CREATE TABLE dir_enc_num (id int4, c1 encrypt_numeric, c11 numeric)", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while CREATE TABLE num_tbl", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/*pgtde_begin_session*/
	pgtde_begin_session(hstmt);

	/* INSERT_TEST */
	printf("\nINSERT_TEST");

	switch (data_kind)
	{
		case BASIC_DATA:
			/* INSERT INTO dir_enc_num */
			sql = "INSERT INTO dir_enc_num VALUES (1, 24197857161011715162171839636988778104, 24197857161011715162171839636988778104)";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_num table", hstmt);	

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* INSERT INTO dir_enc_num */
			sql = "INSERT INTO dir_enc_num VALUES (2, 12345678901234567890123456789012345678, 12345678901234567890123456789012345678)";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_num table", hstmt);	

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
			break;

		case NULL_DATA:
			/* INSERT INTO dir_enc_num */
			sql = "INSERT INTO dir_enc_num VALUES (0, 24197857161011715162171839636988778104, 24197857161011715162171839636988778104)";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_num table", hstmt);	

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* INSERT INTO dir_enc_num */
			sql = "INSERT INTO dir_enc_num VALUES (1, NULL, NULL), (2, NULL, NULL)";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_num table", hstmt);	

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
			break;
			
		default:
			break;
	}		

	/* SELECT c1=c11 */
	sql = "SELECT * FROM dir_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT c1=c11 */
	sql = "SELECT (c1=c11)::text FROM dir_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	
	/* UPDATE_TEST */
	printf("\nUPDATE_TEST");

	switch (data_kind)
	{
		case BASIC_DATA:
			/* UPDATE dir_enc_num SET c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_num SET c1=25.212 WHERE c1=12345678901234567890123456789012345678", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* UPDATE dir_enc_num SET c11 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_num SET c11=25.212 WHERE c11=12345678901234567890123456789012345678", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c11", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
			break;

		case NULL_DATA:
			/* UPDATE dir_enc_num SET c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_num SET c1=24197857161011715162171839636988778104 WHERE c1 IS NULL AND id=2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* UPDATE dir_enc_num SET c11 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_num SET c11=24197857161011715162171839636988778104 WHERE c11 IS NULL AND id=2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* SELECT c1=c11 */
			sql = "SELECT * FROM dir_enc_num";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* SELECT c1=c11 */
			sql = "SELECT (c1=c11)::text FROM dir_enc_num";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* UPDATE dir_enc_num SET c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_num SET c1=NULL WHERE c1=24197857161011715162171839636988778104 AND id=2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* UPDATE dir_enc_num SET c11 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_num SET c11=NULL WHERE c11=24197857161011715162171839636988778104 AND id=2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE dir_enc_bytea SET c11", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
			/* SELECT c1=c11 */
			sql = "SELECT * FROM dir_enc_num";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* SELECT c1=c11 */
			sql = "SELECT (c1=c11)::text FROM dir_enc_num";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
			
		default:
			break;		
	}
	
	/* SELECT c1=c11 */
	sql = "SELECT * FROM dir_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT c1=c11 */
	sql = "SELECT (c1=c11)::text FROM dir_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	
	/* DELETE_TEST */
	printf("\nDELETE_TEST");

	switch (data_kind)
	{
		case BASIC_DATA:
			/* DELETE FROM dir_enc_num WHERE c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "DELETE FROM dir_enc_num WHERE c1=25.212", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while DELETE FROM dir_enc_num WHERE c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
		
		case NULL_DATA:
			/* DELETE FROM dir_enc_num WHERE c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "DELETE FROM dir_enc_num WHERE c1 IS NULL and id=2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while DELETE FROM dir_enc_num WHERE c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
			
		default:
			break;
	}
	
	/* SELECT c1=c11 */
	sql = "SELECT * FROM dir_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT c1=c11 */
	sql = "SELECT (c1=c11)::text FROM dir_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	
	/* SELECT_TEST */
	printf("\nSELECT_TEST");

	switch (data_kind)
	{
		case BASIC_DATA:
			/* SELECT * FROM dir_enc_num WHERE c1 */
			sql = "SELECT * FROM dir_enc_num";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
		
		case NULL_DATA:
			/* SELECT * FROM dir_enc_num WHERE c1 IS NULL and id = 1 */
			sql = "SELECT * FROM dir_enc_num WHERE c1 IS NULL and id = 1";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
			
		default:
			break;
	}
	
	/* SELECT c1=c11 */
	sql = "SELECT * FROM dir_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT c1=c11 */
	sql = "SELECT (c1=c11)::text FROM dir_enc_num";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* create dir_num_tbl */
	sql = "CREATE TABLE dir_num_tbl (id int4, c1 numeric)";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while create dir_num_tbl table", hstmt);	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* INSERT INTO dir_num_tbl */
	sql = "INSERT INTO dir_num_tbl(id, c1) SELECT id, c1 FROM dir_enc_num";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_num_tbl table", hstmt);	
		
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM dir_num_tbl */
	sql = "SELECT * FROM dir_num_tbl";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* select e.c1=t.c1 */
	sql = "SELECT (e.c1=t.c1)::text FROM dir_enc_num e NATURAL JOIN dir_num_tbl t";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* select p.c1=d.c1 */
	sql = "SELECT (p.c1=d.c1)::text FROM dir_enc_num d NATURAL JOIN pre_enc_num p";
	test_execute_sql_and_print(sql);

	printf("\nDIRECT_TEST_COMPLETE\n");
	printf("----------------------------------------\n");

	rc = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("SQLFreeHandle failed", SQL_HANDLE_STMT, hstmt);
		exit(1);
	}
}	

/*
 * Test TDEforPG for ENCRYPT_TIMESTAMP_TEST.
 */
static void encrypt_timestamp_crud(SQLSMALLINT data_kind){
	SQLRETURN rc;
	HSTMT hstmt = SQL_NULL_HSTMT;
	SQLSMALLINT colcount1;
	SQLCHAR *sql;
	int i;

	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("failed to allocate stmt handle", SQL_HANDLE_DBC, conn);
		exit(1);
	}

	/* PREPARE TEST */
	printf("\nSQLPrepare_TEST_START\n");

	/* drop table  pre_enc_bytea */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "DROP TABLE IF EXISTS pre_enc_timestamp,pre_timestamp_tbl,dir_enc_timestamp,dir_timestamp_tbl", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while dropping temp table", hstmt);

	/* create pre_enc_timestamp */
	sql = "CREATE TABLE pre_enc_timestamp (id int4, c1 encrypt_timestamp, c11 timestamp)";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while create pre_enc_timestamp table", hstmt);	
	
	/*pgtde_begin_session*/
	pgtde_begin_session(hstmt);

	printf("\nINSERT_TEST");
	switch (data_kind)
	{
		case BASIC_DATA:
			for (i = 0; i < 5; i++)
			{
				/* Prepare a statement */
				rc = SQLPrepare(hstmt, (SQLCHAR *) "INSERT INTO pre_enc_timestamp VALUES (1, ?, ?) RETURNING (c1)", SQL_NTS);
				CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

				/* bind param  */
				SQLLEN cbParam1 = 0;
				TIMESTAMP_STRUCT param1;
				param1.year = 2017;
				param1.month = 7;
				param1.day = 7 + i;
				param1.hour = 5;
				param1.minute = 50;
				param1.second = 30;
				param1.fraction = 10;

				SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
								 SQL_C_TIMESTAMP,
								 SQL_TIMESTAMP,
								 sizeof(SQL_TIMESTAMP_STRUCT),
								 0,
								 &param1,
								 sizeof(SQL_TIMESTAMP_STRUCT),
								 &cbParam1);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

				SQLLEN cbParam2 = 0;
				TIMESTAMP_STRUCT param2;
				param2.year = 2017;
				param2.month = 7;
				param2.day = 7 + i;
				param2.hour = 5;
				param2.minute = 50;
				param2.second = 30;
				param2.fraction = 10;

				SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, 
								 SQL_C_TIMESTAMP,
								 SQL_TIMESTAMP,
								 sizeof(SQL_TIMESTAMP_STRUCT),
								 0,
								 &param2,
								 sizeof(SQL_TIMESTAMP_STRUCT),
								 &cbParam2);
				CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

				/* Test SQLNumResultCols, called before SQLExecute() */
				rc = SQLNumResultCols(hstmt, &colcount1);
				CHECK_STMT_RESULT(rc, "SQLNumResultCols failed", hstmt);

				/* Execute */
				rc = SQLExecute(hstmt);
				CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);

				/* hstmt FREE */
				rc = SQLFreeStmt(hstmt, SQL_CLOSE);
				CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
			}
			break;
			
		default:
			break;
			
	}
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT * FROM pre_enc_timestamp */
	sql = "SELECT * FROM pre_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT c1=c11 */
	sql = "SELECT (c1=c11)::text FROM pre_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);


	/* UPDATE_TEST */
	/* UPDATE pre_enc_timestamp SET c1 */
	printf("\nUPDATE_TEST");

	switch (data_kind)
	{
		case BASIC_DATA: 
			rc = SQLPrepare(hstmt, (SQLCHAR *) "UPDATE pre_enc_timestamp SET c1=? WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

			/* bind param  */
			SQLLEN cbParam1 = 0;
			TIMESTAMP_STRUCT param1;
			param1.year = 2017;
			param1.month = 7;
			param1.day = 12;
			param1.hour = 5;
			param1.minute = 50;
			param1.second = 30;
			param1.fraction = 10;

			SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							 SQL_C_TIMESTAMP,
							 SQL_TIMESTAMP,
							 sizeof(SQL_TIMESTAMP_STRUCT),
							 0,
							 &param1,
							 sizeof(SQL_TIMESTAMP_STRUCT),
							 &cbParam1);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			SQLLEN cbParam2 = 0;
			TIMESTAMP_STRUCT param2;
			param2.year = 2017;
			param2.month = 7;
			param2.day = 11;
			param2.hour = 5;
			param2.minute = 50;
			param2.second = 30;
			param2.fraction = 10;

			SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, 
							 SQL_C_TIMESTAMP,
							 SQL_TIMESTAMP,
							 sizeof(SQL_TIMESTAMP_STRUCT),
							 0,
							 &param2,
							 sizeof(SQL_TIMESTAMP_STRUCT),
							 &cbParam2);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* UPDATE pre_enc_timestamp SET c11 */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "UPDATE pre_enc_timestamp SET c11=? WHERE c11=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLPrepare failed", hstmt);

			/* bind param  */
			param1.year = 2017;
			param1.month = 7;
			param1.day = 12;
			param1.hour = 5;
			param1.minute = 50;
			param1.second = 30;
			param1.fraction = 10;

			SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							 SQL_C_TIMESTAMP,
							 SQL_TIMESTAMP,
							 sizeof(SQL_TIMESTAMP_STRUCT),
							 0,
							 &param1,
							 sizeof(SQL_TIMESTAMP_STRUCT),
							 &cbParam1);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			param2.year = 2017;
			param2.month = 7;
			param2.day = 11;
			param2.hour = 5;
			param2.minute = 50;
			param2.second = 30;
			param2.fraction = 10;

			SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, 
							 SQL_C_TIMESTAMP,
							 SQL_TIMESTAMP,
							 sizeof(SQL_TIMESTAMP_STRUCT),
							 0,
							 &param2,
							 sizeof(SQL_TIMESTAMP_STRUCT),
							 &cbParam2);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
		
		default:
			break;
	}
		
	/* SELECT * FROM pre_enc_timestamp */
	sql = "SELECT * FROM pre_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT c1=c11 */
	sql = "SELECT (c1=c11)::text FROM pre_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	
	/* DELETE_TEST */
	printf("\nDELETE_TEST");

	/* DELETE FROM pre_enc_timestamp WHERE c1 */
	switch (data_kind)
	{
		case BASIC_DATA: 
			rc = SQLPrepare(hstmt, (SQLCHAR *) "DELETE FROM pre_enc_timestamp WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "DELETE FROM pre_enc_timestamp WHERE c1 failed", hstmt);

			/* bind param  */
			SQLLEN cbParam1 = 0;
			TIMESTAMP_STRUCT param1;
			param1.year = 2017;
			param1.month = 7;
			param1.day = 12;
			param1.hour = 5;
			param1.minute = 50;
			param1.second = 30;
			param1.fraction = 10;

			SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							 SQL_C_TIMESTAMP,
							 SQL_TIMESTAMP,
							 sizeof(SQL_TIMESTAMP_STRUCT),
							 0,
							 &param1,
							 sizeof(SQL_TIMESTAMP_STRUCT),
							 &cbParam1);
			CHECK_STMT_RESULT(rc, "SQLBindParameter failed", hstmt);

			/* Execute */
			rc = SQLExecute(hstmt);
			CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
			break;

		case NULL_DATA: 
			rc = SQLPrepare(hstmt, (SQLCHAR *) "DELETE FROM pre_enc_timestamp WHERE c1=? and id = 2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "DELETE FROM pre_enc_timestamp WHERE c1 failed", hstmt);

			default:
				break;
	}
	
	/* SELECT * FROM pre_enc_timestamp */
	sql = "SELECT * FROM pre_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT c1=c11 */
	sql = "SELECT (c1=c11)::text FROM pre_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	
	/* SELECT_TEST */
	printf("\nSELECT_TEST");

	switch (data_kind)
	{
		case BASIC_DATA:
			/* SELECT c1=c11 FROM pre_enc_timestamp */
			rc = SQLPrepare(hstmt, (SQLCHAR *) "SELECT (c1=c11)::text FROM pre_enc_timestamp WHERE c1=?", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SELECT c1=c11 FROM pre_enc_timestamp WHERE c1 failed", hstmt);

			/* bind param  */
			SQLLEN cbParam1 = 0;
			TIMESTAMP_STRUCT param1;
			param1.year = 2017;
			param1.month = 7;
			param1.day = 10;
			param1.hour = 5;
			param1.minute = 50;
			param1.second = 30;
			param1.fraction = 10;

			SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
							 SQL_C_TIMESTAMP,
							 SQL_TIMESTAMP,
							 sizeof(SQL_TIMESTAMP_STRUCT),
							 0,
							 &param1,
							 sizeof(SQL_TIMESTAMP_STRUCT),
							 &cbParam1);
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
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
			break;
		
			default:
				break;
	}
		
	/* SELECT * FROM pre_enc_timestamp */
	sql = "SELECT * FROM pre_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT c1=c11 */
	sql = "SELECT (c1=c11)::text FROM pre_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* select c1=c11 */
	sql = "SELECT (c1=c11)::text,c1=c11 FROM pre_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* create text_tbl */
	sql = "CREATE TABLE pre_timestamp_tbl (id int4, c1 timestamp)";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while create timestamp_tbl table", hstmt);	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* INSERT INTO timestamp */
	sql = "INSERT INTO pre_timestamp_tbl(id, c1) SELECT id, c1 FROM pre_enc_timestamp";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO timestamp table", hstmt);	
	
	/* select e.c1=t.c1 */
	sql = "SELECT * FROM pre_timestamp_tbl";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while select e.c1=t.c1", hstmt);
	printf("\n");
	printf(sql);
	printf("\n");
	print_result(hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* select e.c1=t.c1 */
	sql = "SELECT (e.c1=t.c1)::text FROM pre_enc_timestamp e NATURAL JOIN pre_timestamp_tbl t";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while select e.c1=t.c1", hstmt);
	printf("\n");
	printf(sql);
	printf("\n");
	print_result(hstmt);

	printf("\nPREPARE_TEST_COMPLETE\n");
	printf("\n----------------------------------------");
	
	/* DIRECT TEST */
	printf("\nDIRECT_TEST_START\n");
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* create dir_enc_timestamp */
	sql = "CREATE TABLE dir_enc_timestamp (id int4, c1 encrypt_timestamp, c11 timestamp)";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while create dir_enc_timestamp table", hstmt);	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);


	printf("\nINSERT_TEST");
	
	switch (data_kind)
	{
		case BASIC_DATA:	
			/* INSERT INTO timestamp */
			sql = "INSERT INTO dir_enc_timestamp VALUES (1, '2017-07-07 05:50:30', '2017-07-07 05:50:30')";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_timestamp table", hstmt);	

			/* Test SQLNumResultCols, called before SQLExecute() */
			rc = SQLNumResultCols(hstmt, &colcount1);
			CHECK_STMT_RESULT(rc, "SQLNumResultCols failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* INSERT INTO timestamp */
			sql = "INSERT INTO dir_enc_timestamp VALUES (1, '2017-07-08 05:50:30', '2017-07-08 05:50:30')";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_timestamp table", hstmt);	

			/* Test SQLNumResultCols, called before SQLExecute() */
			rc = SQLNumResultCols(hstmt, &colcount1);
			CHECK_STMT_RESULT(rc, "SQLNumResultCols failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
			break;
		
		case NULL_DATA:	
			/* INSERT INTO timestamp */
			sql = "INSERT INTO dir_enc_timestamp VALUES (0, '2017-07-07 05:50:30', '2017-07-07 05:50:30')";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_timestamp table", hstmt);	

			/* Test SQLNumResultCols, called before SQLExecute() */
			rc = SQLNumResultCols(hstmt, &colcount1);
			CHECK_STMT_RESULT(rc, "SQLNumResultCols failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);

			/* INSERT INTO timestamp */
			sql = "INSERT INTO dir_enc_timestamp VALUES (1, NULL, NULL), (2, NULL, NULL)";
			rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_enc_timestamp table", hstmt);	

			/* Test SQLNumResultCols, called before SQLExecute() */
			rc = SQLNumResultCols(hstmt, &colcount1);
			CHECK_STMT_RESULT(rc, "SQLNumResultCols failed", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			CHECK_STMT_RESULT(rc, "SQLFreeStmt failed", hstmt);
			break;
		
	case TOAST_DATA:

				/* Execute */
				rc = SQLExecute(hstmt);
				CHECK_STMT_RESULT(rc, "SQLExecute failed", hstmt);
			break;
			
		default:
			break;
	
	}
	
	/* SELECT * FROM dir_enc_timestamp */
	sql = "SELECT * FROM dir_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT c1=c11 */
	sql = "SELECT (c1=c11)::text FROM dir_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	
	/* UPDATE_TEST */
	printf("\nUPDATE_TEST");
	
	switch (data_kind)
	{
		case BASIC_DATA:
			/* UPDATE dir_enc_timestamp SET c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_timestamp SET c1='2017-07-12 05:50:30' WHERE c1='2017-07-08 05:50:30'", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE pre_enc_timestamp SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* UPDATE pre_enc_timestamp SET c11 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_timestamp SET c11='2017-07-12 05:50:30' WHERE c11='2017-07-08 05:50:30'", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed whileUPDATE pre_enc_timestamp SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
		
		case NULL_DATA:
			/* UPDATE dir_enc_timestamp SET c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_timestamp SET c1='2017-07-07 05:50:30' WHERE c1 IS NULL AND id=2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE pre_enc_timestamp SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* UPDATE pre_enc_timestamp SET c11 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_timestamp SET c11='2017-07-07 05:50:30' WHERE c11 IS NULL AND id=2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed whileUPDATE pre_enc_timestamp SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* SELECT * FROM dir_enc_timestamp */
			sql = "SELECT * FROM dir_enc_timestamp";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* SELECT c1=c11 */
			sql = "SELECT (c1=c11)::text FROM dir_enc_timestamp";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* UPDATE dir_enc_timestamp SET c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_timestamp SET c1=NULL WHERE c1='2017-07-07 05:50:30' AND id=2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE pre_enc_timestamp SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

			/* UPDATE pre_enc_timestamp SET c11 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "UPDATE dir_enc_timestamp SET c11=NULL WHERE c11='2017-07-07 05:50:30' AND id=2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed whileUPDATE pre_enc_timestamp SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;
			
		default:
			break;
	}	

	/* SELECT * FROM dir_enc_timestamp */
	sql = "SELECT * FROM dir_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT c1=c11 */
	sql = "SELECT (c1=c11)::text FROM dir_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* DELETE_TEST */
	printf("\nDELETE_TEST");

	switch (data_kind)
	{
		case BASIC_DATA:
			/* DELETE FROM dir_enc_timestamp WHERE c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "DELETE FROM dir_enc_timestamp WHERE c1='2017-07-12 05:50:30'", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE pre_enc_timestamp SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;

		case NULL_DATA:
			/* DELETE FROM dir_enc_timestamp WHERE c1 */
			rc = SQLExecDirect(hstmt, (SQLCHAR *) "DELETE FROM dir_enc_timestamp WHERE c1 IS NULL and id = 2", SQL_NTS);
			CHECK_STMT_RESULT(rc, "SQLExecDirect failed while UPDATE pre_enc_timestamp SET c1", hstmt);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;

			default:
				break;
	}

	/* SELECT * FROM dir_enc_timestamp */
	sql = "SELECT * FROM dir_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT c1=c11 */
	sql = "SELECT (c1=c11)::text FROM dir_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			
	/* SELECT_TEST */
	printf("\nSELECT_TEST\n");

	switch (data_kind)
	{
		case BASIC_DATA:
			/* SELECT * FROM dir_enc_timestamp */
			sql = "SELECT * FROM dir_enc_timestamp";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;

		case NULL_DATA:
			/* SELECT * FROM dir_enc_timestamp WHERE c1 IS NULL*/
			sql = "SELECT * FROM dir_enc_timestamp WHERE c1 IS NULL";
			test_execute_sql_and_print(sql);

			/* hstmt FREE */
			rc = SQLFreeStmt(hstmt, SQL_CLOSE);
			rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
			break;

			default:
				break;
	}
		
	/* SELECT * FROM dir_enc_timestamp */
	sql = "SELECT * FROM dir_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* SELECT c1=c11 */
	sql = "SELECT (c1=c11)::text FROM dir_enc_timestamp";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* create dir_timestamp_tbl */
	sql = "CREATE TABLE dir_timestamp_tbl (id int4, c1 timestamp)";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while create dir_timestamp_tbl table", hstmt);	
	
	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* INSERT INTO dir_timestamp_tbl */
	sql = "INSERT INTO dir_timestamp_tbl(id, c1) SELECT id, c1 FROM dir_enc_timestamp";
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while INSERT INTO dir_timestamp_tbl table", hstmt);	
	
	/* select e.c1=t.c1 */
	sql = "SELECT * FROM dir_timestamp_tbl";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);

	/* select e.c1=t.c1 */
	sql = "SELECT (e.c1=t.c1)::text FROM dir_enc_timestamp e NATURAL JOIN dir_timestamp_tbl t";
	test_execute_sql_and_print(sql);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	
	/* select p.c1=d.c1 */
	sql = "SELECT (p.c1=d.c1)::text FROM dir_enc_timestamp d NATURAL JOIN pre_enc_timestamp p";
	test_execute_sql_and_print(sql);

	printf("\nDIRECT_TEST_COMPLETE\n");
	printf("----------------------------------------\n");

	rc = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	if (!SQL_SUCCEEDED(rc))
	{
		print_diag("SQLFreeHandle failed", SQL_HANDLE_STMT, hstmt);
		exit(1);
	}
}	

void pgtde_begin_session(HSTMT hstmt)
{
	SQLLEN rc;
	SQLCHAR *sql;

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
	
	/* cipher_key enable */
	rc = SQLExecDirect(hstmt, (SQLCHAR *) "Select cipher_key_enable_log()", SQL_NTS);
	CHECK_STMT_RESULT(rc, "SQLExecDirect failed while cipher_key enable", hstmt);

	/* hstmt FREE */
	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);	
}

void test_execute_sql_and_print(SQLCHAR* sql){
	HSTMT hstmt;
	SQLLEN rc;

	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql, SQL_NTS);
	CHECK_STMT_RESULT(rc, sql, hstmt);
	printf("\n");
	printf(sql);
	printf("\n");
	print_result(hstmt);
}

