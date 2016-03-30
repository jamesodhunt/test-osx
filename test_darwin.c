#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/proc_info.h>
#include <libproc.h>

static void
handle_proc_branch_darwin (void)
{
	int                  count = 0;
	pid_t               *pids = NULL;
	pid_t                pid;
	pid_t                current;
	pid_t                ultimate_parent = 0;

	int                  i;
	int                  ret;
	int                  done = false;

	current = getpid ();

	count = proc_listpids (PROC_ALL_PIDS, 0, NULL, 0);
	pids = calloc (count, sizeof (pid_t));
	if (! pids)
		return;

	ret = proc_listpids (PROC_ALL_PIDS, 0, pids, sizeof (pids));
	if (ret < 0)
		goto out;

	/* Calculate the lowest PID number which gives us the ultimate
	 * parent of all processes.
	 */
	ultimate_parent = pids[0];

	for (i = 1; i < count; i++) {
		pid = pids[i];
		if (pid < ultimate_parent)
			ultimate_parent = pid;
	}

	while (! done) {
		for (i = 0; i < count && !done; i++) {
			pid = pids[i];

			if (pid == current) {
				char name[1024];

				ret = proc_name (pid, name, sizeof (name));
				if (! ret)
					goto out;

				if (ultimate_parent == 1 && current == ultimate_parent) {

					/* Found the "last" PID so record it and hop out */
					printf ("%d ('%s')\n", (int)current, name);
					done = true;
					break;
				} else {
					/* Found a valid parent pid */
					printf ("%d ('%s'), ", (int)current, name);
				}

				/* Move on */
				current = pid;
			}
		}
	}

out:
	if (pids)
        free (pids);
}

int
main (int argc, char *argv[])
{
    handle_proc_branch_darwin ();

    exit (EXIT_SUCCESS);
}
