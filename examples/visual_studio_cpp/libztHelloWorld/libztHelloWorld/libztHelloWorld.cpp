// libztHelloWorld - Simple demo with libzt.lib and libzt.dll

#include "stdafx.h"
#include <Windows.h>
#include <stdio.h>
#include <windows.h>
#include <strsafe.h>

#include "..\..\..\..\include\libzt.h"

int main()
{
	printf("waiting for libzt to come online...\n");
	zts_simple_start("dlldir", "17d709436c2c5367");
	printf("started. now performing a socket call\n");
	int fd = zts_socket(AF_INET, SOCK_STREAM, 0);
	printf("fd=%d\n", fd);
	// zts_connect(), zts_bind(), etc...
	zts_stop();
  return 0;
}

