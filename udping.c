// Adam Harvey and Parker Hundley 
// Assignment 2

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Practical.h"
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

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
                exit(EXIT_FAILURE);
        }
    }

if(server_mode){

  char server[6];
  snprintf(server, sizeof(server), "%d", port_number);

  // Construct the server address structure
  struct addrinfo addrCriteria;                   // Criteria for address
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = AF_UNSPEC;             // Any address family
  addrCriteria.ai_flags = AI_PASSIVE;             // Accept on any address/port
  addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram socket
  addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP socket

  struct addrinfo *servAddr; // List of server addresses
  int rtnVal = getaddrinfo(NULL, server, &addrCriteria, &servAddr);
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
}

else{

 if (optind < argc) {
        server_ip_add = argv[optind];
        fprintf(stderr, "Server IP Address: %s\n", server_ip_add);
    } 
    
  char *echoString = argv[2]; // Second arg: word to echo

  char server[6];
  snprintf(server, sizeof(server), "%d", port_number);

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

  // Create a datagram/UDP socket
  int sock = socket(servAddr->ai_family, servAddr->ai_socktype,
      servAddr->ai_protocol); // Socket descriptor for client
  if (sock < 0)
    DieWithSystemMessage("socket() failed");

  // Send the string to the server
  ssize_t numBytes = sendto(sock, echoString, echoStringLen, 0,
      servAddr->ai_addr, servAddr->ai_addrlen);
  if (numBytes < 0)
    DieWithSystemMessage("sendto() failed");
  else if (numBytes != echoStringLen)
    DieWithUserMessage("sendto() error", "sent unexpected number of bytes");

  // Receive a response
  struct sockaddr_storage fromAddr; // Source address of server
  // Set length of from address structure (in-out parameter)
  socklen_t fromAddrLen = sizeof(fromAddr);
  char buffer[MAXSTRINGLENGTH + 1]; // I/O buffer
  numBytes = recvfrom(sock, buffer, MAXSTRINGLENGTH, 0,
      (struct sockaddr *) &fromAddr, &fromAddrLen);
  if (numBytes < 0)
    DieWithSystemMessage("recvfrom() failed");
  else if (numBytes != echoStringLen)
    DieWithUserMessage("recvfrom() error", "received unexpected number of bytes");

  // Verify reception from expected source
  if (!SockAddrsEqual(servAddr->ai_addr, (struct sockaddr *) &fromAddr))
    DieWithUserMessage("recvfrom()", "received a packet from unknown source");

  freeaddrinfo(servAddr);

  buffer[echoStringLen] = '\0';     // Null-terminate received data
  printf("Received: %s\n", buffer); // Print the echoed string

  close(sock);
  exit(0);

}

    return 0;
}