#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int main(int argc, char **argv)
{
	printf("\nProtocol=7.4-0 TEST\n");fflush(stdout);
	printf("\n---------- Protocol=7.4-0 Start ----------\n");fflush(stdout);
	test_connect_ext("Protocol=7.4-0");
	
	tdeforpg_crud(ENCRYPT_BYTEA_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();
	printf("\n---------- Protocol=7.4-0 End ----------\n");fflush(stdout);

	printf("\nProtocol=7.4-1 TEST\n");fflush(stdout);
	printf("\n---------- Protocol=7.4-1 Start ----------\n");fflush(stdout);
	test_connect_ext("Protocol=7.4-1");
	
	tdeforpg_crud(ENCRYPT_BYTEA_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();
	printf("\n---------- Protocol=7.4-1 End ----------\n");fflush(stdout);

	printf("\nProtocol=7.4-2 TEST\n");fflush(stdout);
	printf("\n---------- Protocol=7.4-2 Start ----------\n");fflush(stdout);
	test_connect_ext("Protocol=7.4-2");
	
	tdeforpg_crud(ENCRYPT_BYTEA_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();
	printf("\n---------- Protocol=7.4-2 End ----------\n");fflush(stdout);

	return 0;
}
