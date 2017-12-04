#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void* doit(void* arg);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

#define ThreadCnt 10

typedef struct cache_block{
  char host[MAXLINE];
  char port[MAXLINE];
  char fileP[MAXLINE];
  long data_size;
  char datap[MAX_OBJECT_SIZE];
  struct cache_block *next;
}cache_block_t;

struct cache_head_t{
  cache_block_t *head;
  cache_block_t *tail;
  pthread_rwlock_t sz_rw_lock;
  sem_t mutex;
  int cache_current_size;
}cache;

void init_cache(struct cache_head_t *c) {
  c->head = NULL;
  c->tail = NULL;
  sem_init(&c->mutex, 0, 1);
  pthread_rwlock_init(&c->sz_rw_lock, NULL);
  c->cache_current_size = 0;
}

int checkroom(struct cache_head_t *c, int size) {
  pthread_rwlock_rdlock(&c->sz_rw_lock);
  if (c->cache_current_size + size <= MAX_CACHE_SIZE) {
    sem_wait(&c->mutex);
    pthread_rwlock_unlock(&c->sz_rw_lock);
    return 1;
  }
  sem_wait(&c->mutex);
  pthread_rwlock_unlock(&c->sz_rw_lock);
  return 0;
}

void cache_insert_at_head(struct cache_head_t* c, cache_block_t* b) {
  b->next = c->head;
  c->head = b;
  if (c->tail == NULL) {
    c->tail = c->head;
  }
  sem_post(&c->mutex);
}

void cache_remove_at_tail(struct cache_head_t* c) {
  if (c->head == c->tail) {
    // only one elem remain
    free(c->head);
    c->head = c->tail = NULL;
    sem_post(&c->mutex);
    return;
  }
  // normal
  cache_block_t *p = c->head;
  while (p->next != c->tail) {
    p = p->next;
  }
  p->next = NULL;
  free(c->tail);
  c->tail = p;
  sem_post(&c->mutex);
}


int main(int argc, char **argv)
{
  init_cache(&cache);
  Signal(SIGPIPE, SIG_IGN);
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  int *thread_argp = NULL;
  listenfd = Open_listenfd(argv[1]);
  pthread_t pid;
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
    thread_argp = (int*)Malloc(sizeof(int));
    *thread_argp = connfd;
    Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    pthread_create(&pid, NULL, doit, thread_argp);
  }
  return 0;
}


static cache_block_t* cache_search(struct cache_head_t *c, char *host, char *port, char *fileP) {
  sem_wait(&c->mutex);
  cache_block_t *p = c->head;
  while (p) {
    if (!strcmp(host, p->host)
        && !strcmp(port, p->port)
        && !strcmp(fileP, p->fileP)) {
      // find match
      sem_post(&c->mutex);
      return p;
    }
    p = p->next;
  }
  sem_post(&c->mutex);
  return NULL;
}



