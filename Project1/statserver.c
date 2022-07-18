#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <sys/wait.h>
#include <stdlib.h> 
#include <mqueue.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "cons.h"
int compare(const void * first, const void * second){
	return *(double*)first - *(double*)second;
}
double* get_numbers(char *file_name, int *size){
	FILE *fp = fopen(file_name, "r");
	int count = 0;

	int number;
	while(fscanf(fp, "%d\n", &number) != EOF){
		count++;    
 	}
	fclose(fp);
	fp = fopen(file_name, "r");
	double *contents = malloc(sizeof(double) * count);

	for(int i = 0; i < count; i++){
		fscanf(fp, "%d\n", &number);
		contents[i] = number;
	}
	*size = count;
	fclose(fp);
	return contents;
}
double find_max(char *file_name){
	int size;
	double *numbers = get_numbers(file_name, &size);
	double max = numbers[0];
	
	for (int i =1; i < size; i++){
		if(numbers[i] > max)
			max = numbers[i];	
	}	
	return max;
	
}
double *range(char *file_name, int start, int end, int K){
	int size;
	int diff = 0;
	double *numbers = get_numbers(file_name, &size);
	qsort(numbers, size, sizeof(double), compare);
	int start_idx = 0;
	int end_idx = size -1;	
	while(numbers[start_idx] < (double)start)
		start_idx++;

	while(numbers[end_idx] > (double)end)
		end_idx--;
	
	//if there are more numbers in the range than K	
	if(end_idx - start_idx +1 > K)
		start_idx = start_idx + end_idx - K + 1;	
		

	double *result = malloc(sizeof(double) * K);
	
	for(int i =0; i < K;i++)
		result[i] = 0;
	int counter = start_idx;
	if (K > end_idx - start_idx +1)
		diff = K - (end_idx - start_idx + 1) ;
	for(int i =0; i<end_idx-start_idx +1; i++){
		result[i+diff] = numbers[counter];
		counter++;
	}
	return result;
	
}
int fcount(char *file_name){
	int size;
	get_numbers(file_name, &size);
	return  size;
}

int fcount_range(char *file_name, int start, int end){
	int size;
	double *numbers = get_numbers(file_name, &size);
	int result = 0;

	for(int i =0; i<size; i++){
		if(numbers[i] >= start && numbers[i] <=  end)
			result++; 
	}	
	return (double) result;
}

