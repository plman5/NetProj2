#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Practical.h"
#include <getopt.h>
#include <unistd.h>

int main(int argc, char *argv[]){
  
    int ping_count = 0x7fffffff;
    double ping_interval = 1.0;
    int port_number = 33333;
    int data_size = 12;
    int no_print = 0; // 0 means print all
    int server_mode = 0; // 0 means client mode

    int opt;
    while ((opt = getopt(argc, argv, "cipsnS")) != -1) {
        switch (opt) {
            case 'c':
                ping_count = atoi(optarg);
                break;
            case 'i':
                ping_interval = atof(optarg);
                break;
            case 'p':
                port_number = atoi(optarg);
                break;
            case 's':
                data_size = atoi(optarg);
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

    // Your code to use the parsed options goes here
    // For example, you can print the values:
    printf("Ping Count: %d\n", ping_count);
    printf("Ping Interval: %lf\n", ping_interval);
    printf("Port Number: %d\n", port_number);
    printf("Data Size: %d\n", data_size);
    printf("No Print: %s\n", no_print ? "True" : "False");
    printf("Server Mode: %s\n", server_mode ? "True" : "False");


if(server_mode){

 if (args < 3 || args > 4) // Test for correct number of arguments
    DieWithUserMessage("Parameter(s)",
        "<Server Address/Name> <Echo Word> [<Server Port/Service>]");

  char *server = params[1];     // First arg: server address/name
  char *echoString = params[2]; // Second arg: word to echo

  size_t echoStringLen = strlen(echoString);
  if (echoStringLen > MAXSTRINGLENGTH) // Check input length
    DieWithUserMessage(echoString, "string too long");

  // Third arg (optional): server port/service
  char *servPort = (args == 4) ? params[3] : "echo";

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

else{


    if (args < 3 || args > 4) // Test for correct number of arguments
    DieWithUserMessage("Parameter(s)",
        "<Server Address/Name> <Echo Word> [<Server Port/Service>]");

  char *server = params[1];     // First arg: server address/name
  char *echoString = params[2]; // Second arg: word to echo

  size_t echoStringLen = strlen(echoString);
  if (echoStringLen > MAXSTRINGLENGTH) // Check input length
    DieWithUserMessage(echoString, "string too long");

  // Third arg (optional): server port/service
  char *servPort = (args == 4) ? params[3] : "echo";

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