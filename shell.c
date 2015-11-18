#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <err.h>
#include <unistd.h>

#include "header.h"

int init_cleanup()
{
	system("rm -rf [0-9]*_in_file*");
	system("rm -rf Result_*");
}
int
shell_loop ()
{
  char cmd[CMDLINE_LEN];
  char *params[MAX_ARGS];

	init_cleanup(); //delete this

  while (1)
    {
      printf ("Server > ");
      if (fgets (cmd, sizeof (cmd), stdin) == NULL)
	break;

      if (cmd[strlen (cmd) - 1] == '\n')
	{
	  cmd[strlen (cmd) - 1] = '\0';
	}
      readCommand (cmd, params);
      if (executeCommand (params) == EXIT)
	break;
    }
  return EXIT;
}

void
readCommand (char *cmd, char **params)
{
  int i;
  for (i = 0; i < MAX_ARGS; i++)
    {
      params[i] = strsep (&cmd, " ");
      if (params[i] == NULL)
	break;
    }
}

int
executeCommand (char **params)
{
  if (strcmp (params[0], "quit") == 0)
   {
	system("rm -rf Result_* > /dev/null");
	 return EXIT;
   }
  else if (strcmp (params[0], "help") == 0)
    {
      print_usage ();
      return OK;
    }
  else if (strcmp (params[0], "start") == 0)
   {
      if (params[1] == NULL)
        {
          print_usage ();
          return OK;
        }
      if (strcmp (params[1], "job") == 0 && (params[2] != NULL))
        {
          if (atoi (params[2]) > 2)
            {
              print_usage ();
              return OK;
            }
          if ((params[3]) == NULL)
            {
              print_usage ();
              return OK;
            }
          job_addition(atoi(params[2]),params[3]);
        }
   }
  else if (strcmp (params[0], "show") == 0)
    {
      if (params[1] == NULL)
	{
	  print_usage ();
	  return OK;
	}
      if (strcmp (params[1], "group") == 0 && (params[2] != NULL))
	{
	  if (atoi (params[2]) > 255)
	    {
	      print_usage ();
	      return OK;
	    }
	  show_all_clients (atoi (params[2]));
	}
      if (strcmp (params[1], "messages") == 0)
	{
	  display_msgs_per_group ();
	}
      else if ((strcmp (params[1], "clients") == 0))
	display_clients_in_the_client_array ();
    }
  else if (strcmp ("stats", params[0]) == 0 && (params[1] != NULL))
    {
      int i;
      int value = 0;
      struct timeval timeout;
      fd_set read_fd;
	if( (strcmp("job",params[1]) ==0)) {
		display_jobs_status ();
		return OK;
	}
      if (atoi (params[1]) > 20)
	{
	  print_usage ();
	  return OK;
	}
      while (1)
	{
	  system ("clear");
	  for (i = 0; i < 255; i++)
	    {
	      show_all_clients (i);
	    }
	  printf ("Press any key to exit..");
	  fflush (stdout);
	  FD_ZERO (&read_fd);
	  FD_SET (0, &read_fd);
	  timeout.tv_sec = atoi (params[1]);
	  timeout.tv_usec = 0;
	  switch (select (1, &read_fd, NULL, NULL, &timeout))
	    {
	    case -1:
	      break;
	    case 0:
	      continue;
	    }
	  return OK;
	}
    }
  else if (strlen (params[0]))
    print_usage ();

  return OK;

}

void
print_usage ()
{
  printf ("\n");
  printf ("Available commands \n");
  printf ("-------------------\n");
  printf ("\thelp\n\tquit\n");
  printf ("\tstats <refresh_interval> - Displays client info periodically\n");
  printf ("\tshow <sub-commands> <...>\n");
  printf ("\t  group <id> - Displays client in group id\n");
  printf ("\t  clients  - Displays all clients\n");
  printf ("\t  messages - Displays messages sent by client\n");
  printf ("\tstart job <job_id> <data-set>\n");
  printf ("\t  job_id: 1 -> Min, 2 -> Max\n");
  printf ("\t  data-set: file-name of data-set\n");
}

/*
int main ()
{
	shell_loop();
	
	return 0;
}
*/
