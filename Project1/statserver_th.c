#include <stdio.h>
#include <limits.h>
#include <sys/wait.h>
#include <time.h>
#include <stdlib.h> 
#include <mqueue.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include "cons.h"

#define MAXTHREADS 10
#define MAXFILENAME 100 

double average, average_range;
int count, count_range, max, K; 
double *largest_numbers;

struct arguments{
	char file_name[MAXFILENAME];
	char request[MAXFILENAME];
	int param_count;
	int *params;
};


//compare function for qsort()
int compare(const void * first, const void * second){
	return *(int*)first - *(int*)second;
}

//returns the numbers in an array along with the size of the array
int* get_numbers(char *file_name, int *size){
	FILE *fp = fopen(file_name, "r");
	int count = 0;

	int number;
	while(fscanf(fp, "%d\n", &number) != EOF){
		count++;    
 	}
	fclose(fp);
	fp = fopen(file_name, "r");
	int *contents = malloc(sizeof(int) * count);

	for(int i = 0; i < count; i++){
		fscanf(fp, "%d\n", &number);
		contents[i] = number;
	}
	*size = count;
	fclose(fp);
	return contents;
}

int find_max(char *file_name){
	int size;
	int *numbers = get_numbers(file_name, &size);
	int max = numbers[0];
	
	for (int i =1; i < size; i++){
		if(numbers[i] > max)
			max = numbers[i];	
	}	
	return max;
	
}

double *range(char *file_name, int start, int end, int K, int *count){
	int size;
	int *numbers = get_numbers(file_name, &size);

	qsort(numbers, size, sizeof(int), compare);
	int diff =0;	
	int start_idx = 0;
	int end_idx = size -1;	
	while(numbers[start_idx] < start)
		start_idx++;

	while(numbers[end_idx] > end)
		end_idx--;
	
	//if there are more numbers in the range than K	
	if(end_idx - start_idx +1 > K)
		start_idx = start_idx + end_idx - K + 1;	
	double *result = malloc( sizeof(double) *K ); //malloc & set all to 0
	int counter = start_idx;
	if( K > end_idx - start_idx +1)
		diff = K - (end_idx - start_idx +1);
	//printf("K = %d, startidx = %d endidx = %d calc = %d diff = %d\n", K, start_idx, end_idx, (end_idx - start_idx +1), diff);	

	for(int i =0; i<end_idx-start_idx +1; i++){
		result[i+diff] = (double)numbers[counter];
		counter++;
	}
	*count = end_idx - start_idx + 1;
	//printf("INSIDE RANGE() = %f\n", result[0]);
	return result;
	
}
int fcount(char *file_name){
	int size;
	get_numbers(file_name, &size);
	return size;
}

int fcount_range(char *file_name, int start, int end){
	int size;
	int *numbers = get_numbers(file_name, &size);
	int result = 0;

	for(int i =0; i<size; i++){
		if(numbers[i] >= start && numbers[i] <=  end)
			result++; 
	}	
	return result;
}

double avg_range(char *file_name, int start, int end, int *count){
	int size;
	int *numbers = get_numbers(file_name, &size);
	
	double sum = 0;	
	int obey = 0;
	for (int i = 0; i < size; i++){
		if(numbers[i] >= start && numbers[i] <= end){
			sum += numbers[i];
			obey++;
		}
	}
	*count = obey;
	return sum ;
				
}
double avg(char *file_name, int *count){
	int size;
	int *numbers = get_numbers(file_name, &size);

	*count = size;	

	double sum = 0;	
	for (int i = 0; i < size; i++){
		sum += numbers[i];
	}
	
	return sum;
				
}

static void *thread_task(void *arg){
	int param_count = ((struct arguments *) arg)->param_count;
	int *params = ((struct arguments *) arg)->params;	

	char filename[MAXFILENAME]; 
	char request[MAXFILENAME]; 
	strcpy(filename, ((struct arguments *) arg)->file_name);
	strcpy(request, ((struct arguments *) arg)->request);

	if(param_count == 0){
		if(strcmp(request, "avg") == 0){
			int thread_count;	
			double thread_average = avg(filename, &thread_count);
			average += thread_average;
			count += thread_count;
		}

		else if(strcmp(request, "count") == 0){
			int thread_count = fcount(filename);
			count += thread_count;
		}
		
		else if(strcmp(request, "max") == 0){
			int thread_max = find_max(filename);
			if(thread_max > max)
				max = thread_max;
		}
	}
	//prompts with 2 params: count <start> <end> , avg <start> <end>
	else if(param_count == 2){
		if(strcmp(request, "avg") == 0){
			int thread_count;
			double thread_average = avg_range(filename, params[0], params[1], &thread_count);
			count_range += thread_count;
			average_range += thread_average;
		}
		else if(strcmp(request, "count") == 0){
			int thread_count = fcount_range(filename, params[0], params[1]);
			count_range += thread_count;
		}
	}
	else{
		int size;
		K = params[2]; 
		double *numbers = range(filename, params[0], params[1], K, &size); 
		double *result_arr = malloc(sizeof(double) * K);
		int index = K - 1, p1 = K-1, p2 = K-1;
		while(index >= 0){
			if(numbers[p1] > largest_numbers[p2]){
				result_arr[index] = numbers[p1];
				p1--;
			}
			else{
				result_arr[index] = largest_numbers[p2];
				p2--;
			}
			index--;
		}
		for(int j = 0; j < K; j++)
			largest_numbers[j] = result_arr[j];
			
	
	}
	pthread_exit(NULL);
}

