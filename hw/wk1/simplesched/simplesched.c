
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <stdint.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int policy, rtnCode;
	struct sched_param param;

	policy = sched_getscheduler(getpid());
	printf("current policy: %d\n", policy);

	rtnCode = sched_getparam(getpid(), &param);
	if (rtnCode) {
		perror("sched_getparam");
		return -1;
	}

	policy = SCHED_FIFO;
	param.sched_priority = sched_get_priority_min(policy);
	rtnCode = sched_setscheduler(getpid(), policy, &param);
	if (rtnCode) {
		printf("could not change policy to SCHED_FIFO\n");
		perror("sched_setscheduler");
		return -1;
	} else {
		printf("successfully changed policy to SCHED_FIFO!\n");
	}
	return 0;
}