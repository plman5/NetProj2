#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Practical.h"
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

struct ThreadData {

int ping_count;
int received_pings;
struct timespec start_time;
long long min_rtt;
long long max_rtt;
long long sum_rtt;
double ping_interval;
int port_number;
char* server_ip_add;

};


void *senderThreadFunction(void *data) {
    struct ThreadData *threadData = (struct ThreadData *)data;
    int ping_count = threadData->ping_count;
    char *server_ip_add = threadData->server_ip_add;
    int port_number = threadData->port_number;
    double ping_interval = threadData->ping_interval;

    struct addrinfo *servAddr;
    if (getServerAddress(server_ip_add, port_number, &servAddr) != 0) {
        fprintf(stderr, "Error getting server address\n");
        return NULL;
    }

    int sock = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);

    if (sock < 0) {
        DieWithSystemMessage("socket() failed");
        return NULL;
    }

    char echoString[MAXSTRINGLENGTH];
    size_t echoStringLen;

    for (int i = 0; i < ping_count; i++) {
        // Construct the ping message (you can customize this part)
        snprintf(echoString, sizeof(echoString), "Ping %d", i + 1);
        echoStringLen = strlen(echoString);

        // Send the ping message
        ssize_t numBytes = sendto(sock, echoString, echoStringLen, 0,
                                  servAddr->ai_addr, servAddr->ai_addrlen);

        if (numBytes < 0) {
            DieWithSystemMessage("sendto() failed");
        } else if (numBytes != echoStringLen) {
            DieWithUserMessage("sendto() error", "sent unexpected number of bytes");
        }

        // Sleep for the specified ping interval
        if (i < ping_count - 1) {
            struct timespec sleepTime;
            sleepTime.tv_sec = (time_t)ping_interval;
            sleepTime.tv_nsec = (long)((ping_interval - (time_t)ping_interval) * 1e9);

            nanosleep(&sleepTime, NULL);
        }

        threadData->received_pings++;
    }

    freeaddrinfo(servAddr);
    close(sock);

    return NULL;
}

void *receiverThreadFunction(void *data) {
    struct ThreadData *threadData = (struct ThreadData *)data;
    int ping_count = threadData->ping_count;
    int port_number = threadData->port_number;

    struct addrinfo *servAddr;
    if (getServerAddress(NULL, port_number, &servAddr) != 0) {
        fprintf(stderr, "Error getting server address\n");
        return NULL;
    }

    int sock = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);

    if (sock < 0) {
        DieWithSystemMessage("socket() failed");
        return NULL;
    }

    char buffer[MAXSTRINGLENGTH + 1];
    socklen_t fromAddrLen;

    for (int i = 0; i < ping_count; i++) {
        ssize_t numBytes = recvfrom(sock, buffer, MAXSTRINGLENGTH, 0,
                                    servAddr->ai_addr, &fromAddrLen);

        if (numBytes < 0) {
            DieWithSystemMessage("recvfrom() failed");
        } else {
            buffer[numBytes] = '\0';
            printf("Received: %s\n", buffer);

            // Calculate RTT here (you can customize this part)
        }
    }

    freeaddrinfo(servAddr);
    close(sock);

    return NULL;
}


