#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dlfcn.h>
#include "tlv.h"
#include "header.h"

void
DumpRawBytes (unsigned char *F, unsigned short Count, char *Title);

int
init_tlv_state (TLV_State * tlv_s)
{
  tlv_s->r = 0;
  tlv_s->tlv_is_complete = true;
  tlv_s->tlv_header = NULL;
  tlv_s->tlv_data = NULL;
  tlv_s->c = 0;
  tlv_s->tlv_header_is_complete = true;
  tlv_s->len_bytes_processed = 0;
  tlv_s->len_is_not_complete = false;
  tlv_s->remaining_header = 0;
  tlv_s->remaining_bytes = 0;
  memset (&tlv_s->tlv_header, 0x00, sizeof (tlv_header_t));
  memset (&tlv_s->current_tlv_header, 0x00, sizeof (tlv_header_t));
  return 0;
}

int
assemble_tlv (void *recvBuff, unsigned int Bufflen, TLV_State * tlv_s,
	      int client_socket_fd)
{
start:
  if (((Bufflen - tlv_s->r) <= 0) && (tlv_s->tlv_is_complete == true))
    {
      tlv_s->r = 0;
      //printf ("Nothing to read \n");
      return NO_TLV_DATA;
    }
  if (((Bufflen - tlv_s->r) < sizeof (tlv_header_t))
      && tlv_s->tlv_header_is_complete && (tlv_s->remaining_bytes == 0))
    {
      /* header is split across 2 packets */
      tlv_s->current_tlv_header.type = *((char *) (recvBuff + tlv_s->r));
      tlv_s->r += sizeof (tlv_s->current_tlv_header.type);
      tlv_s->tlv_header_is_complete = false;
      tlv_s->len_bytes_processed = 0;
    }
  if (tlv_s->tlv_header_is_complete == false)
    {
      if ((Bufflen - tlv_s->r) == 0)
	{
	  tlv_s->r = 0;
	  return TLV_CONTINUE;
	}
      tlv_s->remaining_header = Bufflen - tlv_s->r;
      while ((tlv_s->len_bytes_processed <
	      sizeof (tlv_s->current_tlv_header.length)))
	{
	  /* assemble tlv_length split accross packets */
	  if (tlv_s->remaining_header == 0)
	    break;
	  memcpy ((char *) &tlv_s->current_tlv_header.length,
		  recvBuff + tlv_s->r, 1);
	  tlv_s->r += 1;
	  tlv_s->remaining_header -= 1;
	  tlv_s->len_bytes_processed++;
	  if (tlv_s->len_bytes_processed <
	      sizeof (tlv_s->current_tlv_header.length))
	    tlv_s->len_is_not_complete = true;
	  else
	    tlv_s->len_is_not_complete = false;
	}
      if ((tlv_s->len_bytes_processed ==
	   sizeof (tlv_s->current_tlv_header.length)))
	{
	  tlv_s->tlv_header_is_complete = true;
	  tlv_s->len_is_not_complete = false;
	  // TODO : Below assignment works only for little-endian systems
	  //        need to add ntohl() on big-endian systems
	  tlv_s->remaining_bytes = tlv_s->current_tlv_header.length;
	//	printf("TLV LEN1: %d\n",tlv_s->current_tlv_header.length);
	}
      if (tlv_s->len_is_not_complete)
	{
	  tlv_s->r = 0;
	//	printf("Return 1\n");
	  return TLV_CONTINUE;
	}
    }
  if (((Bufflen - tlv_s->r) >= sizeof (tlv_header_t))
      && (tlv_s->tlv_is_complete))
    {
      /* new tlv header is available */
      tlv_s->tlv_header = recvBuff + tlv_s->r;
      tlv_s->remaining_bytes = ntohl (tlv_s->tlv_header->length);
	//printf("TLV LEN2: %d : %d\n",tlv_s->remaining_bytes,tlv_s->tlv_header->length );
	tlv_s->r += sizeof (tlv_header_t);
      if (tlv_s->tlv_data != NULL)
	free (tlv_s->tlv_data);
      tlv_s->tlv_data = NULL;
      tlv_s->current_tlv_header.type = tlv_s->tlv_header->type;
      tlv_s->current_tlv_header.length = ntohl (tlv_s->tlv_header->length);
    }

  if (tlv_s->remaining_bytes > (Bufflen - tlv_s->r))
    {
      /* continue filling partial data */
      tlv_s->tlv_data = realloc (tlv_s->tlv_data, Bufflen - tlv_s->r);
      if (tlv_s->tlv_data == NULL)
	printf ("Realloc ERROR !!\n");
      memcpy (tlv_s->tlv_data + tlv_s->c, recvBuff + tlv_s->r,
	      Bufflen - tlv_s->r);
      tlv_s->remaining_bytes -= Bufflen - tlv_s->r;
      tlv_s->c += Bufflen - tlv_s->r;
      tlv_s->r = 0;
      tlv_s->tlv_is_complete = false;
	//printf("RMB : %d READ %d \n",tlv_s->remaining_bytes,(Bufflen - tlv_s->r));
      return TLV_CONTINUE;
    }
  else
    {
      if (tlv_s->tlv_is_complete)
	{
	  /* data is complete with this */
	void *np = NULL;
	  /*tlv_s->tlv_data*/ np = realloc (tlv_s->tlv_data, tlv_s->remaining_bytes);
	if(!np)
	{
		perror("Realloc EERROORR!!\n");	
	}
	tlv_s->tlv_data = np;
	//printf("After Realloc1  DP : %p \n",tlv_s->tlv_data);
	//printf("RB1: %d \n",tlv_s->remaining_bytes);
	//DumpRawBytes ((unsigned char *) recvBuff,
        //          Bufflen, "Recv Buffer");
	  memcpy (tlv_s->tlv_data, recvBuff + tlv_s->r,
		  tlv_s->remaining_bytes);
	}
      else
	{
	  /* partial data in this packet */
	void *np = NULL;
	DumpRawBytes ((unsigned char *) tlv_s->tlv_data,
                 tlv_s->c  , "Before Realloced Mem");
          /*tlv_s->tlv_data*/ np = realloc (tlv_s->tlv_data, tlv_s->remaining_bytes);
        if(!np)
        {
                perror("Realloc EERROORR!!\n");
        }
        tlv_s->tlv_data = np;

	//printf("After Realloc2  DP : %p \n",tlv_s->tlv_data);
	//printf("RB2: %d C: %d \n",tlv_s->remaining_bytes,tlv_s->c);	
	//DumpRawBytes ((unsigned char *) tlv_s->tlv_data,
        //         tlv_s->c+ tlv_s->remaining_bytes , "Before copy Realloced Mem");
	  memcpy (tlv_s->tlv_data + tlv_s->c, recvBuff,
		  tlv_s->remaining_bytes);
	//DumpRawBytes ((unsigned char *) tlv_s->tlv_data,
        //         tlv_s->c+  tlv_s->remaining_bytes , "After copy Realloced Mem");
	}
      tlv_s->r = tlv_s->r + tlv_s->remaining_bytes;
      tlv_s->remaining_bytes = 0;
      tlv_s->tlv_is_complete = true;
      tlv_s->c = 0;
    }


  if (tlv_s->tlv_is_complete == true)
    {
      /* tlv header and data processed, handle below */
      /* handler SHOULD free tlv_data */
	//printf("Incoming header len : %d DP : %p \n",tlv_s->current_tlv_header.length,tlv_s->tlv_data);
	//DumpRawBytes ((unsigned char *)  tlv_s->tlv_data, tlv_s->current_tlv_header.length, "TLV data");
      switch (tlv_s->current_tlv_header.type)
	{
	case TEXT:
	  TLV_callbacks.print_text ((char *) tlv_s->tlv_data);
	  tlv_s->tlv_data = NULL;
	  tlv_s->current_tlv_header.type = 0xFF;
	  tlv_s->current_tlv_header.length = 0;
	  break;
	case FILE_WRITE:
	  TLV_callbacks.file_write (client_socket_fd,
				    (void *) tlv_s->tlv_data,
				    tlv_s->current_tlv_header.length);
	  if (tlv_s->tlv_data != NULL)
	    free (tlv_s->tlv_data);
	  tlv_s->tlv_data = NULL;
	  tlv_s->current_tlv_header.type = 0xFF;
	  tlv_s->current_tlv_header.length = 0;
	  break;
	case FILE_APPEND:
	  //printf("DP in FA : %p \n",tlv_s->tlv_data);
	  TLV_callbacks.file_append (client_socket_fd,
                                    (void *) tlv_s->tlv_data,
                                    tlv_s->current_tlv_header.length);
	//printf("DP af FA : %p \n",tlv_s->tlv_data);fflush(stdout);
          if (tlv_s->tlv_data != NULL)
          { //printf("freeing %p \n",tlv_s->tlv_data); fflush(stdout);
		  free (tlv_s->tlv_data);
	}
          tlv_s->tlv_data = NULL;
          tlv_s->current_tlv_header.type = 0xFF;
          tlv_s->current_tlv_header.length = 0;
	  break;
	case REGISTER_GROUPS:
	  TLV_callbacks.reg_group_id (client_socket_fd,
				      (void *) tlv_s->tlv_data,
				      tlv_s->current_tlv_header.length);
	  if (tlv_s->tlv_data != NULL)
	    free (tlv_s->tlv_data);
	  tlv_s->tlv_data = NULL;
	  tlv_s->current_tlv_header.type = 0xFF;
	  tlv_s->current_tlv_header.length = 0;
	  break;
	case KEEP_ALIVE:
	  TLV_callbacks.keepalive ((char *) tlv_s->tlv_data);
	  if (tlv_s->tlv_data != NULL)
	    free (tlv_s->tlv_data);
	  tlv_s->tlv_data = NULL;
	  tlv_s->current_tlv_header.type = 0xFF;
	  tlv_s->current_tlv_header.length = 0;
	  break;
	case EXECUTE_JOB_ID:
	  TLV_callbacks.execute_job (client_socket_fd,
				     (char *) tlv_s->tlv_data);
	  if (tlv_s->tlv_data != NULL)
	    free (tlv_s->tlv_data);
	  tlv_s->tlv_data = NULL;
	  tlv_s->current_tlv_header.type = 0xFF;
	  tlv_s->current_tlv_header.length = 0;
	  break;
	case RESULT:
	  TLV_callbacks.job_result (client_socket_fd,
				    (int *) tlv_s->tlv_data);
	  if (tlv_s->tlv_data != NULL)
	    free (tlv_s->tlv_data);
	  tlv_s->tlv_data = NULL;
	  tlv_s->current_tlv_header.type = 0xFF;
	  tlv_s->current_tlv_header.length = 0;
	  break;

	default:
	  tlv_s->r = 0;
	  if (tlv_s->tlv_data != NULL)
	    free (tlv_s->tlv_data);
	  printf ("Unknown data format received, discarding packet \n");
	}
      /* process next tlv in same packet, if any */
      goto start;
      //return TLV_CONTINUE;
    }
  else
	{
	tlv_s->tlv_is_complete = false;
    return TLV_CONTINUE;
	}
}

