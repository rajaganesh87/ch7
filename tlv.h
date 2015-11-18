#ifndef _TLV_H_
#define _TLV_H_
#include <stdbool.h>

#define NO_TLV_DATA 	0
#define TLV_CONTINUE	1

#define TEXT		1
#define FILE_WRITE	2
#define REGISTER_GROUPS 3
#define KEEP_ALIVE      4
#define EXECUTE_JOB_ID  5
#define RESULT		6
#define FILE_APPEND	7

#define JOB_MIN 	1
#define JOB_MAX		2

#define CLIENT_KA_INT	60

#if 0
Client Registration TLV:Tag - registration
  type - data - int Client Status TLV:Tag - Keepalive type - data -
status (BUSY / FREE)
     Server POST TLV:Tag - POST
       type -
       data
       - data_type (text / bytes)
  - function_to_Execute
  Server GET_RESULT Tag - GET type - data - -data_type (text / bytes)
     Server COMMANDS:Tag - COMMAND
       type - int data - send job - Execute - Abort / kill
#endif
     typedef struct __attribute__ ((packed))
{
  char srv_to_client_command;
  int execute_function;
} server_command_t;

//extern void
//DumpRawBytes(unsigned char *F, unsigned short Count, char *Title);

typedef struct __attribute__ ((packed))
{
  char filename[10];
  char *bytes;
} file_write_t;

typedef struct __attribute__ ((packed))
{
  char job_id;
} job_execute_t;

typedef struct __attribute__ ((packed))
{
  char *result_bytes;
} job_result_t;

typedef struct __attribute__ ((packed))
{
  unsigned char type;
  unsigned int length;
} tlv_header_t;

typedef struct
{
  tlv_header_t current_tlv_header;
  int remaining_header;
  int remaining_bytes;
  int r;
  int c;
  tlv_header_t *tlv_header;
  void *tlv_data;
  int len_bytes_processed;
  bool len_is_not_complete;
  bool tlv_header_is_complete;
  bool tlv_is_complete;
} TLV_State;

int init_tlv_state (TLV_State * tlv_s);
int assemble_tlv (void *recvBuff, unsigned int Bufflen,
		  TLV_State * current_tlv, int client_socket_fd);

typedef void (*print_text_function) (char *data);
typedef void (*file_write_function) (int client_fd, const void *data, int len);
typedef void (*register_group_id_function) (int client_fd, void *data,
					    int len);
typedef void (*execute_job_function) (int sfd, void *job_data);
typedef void (*job_result_save) (int client_fd, int *job_result_data);
typedef void (*keepalive_function) (char *data);
typedef void (*file_append_function) (int client_fd, const void *data, int len);
typedef struct
{
  print_text_function print_text;
  file_write_function file_write;
  file_append_function file_append;
  register_group_id_function reg_group_id;
  execute_job_function execute_job;
  job_result_save job_result;
  keepalive_function keepalive;
} TLVFunctionsCallback_t;

#if 0
typedef struct
{
  tlv_header_t current_tlv_header;
  int remaining_header;
  int remaining_bytes;
  int r;
  int c;
  void *tlv_header;
  void *tlv_data;
  int tlv_header_is_complete;
  int len_bytes_processed;
  bool len_is_not_complete;
  bool tlv_header_is_complete;
  bool tlv_is_complete;
} TLV_State;
#endif
extern TLVFunctionsCallback_t TLV_callbacks;
#endif
