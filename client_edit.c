/*Client code for multicasting */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stddef.h>
#include <sys/epoll.h>
#include <math.h>
#include <time.h>
	#include "tlv.h"
	
	#define MAX_SIZE 1025
	#define port 3000
	#define ip_addr "127.0.0.1"
	#define MAXEVENTS 1
	
	
	struct keep_alive_argument
	{
	   int socket_id;
	   int number_of_groups;
	};
		
	pthread_t keep_alive_thread_id;
	pthread_t process_job_thread_id;
	pthread_t listen_server_thread_id;
	
	/* Make server socket non-blocking */
static int
make_socket_non_blocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      perror ("\n fcntl");
      return -1;
    }
  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
    {
      perror ("\n fcntl");
      return -1;
    }
  return 0;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/* Keep alive thread in client */
	void * keep_alive_handler(void * arg)
	{
	  char sendBuff[MAX_SIZE];
	  char type = KEEP_ALIVE;
	  int len = 1;
	  struct keep_alive_argument *alive_arg = (struct keep_alive_argument *)arg;
	  memcpy (sendBuff, &type, sizeof (type));
	  //TODO: len is size total bytes, instead of unit size
	  len = htonl (sizeof (int));
	  memcpy (sendBuff + sizeof (type), &len, sizeof (len));
	  memcpy(sendBuff + sizeof(tlv_header_t),&type,sizeof(type));
	  while(1)
	{
	  sleep(2);
	  write (alive_arg->socket_id, sendBuff, 1 + sizeof (tlv_header_t));
	}
	 return NULL;
	}
	
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	/* Job processing */
	void * process_job(void * arg)
	{
	   int r = 0;
	   unsigned char recvbuff[2000];
	   int * client_id = (int*) arg;
	   int socket_id = *client_id;
	   TLV_State tlvs;
	    memset(recvbuff,0 , sizeof(recvbuff));
	   init_tlv_state(&tlvs);
	   while(r= read(socket_id , recvbuff , sizeof(recvbuff)))
	  {
	    assemble_tlv(recvbuff , r , &tlvs , socket_id);
	  }
	   if(r<0)
	{
	   printf("\nRead error ");
	}
	
	  //recieve tlv
	
	   //extract tlv form array of integers
	   //so file perform operation
	   // get result
	  // make tlv
	  // send tlv	
	}
	
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*Thread handler to listen incoming server jobs */
void * listen_server_handler (void *p_client_sd)
{
  puts ("\n Waiting for incoming connections...\n");
  int c = sizeof (struct sockaddr_in);
  int client_sd, s, efd, n, i;
  struct epoll_event event;
  struct epoll_event *new_events;

  client_sd = *((int *) (p_client_sd));
  efd = epoll_create1 (0);
  if (efd == -1)
    {
      perror ("\n epoll_create");
      exit (1);
    }

  event.data.fd = client_sd;
  event.events = EPOLLIN | EPOLLET;
  s = epoll_ctl (efd, EPOLL_CTL_ADD, client_sd, &event);
  if (s == -1)
    {
      perror ("\n epoll_ctl");
      pthread_exit (NULL);
    }

	/* Buffer where events are returned */
	new_events = (struct epoll_event *) calloc (MAXEVENTS, sizeof event);

  while (1)
    {
      n=epoll_wait (efd, new_events, MAXEVENTS, -1);
      
      for (i = 0; i < n; i++)
	  {

	  /* An error has occured on this fd, or the socket is not ready for reading */
	  if ((new_events[i].events & EPOLLERR) || (new_events[i].events & EPOLLHUP) || (!(new_events[i].events & EPOLLIN)))
	    {
	    	perror ("epoll error\n");
	    	close (new_events[i].data.fd);
	    }

	  /* We have a notification on the listening socket, which means one or more incoming connections. */
	  else
	    {
	    	if (pthread_create (&(process_job_thread_id), NULL,&process_job, (void *) &client_sd) < 0)
			{
				perror ("\n Could not create Job thread");
			}
	    }
	    
	    pthread_join( process_job_thread_id,  NULL);
    }
 }
    
}	

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	/* Main Function */
	int main (int argc, char *argv[])
	{
	  int client_sd, read_size;
	  struct keep_alive_argument alive_arg;
	  struct sockaddr_in serv_addr;
	  char sendBuff[MAX_SIZE];
	  int *buffer, i;
	  int len, s;
	  char type = REGISTER_GROUPS;
	  
	  //Creation of client socket
	  client_sd = socket (AF_INET, SOCK_STREAM, 0);
	  if (client_sd == -1)
	    {
	      perror ("\n Server Socket is not created \n");
	      exit (1);
	    }
	
	  //Server details
	  bzero ((char *) &serv_addr, sizeof (serv_addr));
	 serv_addr.sin_family = AF_INET;
	  serv_addr.sin_addr.s_addr = inet_addr (ip_addr);
	  serv_addr.sin_port = htons (port);
	  
	  // Make Socket non-blocaking
  	s = make_socket_non_blocking (client_sd);
  	if (s == -1)
    {
      perror ("\n Client Socket unable to set as non-blocking \n");
      exit (1);
    }
	
	  //Connect to server
	  if (connect (client_sd, (struct sockaddr *) &serv_addr, sizeof (serv_addr))
	      < 0)
	    {
	      printf ("Failed to connect to server\n");
	      return -1;
	    }
	
	  printf ("\nConnected successfully\n");
	  memset(sendBuff , 0 , sizeof(sendBuff));
	  memcpy (sendBuff, &type, sizeof (type));
	  
	  //TODO: len is size total bytes, instead of unit size
	  
	  /* Sending group info*/
	  len = htonl (sizeof (int) * (argc - 1));
	  memcpy (sendBuff + sizeof (type), &len, sizeof (len));
	  i = 0;
	  len = sizeof (int) * (argc - 1);
	  while (i < (argc - 1))
	    {
	      int a = atoi (argv[i+1]);
	      a = htonl (a);
	      memcpy (sendBuff + sizeof (type) + sizeof (len) + (i * sizeof (int)),
	              &a, sizeof (int));
	      i++;
	    }
	   // DumpRawBytes((unsigned char*)sendBuff,len+sizeof(tlv_header_t),"Args Buffer");
	    write (client_sd, sendBuff, len  + sizeof (tlv_header_t));
	
	   alive_arg.socket_id = client_sd;
	   alive_arg.number_of_groups = argc-1; 
	
	  	/*Keep alive thread*/
		pthread_create (&(keep_alive_thread_id), NULL, &keep_alive_handler, (void *) &alive_arg);
		
	
	   /* Job thread */
	 	pthread_create (&(listen_server_thread_id), NULL, &listen_server_handler, (void *) &client_sd);
	
	  pthread_join( keep_alive_thread_id , NULL);
	  pthread_join( listen_server_thread_id,  NULL);
	
	  close (client_sd);
	  return 0;
	
	}


