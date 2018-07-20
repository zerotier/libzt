#include <stdio.h>
#include <unistd.h>

#include "libzt.h"

int main()
{
	printf("Starting ZT service");
	zts_startjoin("my_config_path",0x0000000000000000);

	printf("Dummy. Going into infinite loop. Ping me or something\n");
	while(1) {
		sleep(1);
	}
}
