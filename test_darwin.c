#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdbool.h>
#include <errno.h>

#include <sys/sysctl.h>

#define mib_len(mib) ((sizeof (mib) / sizeof(*mib)) - 1)

static void
handle_proc_branch_darwin (void)
{
	struct kinfo_proc   *procs = NULL;
	struct kinfo_proc   *p;
	static const int     mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
	int                  ret;
	int                  done = false;
	size_t               bytes;
	size_t               count;
	size_t               i;
	pid_t                current;
	pid_t                ultimate_parent = 0;

	/* arbitrary */
	int                  attempts = 20;

	current = getpid ();

	printf ("%s:%d: current=%u\n", __func__, __LINE__, (unsigned)current);

	/* XXX: This system interface seems horribly racy - what if the numbe of pids
	 * XXX: changes between the 1st and 2nd call to sysctl() ???
	 */
	while (! done) {
		attempts--;
		if (! attempts)
			return;

		/* determine how much space required to store details of all
		 * processes.
		 */
		ret = sysctl ((int *)mib, mib_len (mib), NULL, &bytes, NULL, 0);
		if (ret < 0)
			return;

		count = bytes / sizeof (struct kinfo_proc);
		printf ("%s:%d: bytes=%u\n", __func__, __LINE__, (unsigned)bytes);
		printf ("%s:%d: proc count=%u\n", __func__, __LINE__, (unsigned)count);

		/* allocate space */
		procs = calloc (count, sizeof (struct kinfo_proc));
		if (! procs)
			return;

		/* request the details of the processes */
		ret = sysctl ((int *)mib, mib_len (mib), procs, &bytes, NULL, 0);
		if (ret < 0) {
			free (procs);
			procs = NULL;
			if (errno != ENOMEM) {
				/* unknown error, so give up */
				return;
			}
		} else {
			printf ("%s:%d: loaded all procs\n", __func__, __LINE__);
			done = true;
		}
	}

	printf ("%s:%d:\n", __func__, __LINE__);

	if (! done)
		goto out;

	printf ("%s:%d:\n", __func__, __LINE__);

	/* reset */
	done = false;

	/* Calculate the lowest PID number which gives us the ultimate
	 * parent of all processes.
	 */
	p = &procs[0];
	ultimate_parent = p->kp_proc.p_pid;
	printf ("%s:%d: ultimate_parent=%u\n", __func__, __LINE__, (unsigned)ultimate_parent);

	for (i = 1; i < count; i++) {
		p = &procs[i];
		if (p->kp_proc.p_pid < ultimate_parent)
			ultimate_parent = p->kp_proc.p_pid;
	}

	printf ("%s:%d: final ultimate_parent=%u\n", __func__, __LINE__, (unsigned)ultimate_parent);

	while (! done) {
		for (i = 0; i < count && !done; i++) {
			p = &procs[i];
			printf ("%s:%d: i=%d\n", __func__, __LINE__, (int)i);

			if (p->kp_proc.p_pid == current) {
				printf ("%s:%d:\n", __func__, __LINE__);

				if (! ultimate_parent && current == ultimate_parent) {

					/* Found the "last" PID so record it and hop out */
					printf ("%d ('%s')\n", (int)current, p->kp_proc.p_comm);
					done = true;
					break;
				} else {
					/* Found a valid parent pid */
					printf ("%d ('%s')\n",
							(int)current, p->kp_proc.p_comm);
				}

				/* Move on */
				current = p->kp_eproc.e_ppid;
				printf ("%s:%d: changed current to %u\n", __func__, __LINE__, (unsigned)current);
			}
		}
	}

out:
	if (procs)
		free (procs);
}

int
main (int argc, char *argv[])
{
	handle_proc_branch_darwin ();

	exit (EXIT_SUCCESS);
}
