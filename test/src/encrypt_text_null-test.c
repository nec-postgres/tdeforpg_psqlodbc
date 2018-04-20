#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int main(int argc, char **argv)
{
	test_connect_ext("Debug=1");

	printf("\n===========ENCRYPT_TEXT_TEST_NULL_DATA_START===============\n");
	
	tdeforpg_crud(ENCRYPT_TEXT_TEST, NULL_DATA);
	
	test_disconnect();

	printf("\n===========ENCRYPT_TEXT_TEST_NULL_DATA_END==============\n");

	return 0;
}
