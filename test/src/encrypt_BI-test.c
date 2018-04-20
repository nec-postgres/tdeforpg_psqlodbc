#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int main(int argc, char **argv)
{
	printf("\nBI=-5 TEST\n");fflush(stdout);
	printf("\n---------- BI=-5 Start ----------\n");fflush(stdout);	
	test_connect_ext("BI=-5");
	
	tdeforpg_crud(ENCRYPT_BYTEA_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();
	printf("\n---------- BI=-5 End ----------\n");fflush(stdout);	

	printf("\nBI=2 TEST\n");fflush(stdout);
	printf("\n---------- BI=2 Start ----------\n");fflush(stdout);	
	test_connect_ext("BI=2");
	
	tdeforpg_crud(ENCRYPT_BYTEA_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();
	printf("\n---------- BI=12 End ----------\n");fflush(stdout);	

	printf("\nBI=12 TEST\n");fflush(stdout);
	printf("\n---------- BI=12 Start ----------\n");fflush(stdout);	
	test_connect_ext("BI=12");
	
	tdeforpg_crud(ENCRYPT_BYTEA_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();
	printf("\n---------- BI=12 End ----------\n");fflush(stdout);	

	return 0;
}