void
Handle_text (char *text)
{
  printf ("%s\n", text);
  free (text);
}

/*****************************************************************************/
void
DumpRawBytes (unsigned char *F, unsigned short Count, char *Title)
{
  unsigned short i = 0;
  unsigned short j = 0;
  char data[16] = { 0 };

  printf
    ("========================================================================\n");
  printf ("===== %s\n", Title);
  printf
    ("========================================================================");
  while (i < Count)
    {
      if ((i % 16) == 0)
	{
	  for (j = 0; j < 16; j++)
	    {
	      if (i != 0)
		isalnum (data[j]) ? printf ("%c", data[j]) : printf (".");
	    }
	  printf ("\n   ");
	  j = 0;
	}
      printf ("%02X ", *(F + i));
      data[j++] = *(F + i);
      i++;
    }				/* end while */
  j = 16 - (i % 16);
  while (j--)
    printf ("   ");
  for (j = 0; j < (i % 16); j++)
    {
      isalnum (data[j]) ? printf ("%c", data[j]) : printf (".");
    }
  printf
    ("\n========================================================================\n");

}				/* end routine */

void
Handle_file_write (int client_socket_fd, const void *filedata, int file_size_bytes)
{
  char filename[64] = { 0 };
  FILE *fp = NULL;
  int i = 0;
  //DumpRawBytes ((unsigned char *) filedata, file_size_bytes, "Filedata");
  strncpy (filename, filedata, 10);
  fp = fopen (filename, "wb");
	//printf("FSB : %d\n",file_size_bytes);
  if ((file_size_bytes - 10 )> 0)
    {
      fwrite ((char *) (filedata + 10 ), 1,
                      ((file_size_bytes - 10)), fp);
    }
  fclose (fp);
}

