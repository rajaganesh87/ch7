#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include<stdbool.h>
enum STATUS
{ FREE = 0, BUSY = 1 };

#define MAX_NUMBER_OF_CLIENTS 255000
#define NUMBER_OF_GROUPS 1000
#define MAX_SIZE	(4*1024)
#define MAX_CLIENTS_IN_GROUP	255


#define OK              0
#define EXIT            100
#define MAX_ARGS        5
#define CMDLINE_LEN     80

void readCommand (char* cmd,  char** params);
int executeCommand (char** params);
void print_usage(void);
int shell_loop(void);

enum job_status
{ NOT_STARTED = -1, IN_PROGRESS = 0, COMPLETE };

extern struct job_list *job_list_head;

struct job
{
  int job_id;
  int job_operation;
  int group_id;
  int number_of_clients;
  int status;
  int result;
};


struct job_list
{
  struct job job_info;
  struct job_list *next;
};


//struct job_list *job_list_head = NULL;;

struct client_info
{

  int client_socket_fd;
  int client_status;
  int job_id;
  char *msg;
  int *groups_joined;
  int number_of_groups;
  time_t timestamp;
  pthread_t client_thread_id;
};


typedef struct client_info CLIENT_INFO;
CLIENT_INFO clients[MAX_NUMBER_OF_CLIENTS];

struct client
{
  int client_socket_fd;
  struct client *next;
};

typedef struct client CLIENT;

struct group
{

  int group_status;
  int job_id;
  int num_of_clients;
  struct client *head;
};

typedef struct group GROUP;

GROUP multicast_groups[NUMBER_OF_GROUPS];


void initialize_client_array ();
int hash_func (int socket_fd);
int add_client (int socket_fd, int *groups, int number_of_groups);
int remove_client (int socket_fd);
int add_client_msg (int client_socket_fd, char *msg);
void display_clients_in_the_client_array ();
void initialize_group_array ();
void add_a_client_to_groups (int client_socket_fd, int *groups,
			     int number_of_groups);

void remove_client_from_groups (int client_socket_fd, int groups_joined[],
				int number_of_groups);

void display_msgs_per_group ();

void show_all_clients (int group_id);

int find_free_group ();

void find_free_clients_in_a_group (int group_id, int *size,
				   int **free_clients);

void update_group_with_job_assigned (int group_id, int job_id);

int is_client_present (int client_socket_fd, int *clients_assigned, int size);

void update_client_with_job_assigned (int job_id, int group_id,
				      int *clients_assigned, int size);

void update_client_status_to_free (int client_socket_fd, int job_id);

void update_group_status_to_free (int group_id, int job_id);



int add_job (int job_id, int job_operation);

int remove_job (int job_id);

int get_group_id_from_job (int job_id);

int update_job_status_and_group (int job_id, int group_id,
				 int number_of_clients);

int update_job_status_to_completed (int job_id, int result);

bool time_to_compute_final_result (int job_id, int number_of_results);

void display_jobs_status ();
