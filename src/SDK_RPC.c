/*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2015  ZeroTier, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * ZeroTier may be used and distributed under the terms of the GPLv3, which
 * are available at: http://www.gnu.org/licenses/gpl-3.0.html
 *
 * If you would like to embed ZeroTier into a commercial application or
 * redistribute it in a modified binary form, please contact ZeroTier Networks
 * LLC. Start here: http://www.zerotier.com/
 */

#ifdef USE_GNU_SOURCE
#define _GNU_SOURCE
#endif

#if defined(__linux__)
#include <sys/syscall.h>
#endif

// For defining the Android direct-call API
#if defined(__ANDROID__)
    #include <jni.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdint.h>
#include <sys/socket.h>
//#include <sys/un.h>
#include <strings.h>

#include "SDK.h"
#include "SDK_RPC.h"
#include "SDK_Debug.h"


// externs common between SDK_Intercept and SDK_Socket from SDK.h
int (*realsocket)(SOCKET_SIG);
int (*realconnect)(CONNECT_SIG);

#ifdef __cplusplus
extern "C" {
#endif
    
#define SERVICE_CONNECT_ATTEMPTS 30
    
ssize_t sock_fd_write(int sock, int fd);
ssize_t sock_fd_read(int sock, void *buf, ssize_t bufsize, int *fd);
    
static int rpc_count;

static pthread_mutex_t lock;
void rpc_mutex_init() {
  if(pthread_mutex_init(&lock, NULL) != 0) {
    fprintf(stderr, "error while initializing service call mutex\n");
  }
}
void rpc_mutex_destroy() {
  pthread_mutex_destroy(&lock);
}

/* 
 * Reads a new file descriptor from the service 
 */
int get_new_fd(int sock)
{
  char buf[BUF_SZ];
  int newfd;
  ssize_t size = sock_fd_read(sock, buf, sizeof(buf), &newfd);
  if(size > 0)
    return newfd;
  return -1;
}

/*
 * Reads a return value from the service and sets errno (if applicable)
 */
int get_retval(int rpc_sock)
{
  if(rpc_sock >= 0) {
    int retval;
    int sz = sizeof(char) + sizeof(retval) + sizeof(errno);
    char retbuf[BUF_SZ];
    memset(&retbuf, 0, sz);
    long n_read = read(rpc_sock, &retbuf, sz);
    if(n_read > 0) {
      memcpy(&retval, &retbuf[1], sizeof(retval));
      memcpy(&errno, &retbuf[1+sizeof(retval)], sizeof(errno));
      return retval;
    }
  }
  return -1;
}

int load_symbols_rpc()
{
#if defined(SDK_BUNDLED) || defined(__IOS__) || defined(__UNITY_3D__)
  realsocket = dlsym(RTLD_NEXT, "socket");
  realconnect = dlsym(RTLD_NOW, "connect");
  if(!realconnect || !realsocket)
    return -1;
#endif
  return 1;
}

int rpc_join(char * sockname)
{
  LOGV("RPC = %s\n", sockname);
  if(sockname == NULL) {
    printf("Warning, rpc netpath is NULL\n");
  }
  if(!load_symbols_rpc())
    return -1;
  struct sockaddr_un addr;
  int conn_err = -1, attempts = 0;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sockname, sizeof(addr.sun_path)-1);
  int sock;

#if defined(__ANDROID__)
  if((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
#else
  if((sock = realsocket(AF_UNIX, SOCK_STREAM, 0)) < 0){
#endif
    LOGV(stderr, "Error while creating RPC socket\n");
    return -1;
  }
  while((conn_err != 0) && (attempts < SERVICE_CONNECT_ATTEMPTS)){
    #if defined(__ANDROID__)
      if((conn_err = connect(sock, (struct sockaddr*)&addr, sizeof(addr))) != 0) {
    #else
      if((conn_err = realconnect(sock, (struct sockaddr*)&addr, sizeof(addr))) != 0) {
    #endif
      LOGV("Error while connecting to RPC socket. Re-attempting...\n");
      sleep(1);
    }
    else
      return sock;
    attempts++;
  }
  return -1;
}

/*
 * Send a command to the service 
 */
int rpc_send_command(char *path, int cmd, int forfd, void *data, int len)
{
  pthread_mutex_lock(&lock);
  char c, padding[] = {PADDING};
  char cmdbuf[BUF_SZ], CANARY[CANARY_SZ+PADDING_SZ], metabuf[BUF_SZ];

  memcpy(CANARY+CANARY_SZ, padding, sizeof(padding));
  uint64_t canary_num;
  // ephemeral RPC socket used only for this command
  int rpc_sock = rpc_join(path);
  // Generate token
  int fdrand = open("/dev/urandom", O_RDONLY);
  if(read(fdrand, &CANARY, CANARY_SZ) < 0) {
     fprintf(stderr,"unable to read from /dev/urandom for RPC canary data\n");
     return -1;  
  }
  memcpy(&canary_num, CANARY, CANARY_SZ);  
  cmdbuf[CMD_ID_IDX] = cmd;
  memcpy(&cmdbuf[CANARY_IDX], &canary_num, CANARY_SZ);
  memcpy(&cmdbuf[STRUCT_IDX], data, len);

  rpc_count++;
  memset(metabuf, 0, BUF_SZ);
#if defined(__linux__)
  #if !defined(__ANDROID__)
    pid_t pid = 5; //syscall(SYS_getpid);
    pid_t tid = 4;//syscall(SYS_gettid);
  #else
    // Dummy values
    pid_t pid = 5;
    pid_t tid = gettid();
  #endif
#endif
  char timestring[20];
  time_t timestamp;
  timestamp = time(NULL);
  strftime(timestring, sizeof(timestring), "%H:%M:%S", localtime(&timestamp));
#if defined(__linux__)
  memcpy(&metabuf[IDX_PID],     &pid,         sizeof(pid_t)      ); /* pid       */
  memcpy(&metabuf[IDX_TID],     &tid,         sizeof(pid_t)      ); /* tid       */
#endif
  memcpy(&metabuf[IDX_TIME],    &timestring,   20                ); /* timestamp */

  /* Combine command flag+payload with RPC metadata */
  memcpy(metabuf, RPC_PHRASE, RPC_PHRASE_SZ); // Write signal phrase
  memcpy(&metabuf[IDX_PAYLOAD], cmdbuf, len + 1 + CANARY_SZ);
  // Write RPC
  long n_write = write(rpc_sock, &metabuf, BUF_SZ);
  if(n_write < 0) {
    fprintf(stderr, "Error writing command to service (CMD = %d)\n", cmdbuf[CMD_ID_IDX]);
    errno = 0;
  }
  // Write token to corresponding data stream
  if(read(rpc_sock, &c, 1) < 0) {
    fprintf(stderr, "unable to read RPC ACK byte from service.\n");
    return -1;
  }
  if(c == 'z' && n_write > 0 && forfd > -1){
    if(send(forfd, &CANARY, CANARY_SZ+PADDING_SZ, 0) < 0) {
        perror("send: \n");
      fprintf(stderr,"unable to write canary to stream (fd=%d)\n", forfd);
      return -1;
    }
  }
  // Process response from service
  int ret = ERR_OK;
  if(n_write > 0) {
    if(cmdbuf[CMD_ID_IDX]==RPC_SOCKET) {
      pthread_mutex_unlock(&lock);
      return rpc_sock; // Used as new socket
    }
    if(cmdbuf[CMD_ID_IDX]==RPC_CONNECT
      || cmdbuf[CMD_ID_IDX]==RPC_BIND
      || cmdbuf[CMD_ID_IDX]==RPC_LISTEN) {
      ret = get_retval(rpc_sock);
    }
    if(cmdbuf[CMD_ID_IDX]==RPC_GETSOCKNAME) {
      pthread_mutex_unlock(&lock);
      return rpc_sock; // Don't close rpc here, we'll use it to read getsockopt_st
    }
  }
  else
    ret = -1;
  close(rpc_sock); // We're done with this RPC socket, close it (if type-R)
  pthread_mutex_unlock(&lock);
  return ret;
}

/* 
 * Send file descriptor 
 */
ssize_t sock_fd_write(int sock, int fd)
{
  ssize_t size;
  struct msghdr msg;
  struct iovec iov;
  char buf = '\0';
  int buflen = 1;
  union {
        struct cmsghdr  cmsghdr;
    char control[CMSG_SPACE(sizeof (int))];
  } cmsgu;
  struct cmsghdr *cmsg;
  iov.iov_base = &buf;
  iov.iov_len = buflen;
  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  if (fd != -1) {
      msg.msg_control = cmsgu.control;
      msg.msg_controllen = sizeof(cmsgu.control);
      cmsg = CMSG_FIRSTHDR(&msg);
      cmsg->cmsg_len = CMSG_LEN(sizeof (int));
      cmsg->cmsg_level = SOL_SOCKET;
      cmsg->cmsg_type = SCM_RIGHTS;
      *((int *) CMSG_DATA(cmsg)) = fd;
  } else {
      msg.msg_control = NULL;
      msg.msg_controllen = 0;
  }
  size = sendmsg(sock, &msg, 0);
  if (size < 0)
      perror ("sendmsg");
  return size;
}
/* 
 * Read a file descriptor 
 */
ssize_t sock_fd_read(int sock, void *buf, ssize_t bufsize, int *fd)
{    
  ssize_t size;
  if (fd) {
    struct msghdr msg;
    struct iovec iov;
    union {
      struct cmsghdr cmsghdr;
      char control[CMSG_SPACE(sizeof (int))];
    } cmsgu;
        
    struct cmsghdr *cmsg;
    iov.iov_base = buf;
    iov.iov_len = bufsize;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
      
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgu.control;
    msg.msg_controllen = sizeof(cmsgu.control);
    size = recvmsg (sock, &msg, 0);
      
    if (size < 0)
      return -1;
    cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
      if (cmsg->cmsg_level != SOL_SOCKET) {
        fprintf (stderr, "invalid cmsg_level %d\n",cmsg->cmsg_level);
        return -1;
      }
      if (cmsg->cmsg_type != SCM_RIGHTS) {
          fprintf (stderr, "invalid cmsg_type %d\n",cmsg->cmsg_type);
          return -1;
      }
      *fd = *((int *) CMSG_DATA(cmsg));
    } else {
*fd = -1;}
  } else {
    size = read (sock, buf, bufsize);
    if (size < 0) {
      fprintf(stderr, "sock_fd_read(): read: Error\n");
      return -1;
    }
  }
  return size;
}
    
#ifdef __cplusplus
}
#endif
