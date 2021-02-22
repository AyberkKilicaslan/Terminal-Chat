#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#define LENGTH 2048  //buffer size
// Global variables
int clientControlFlag = 0;
int sockfd = 0;
char userInfo[32]; //max length
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
void overwrite()
{
  printf("%s", "--> ");
  fflush(stdout);
}
//trim function to remove /n to ignore next line and gives more smooth view
void ignoreNextLine(char *arr, int length)
{
  int i;
  for (i = 0; i < length; i++)
  {
    if (arr[i] == '\n')
    {
      arr[i] = '\0';
      break;
    }
  }
}
void send_msg_handler()
{
  char message[LENGTH] = {};
  while (1)
  {
    overwrite();
    fgets(message, LENGTH, stdin);
    ignoreNextLine(message, LENGTH);
    //if user enters -exit then break the connection easily
    if (strcmp(message, "-exit") == 0)
    {
      clientControlFlag = 1;
      break;
    }
    else
    {
      send(sockfd, message, strlen(message), 0);
    }

    bzero(message, LENGTH);
  }
}
void recv_msg_handler()
{

  char message[LENGTH] = {};
  while (1)
  {
    int receive = recv(sockfd, message, LENGTH, 0);
    if (receive > 0)
    {
      printf("%s", message);
      overwrite();
    }
    else if (receive == 0)
    {
      break;
    }
    memset(message, 0, sizeof(message));
  }
}
int main(int argc, char **argv)
{
  printf("Enter your phone number : ");
  fgets(userInfo, 32, stdin);
  ignoreNextLine(userInfo, strlen(userInfo));
  struct sockaddr_in server_addr;
  // Socket init
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(3205);
  //connection to host
  int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (err == -1)
  {
    printf("Connection failed\n");
    return 0;
  }
  send(sockfd, userInfo, 32, 0);
  //shows functionaliy of the app
  printf("You are online on Whatsapp\n");
  printf("1) -gcreate let you create group\n");
  printf("*) -gcreate ONLY TAKES PHONE NUMBER DONT USE +\n");
  printf("2) -join let you join to group or create private chat\n");
  printf("3) -send let you message to the group\n");
  printf("4) -exit groupname let you exit the group\n");
  printf("5) -exit let you quit the program\n");
  printf("6) -whoami let you see your number\n");
  pthread_t send_msg_thread;
  //createing the threads
  if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) != 0)
  {
    printf("thread creation failed\n");
    return 0;
  }
  pthread_t recv_msg_thread;
  if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) != 0)
  {
    printf("thread creation failed\n");
    return 0;
  }
  while (1)
  {
    if (clientControlFlag)
    {
      printf("This user logged out.\n");
      break;
    }
  }
  close(sockfd);
  return 1;
}