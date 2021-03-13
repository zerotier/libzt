/**
 * Demonstrate that we can compile, link, and run an application
 * with libzt and successfully get an error from the socket API.
 */

#include <cstdlib>
#include <ZeroTierSockets.h>

int main()
{
	return zts_socket(0,0,0) != -2; // We expect -2 from an uninitialized libzt instance
}