static void constructHDR(int clifd, char *Request, char *host, char *port, char *fp) {
  rio_t toCliRio;
  rio_readinitb(&toCliRio, clifd);

  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  if (!rio_readlineb(&toCliRio, buf,MAXLINE)) {
    pthread_exit(NULL);
  }
  
  
  sscanf(buf, "%s %s %s", method, uri, version);

  if (strcasecmp(method, "GET")) {
    pthread_exit(NULL);
  }

  char *fileP = strstr(uri+7, "/");
  
  *fileP = '\0';
  char *simicol = strstr(uri+7, ":");
  sprintf(port, "80");
  if (simicol) {
    *simicol = '\0';
    sprintf(port, "%s", simicol+1);
  }
  sprintf(host, "%s", uri+7);
  if (strcmp(host, "127.0.0.1") && strcmp(host, "localhost")) {
    pthread_exit(NULL);
  }
  if (!strcmp(host, "localhost")) {
    sprintf(host, "127.0.0.1");
  }
  // get optional port;
  *fileP = '/';
  printf("REQUEST line: %s", buf);
  printf("uri: %s file: %s\n", uri, fileP);
  printf("BEGIN SEARCH\n");
  cache_block_t *res = cache_search(&cache, host, port, fileP);
  if (res) {
    printf("FOUND:\n");
    
    FILE* return_filed = fopen("log", "w");
    fwrite(res->datap, 1, res->data_size, return_filed);
    rio_writen(clifd, res->datap, res->data_size);
    pthread_exit(NULL);
  }
  printf("NOT FOUND\n");
  sprintf(fp, "%s", fileP);
  sprintf(Request, "GET %s HTTP/1.0\r\n", fileP);
  sprintf(Request, "%sHost: %s\r\n", Request, host);
  sprintf(Request, "%sConnection: close\r\n", Request);
  sprintf(Request, "%sProxy-Connection: close\r\n", Request);
  sprintf(Request, "%s%s", Request, user_agent_hdr);
  while (strcmp(buf, "\r\n")) {
    rio_readlineb(&toCliRio, buf, MAXLINE);
    if (!strstr(buf, "Host")
        && !strstr(buf, "Connection")
        && !strstr(buf, "Proxy-Connection") 
       && !strstr(buf, "User-Agent")) {
      sprintf(Request, "%s%s", Request, buf);
    }
  }
  
  sprintf(Request, "%s\r\n", Request);
  //printf("return response : %s", Request);
    
}


void*
doit(void* arg) {
  pthread_detach(pthread_self());
  int clifd = *(int*)arg;
  free(arg);
  char Request[MAXLINE], host[MAXLINE], port[MAXLINE], buf[MAXLINE], fileP[MAXLINE];
  
  constructHDR(clifd, Request, host, port, fileP);

  int toServFd;
  // open connection with server
  printf("host: %s port: %s file: %s\n", host, port, fileP);
  if ((toServFd = open_clientfd(host, port)) < 0) {
    fprintf(stderr, "Error");
    pthread_exit(NULL);
  }
  rio_t toServRio;
  rio_readinitb(&toServRio, toServFd);
  // send request
  if (rio_writen(toServFd, Request, strlen(Request)) < 0) {
    fprintf(stderr, "Error writing to server\n");
  }
  char tmp[4196] = {0};
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
  cache_block_t *new_block = (cache_block_t*)Malloc(sizeof(cache_block_t));
  memcpy(new_block->host, host, strlen(host));
  memcpy(new_block->port, port, strlen(port));
  new_block->data_size = strlen(tmp);
  memcpy(new_block->fileP, fileP, strlen(fileP));
  memcpy(new_block->datap, tmp, new_block->data_size);
  rio_writen(clifd, tmp, strlen(tmp));
    
  int size_bk = DataSize;
  // read content
  while (DataSize > 0) {
    ssize_t n;
    n = rio_readnb(&toServRio, buf, MAXLINE);
    if (n <= 0) {
      break;
    }
    char *dstp = new_block->datap + new_block->data_size;
    //printf("go write memory [%lx-%lx]\n", (unsigned long)dstp, (unsigned long)(dstp+n));
    memcpy(dstp, buf, n);
    //printf("before add size: %ld n: %ld\n", new_block->data_size, n);
    new_block->data_size += n;
    //printf("after add new size :%ld\n", new_block->data_size);
    rio_writen(clifd, buf, n);
    DataSize -= n;
  }

  if (checkroom(&cache, size_bk)) {
    cache_insert_at_head(&cache, new_block);
  } else {
    while (!checkroom(&cache, size_bk)) {
      cache_remove_at_tail(&cache);
    }
    cache_insert_at_head(&cache, new_block);
  }
  if (cache.head) {
    printf("Add %s %s %s %ld\n", cache.head->host, cache.head->port, cache.head->fileP, cache.head->data_size);
  }
  Close(clifd);
  pthread_exit(NULL);
}



