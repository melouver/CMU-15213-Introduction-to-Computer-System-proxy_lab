#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

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
static void constructHDR(int clifd, char *Request, char *host, char *port) {
  rio_t toCliRio;
  Rio_readinitb(&toCliRio, clifd);

  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  if (!Rio_readlineb(&toCliRio, buf,MAXLINE)) {
    return;
  }
  
  printf("REQUEST line: %s\n", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  if (strcasecmp(method, "GET")) {
    return;
  }

  char *fileP = strstr(uri+7, "/");
  printf("uri: %s file: %s\n", uri, fileP);
  *fileP = '\0';
  char *simicol = strstr(uri+7, ":");
  sprintf(port, "80");
  if (simicol) {
    *simicol = '\0';
    sprintf(port, "%s", simicol+1);
  }
  sprintf(host, "%s", uri+7);
  printf("host: %s port: %s\n", host, port);
  // get optional port;
  *fileP = '/';
  

  sprintf(Request, "GET %s HTTP/1.0\r\n", fileP);
  sprintf(Request, "%sHost: %s\r\n", Request, host);
  sprintf(Request, "%sConnection: close\r\n", Request);
  sprintf(Request, "%sProxy-Connection: close\r\n", Request);
  sprintf(Request, "%s%s", Request, user_agent_hdr);
  while (strcmp(buf, "\r\n")) {
    Rio_readlineb(&toCliRio, buf, MAXLINE);
    if (!strstr(buf, "Host")
        && !strstr(buf, "Connection")
        && !strstr(buf, "Proxy-Connection")
        && !strstr(buf, "User-Agent")) {
      printf("other hdr : %s ", buf);
      sprintf(Request, "%s%s", Request, buf);
    }
  }
  
  
  sprintf(Request, "%s\r\n", Request);
  printf("return response : %s", Request);
}


void doit(int clifd) {
 
  char Request[MAXLINE], host[MAXLINE], port[MAXLINE], buf[MAXLINE];
  
  constructHDR(clifd, Request, host, port);

  int toServFd;
  // open connection with server
  if ((toServFd = open_clientfd(host, port)) < 0) {
    fprintf(stderr, "Error");
    exit(-1);
  }
  rio_t toServRio;
  Rio_readinitb(&toServRio, toServFd);
  // send request
  if (rio_writen(toServFd, Request, strlen(Request)) < 0) {
    fprintf(stderr, "Error writing to server\n");
  }
  char tmp[MAXLINE] = {0};
  int DataSize = 0;
  //read response header
  while (rio_readlineb(&toServRio, buf, MAXLINE) > 0) {
    sprintf(tmp, "%s%s", tmp, buf);
    if (!strcasecmp(buf, "\r\n")) {
      break;
    }
    char *contentP = NULL;
    contentP = strstr(buf, "Content-length");
    if (contentP) {
      sscanf(contentP+strlen("Content-length")+1, "%d", &DataSize);
    }
  }
  printf("size: %d \n", DataSize);
  rio_writen(clifd, tmp, strlen(tmp));
  
  memset(tmp, 0, MAXLINE);
  // read content
  while (DataSize > 0) {
    ssize_t n;
    n = Rio_readnb(&toServRio, buf, MAXLINE);
    printf("n: %lu\n", n);
    if (n <= 0) {
      break;
    }
    Rio_writen(clifd, buf, n);
    DataSize -= n;
  }
  printf("remain size: %d \n", DataSize);
  
  
  
  
  
  
  
}

 