double avg_range(char *file_name, int start, int end, int *cnt){
	int size;
	double *numbers = get_numbers(file_name, &size);
	
	double sum = 0;	
	int count = 0;
	for (int i = 0; i < size; i++){
		if(numbers[i] >= start && numbers[i] <= end){
			sum += numbers[i];
			count++;
		}
	}
	*cnt = count; //pass by ref count	
	return sum;
				
}
double avg(char *file_name, int *count){
	int size;
	double *numbers = get_numbers(file_name, &size);
	
	double sum = 0;	
	for (int i = 0; i < size; i++){
		sum += numbers[i];
	}
	*count = size;	
	return sum;
				
}
int main(int argc, char **argv){
	//clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
	int files = atoi(argv[1]); 
	mqd_t mq, secondmq;
	struct item *incoming;
	struct result outgoing;
	struct mq_attr mq_attr;
	int buflen;
	char *bufptr;
	int n;
	pid_t pid; 

	
	
	mq = mq_open(FIRSTMQ, O_RDWR | O_CREAT | O_TRUNC, 0666, NULL);
	if( mq == -1){
		perror("Unable to open mqueue");
		exit(1);
	}
	
	mq_getattr(mq, &mq_attr);	
	buflen = mq_attr.mq_msgsize;
	bufptr = (char *) malloc(buflen);

	

	while(1){
		double average = 0; 
		double average_range = 0; 
		int count = 0;
		int count_range = 0;
		double max = 0;
		int K;
		double *largest_numbers;
		double *result_arr;
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

		//creating the pipes
		int fd[2*files];
		if( pipe(fd) == -1){
			printf("Could not create pipe\n");
			exit(1);
		}
		for(int i = 0; i < files; i++){
			if(pipe(&fd[2*i]) == -1){
				printf("Couldnt create pipe for %dth child\n", i+1);
				exit(1);	
			}
		}
		
		//creating child processes
		for(int i = 0; i < files; i++){
			if((pid = fork()) == 0){
				double end_signal = -1;

				//prompts without extra parameters
				if(incoming->param_count == 0){
						if(strcmp(incoming->request, "avg") == 0){
							int size;
							double average = avg(argv[i+2], &size);
							double d_size = (double) size;
							write(fd[i*2+1], &average, sizeof(average));
							write(fd[i*2+1], &d_size , sizeof(d_size));
							write(fd[i*2+1], &end_signal, sizeof(end_signal));
						}
						else if(strcmp(incoming->request, "count") == 0){
							int child_count = fcount(argv[i+2]);
							double dchild_count = (double) child_count;
							write(fd[i*2+1], &dchild_count, sizeof(dchild_count));
							write(fd[i*2+1], &end_signal, sizeof(end_signal));
						}
						
						else if(strcmp(incoming->request, "max") == 0){
							double maximum = find_max(argv[i+2]);
							write(fd[i*2+1], &maximum, sizeof(maximum));
							write(fd[i*2+1], &end_signal, sizeof(end_signal));
						}
				}
				//prompts with 2 params: count <start> <end> , avg <start> <end>
				else if(incoming->param_count == 2){
					// avg <start> <end>
					if(strcmp(incoming->request, "avg") == 0){
						int size;
						double average = avg_range(argv[i+2], incoming->params[0], incoming->params[1], &size);
						double d_size = (double) size;
						write(fd[i*2+1], &average, sizeof(average));
						write(fd[i*2+1], &d_size, sizeof(size));
						write(fd[i*2+1], &end_signal, sizeof(end_signal));
					}
					else if(strcmp(incoming->request, "count") == 0){
						int child_count =  fcount_range(argv[i+2], incoming->params[0], incoming->params[1]);
						double dchild_count = (double) child_count;
						write(fd[i*2+1], &dchild_count, sizeof(dchild_count));
						write(fd[i*2+1], &end_signal, sizeof(end_signal));
					}

				}
				//range start end K
				else{
					//int size = incoming->params[2];
					double *numbers = range(argv[i+2], incoming->params[0], incoming->params[1], K); 

					for(int j = 0; j < K; j++)
						write(fd[i*2+1], &numbers[j], sizeof(numbers[j]));

					write(fd[i*2+1], &end_signal, sizeof(end_signal));
				}
				close(fd[i*2]); //close the reading end
				close(fd[i*2+1]); //close the writing end
				exit(0);
			}
			else{
				double receive;
				int cnter = 0;
				double *received_numbers;
				close(fd[i*2+1]); //close the writing end
				if(strcmp(incoming->request, "avg") == 0 && incoming->param_count == 0){
					while(read(fd[i*2], &receive, sizeof(receive)) > 0){
						if(receive != -1){
							if (cnter == 0) // the sum from the child
								average += receive;	
							else // size
								count += receive;
						}
						cnter++;
					}	
				}
				else if(strcmp(incoming->request, "avg") == 0 && incoming->param_count == 2){
					while(read(fd[i*2], &receive, sizeof(receive)) > 0){
						if(receive != -1){
							if (cnter == 0) // the sum from the child
								average_range += receive;	
							else // size
								count_range += receive;
						}
						cnter++;
					}
				}	
				else if(strcmp(incoming->request, "count") == 0 && incoming->param_count == 0){
					while(read(fd[i*2], &receive, sizeof(receive)) > 0){
						if(receive != -1)
							count += receive;	
					}
				}
				else if(strcmp(incoming->request, "count") == 0 && incoming->param_count == 2){
					while(read(fd[i*2], &receive, sizeof(receive)) > 0){
						if(receive != -1)
							count_range += receive;	
					}
				}
				else if(strcmp(incoming->request, "max") == 0 ){
					while(read(fd[i*2], &receive, sizeof(receive)) > 0){
						if(receive != -1){
							if(receive > max)
								max = receive;
						}
						
					}
				}
				else{
					received_numbers = malloc(sizeof(double) * K);
					result_arr = malloc(sizeof(double) *K);
					int counter = 0;
					while(read(fd[i*2], &receive, sizeof(receive)) > 0){
						if(receive != -1){
							received_numbers[counter] = receive;
							counter++;
						}
					}
					
					int index =K-1;
					int p1 = K-1;
					int p2 = K-1;
					while(index >= 0){
						if(received_numbers[p1] > largest_numbers[p2]){
							result_arr[index] = received_numbers[p1];
							p1--;
						}
						else{
							result_arr[index] = largest_numbers[p2];
							p2--;
						}
						index--;
					}	
					for(int a =0; a < K; a++){
						largest_numbers[a] = result_arr[a];
					}
						
				}
			}//parent
		}
			
		
		if(strcmp(incoming->request, "avg") == 0 && incoming->param_count == 0){
			outgoing.size = 1; // sending one item

			outgoing.nums[0] = average / (double)count;
		
		}

		else if(strcmp(incoming->request, "max") == 0 ){
			outgoing.size = 1; //sending one item
			outgoing.nums[0] = max;
		
		}
		else if(strcmp(incoming->request, "avg") == 0 && incoming->param_count == 2){
			outgoing.size = 1; 
			outgoing.nums[0] = average_range / (double) count_range ;
		
		}
		else if(strcmp(incoming->request, "count") == 0 && incoming->param_count == 0){
			outgoing.size = 1;  
			outgoing.nums[0] =  count;
		}

		else if(strcmp(incoming->request, "count") == 0 && incoming->param_count == 2){
			outgoing.size = 1; 
			outgoing.nums[0] =  count_range;
		}
		else{
			outgoing.size = K;
			for(int i =0; i < K; i++)
				outgoing.nums[i] = largest_numbers[i];
		}
		//creating the second mq for sending the results	
		secondmq = mq_open(SECONDMQ, O_RDWR);
		if(secondmq == -1){
			perror("Unable to open the second mq");
			exit(1);
		}
		
		int n = mq_send(secondmq, (char *) &outgoing, sizeof(struct result), 0);
		if(n == -1){
			perror("mq_send failure -- server");
			exit(1);
		}	
		
		/*clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    	printf ("Total time = %f seconds\n",
            (end.tv_nsec - begin.tv_nsec) / 1000000000.0 +
            (end.tv_sec  - begin.tv_sec)); */
	}	
		

	mq_close(mq);

}
