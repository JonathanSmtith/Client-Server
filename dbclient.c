#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BUF 256
#include "msg.h"
#include <inttypes.h>

void Usage(char *progname);

int LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen);

int Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd);
void readCheck(int res, int socketfd);
void writeCheck(int res, int socketfd);

int 
main(int argc, char **argv) {
  if (argc != 3) {
    Usage(argv[0]);
  }

  unsigned short port = 0;
  if (sscanf(argv[2], "%hu", &port) != 1) {
    Usage(argv[0]);
  }

  // Get an appropriate sockaddr structure.
  struct sockaddr_storage addr;
  size_t addrlen;
  if (!LookupName(argv[1], port, &addr, &addrlen)) {
    Usage(argv[0]);
  }

  // Connect to the remote host.
  int socket_fd;
  if (!Connect(&addr, addrlen, &socket_fd)) {
    Usage(argv[0]);
  }

  // Read something from the remote host.
  // Will only read BUF-1 characters at most.
  /*
  char readbuf[BUF];
  int res;
  while (1) {
    res = read(socket_fd, readbuf, BUF-1);
    if (res == 0) {
      printf("socket closed prematurely \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    if (res == -1) {
      if (errno == EINTR)
        continue;
      printf("socket read failure \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    readbuf[res] = '\0';
    printf("%s", readbuf);
    break;
  }
  

  // Write something to the remote host
  while (1) {
    int wres = write(socket_fd, cmd, sizeof(char)*128);
    if (wres == 0) {
     printf("socket closed prematurely \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    if (wres == -1) {
      if (errno == EINTR)
        continue;
      printf("socket write failure \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    break;
  }*/

  while(1){
	int cmd = 0;
	struct msg usr;
	//char cId;
	printf("Enter your choice (1 to put, 2 to get, 3 to delete, 0 to quit): ");
	scanf("%d", &cmd);// get the user choice
	getchar();
	usr.type = cmd;

	if(cmd == 1){ // put
		printf("Enter the student name: "); // get name
		fflush(stdin);
		fgets(usr.rd.name, MAX_NAME_LENGTH, stdin);
		usr.rd.name[strlen(usr.rd.name)-1] = '\0';

		printf("Enter the student id: "); // get id
		scanf("%d", &usr.rd.id);

		/*sprintf(cId, "%u", usr->rd.id); // change id to char
		strcpy(usr->rd.pad, cId); // put cId in to pad
		strcat(usr->rd.pad, usr->rd.name); // concatenate id and name*/

		write(socket_fd, &usr, sizeof(struct msg));// write to server
	}
	else if(cmd == 2){ // get
		printf("Enter the id: ");//get id
		scanf("%u", &usr.rd.id);

		write(socket_fd, &usr, sizeof(struct msg));// write to server
	}
	else if(cmd == 3){ // delete
		printf("Enter the id: "); // get id 
		scanf("%u",&usr.rd.id);

		write(socket_fd, &usr, sizeof(struct msg));// write to server
	}
	else if(cmd == 0){// close client
		 close(socket_fd);
		 return EXIT_SUCCESS;
	}
	else{ //if any other number then go to top of the loop
		printf("Invalid option");
		continue;
	}
	
	read(socket_fd, &usr ,sizeof(struct msg));// read from server
	
	if(cmd == 1){ // put
		if(usr.type == 4){ // if success
			printf("Put success\n");
		}
		else if(usr.type == 5){ // if fail
			printf("Put failed\n");
		}
	}
	else if(cmd == 2){ // get
		if(usr.rd.id == 0){
			usr.type = 5;
		}
		if(usr.type == 4){ // if success print name and id
			printf("Get success\n");
			printf("name: %s\n", usr.rd.name);
			printf("id: %u\n", usr.rd.id);
		}
		else if(usr.type == 5){// if fail
			printf("Get failed");
		}
	}
	else if(cmd == 3){ // delete
		if(usr.type == 4){// if succes print name and id
			printf("Delete success\n");
			printf("name: %s\n", usr.rd.name);
			printf("id: %u\n", usr.rd.id);
		}
		else if(usr.type == 5){ // if fail
			printf("Delete failed\n");
		}
	}
  }
  //clean up
  close(socket_fd);
  return EXIT_SUCCESS;
}

void 
Usage(char *progname) {
  printf("usage: %s  hostname port \n", progname);
  exit(EXIT_FAILURE);
}

int 
LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen) {
  struct addrinfo hints, *results;
  int retval;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // Do the lookup by invoking getaddrinfo().
  if ((retval = getaddrinfo(name, NULL, &hints, &results)) != 0) {
    printf( "getaddrinfo failed: %s", gai_strerror(retval));
    return 0;
  }

  // Set the port in the first result.
  if (results->ai_family == AF_INET) {
    struct sockaddr_in *v4addr =
            (struct sockaddr_in *) (results->ai_addr);
    v4addr->sin_port = htons(port);
  } else if (results->ai_family == AF_INET6) {
    struct sockaddr_in6 *v6addr =
            (struct sockaddr_in6 *)(results->ai_addr);
    v6addr->sin6_port = htons(port);
  } else {
    printf("getaddrinfo failed to provide an IPv4 or IPv6 address \n");
    freeaddrinfo(results);
    return 0;
  }

  // Return the first result.
  assert(results != NULL);
  memcpy(ret_addr, results->ai_addr, results->ai_addrlen);
  *ret_addrlen = results->ai_addrlen;

  // Clean up.
  freeaddrinfo(results);
  return 1;
}

int 
Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd) {
  // Create the socket.
  int socket_fd = socket(addr->ss_family, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    printf("socket() failed: %s", strerror(errno));
    return 0;
  }

  // Connect the socket to the remote host.
  int res = connect(socket_fd,
                    (const struct sockaddr *)(addr),
                    addrlen);
  if (res == -1) {
    printf("connect() failed: %s", strerror(errno));
    return 0;
  }

  *ret_fd = socket_fd;
  return 1;
}
