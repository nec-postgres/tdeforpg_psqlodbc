#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int main(int argc, char **argv)
{
	test_connect_ext("ShowOidColumn=1");
	
	tdeforpg_crud(ENCRYPT_BYTEA_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TEXT_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_NUMERIC_TEST, BASIC_DATA);
	
	tdeforpg_crud(ENCRYPT_TIMESTAMP_TEST, BASIC_DATA);
	
	test_disconnect();

	return 0;
}
