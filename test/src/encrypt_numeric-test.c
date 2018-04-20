#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int main(int argc, char **argv)
{
	test_connect_ext("Debug=1");
//	test_connect_ext("UnknownSizes=2");
//	tdeforpg_crud(ENCRYPT_BYTEA_TEST, NULL_DATA);
	
//	tdeforpg_crud(ENCRYPT_BYTEA_TEST, BASIC_DATA);
	
//	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
	printf("\n===========ENCRYPT_NUMERIC_TEST_BASIC_DATA_START===============\n");

	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);

//	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, NULL_DATA);

//	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();

	printf("\n===========ENCRYPT_NUMERIC_TEST_BASIC_DATA_END===============\n");

	return 0;
}