int main(int argc, char *argv[]){

    int ping_count = 0x7fffffff;
    double ping_interval = 1.0;
    int port_number = 33333;
    int data_size = 12;
    int no_print = 0; // 0 means print all
    int server_mode = 0; // 0 means client mode
    char* server_ip_add = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "c:i:p:s:nS")) != -1) {
        switch (opt) {
            case 'c':
                ping_count = atoi(optarg);
		fprintf(stderr, "Count: %d\n", ping_count);
                break;
            case 'i':
                ping_interval = atof(optarg);
		fprintf(stderr, "Interval: %lf\n", ping_interval);
                break;
            case 'p':
		port_number = atoi(optarg);
		fprintf(stderr, "Port: %d\n", port_number);
	        break;
            case 's':
                data_size = atoi(optarg);
		fprintf(stderr, "Size: %d\n", data_size);
                break;
            case 'n':
                no_print = 1; // Set the flag to print summary stats only
                break;
            case 'S':
                server_mode = 1; // Set the flag to operate in server mode
		break;
	default:
                fprintf(stderr, "Usage: %s [-c count] [-i interval] [-p port] [-s size] [-n] [-S]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

struct ThreadData threadData;
pthread_t senderThread, receiverThread;

threadData.ping_count = ping_count;
threadData.ping_interval = ping_interval;
threadData.port_number = port_number;
threadData.server_ip_add = server_ip_add;

if(server_mode){

        if (port_number == -1) {
            fprintf(stderr, "Server mode requires specifying a port with -p.\n");
            exit(EXIT_FAILURE);
        }

  
  char service[6];
  snprintf(service, sizeof(service), "%d", port_number);



  // Construct the server address structure
  struct addrinfo addrCriteria;                   // Criteria for address
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = AF_UNSPEC;             // Any address family
  addrCriteria.ai_flags = AI_PASSIVE;             // Accept on any address/port
  addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram socket
  addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP socket

  struct addrinfo *servAddr; // List of server addresses
  int rtnVal = getaddrinfo(NULL, service, &addrCriteria, &servAddr);
  if (rtnVal != 0)
    DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));

  // Create socket for incoming connections
  int sock = socket(servAddr->ai_family, servAddr->ai_socktype,
      servAddr->ai_protocol);
  if (sock < 0)
    DieWithSystemMessage("socket() failed");

  // Bind to the local address
  if (bind(sock, servAddr->ai_addr, servAddr->ai_addrlen) < 0)
    DieWithSystemMessage("bind() failed");

  // Free address list allocated by getaddrinfo()
  freeaddrinfo(servAddr);

  for (;;) { // Run forever
    struct sockaddr_storage clntAddr; // Client address
    // Set Length of client address structure (in-out parameter)
    socklen_t clntAddrLen = sizeof(clntAddr);

    // Block until receive message from a client
    char buffer[MAXSTRINGLENGTH]; // I/O buffer
    // Size of received message
    ssize_t numBytesRcvd = recvfrom(sock, buffer, MAXSTRINGLENGTH, 0,
        (struct sockaddr *) &clntAddr, &clntAddrLen);
    if (numBytesRcvd < 0)
      DieWithSystemMessage("recvfrom() failed");

    fputs("Handling client ", stdout);
    PrintSocketAddress((struct sockaddr *) &clntAddr, stdout);
    fputc('\n', stdout);

    // Send received datagram back to the client
    ssize_t numBytesSent = sendto(sock, buffer, numBytesRcvd, 0,
        (struct sockaddr *) &clntAddr, sizeof(clntAddr));
    if (numBytesSent < 0)
      DieWithSystemMessage("sendto() failed)");
    else if (numBytesSent != numBytesRcvd)
      DieWithUserMessage("sendto()", "sent unexpected number of bytes");
  }

  close(sock);
  exit(0);
}

else{

 if (optind < argc) {
        server_ip_add = argv[optind];
        fprintf(stderr, "Server IP Address: %s\n", server_ip_add);
    } else {
        fprintf(stderr, "Server IP Address is required.\n");
        exit(EXIT_FAILURE);
    }

  char server[6];
  snprintf(server, sizeof(server), "%d", port_number);

  char *echoString = argv[2]; // Second arg: word to echo

  size_t echoStringLen = strlen(echoString);
  if (echoStringLen > MAXSTRINGLENGTH) // Check input length
    DieWithUserMessage(echoString, "string too long");

  // Third arg (optional): server port/service
  char *servPort = server_ip_add;

  // Tell the system what kind(s) of address info we want
  struct addrinfo addrCriteria;                   // Criteria for address match
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = AF_UNSPEC;             // Any address family
  // For the following fields, a zero value means "don't care"
  addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram sockets
  addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP protocol

  // Get address(es)
  struct addrinfo *servAddr; // List of server addresses
  int rtnVal = getaddrinfo(server, servPort, &addrCriteria, &servAddr);
  if (rtnVal != 0)
    DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));


       if (pthread_create(&senderThread, NULL, senderThreadFunction, &threadData) != 0) {
            DieWithSystemMessage("pthread_create(senderThread) failed");
        }
        if (pthread_create(&receiverThread, NULL, receiverThreadFunction, &threadData) != 0) {
            DieWithSystemMessage("pthread_create(receiverThread) failed");
        }

}

    pthread_join(senderThread, NULL);
    pthread_join(receiverThread, NULL);

    return 0;
}
