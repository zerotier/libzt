#include <stdio.h>
#include <unistd.h>

#include "libzt.h"

int main()
{
	printf("Starting ZT service");
	zts_startjoin("ztp","17d709436c2c5367");

	printf("Dummy. Going into infinite loop. Ping me or something\n");
	while(1) {
		sleep(1);
	}
}
