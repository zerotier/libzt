ZeroTier SDK
======

[![irc](https://img.shields.io/badge/IRC-%23zerotier%20on%20freenode-orange.svg)](https://webchat.freenode.net/?channels=zerotier)

***

## Example

```
	std::string str = "welcome to the machine";
	zts_start(path);
	while(!zts_service_running())
		sleep(1);
	zts_join_network(nwid);
	int err, sockfd;
	while(!zts_has_address(nwid))
		sleep(1);
	if((sockfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("error creating ZeroTier socket");
		exit(0);
	}
	if((err = zts_connect(sockfd, (const struct sockaddr *)addr, sizeof(addr))) < 0) {
		printf("error connecting to remote host (%d)\n", err);
		exit(0);
	}
	int wrote = zts_write(sockfd, str.c_str(), str.length());
	err = zts_close(sockfd);
```

Bindings also exist for [many popular languages]().

## Build Targets
### Static Library
 - `make static_lib`: Will output to `build/`

### Tests
 - `make tests`: Will output to `build/tests/`

Then run the comprehensive test suite with whatever configuration you need. For instance:

To run a single-test IPv4 server on port 8787:

  - Host 1: `./build/test/comprehensive c7cd7c9e1b0f52a2 simple 4 server 8787`
  - Host 2: `./build/test/comprehensive c7cd7c9e1b0f52a2 simple 4 client 10.9.9.40 8787`

## Using Language Bindings
`SDK_LANG_JAVA=1`
`SDK_LANG_CSHARP=1`
`SDK_LANG_PYTHON=1`
`SDK_LANG_GO=1`
