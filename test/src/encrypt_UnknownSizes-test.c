#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int main(int argc, char **argv)
{
	
	printf("\nUnknownSizes=0 TEST\n");fflush(stdout);
	printf("\n---------- UnknownSizes=0 Start ----------\n");fflush(stdout);
	test_connect_ext("UnknownSizes=0");
	
	tdeforpg_crud(ENCRYPT_BYTEA_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();
	printf("\n---------- UnknownSizes=0 End ----------\n");fflush(stdout);
	
	printf("\nUnknownSizes=0 TEST\n");fflush(stdout);
	printf("\n---------- UnknownSizes=0 Start ----------\n");fflush(stdout);
	test_connect_ext("UnknownSizes=1");
	
	tdeforpg_crud(ENCRYPT_BYTEA_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();
	printf("\n---------- UnknownSizes=0 End ----------\n");fflush(stdout);
	
	
	printf("\nUnknownSizes=0 TEST\n");fflush(stdout);
	printf("\n---------- UnknownSizes=0 Start ----------\n");fflush(stdout);
	test_connect_ext("UnknownSizes=2");
	
	tdeforpg_crud(ENCRYPT_BYTEA_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();
	printf("\n---------- UnknownSizes=0 End ----------\n");fflush(stdout);

	return 0;
}
