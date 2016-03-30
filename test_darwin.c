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

	printf ("%s:%d: current=%u\n", __func__, __LINE__, (unsigned)current);

	count = proc_listpids (PROC_ALL_PIDS, 0, NULL, 0);
	printf ("%s:%d: count=%lu\n", __func__, __LINE__, (unsigned long int)count);

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
	printf ("%s:%d: ultimate_parent=%u\n", __func__, __LINE__, (unsigned)ultimate_parent);

	for (i = 1; i < count; i++) {
		pid = pids[i];
		if (pid < ultimate_parent) {
			ultimate_parent = pid;
		}
	}

	printf ("%s:%d: ultimate_parent=%u\n", __func__, __LINE__, (unsigned)ultimate_parent);

	while (! done) {
		for (i = 0; i < count && !done; i++) {
			pid = pids[i];
			printf ("%s:%d: i=%d, pid=%d\n", __func__, __LINE__, i, (unsigned)pid);

			if (pid == current) {
				char name[1024];

				ret = proc_name (pid, name, sizeof (name));
				if (! ret)
					goto out;

				printf ("%s:%d: i=%d, pid=%d, name='%s'\n", __func__, __LINE__, i, (unsigned)pid, name);

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
				printf ("%s:%d: changed current to %u\n", __func__, __LINE__, (unsigned)current);
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
