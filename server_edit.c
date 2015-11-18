/*Server Code for Multicasting */

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
#include"header.h"
#include "tlv.h"

#include "thpool.h"
#include <dlfcn.h>

#define BUFF_SIZE (4*1024)
#define PORT 3000
#define IP_ADDR "127.0.0.1"
#define MAXEVENTS 1000
#define FILE_CHUNK 64

int multi_cast_group = NUMBER_OF_GROUPS, max_clients = MAX_CLIENTS_IN_GROUP, client_count = 0;
int total_clients, efd;
int server_sd, bind_ret, listen_ret, accept_ret, connect_sd, backlog =
  10000, *new_sock;
char buff[BUFF_SIZE], *message;
struct sockaddr_in server_info, client_info;
pthread_t client_thread_id[MAX_NUMBER_OF_CLIENTS]; 
pthread_t thread_id[2];
 threadpool thpool_job;
pthread_t job_thread_id[2], mgroup_job_thread_id[NUMBER_OF_GROUPS];
int job_id = 0;
struct mg_argument
{
  int group_id;
  int job_id;
  char filename[100];
};

struct client_argument
{
  int client_socket_id;
  char file_name[50];
  int i;
  int *result_array;
  int mg_id;
  int job_operation;
};

struct epoll_event *new_events;

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


		/* Thread handler for client handling activities */
void *
client_Handler (void *argument)
{
  //Get the client socket descriptor
  struct client_argument *client_arg = (struct client_argument *) argument;
  int client_sock = client_arg->client_socket_id;
  char file_name[50];
  int count = client_arg->i;
  char *buffer, buf[10];
  int len, i = 0, copied = 0;
  char sendBuff[MAX_SIZE];
  FILE *fp = NULL;
  unsigned char type;
  memset (sendBuff, 0, sizeof (sendBuff));
#if 0
  /* printf ("\n WE ARE CELEBRATING GANESH CHATURTHI  :)"); */
#endif
i=0;
fp = fopen (client_arg->file_name, "r");
struct stat sb;
  if (stat (client_arg->file_name, &sb) == -1)
    {
	perror ("stat");
	printf("%s\n",client_arg->file_name);
	pthread_exit(NULL);
    }
  
while (i < sb.st_size)
{
	if((sb.st_size - i) > FILE_CHUNK)
		len = htonl(10 + FILE_CHUNK);
	else
		len = htonl(10 + sb.st_size - i);
	if(i == 0)
	{
		type = FILE_WRITE;

  memcpy (sendBuff, &type, sizeof (type));
  			memcpy (sendBuff + sizeof (type), &len, sizeof (len));
 		 memset (buf, 0, sizeof (buf));
		  strncpy (buf, "b.txt", 10);
		  memcpy (sendBuff + sizeof (type) + sizeof (len), buf, 10);
		  copied = fread (sendBuff + sizeof (tlv_header_t) + 10 , 1, FILE_CHUNK, fp);
      if (copied < FILE_CHUNK)
        i = i + copied;
      else
        i = i + FILE_CHUNK;
        //write (client_sock, sendBuff, ntohl (len) + sizeof (tlv_header_t));
	write (client_sock, sendBuff, copied+10 + sizeof (tlv_header_t));
	}
	else
	{
		type = FILE_APPEND;

  memcpy (sendBuff, &type, sizeof (type));
  			memcpy (sendBuff + sizeof (type), &len, sizeof (len));
 		 memset (buf, 0, sizeof (buf));
		  strncpy (buf, "b.txt", 10);
		  memcpy (sendBuff + sizeof (type) + sizeof (len), buf, 10);
		  //fseek(fp, i, SEEK_SET);
		  copied = fread (sendBuff + sizeof (tlv_header_t) + 10, 1, FILE_CHUNK, fp);
      if (copied < FILE_CHUNK)
        i = i + copied;
      else
        i = i + FILE_CHUNK;
        //write (client_sock, sendBuff, ntohl (len) + sizeof (tlv_header_t));
	write (client_sock, sendBuff, 10 + copied + sizeof (tlv_header_t));
	}
	//DumpRawBytes ((unsigned char *) sendBuff,
        //          ntohl (len) + sizeof (tlv_header_t), "Send Buffer");
                    
      memset (sendBuff, 0, sizeof (sendBuff));
}
  fclose (fp);
  
 
 memset (sendBuff, 0, sizeof (sendBuff));
  type = EXECUTE_JOB_ID;
  len = sizeof (int) * 2;
  len = htonl (len);
  int mg_id = client_arg->mg_id;
  int job_operation = client_arg->job_operation;
  int job_id = multicast_groups[mg_id].job_id;
  job_operation = htonl (job_operation);
  job_id = htonl (job_id);
  memcpy (sendBuff, &type, sizeof (type));
  memcpy (sendBuff + sizeof (type), &len, sizeof (len));
  memcpy (sendBuff + sizeof (tlv_header_t), &job_id, sizeof (job_id));
  memcpy (sendBuff + sizeof (tlv_header_t) + sizeof (int), &job_operation,
	  sizeof (job_operation));
  /* DumpRawBytes ((unsigned char *) sendBuff,
		sizeof (tlv_header_t) + sizeof (int) + sizeof (int),
		"EXEC JOB");
  */
  write (client_sock, sendBuff,
	 sizeof (tlv_header_t) + sizeof (int) + sizeof (int));
  return NULL;
}

		/*Thread handler for Multicast group handling activities */
