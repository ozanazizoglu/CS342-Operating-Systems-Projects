#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <unistd.h>
#include "cons.h"
#include <errno.h>
#include <string.h>
#define MAX_SIZE 512
int main(int argc, char **argv)
{

	mqd_t mq, secondmq;
	struct mq_attr mq_attr;
	struct item sending;
	struct result *incoming;
	int n;
	int buflen;
	char *bufptr;

	//second mq
	secondmq = mq_open(SECONDMQ, O_RDWR | O_CREAT | O_TRUNC, 0666, NULL);
	if (secondmq == -1)
	{
		perror("Unable to open secondmq");
		exit(1);
	}
	mq_getattr(secondmq, &mq_attr);
	buflen = mq_attr.mq_msgsize;
	bufptr = (char *)malloc(buflen);
	while (1)
	{

		char *input = malloc(MAX_SIZE);
		int param_count = 0;
		for (int i = 0; i < 3; i++)
			sending.params[i] = 0;

		printf("Please enter your command:\n");
		fgets(input, MAX_SIZE, stdin);

		char *pch = strtok(input, " ");

		while (pch != NULL)
		{
			if (param_count == 0)
			{
				strcpy(sending.request, pch);
			}
			else
			{
				int param = atoi(pch);
				sending.params[param_count - 1] = param;
			}

			pch = strtok(NULL, " ");
			param_count++;
		}
		sending.param_count = param_count - 1;
		//first mq
		mq = mq_open(FIRSTMQ, O_RDWR);
		if (mq == -1)
		{
			perror("Unable to open mqueue");
			exit(1);
		}
		n = mq_send(mq, (char *)&sending, sizeof(struct item), 0);
		if (n == -1)
		{
			perror("mq_send failure");
			exit(1);
		}

		sleep(1);
		//retrieve results
		int m = mq_receive(secondmq, (char *)bufptr, buflen, NULL);
		if (m == -1)
		{
			perror("mq receive failure");
			exit(1);
		}
		incoming = (struct result *)bufptr;
		printf("Result nums: [");
		for (int i = 0; i < incoming->size; i++)
			printf("%.2f ", incoming->nums[i]);

		printf("]\n");
	}

	mq_close(mq);
}
