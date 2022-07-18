#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#define MAXBURST 1000

typedef struct queue{
	struct queue *next;
	int process;
}queue;
int check_element(queue *head, int last){
	int result = -1; // return 0 if last process is in the queue
	if (head -> process == last){
		result = 0;
		return result;
	}
	queue *cur = head;
	while(cur != NULL){
		if(cur -> process == last){
			result  = 0;
			return result;
		}
		cur = cur -> next;
	}
	return result;
}
void insert_end(queue **head, int process){
	queue *new_node = malloc(sizeof(queue));
	if(*head == NULL){
		new_node->process = process;
		new_node->next = NULL;
		*head = new_node;
		return;
	}
	//no duplicates in this town
	if((*head) -> process == process)
		return;

	queue *cur = *head;
	while(cur -> next != NULL){
		if(cur->process == process)
			return;
		cur = cur -> next;
	}
	if(cur -> process == process)
		return;
	new_node -> next = NULL;
	new_node -> process = process;
	cur->next = new_node;
}
void insert_beginning(queue **head, int process){
	queue *new_node = malloc(sizeof(queue));
	if(*head == NULL){
		new_node->process = process;
		new_node->next = NULL;
		*head = new_node;
		return;
	}
	new_node -> next = *head;
	new_node -> process = process;
	*head = new_node;
}

void delete_queue(queue **head, int d_process){

	if((*head) -> process == d_process){
		*head = (*head) -> next;
		return;
	}
	
	queue *cur = *head;
	queue *prev;
	while(cur -> process != d_process ){
		prev = cur;
		cur = cur ->next;
	}
	prev -> next = cur -> next;
}
void print_queue(queue *head){
	queue *cur = head;
	while( cur != NULL){
		printf("%d -> ", cur -> process);
		cur = cur -> next;
	}
	printf("\n");
}
void swap(int *a, int *b)
{
	int temp = *a;
	*a = *b;
	*b = temp;
}
int read_file(char *file_name, int **priorities, int **arrivals, int **bursts)
{
	FILE *fp = fopen(file_name, "r");
	int count = 0, p, a, b;
	char ch;

	while (!feof(fp))
	{
		ch = fgetc(fp);
		if (ch == '\n')
			count++;
	}
	fclose(fp);

	// malloc for the arrays
	(*priorities) = (int *)malloc(sizeof(int *) * count);
	(*arrivals) = (int *)malloc(sizeof(int *) * count);
	(*bursts) = (int *)malloc(sizeof(int *) * count);

	fp = fopen(file_name, "r");

	for (int i = 0; i < count; i++)
	{
		fscanf(fp, "%d %d %d\n", &p, &a, &b);
		(*priorities)[i] = p;
		(*arrivals)[i] = a;
		(*bursts)[i] = b;
	}
	fclose(fp);
	return count;
}

int srtf_finished(int *arr, int size)
{
	//in SRTF if a process has finished burst is set to MAXBURST which is larger than given
	// MAXBURST in the assignment. we use this to check if the process has finished
	int result = 0;
	for (int i = 0; i < size; i++)
	{
		if (arr[i] != MAXBURST)
		{
			result = -1;
			break;
		}
	}
	return result;
}

int rr_finished(int *arr, int size)
{

	int result = 0;
	for (int i = 0; i < size; i++){
		if (arr[i] != 0){
			result = -1;
			break;
		}
	}

	return result;
}
int check_flags(int *arr, int size)
{
	int result = 0;
	for (int i = 0; i < size; i++){
		if (arr[i] == 0){ //if any of the flags is 0, not all of them are processed yet.
			result = -1;
			break;
		}
	}
	return result;
}


