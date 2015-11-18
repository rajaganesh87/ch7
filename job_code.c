#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "header.h"

struct job_list *job_list_head = NULL;;

int
add_job (int job_id, int job_operation)
{
  struct job_list *job1 =
    (struct job_list *) malloc (sizeof (struct job_list));
  job1->job_info.job_id = job_id;
  job1->job_info.job_operation = job_operation;
  job1->job_info.group_id = 0;
  job1->job_info.number_of_clients = 0;
  job1->job_info.status = NOT_STARTED;
  job1->job_info.result = -1;
  if (job_list_head == NULL)
    {
      job_list_head = job1;
      job_list_head->next = NULL;
    }

  else
    {
      job1->next = job_list_head;
      job_list_head = job1;
    }
  return 0;
}

int
remove_job (int job_id)
{
  struct job_list *prev = NULL;
  struct job_list *curr = job_list_head;
  if (curr == NULL)
    return -1;
  if (curr->job_info.job_id == job_id)
    {
      job_list_head = job_list_head->next;
      return 0;
    }
  while (curr != NULL)
    {
      if (curr->job_info.job_id != job_id)
	{
	  prev = curr;
	  curr = curr->next;
	}
      else
	break;
    }
  if (curr->job_info.job_id == job_id)
    prev->next = curr->next;
  free (curr);
  return 0;
}

int get_num_client(int job_id)
{
	  struct job_list *prev = NULL;
  struct job_list *curr = job_list_head;
  if (curr == NULL)
    return -1;
  if (curr->job_info.job_id == job_id)
    {
      return (curr->job_info.number_of_clients);
    }
}

int
get_group_id_from_job (int job_id)
{
  struct job_list *prev = NULL;
  struct job_list *curr = job_list_head;
  if (curr == NULL)
    return -1;
  if (curr->job_info.job_id == job_id)
    {
      return (curr->job_info.group_id);
    }
  while (curr != NULL)
    {
      if (curr->job_info.job_id != job_id)
	{
	  prev = curr;
	  curr = curr->next;
	}
      else
	break;
    }
  if (curr->job_info.job_id == job_id)
    {
      return (curr->job_info.group_id);
    }

  return -1;
}
int get_job_op_from_id (int job_id)
{
	 struct job_list *prev = NULL;
  struct job_list *curr = job_list_head;
  if (curr == NULL)
    return -1;
  if (curr->job_info.job_id == job_id)
    {
      return (curr->job_info.job_operation);
    }
  while (curr != NULL)
    {
      if (curr->job_info.job_id != job_id)
        {
          prev = curr;
          curr = curr->next;
        }
      else
        break;
    }
  if (curr->job_info.job_id == job_id)
    {
      return (curr->job_info.job_operation);
    }

  return -1;
}
int
update_job_status_and_group (int job_id, int group_id, int number_of_clients)
{
  struct job_list *prev = NULL;
  struct job_list *curr = job_list_head;
  if (curr == NULL)
    return -1;
  if (curr->job_info.job_id == job_id)
    {
      curr->job_info.group_id = group_id;
      curr->job_info.status = IN_PROGRESS;
      curr->job_info.number_of_clients = number_of_clients;
      return 0;
    }
  while (curr != NULL)
    {
      if (curr->job_info.job_id != job_id)
	{
	  prev = curr;
	  curr = curr->next;
	}
      else
	break;
    }
  if (curr->job_info.job_id == job_id)
    {
      curr->job_info.group_id = group_id;
      curr->job_info.status = IN_PROGRESS;
      curr->job_info.number_of_clients = number_of_clients;
    }

  return 0;
}

bool
time_to_compute_final_result (int job_id, int number_of_results)
{
  struct job_list *prev = NULL;
  struct job_list *curr = job_list_head;
  if (curr == NULL)
    return -1;
  if (curr->job_info.job_id == job_id)
    {
      if (curr->job_info.number_of_clients == number_of_results)
	{
		printf("### %d %d \n",curr->job_info.number_of_clients, number_of_results);
	return 1;
	}
      else
	return 0;
    }
  while (curr != NULL)
    {
      if (curr->job_info.job_id != job_id)
	{
	  prev = curr;
	  curr = curr->next;
	}
      else
	break;
    }
  if (curr->job_info.job_id == job_id)
    {
      if (curr->job_info.number_of_clients == number_of_results)
	return 1;
      else
	return 0;
    }
  return 0;

}

int
update_job_status_to_completed (int job_id, int result)
{
  struct job_list *prev = NULL;
  struct job_list *curr = job_list_head;
  if (curr == NULL)
    return -1;
  if (curr->job_info.job_id == job_id)
    {
      curr->job_info.status = COMPLETE;
      curr->job_info.result = result;
      return 0;
    }
  while (curr != NULL)
    {
      if (curr->job_info.job_id != job_id)
	{
	  prev = curr;
	  curr = curr->next;
	}
      else
	break;
    }
  if (curr->job_info.job_id == job_id)
    {
      curr->job_info.status = COMPLETE;
      curr->job_info.result = result;

    }

  return 0;
}


void
display_jobs_status ()
{

  struct job_list *job1;
  job1 = job_list_head;
  printf ("STATUS OF JOBS\n===================\n");
  fflush (stdout);
  while (job1->next != NULL)
    {
      printf ("Job %d   Status %d   Result %d \n", job1->job_info.job_id,
	      job1->job_info.status, job1->job_info.result);
      fflush (stdout);
      job1 = job1->next;
    }
  printf ("Job %d   Status %d   Result %d \n", job1->job_info.job_id,
	  job1->job_info.status, job1->job_info.result);
  fflush (stdout);

}

/*
int main()
{

 add_job(1); 
 add_job(2);
 add_job(3);
 display_jobs_status();
 remove_job(2);
 display_jobs_status();

 return 0;
}

*/
