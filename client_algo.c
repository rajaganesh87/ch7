#include "header.h"
#include <string.h>
#include <time.h>

void
initialize_client_array ()
{
  int i;
  for (i = 0; i < MAX_NUMBER_OF_CLIENTS; i++)
    {
      clients[i].client_socket_fd = -1;
      clients[i].msg = NULL;
      clients[i].groups_joined = NULL;
      clients[i].number_of_groups = 0;
      clients[i].timestamp = 0;
    }
}



int
hash_func (int socket_fd)
{
  return (socket_fd % 255000);
}



// groups is a pointer to an integer array of groups to which client wishes to join
int
add_client (int socket_fd, int *groups, int number_of_groups)
{

  int i = hash_func (socket_fd);
  // Check if no other client is already present
  if (clients[i].client_socket_fd == -1)
    {
      int j;
      int *ptr = malloc (sizeof (int) * number_of_groups);

      for (j = 0; j < number_of_groups; j++)
	{
	  ptr[j] = groups[j];
	}

      clients[i].client_socket_fd = socket_fd;
      clients[i].msg = NULL;
      clients[i].groups_joined = ptr;
      clients[i].number_of_groups = number_of_groups;
      clients[i].timestamp = time (NULL);

      return 0;
    }
  else
    {
      printf ("Oops! Client not added..");
      return -1;
    }
}



int
remove_client (int socket_fd)
{
  int i = hash_func (socket_fd);
  // Found the client in the array
  if (clients[i].client_socket_fd == socket_fd)
    {
      clients[i].client_socket_fd = -1;
      clients[i].msg = NULL;

      // When add client is called, we pass a pointer to an array of groups client wishes to join(memory allocated using malloc for the array). Free the memory while removing the client
      free (clients[i].groups_joined);
      clients[i].groups_joined = NULL;
      clients[i].number_of_groups = 0;
      return 0;
    }
  else
    {
      printf ("Oops..Client not found!!");
      return -1;
    }
}



int
add_client_msg (int client_socket_fd, char *msg)
{
  int i = hash_func (client_socket_fd);
  if (clients[i].client_socket_fd == client_socket_fd)
    {
      char *buffer;
      buffer = calloc (MAX_SIZE, sizeof (char));
      strcpy (buffer, msg);
      clients[i].msg = buffer;
      return 0;
    }
  else
    {
      printf ("Client not found..can't add msg \n");
      return -1;
    }

}

void
display_clients_in_the_client_array ()
{

  int i;
	printf("ID\tMembership Count\tGroups...\n");
  for (i = 0; i < MAX_NUMBER_OF_CLIENTS; i++)
    {
      if (clients[i].client_socket_fd != -1)
	{
	  printf ("%d\t", clients[i].client_socket_fd);
	  printf ("\t%d\t", clients[i].number_of_groups);
	  int j;
	  int *ptr = clients[i].groups_joined;
	  for (j = 0; j < clients[i].number_of_groups; j++)
	    {
	      printf ("%d,", ptr[j]);
	    }
		printf("\n");
	}
    }
}