void *
multicast_Group_Handler (void *arguments)
{

  int mg_id = ((struct mg_argument *) arguments)->group_id, i;
  int job_number = ((struct mg_argument *) arguments)->job_id;
  char file_name[15] = {0}, client_file_name[50] = {0}, number[7]={0};
  int size, *arr_clients;
  struct client_argument *client_arg;
  int lines;
  int result_array[255];
  char cmd[256] = {0};
  //Need to add function to compare multicast group id based on group id and assign that mg group here 
  FILE *fp;
  FILE *popen ();
  threadpool thpool_client = thpool_init(16);
  strncpy (file_name, ((struct mg_argument *) arguments)->filename,
	   sizeof (file_name));
  snprintf (cmd, sizeof (cmd), "wc -l %s | awk '{print $1}'", file_name);
  if (!(fp = popen (cmd, "r")))
    {
      system ("touch popen_err.dbg");
      pthread_exit (NULL);
    }
  memset(cmd,0,sizeof(cmd));
  while (fgets (cmd, sizeof (cmd), fp) != NULL)
    {
      lines = atoi (cmd);
    }

  pclose (fp);

  find_free_clients_in_a_group (mg_id, &size, &arr_clients);
  //DELETE THIS
  update_client_with_job_assigned (multicast_groups[mg_id].job_id, mg_id,
                                   arr_clients, size);
  update_job_status_and_group (multicast_groups[mg_id].job_id, mg_id, size);

  if (size == 0)
    size = 1;
  client_arg =
    (struct client_argument *) malloc (size *
				       sizeof (struct client_argument));
  lines = floor ((double) lines / size);
  memset (cmd, 0x00, sizeof (cmd));
  sprintf (cmd, "split -l %d -a 6 --numeric-suffixes %s %d_in_file", lines,
	   file_name,multicast_groups[mg_id].job_id);
  system (cmd);
  show_all_clients (1);
  for (i = 0; i < size; i++)
    {
      client_arg[i].client_socket_id = arr_clients[i];
      client_arg[i].i = i;
      client_arg[i].result_array = result_array;
	memset(client_arg[i].file_name,0x00,sizeof(client_arg[i].file_name));
      sprintf(client_arg[i].file_name,"%d",multicast_groups[mg_id].job_id);
      strcat (client_arg[i].file_name, "_in_file");
      if (i >= 0 && i < 10)
	{
	  snprintf (number, 7, "%d", i);
	  strcat (client_arg[i].file_name, "00000");
	}
      else if (i >= 10 && i < 100)
	{
	  snprintf (number, 7, "%d", i);
	  strcat (client_arg[i].file_name, "0000");
	}
	else if (i >= 100 && i < 1000)
        {
          snprintf (number, 7, "%d", i);
          strcat (client_arg[i].file_name, "000");
        }
	else if (i >= 1000 && i < 10000)
	        {
	          snprintf (number, 7, "%d", i);
	          strcat (client_arg[i].file_name, "00");
	        }
	else if (i >= 10000 && i < 100000)
	        {
	          snprintf (number, 7, "%d", i);
	          strcat (client_arg[i].file_name, "0");
	        }
      else if (i >= 100000)
	{
	  snprintf (number, 7, "%d", i);
	}
      strcat (client_arg[i].file_name, number);
      client_arg[i].mg_id = mg_id;
      client_arg[i].job_operation = job_number;
  //    if (pthread_create
//	  (&client_thread_id[i], NULL, &client_Handler,
//	   (void *) &client_arg[i]) < 0)
//	{
//	  perror ("\n Could not create thread");
//	}
  //    pthread_join (client_thread_id[i], NULL);
        thpool_add_work(thpool_client ,(void *) client_Handler , (void * )&client_arg[i]);
       
    }
    thpool_wait(thpool_client);

  //remove_job(multicast_groups[mg_id].job_id);
  return NULL;
}


		/* Job Addition function */