void
Handle_file_append (int client_socket_fd, const void *filedata, int file_size_bytes)
{
  char filename[64] = { 0 };
  FILE *fp = NULL;
  int i = 0;
  //DumpRawBytes ((unsigned char *) filedata, file_size_bytes, "Filedata"); 
  strncpy (filename, filedata, 10);
  fp = fopen (filename, "ab+");
 if ((file_size_bytes - 10 )> 0)
    {
              fwrite ( ((char*)filedata + 10 ), 1,
                      ((file_size_bytes - 10)), fp);
    }
  fclose (fp);
}

void
Handle_group_id_regs (int client_socket_fd, void *group_ids, int size)
{
  int i = 0, j = 0;
  int *group_array;
  j = size / sizeof (int);

  group_array = (int *) malloc (size);
  //DumpRawBytes ((unsigned char *) group_ids, size, "Num data");
  while (size > 0)
    {
      int a;
      memcpy (&a, group_ids + (i * sizeof (int)), sizeof (int));
      group_array[i] = ntohl (a);
      i++;
      size -= sizeof (int);
    }
  add_a_client_to_groups (client_socket_fd, group_array, j);
  add_client (client_socket_fd, group_array, j);
  //display_clients_in_the_client_array ();
}

void
Handle_keepalive (char *dummy)
{
  ;
}