void SRTF(int processes, int *priorities, int *arrivals, int *bursts)
{
	int bursts_cpy[processes];
	memcpy(bursts_cpy, bursts, sizeof(int) * processes);
	int *waiting_times = calloc(sizeof(int), processes); //create waiting time for each process, init to 0 with calloc.

	int current_time = 0; //total time to finish, current passed time
	int current = 0;

	//loop until all the bursts are finished.
	//change to for i < max burst + (largest waiting - first waiting)
	for (;;){
		if (srtf_finished(bursts, processes) == 0){
			break; // if all the bursts have finished, we're done.
		}

		for (int i = 0; i < processes; i++)
		{
			//if a process has arrived(arrival time of the process less than current time)
			//and that process has lower burst time than current one; take it to the front and execute that process.
			if (arrivals[i] <= current_time && bursts[i] <= bursts[current])
			{
				current = i;
			}
		}

		// maybe find the next executed item, then increment in bulk
		bursts[current]--; //run the process for 1 unit of time, check again, repeat.
		current_time++;	   //increase the current time

		//if a process finishes executing, set the burst time of that process to some high number
		//so that it won't be considered
		if (bursts[current] == 0)
			bursts[current] = MAXBURST;

		//increse the waiting times of other processes
		for (int i = 0; i < processes; i++)
		{
			if (current != i && arrivals[i] < current_time && bursts[i] != MAXBURST)
			{
				waiting_times[i]++;
			}
		}

		//printf("Process %d ran for 1 ms\n", current + 1);
	}

	memcpy(bursts, bursts_cpy, sizeof(int) * processes); //restore the bursts to original

	int avg_turnaround = 0;
	int turnaround = 0;
	for (int i = 0; i < processes; i++)
	{
		turnaround = waiting_times[i] + bursts[i];
		avg_turnaround += turnaround;
		//printf("Turnaround for %d = %d\n", i + 1, turnaround);
		//printf("waiting times for %d = %d\n", i + 1, waiting_times[i]);
	}
	avg_turnaround /= processes;
	printf("SRTF average turnaround: %d\n", avg_turnaround);
}
void SJF(int processes, int *priorities, int *arrivals, int *bursts)
{
	int bursts_cpy[processes];
	int arrivals_cpy[processes];
	memcpy(bursts_cpy, bursts, sizeof(int) * processes);
	memcpy(arrivals_cpy, arrivals, sizeof(int) * processes);
	//int order[processes];
	int *waiting_times = calloc(sizeof(int), processes); //create waiting time for each process, init to 0 with calloc.
	//for (int i = 0; i < processes; i++)
	//	order[i] = priorities[i] - 1;

	int total = 0, current = 0;			  //total time to finish, current passed time
	int next, nexts_burst, nexts_arrival; //next going process, initially the first one
	for (int i = 0; i < processes; i++)
	{
		next = i;
		nexts_burst = bursts[i];
		nexts_arrival = arrivals[i];

		for (int j = i + 1; j < processes; j++)
		{
			if (arrivals[j] <= current && bursts[j] < bursts[i])
			{
				//update the next
				next = j;
				nexts_burst = bursts[j];
				nexts_arrival = arrivals[j];
			}
		}
		current += nexts_arrival + nexts_burst;
		swap(&arrivals[i], &arrivals[next]);

		swap(&bursts[i], &bursts[next]);
		//swap(&order[i], &order[next]);
	}

	current = 0;
	for (int i = 0; i < processes; i++)
	{
		waiting_times[i] = current - arrivals[i];
		total += bursts[i];
		if (arrivals[i] > current)
		{
			total += (arrivals[i] - current);
		}
		current = total;
	}

	int avg_turnaround = 0;
	int turnaround = 0;
	for (int i = 0; i < processes; i++)
	{
		turnaround = waiting_times[i] + bursts[i];
		avg_turnaround += turnaround;
		//printf("Turnaround for %d = %d\n", i, turnaround);
	}
	avg_turnaround /= processes;

	memcpy(bursts, bursts_cpy, sizeof(int) * processes);
	memcpy(arrivals, arrivals_cpy, sizeof(int) * processes);
	printf("SJF average turnaround: %d\n", avg_turnaround);
}
void FCFS(int processes, int *priorities, int *arrivals, int *bursts)
{
	// for (int i = 0; i < processes; i++)
	// {
	// 	printf("arrivals[%d] = %d || burst[%d] = %d\n", i, arrivals[i], i, bursts[i]);
	// }
	int total = 0, current = 0;
	int *waiting_times = calloc(sizeof(int), processes); //create waiting time for each process, init to 0 with calloc.

	for (int i = 0; i < processes; i++)
	{

		waiting_times[i] = current - arrivals[i];
		total += bursts[i];
		if (arrivals[i] > current)
		{
			total += (arrivals[i] - current);
		}

		current = total;
	}
	int avg_turnaround = 0;
	int turnaround = 0;
	for (int i = 0; i < processes; i++)
	{
		turnaround = waiting_times[i] + bursts[i];
		avg_turnaround += turnaround;
	}

	avg_turnaround /= processes;
	printf("FCFS average turnaround: %d\n", avg_turnaround);
}
void test_SRTF(int processes, int *priorities, int *arrivals, int *bursts){
		
}
void RR(int processes, int *priorities, int *arrivals, int *bursts, int quantum)
{
	//copy the burst times to a temp array since they will be changed.
	int bursts_cpy[processes];
	memcpy(bursts_cpy, bursts, sizeof(int) * processes);

	int *waiting_times = calloc(sizeof(int), processes); //create waiting time for each process, init to 0 with calloc.
	int current_time = 0;
	int current = 0; // first exec'd process
	int end = -1;
	queue *ready_queue = NULL; //ready queue
	
	for (;;){

		if (rr_finished(bursts, processes) == 0){
			break; //finish execution if all bursts = 0
		}

		for (int i = 0; i < processes; i++){
			// if the last process is in the queue, no more queue inserts
			if(ready_queue != NULL && check_element(ready_queue, processes -1)== 0){
				break;
			}
			if (i != end && bursts[i] != 0 && arrivals[i] <= current_time ){
				insert_end(&ready_queue, i); // insert to the ready queue, end goes first 3 -> 2 -> 1 queue means 3 goes first, then 2, then 1
			}
		}


		int this_turn = 0; // time passed in this quarter, maximum value would be equal to quantum
		if(end != -1 && bursts[end] != 0)
			insert_end(&ready_queue, end); //insert current tot he back of the ready queue

		current = ready_queue -> process; //get the current process from the head of the ready queue

		delete_queue(&ready_queue, current); // remove it from the queue, add it to the end if it's not finished.
		if (bursts[current] <= quantum){
			this_turn = bursts[current];
			current_time += bursts[current];
			bursts[current] = 0;
			end = -1; // this process is NOT going to the back of the queue, it's finished.
		}
		else{
			this_turn = quantum;
			current_time += quantum;
			bursts[current] -= quantum;
			end = current; // this process going to the back of the queue
		}

		for (int i = 0; i < processes; i++){
			if (i == current)
				continue;

			if (arrivals[i] <= current_time && bursts[i] != 0){

				if (waiting_times[i] == 0){
					waiting_times[i] += (current_time - arrivals[i]);
					if(arrivals[i] == 0) //hacking my way outta here
						waiting_times[i] -= (bursts_cpy[i] - bursts[i]);
				}
				else
					waiting_times[i] += this_turn;
			}
		}
	} 

	//retrieve the original values of bursts
	memcpy(bursts, bursts_cpy, sizeof(int) * processes);
	int avg_turnaround = 0;
	int turnaround = 0;
	for (int i = 0; i < processes; i++){
		turnaround = waiting_times[i] + bursts[i];
		avg_turnaround += turnaround;
	}
	avg_turnaround /= processes;

	printf("RR average turnaround: %d\n", avg_turnaround);
}
int main(int argc, char **argv)
{
	char *file_name = argv[1];
	int quantum = atoi(argv[2]);
	int *priorities, *arrivals, *bursts;
	int processes = read_file(file_name, &priorities, &arrivals, &bursts);

	FCFS(processes, priorities, arrivals, bursts);
	SJF(processes, priorities, arrivals, bursts);
	SRTF(processes, priorities, arrivals, bursts);
	RR(processes, priorities, arrivals, bursts, quantum); 
	//queue *head = NULL;
	//insert_end(&head, 1);

		
}
