// Comprehensive stress test for socket-like API

#include <stdio.h>

#include "ZeroTierSDK.h"

int main()
{
	printf("zts_core_version = %s\n", zts_core_version());

	zts_start("./ztsdk"); // starts ZeroTier core, generates id in ./zt


	zts_stop();
	return 0;
}