void *
job_addition (int job_number, char* file_name)
{
  struct mg_argument arguments;
  int group_id;
  
  group_id = find_free_group ();
  job_id += 1;
  add_job (job_id, job_number);
  update_group_with_job_assigned (group_id, job_id);

  arguments.group_id = group_id;
   fflush(stdout);
  arguments.job_id = job_number;
  strcpy (arguments.filename, file_name);
thpool_add_work(thpool_job, (void *)&multicast_Group_Handler , (void *)&arguments);
//  pthread_create (&(mgroup_job_thread_id[group_id]), NULL,
//		  &multicast_Group_Handler, (void *) &arguments);
 // pthread_join (mgroup_job_thread_id[group_id], NULL);
}

		/*Thread handler for timer */
void *
timer_handler (void *arg)
{
  int i, hash;
  time_t systime;
  systime = time (NULL);

  while (1)
    {
      sleep (CLIENT_KA_INT+60);
      if (client_count != 0)
	{
	  for (i = 0; i < total_clients; i++)
	    {
	      systime = time (NULL);
	      if (clients[i].client_socket_fd != -1)
		{
		  if (difftime (systime, clients[i].timestamp) > 120)
		    {
		      printf ("\n Closed connection on descriptor %d\n",
			      clients[i].client_socket_fd);
		      pthread_cancel (client_thread_id[i]);
		      remove_client (clients[i].client_socket_fd);
		      close (clients[i].client_socket_fd);
		    }
		  else
		    continue;
		}
	    }
	}
    }
  pthread_exit (NULL);
}
	/* Thread for client communication */
void *
client_Communication (void *sock_sd)
{
  //Get the socket descriptor
  int client_sd = *(int *) sock_sd;
  int r, hash, read_size, buffer[MAX_SIZE], *group;
  int32_t arrayreceived[MAX_SIZE];
  char *message, client_message[MAX_SIZE];
  TLV_State tlv_s;

  hash = hash_func (client_sd);
  int index = hash_func (client_sd);
  {
    memset (arrayreceived, 0, sizeof (arrayreceived));
    init_tlv_state (&tlv_s);
    r = read (client_sd, arrayreceived, sizeof (arrayreceived));

    if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
      {
	/* We have processed all incoming connections. */

      }
    else if (read_size == -1)
      {
	perror ("\nrecv failed");
	printf ("\n thread id %ld in read size=-1", pthread_self ());
      }
    else
      {
	//DumpRawBytes ((unsigned char *) arrayreceived, r,
	//	      "Send Buffer before read");
	assemble_tlv (arrayreceived, r, &tlv_s, client_sd);
	clients[hash].timestamp = time (NULL);
      }
  }

}

	/*Thread handler to add Clients */
