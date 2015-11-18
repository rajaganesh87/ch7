#include <pthread.h>
#include "header.h"
void
initialize_group_array ()
{
  int i;
  for (i = 0; i < NUMBER_OF_GROUPS; i++)
    {
      multicast_groups[i].num_of_clients = 0;
      multicast_groups[i].group_status = FREE;
      multicast_groups[i].job_id = -1;
      multicast_groups[i].head = NULL;
    }
}

void
add_a_client_to_groups (int client_socket_fd, int *groups,
			int number_of_groups)
{

  printf ("Adding client %d to groups specified \n", client_socket_fd);
  CLIENT *client;
  int i;
  for (i = 0; i < number_of_groups; i++)
    {
      client = (struct client *) malloc (sizeof (struct client));
      client->client_socket_fd = client_socket_fd;
      client->next = NULL;
      printf ("Adding client %d to group %d \n", client_socket_fd, groups[i]);
      multicast_groups[groups[i]].num_of_clients =
	multicast_groups[groups[i]].num_of_clients + 1;
      if (multicast_groups[groups[i]].head == NULL)
	{
	  multicast_groups[groups[i]].head = client;
	}
      else
	{
	  struct client *temp = multicast_groups[groups[i]].head;
	  multicast_groups[groups[i]].head = client;
	  client->next = temp;
	}
    }
}

void
remove_client_from_groups (int client_socket_fd, int groups_joined[],
			   int number_of_groups)
{

  int i;
  for (i = 0; i < number_of_groups; i++)
    {
      if (multicast_groups[groups_joined[i]].head != NULL)
	{
	  struct client *prev = multicast_groups[groups_joined[i]].head;
	  if (prev->client_socket_fd == client_socket_fd)
	    {
	      multicast_groups[groups_joined[i]].head = prev->next;
	      prev->next = NULL;
	      free (prev);
	    }
	  else
	    {
	      struct client *current;
	      while ((current = prev->next) != NULL)
		{
		  if (current->client_socket_fd == client_socket_fd)
		    {
		      prev->next = current->next;
		      current->next = NULL;
		      free (current);
		      break;
		    }
		  else
		    {
		      prev = current;
		    }
		}
	    }
	}
    }
}



int
find_free_group ()
{

  int i;
  for (i = 0; i < NUMBER_OF_GROUPS; i++)
    {
      if (multicast_groups[i].group_status == FREE
	  && multicast_groups[i].head != NULL)
	{
	  return i;
	}
    }
  return -1;
}



void
find_free_clients_in_a_group (int group_id, int *size, int **free_clients)
{

  int count = 0;
  int fd, i;

  if (multicast_groups[group_id].head != NULL)
    {
      CLIENT *curr = multicast_groups[group_id].head;
      while (curr != NULL)
	{
	  fd = curr->client_socket_fd;
	  i = hash_func (fd);
	  if (clients[i].client_socket_fd == fd
	      && clients[i].client_status == FREE)
	    count++;
	  curr = curr->next;
	}

      *free_clients = (int *) malloc (sizeof (int) * count);
      *size = count;
      printf ("NUMBER OF FREE CLIENTS : %d\n", count);
      curr = multicast_groups[group_id].head;
      int j = 0;
      while (curr != NULL)
	{
	  fd = curr->client_socket_fd;
	  i = hash_func (fd);
	  if (clients[i].client_socket_fd == fd
	      && clients[i].client_status == FREE)
	    {
	      (*free_clients)[j] = clients[i].client_socket_fd;
	      j++;
	    }
	  curr = curr->next;
	}
    }
}



void
update_group_with_job_assigned (int group_id, int job_id)
{
  multicast_groups[group_id].job_id = job_id;
  multicast_groups[group_id].group_status = BUSY;
}

int
is_client_present (int client_socket_fd, int *clients_assigned, int size)
{

  int i;
  for (i = 0; i < size; i++)
    {
      if (clients_assigned[i] == client_socket_fd)
	return 1;
    }
  return 0;
}



void
update_client_with_job_assigned (int job_id, int group_id,
				 int *clients_assigned, int size)
{

  int i;

  struct client *curr = multicast_groups[group_id].head;
  while (curr != NULL)
    {
      if (is_client_present (curr->client_socket_fd, clients_assigned, size))
	{
	  i = hash_func (curr->client_socket_fd);
	  if (clients[i].client_socket_fd == curr->client_socket_fd)
	    {
	      clients[i].job_id = job_id;
	      clients[i].client_status = BUSY;
	    }
	}
      curr = curr->next;
    }
}



void
update_client_status_to_free (int client_socket_fd, int job_id)
{

  int i;

  i = hash_func (client_socket_fd);
  if (clients[i].client_socket_fd == client_socket_fd)
    {
      if (clients[i].job_id == job_id)
	clients[i].client_status = FREE;
    }
}



void
update_group_status_to_free (int group_id, int job_id)
{
  if (multicast_groups[group_id].job_id == job_id)
    {
      multicast_groups[group_id].group_status = FREE;
    }
}



void
display_msgs_per_group ()
{

  int i;
  for (i = 0; i < 1000; i++)
    {
      if (multicast_groups[i].head != NULL)
	{
	  printf ("Group %d \n--------\n", i);
	  CLIENT *client = multicast_groups[i].head;
	  while (client != NULL)
	    {
	      int j = hash_func (client->client_socket_fd);
	      printf ("Client %d : Status %s\n", client->client_socket_fd,
		      ((clients[j].client_status == FREE) ? "FREE" : "BUSY"));
	      client = client->next;
	    }
	}
    }
}

void
show_all_clients (int group_id)
{

  CLIENT *client = multicast_groups[group_id].head;
  if (client != NULL)
    printf ("Group %d \n", group_id);
  while (client != NULL)
    {
      int j = hash_func (client->client_socket_fd);
      printf ("\tClient: %d \n", client->client_socket_fd);
      printf ("\tStatus  : %s \n", ((clients[j].client_status == FREE) ? "FREE" : "BUSY"));
      if (client->next == NULL)
	{
	  return;
	}
      client = client->next;
    }
}
