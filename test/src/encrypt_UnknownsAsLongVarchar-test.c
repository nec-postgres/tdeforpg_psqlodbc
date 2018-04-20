#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int main(int argc, char **argv)
{
	printf("\nUnknownsAsLongVarchar=0 TEST\n");fflush(stdout);
	printf("\n---------- UnknownsAsLongVarchar=0 Start ----------\n");fflush(stdout);
	test_connect_ext("UnknownsAsLongVarchar=0");
	
	tdeforpg_crud(ENCRYPT_BYTEA_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();
	printf("\n---------- UnknownsAsLongVarchar=0 End ----------\n");fflush(stdout);
	
	printf("\nUnknownsAsLongVarchar=1 TEST\n");fflush(stdout);
	printf("\n---------- UnknownsAsLongVarchar=1 Start ----------\n");fflush(stdout);
	test_connect_ext("UnknownsAsLongVarchar=1");
	
	tdeforpg_crud(ENCRYPT_BYTEA_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();
	printf("\n---------- UnknownsAsLongVarchar=1 End ----------\n");fflush(stdout);
	
	return 0;
}
