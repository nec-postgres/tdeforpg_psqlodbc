#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int main(int argc, char **argv)
{
	test_connect_ext("Debug=1");
//	test_connect_ext("UnknownSizes=2");
//	tdeforpg_crud(ENCRYPT_BYTEA_TEST, NULL_DATA);
	
	printf("\n===========ENCRYPT_INTEGER_TEST_BASIC_DATA_START===============\n");

	tdeforpg_crud(ENCRYPT_INTEGER_TEST, BASIC_DATA);

//	tdeforpg_crud(ENCRYPT_BYTEA_TEST, TOAST_DATA);


//	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
//	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);
	
//	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();
	// SQL_BIGINT -5
	printf("\n===========ENCRYPT_INTEGER_TEST: int8 as bigint ===============\n");
	test_connect_ext("int8_as = -5");
	tdeforpg_crud(ENCRYPT_INTEGER_TEST, BASIC_DATA);
	test_disconnect();
	
	// SQL_NUMERIC 2
	printf("\n===========ENCRYPT_INTEGER_TEST: int8 as numeric ===============\n");
	test_connect_ext("int8_as = 2");
	tdeforpg_crud(ENCRYPT_INTEGER_TEST, BASIC_DATA);
	test_disconnect();	
	
	// SQL_DOUBLE 8
	printf("\n===========ENCRYPT_INTEGER_TEST: int8 as double ===============\n");
	test_connect_ext("int8_as = 8");
	tdeforpg_crud(ENCRYPT_INTEGER_TEST, BASIC_DATA);
	test_disconnect();	
	
	// SQL_INTEGER  12
	printf("\n===========ENCRYPT_INTEGER_TEST: int8 as varchar ===============\n");
	test_connect_ext("int8_as = 12");
	tdeforpg_crud(ENCRYPT_INTEGER_TEST, BASIC_DATA);
	test_disconnect();	
	
	return 0;
}
