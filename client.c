/*Client code for multicasting */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "tlv.h"

#define MAX_SIZE (1024*4)
#define SERVER_PORT 3000
#define IPADDR "127.0.0.1"
#define TLV_HEADER_SIZE  5
#define TLV_DATA_CHUNK	64
#define FILENAME_SIZE 10
#define TLV_BUFF_SIZE  (FILENAME_SIZE+ TLV_DATA_CHUNK + TLV_HEADER_SIZE)

struct keep_alive_argument
{
  int socket_id;
  int number_of_groups;
};

pthread_t keep_alive_thread_id;
pthread_t process_job_thread_id;

/* Keep alive thread in client */
void *
keep_alive_handler (void *arg)
{
  char sendBuff[MAX_SIZE];
  char type = KEEP_ALIVE;
  int len = 1;
  struct keep_alive_argument *alive_arg = (struct keep_alive_argument *) arg;
  memcpy (sendBuff, &type, sizeof (type));
  len = htonl (sizeof (type));
  memcpy (sendBuff + sizeof (type), &len, sizeof (len));
  memcpy (sendBuff + sizeof (tlv_header_t), &type, sizeof (type));
  while (1)
    {
      sleep (CLIENT_KA_INT);
	write (alive_arg->socket_id, sendBuff, 1 + sizeof (tlv_header_t));
    }
  return NULL;
}

/* Job processing */
void *
process_job (void *arg)
{
  int r = 0;
  unsigned char recvbuff[MAX_SIZE];
  int *client_id = (int *) arg;
  int socket_id = *client_id;
  TLV_State tlvs;
	init_tlv_state (&tlvs);
 while(1) {
  memset (recvbuff, 0, sizeof (recvbuff));
  init_tlv_state (&tlvs);
  while (r = read (socket_id, recvbuff, TLV_BUFF_SIZE))
    {
      assemble_tlv (recvbuff, r, &tlvs, socket_id);
    }
  if (r < 0)
    {
      printf ("\nRead error ");
    }
}
}

/* Main Function */
int
main (int argc, char *argv[])
{
  int client_sd, read_size;
  struct keep_alive_argument alive_arg;
  struct sockaddr_in serv_addr;
  char sendBuff[MAX_SIZE];
  int *buffer, i;
  int len;
  char type = REGISTER_GROUPS;

  //Creation of client socket
  client_sd = socket (AF_INET, SOCK_STREAM, 0);
  if (client_sd == -1)
    {
      perror ("\n Server Socket is not created \n");
      exit (1);
    }

  if((argv[1] != NULL) && (NULL == strstr(argv[1],".")))
  {
      printf("Invalid IP address\n");
      exit(1);
  }
  //Server details
  bzero ((char *) &serv_addr, sizeof (serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr (argv[1]);
  serv_addr.sin_port = htons (SERVER_PORT);

  //Connect to server
  if (connect (client_sd, (struct sockaddr *) &serv_addr, sizeof (serv_addr))
      < 0)
    {
      printf ("Failed to connect to server\n");
      return -1;
    }

  printf ("\nConnected successfully\n");
  memset (sendBuff, 0, sizeof (sendBuff));
  memcpy (sendBuff, &type, sizeof (type));

  /* Sending group info */
  len = htonl (sizeof (int) * (argc - 2));
  memcpy (sendBuff + sizeof (type), &len, sizeof (len));
  i = 0;
  len = sizeof (int) * (argc - 2);
  while (i < (argc - 2))
    {
      int a = atoi (argv[i + 2]);
      a = htonl (a);
      memcpy (sendBuff + sizeof (type) + sizeof (len) + (i * sizeof (int)),
	      &a, sizeof (int));
      i++;
    }
  //DumpRawBytes ((unsigned char *) sendBuff, len + sizeof (tlv_header_t),
  //		"Args Buffer");
  write (client_sd, sendBuff, len + sizeof (tlv_header_t));

  alive_arg.socket_id = client_sd;
  alive_arg.number_of_groups = argc - 1;

  /*Keep alive thread */
  pthread_create (&(keep_alive_thread_id), NULL, &keep_alive_handler,
		  (void *) &alive_arg);


  /* Job thread */
  pthread_create (&(process_job_thread_id), NULL, &process_job,
		  (void *) &client_sd);

  pthread_join (keep_alive_thread_id, NULL);
  pthread_join (process_job_thread_id, NULL);

  close (client_sd);
  return 0;

}