int main(int argc, char **argv){
	//clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
	int threads = atoi(argv[1]); 
	mqd_t mq, secondmq;
	struct item *incoming;
	struct result outgoing;
	struct mq_attr mq_attr;
	int buflen, n, ret;
	char *bufptr;

	//threads
	pthread_t tids[MAXTHREADS]; 
	struct arguments args[MAXTHREADS];	
	
	
	mq = mq_open(TFIRSTMQ, O_RDWR | O_CREAT | O_TRUNC, 0666, NULL);
	if( mq == -1){
		perror("Unable to open mqueue");
		exit(1);
	}
	
	mq_getattr(mq, &mq_attr);	
	buflen = mq_attr.mq_msgsize;
	bufptr = (char *) malloc(buflen);



	while(1){
		average = 0; 
		average_range = 0; 
		count = 0;
		count_range = 0;
		max = -1;
		n = mq_receive(mq, (char *) bufptr, buflen, NULL);
		if (n == -1){
			perror("mq_receive failure :((");
			exit(1);
		}
			
		incoming = (struct item *) bufptr;
		int ln = strlen(incoming->request);
		if((ln>0) && (incoming->request[ln-1] == '\n'))
			incoming->request[ln-1] = '\0';

		if (incoming->param_count == 3){
			K = incoming->params[2];
			largest_numbers = malloc(sizeof(double) * K);
			
			for(int i =0; i < K; i++)
				largest_numbers[i] = -1;
		}
		for (int i = 0; i < threads; i++){

			strcpy(args[i].file_name, argv[i+2]);
			strcpy(args[i].request, incoming->request);
			args[i].param_count = incoming->param_count;
			args[i].params = incoming->params;
			ret = pthread_create(&(tids[i]), NULL, thread_task,	(void *) &(args[i]));
			if( ret != 0){
				printf("thread create failed\n");
				exit(1);
			}
		}	

		for( int i = 0; i < threads; i++){
			ret = pthread_join(tids[i], NULL);
			if(ret != 0){
				printf("thread join fail");
				exit(0);
			}
		}

		if(strcmp(incoming->request, "avg") == 0 && incoming->param_count == 0){
			outgoing.size = 1; // sending one item
			outgoing.nums[0] = average / (double) count;
		
		}

		else if(strcmp(incoming->request, "max") == 0 ){
			outgoing.size = 1; // sending one item
			outgoing.nums[0] = (double) max;
		
		}
		else if(strcmp(incoming->request, "avg") == 0 && incoming->param_count == 2){
			outgoing.size = 1; // sending one item
			outgoing.nums[0] = average_range / (double) count_range;
		
		}
		else if(strcmp(incoming->request, "count") == 0 && incoming->param_count == 0){
			outgoing.size = 1; // sending one item
			outgoing.nums[0] = (double) count;
		}

		else if(strcmp(incoming->request, "count") == 0 && incoming->param_count == 2){
			outgoing.size = 1; // sending one item
			outgoing.nums[0] =  (double) count_range;
		}
		else{
			outgoing.size = K;
			for(int i =0; i < K; i++)
				outgoing.nums[i] = largest_numbers[i];
		}
	
		//creating the second mq for sending the results	
		secondmq = mq_open(TSECONDMQ, O_RDWR);
		if(secondmq == -1){
			perror("Unable to open the second mq");
			exit(1);
		}
		int r= mq_send(secondmq, (char *) &outgoing, sizeof(struct result), 0);
		if(r == -1){
			perror("mq_send failure -- server");
			exit(1);
		}	
		/*clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    	printf ("Total time = %f seconds\n",
            (end.tv_nsec - begin.tv_nsec) / 1000000000.0 +
            (end.tv_sec  - begin.tv_sec));
		exit(0); */

	}//while(1)
	
		mq_close(mq);
}