void *
add_client_handler (void *p_server_sd)
{
  puts ("\n Waiting for incoming connections...\n");
  int c = sizeof (struct sockaddr_in);
  int server, s, hash;
  struct epoll_event event;
  pthread_t timer_thread, tid = pthread_self ();

  server = *((int *) (p_server_sd));
  efd = epoll_create1 (0);
  if (efd == -1)
    {
      perror ("\n epoll_create");
      pthread_exit (NULL);
    }

  event.data.fd = server;
  event.events = EPOLLIN | EPOLLET;
  s = epoll_ctl (efd, EPOLL_CTL_ADD, server, &event);
  if (s == -1)
    {
      perror ("\n epoll_ctl");
      pthread_exit (NULL);
    }

  /* Buffer where events are returned */
  new_events = calloc (MAXEVENTS, sizeof event);

  if (pthread_create (&timer_thread, NULL, timer_handler, NULL) < 0)
    {
      perror ("\n Could not create thread");
    }

  while (1)
    {
      int n, i;
      n = epoll_wait (efd, new_events, MAXEVENTS, -1);

      for (i = 0; i < n; i++)
	{

	  /* An error has occured on this fd, or the socket is not ready for reading */
	  if ((new_events[i].events & EPOLLERR)
	      || (new_events[i].events & EPOLLHUP)
	      || (!(new_events[i].events & EPOLLIN)))
	    {
	      perror ("epoll error\n");
	      close (new_events[i].data.fd);
	    }

	  /* We have a notification on the listening socket, which means one or more incoming connections. */
	  else if (server == new_events[i].data.fd)
	    {
	      while (1)
		{
		  accept_ret =
		    accept (server, (struct sockaddr *) &client_info,
			    (socklen_t *) & c);
		  if (accept_ret == -1)
		    {
		      if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
			{
			  /* We have processed all incoming connections. */
			  break;
			}
		      else
			{
			  perror ("\n accept failed for client");
			  break;
			}
		    }
		  puts ("\n Client connection accepted");

		  event.data.fd = accept_ret;
		  event.events = EPOLLIN | EPOLLET;
		  s = epoll_ctl (efd, EPOLL_CTL_ADD, accept_ret, &event);
		  if (s == -1)
		    {
		      perror ("\n epoll_ctl for accepted fd cant be created");
		      pthread_exit (NULL);
		    }
		  hash = hash_func (accept_ret);
		  client_count = client_count + 1;
		}
	    }

	  else
	    {
	      if (pthread_create
		  (&client_thread_id[client_count], NULL,
		   client_Communication, (void *) &new_events[i].data.fd) < 0)
		{
		  perror ("\n Could not create thread");
		}
	    }
	}

    }

  pthread_join (timer_thread, NULL);

}

	/*Thread handler for Shell */
void *
shell_handler (void *server_sd)
{
  char end[10];
  int i;
  int server = *(int *) server_sd;
  
     if(shell_loop() == EXIT)
     {
     for(i=0; i<=client_count; i++)
     {
     pthread_cancel(client_thread_id[i]);
     }
     pthread_cancel(thread_id[0]);
     pthread_cancel(thread_id[1]);
     pthread_exit(NULL);
     }
 

  pthread_exit (NULL);
  exit(0);
}

		/*start of main function */
int
main (void)
{
  int err, s;
  total_clients = multi_cast_group * max_clients;
  int job_add;
  char file_name[50];

  //Thread creation for Shell
  err =
    pthread_create (&(thread_id[0]), NULL, &shell_handler,
		    (void *) &server_sd);
  if (err != 0)
    printf ("\n can't create add _client thread :[%s]", strerror (err));
  else
    printf ("\n Shell created successfully\n");

  //Server socket creation
  server_sd = socket (AF_INET, SOCK_STREAM, 0);
  if (server_sd == -1)
    {
      perror ("\n Server Socket is not created \n");
      exit (1);
    }

  // Socket initialzation
  memset (&server_info, 0, sizeof (server_info));
  server_info.sin_family = AF_INET;
  server_info.sin_port = htons (PORT);
  server_info.sin_addr.s_addr = inet_addr (IP_ADDR);

  //Binding the socket
  bind_ret =
    bind (server_sd, (struct sockaddr *) &server_info, sizeof (server_info));

  if (bind_ret == -1)
    {
      perror ("\n Server Socket not able to bind \n");
      exit (1);
    }
#if 1
  // Make Socket non-blocaking
  s = make_socket_non_blocking (server_sd);
  if (s == -1)
    {
      perror ("\n Server Socket unable to set as non-blocking \n");
      exit (1);
    }
#endif

  printf ("\n-----------Server has started------------\n");
  initialize_group_array ();
  initialize_client_array ();
  
  //Server listening for clients
  listen_ret = listen (server_sd, backlog);
  if (listen_ret == -1)
    {
      perror ("\n Server Socket is not listening \n");
    }
   thpool_job = thpool_init(24);
  //Thread creation for timer
  err =
    pthread_create (&(thread_id[1]), NULL, &add_client_handler,
		    (void *) &server_sd);
  if (err != 0)
    printf ("\n can't create add _client thread :[%s]", strerror (err));
  else
    printf ("\n Keepalive monitor created successfully\n");

  pthread_join (thread_id[0], NULL);
  pthread_join (thread_id[1], NULL);

  thpool_wait(thpool_job);
  return 0;
}
