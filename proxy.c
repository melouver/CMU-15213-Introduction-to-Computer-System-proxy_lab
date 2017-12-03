#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);

/* You won't lose style points for including this long line in your code */
//static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
    Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);
    Close(connfd);
  }
  return 0;
}

void doit(int clifd) {
  //goto test;
  rio_t toCli;
  Rio_readinitb(&toCli, clifd);

  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  if (!Rio_readlineb(&toCli, buf,MAXLINE)) {
    return;
  }
  printf("REQUEST: %s\n", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  if (strcasecmp(method, "GET")) {
    return;
  }
 
  char Request[MAXLINE], host[MAXLINE], port[MAXLINE];
  
  char *fileP = strstr(uri+7, "/");
  printf("uri: %s file: %s\n", uri, fileP);
  *fileP = '\0';
  sprintf(host, "%s", uri+7);
  *fileP = '/';
  sprintf(port, "10000");
  ///cgi-bin/adder?1&2
  sprintf(host, "127.0.0.1");
  sprintf(Request, "GET %s HTTP/1.0\r\n\r\n", fileP);
  int clientfd;
  
  if ((clientfd = open_clientfd(host, port)) < 0) {
    fprintf(stderr, "Error");
    exit(-1);
  }
  rio_t toServ;
  Rio_readinitb(&toServ, clientfd);
  
  if (rio_writen(clientfd, Request, strlen(Request)) < 0) {
    fprintf(stderr, "Error writing to server\n");
  }
  char tmp[MAXLINE] = {0};
  int size = 0;
  while (rio_readlineb(&toServ, buf, MAXLINE) > 0) {
    sprintf(tmp, "%s%s", tmp, buf);
    printf("--%s", buf);
    if (!strcasecmp(buf, "\r\n")) {
      break;
    }
    char *contentP = NULL;
    contentP = strstr(buf, "Content-length");
    if (contentP) {
      sscanf(contentP+strlen("Content-length")+1, "%d", &size);
    }
  }
  printf("size: %d \n", size);
  rio_writen(clifd, tmp, strlen(tmp));
  
  memset(tmp, 0, MAXLINE);
  ssize_t n;
  while (size > 0) {
    n = Rio_readnb(&toServ, buf, MAXLINE);
    printf("n: %lu\n", n);
    if (n <= 0) {
      break;
    }
    Rio_writen(clifd, buf, n);
    size -= n;
  }
  printf("remain size: %d \n", size);
  
  
  
  
  
  
  
}

 