void
Handle_execute_job (int sfd, void *jobdata)
{
  int job = -1;
  void *handle;
  int (*min_max) (char *filename);
  char *error;

  int job_operation = -1;
  //DumpRawBytes ((unsigned char *) jobdata, 8, "CLIENT EXEC JOB");
  memcpy (&job, (int *) jobdata, sizeof (int));
  memcpy (&job_operation, (int *) (jobdata) + 1, sizeof (int));
  job = ntohl (job);
  job_operation = ntohl (job_operation);
  printf (">>>>>>>>> Executing Job ID %d Job Num:%d <<<<<<<<<<\n", job,
	  job_operation);
  fflush (stdout);
  handle = dlopen ("./libminmax.so.1.0.1", RTLD_LAZY);
  if (!handle)
    {
      fputs (dlerror (), stderr);
      exit (1);
    }
	//sleep(15);
  switch (job_operation)
    {
    case JOB_MIN:
      min_max = dlsym (handle, "minimum_of_numbers");
      break;
    case JOB_MAX:
      min_max = dlsym (handle, "maximum_of_numbers");
      break;
    default:
      printf ("Invalid job Id :%d\n", job);
    }
  if ((error = dlerror ()) != NULL)
    {
      fputs (error, stderr);
      exit (1);
    }
  int result = (*min_max) ("b.txt");
  result = htonl (result);
  char type = RESULT;
  int len = htonl (sizeof (int) * 2);
  char sendBuff[MAX_SIZE];
  memset (sendBuff, 0, sizeof (sendBuff));
  memcpy (sendBuff, &type, sizeof (type));
  memcpy (sendBuff + sizeof (type), &len, sizeof (int));
  memcpy (sendBuff + sizeof (type) + sizeof (int), &result, sizeof (result));
  job = htonl (job);
  memcpy (sendBuff + sizeof (tlv_header_t) + sizeof (int), &job,
	  sizeof (job));
  //DumpRawBytes ((unsigned char *) sendBuff,
//		sizeof (result) + sizeof (tlv_header_t) + sizeof (job),
//		"REUSLT JOB");
  write (sfd, sendBuff,
	 sizeof (result) + sizeof (tlv_header_t) + sizeof (job));
  unlink ("b.txt");
	printf("Job execution complete !!\n");fflush(stdout);
}

void
Handle_result (int client_socket_fd, int *result_bytes)
{
  FILE *fp1;
  char buff[100] = { 0 };
  char filename[50] = { 0 }, job_str[50] =
  {
  0};

static int hack = 0;
  int result = ntohl (*result_bytes);
  int job_id = ntohl (*(result_bytes + 1));
  strcpy (filename, "Result_");
  snprintf (job_str, sizeof (job_str), "%d", job_id);
  strcat (filename, job_str);
  FILE *fp = fopen (filename, "a+");
  fprintf (fp, "%d\n", result);
  fclose (fp);
  update_client_status_to_free (client_socket_fd, job_id);
  int number_of_results = 0;
  char cmd[128] = { 0 };
  snprintf (cmd, sizeof (cmd), "wc -l %s | awk '{print $1}'", filename);
  if (!(fp1 = popen (cmd, "r")))
    {
      system ("touch ropen_err.dbg");
      exit (1);
    }

  while (fgets (buff, sizeof (buff), fp1) != NULL)
    {
      number_of_results = atoi (buff);
    }

  if (time_to_compute_final_result (job_id, number_of_results))
    {
      int group_id = get_group_id_from_job (job_id);
      update_group_status_to_free (group_id, job_id);
      int job = -1;
      void *handle;
      int (*min_max) (char *filename);
      char *error;
      handle = dlopen ("./libminmax.so.1.0.1", RTLD_LAZY);
      if (!handle)
	{
	  fputs (dlerror (), stderr);
	  exit (1);
	}
	job = get_job_op_from_id(job_id);
      switch (job)
	{
	case JOB_MIN:
	  min_max = dlsym (handle, "minimum_of_numbers");
	  break;
	case JOB_MAX:
	  min_max = dlsym (handle, "maximum_of_numbers");
	  break;
	default:
	  printf ("Invalid job Id :%d\n", job);
	}
      if ((error = dlerror ()) != NULL)
	{
	  fputs (error, stderr);
	  exit (1);
	}
      result = (*min_max) (filename);
      printf ("\nJob %d complete !\n", job_id);
      update_job_status_to_completed (job_id, result);
      //display_jobs_status ();
      //unlink(filename);
    }
}

TLVFunctionsCallback_t TLV_callbacks = {
  .print_text = Handle_text,
  .file_write = Handle_file_write,
  .reg_group_id = Handle_group_id_regs,
  .keepalive = Handle_keepalive,
  .execute_job = Handle_execute_job,
  .job_result = Handle_result,
  .file_append = Handle_file_append,
};
