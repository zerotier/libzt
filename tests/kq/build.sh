gcc kq_test_client.c ../../kq.c ../../zt_api.c ../../RPC.c -DZT_SDK_DEBUG -DNETCON_INTERCEPT -o kq_test_client
#gcc kq_test_server.c ../../kq.c -o kq_test_server

gcc server.c -o server
#gcc client.c -o client