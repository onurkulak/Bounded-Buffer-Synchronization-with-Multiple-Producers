#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

int numSt;
pthread_mutex_t consMutex;
int somethingToConsume;
typedef struct {
int sid;
char firstname[64];
char lastname[64];
double cgpa;
}student;


typedef struct {
    student* students; // the buffer
    size_t len; // number of items in the buffer
    int head, tail;
    pthread_mutex_t mutex; // needed to add/remove data from the buffer
    pthread_cond_t can_produce; // signaled when items are removed
    pthread_cond_t* can_consume; // signaled when items are added
} buffer_t;

int N;
int bufferSize;
FILE *outp;
FILE **inpPointers;
buffer_t* buffers;
int threadsAlive;
void* producer(void *param){

	int id = *((int*)param); 
	int read;
	buffer_t* b = buffers+id;

	while(fscanf(inpPointers[id],"%d", &read)!=EOF){
		if(read==id){
			pthread_mutex_lock(&b->mutex);
			//printf("%d gonna send\n", id);
			if(b->tail==b->head+b->len){
				//printf("%d waiting produce signal\n", id);
				pthread_cond_wait(&b->can_produce,&b->mutex);
				//printf("%d muradına erdi\n", id);
			}
			fscanf(inpPointers[id],"%d %s %s %lf", 
				&(b->students[b->tail%b->len].sid), 
				(b->students[b->tail%b->len].firstname), 
				(b->students[b->tail%b->len].lastname), 
				&(b->students[b->tail%b->len].cgpa));
			//printf("%d produced %d\n", id, b->students[b->tail%b->len].sid);
			b->tail++;
			pthread_mutex_lock(&consMutex);
			somethingToConsume++;
			pthread_mutex_unlock(&consMutex);
			pthread_cond_signal(b->can_consume);
			pthread_mutex_unlock(&b->mutex);
			//printf("%d sent those niggas\n", id);
		}
		else 
			fscanf(inpPointers[id], "%*[^\n]\n");
	}
	//printf("%d producer finished\n", id);
	fclose(inpPointers[id]);
}

int compare(const void *a, const void *b)
{
  student xx = *(student*)a;
  student yy = *(student*)b;
  if (xx.sid<yy.sid) return -1;
  if (xx.sid>yy.sid) return 1;
  return 0;
}


void* consumer(void *param){
	student* niggas = malloc(sizeof(student)*numSt);
	int stCount = 0;	
	
	pthread_cond_t* can_consume = (pthread_cond_t*)param;
	pthread_mutex_lock(&consMutex);
	while(threadsAlive||somethingToConsume){
		pthread_mutex_unlock(&consMutex);
		for(int i = 0; i < N; i++){
			
			pthread_mutex_lock(&buffers[i].mutex);
			if(buffers[i].head!=buffers[i].tail)
			{ 
				//printf("consumer acquired the lock..%d\n",i);
				while(buffers[i].head!=buffers[i].tail){
					niggas[stCount++] = buffers[i].students[buffers[i].head%buffers[i].len];
					//printf("%d consumed %d\n", i, buffers[i].students[buffers[i].head%buffers[i].len].sid);
					buffers[i].head++;
					pthread_mutex_lock(&consMutex);
					somethingToConsume--;
					pthread_mutex_unlock(&consMutex);
				}
				pthread_cond_signal(&buffers[i].can_produce);
				
			}
			pthread_mutex_unlock(&buffers[i].mutex);
		}
		pthread_mutex_lock(&consMutex);
		if(!somethingToConsume&&threadsAlive){
			//printf("waiting for a change\n");
			pthread_cond_wait(can_consume, &consMutex);
			//printf("consumer waited..\n");
		}
	}
	qsort(niggas, stCount, sizeof(student), compare);
	for(int i = 0; i < stCount; i++)
		fprintf(outp,"%d %s %s %.2f\n", niggas[i].sid, niggas[i].firstname, niggas[i].lastname, niggas[i].cgpa);
	fclose(outp);
	//printf("consumer closing..\n");
}


int  main(int argc, char const *argv[])
{
	somethingToConsume=0;
	consMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	N = atoi(argv[1]);
	bufferSize = atoi(argv[2]);
	inpPointers = malloc(sizeof(FILE*)*N);
	for(int i = 0; i < N; i++)
		inpPointers[i] = fopen(argv[3],"r");
	FILE* wc = fopen(argv[3],"r");
	outp = fopen(argv[4],"w");
	numSt = 0;
	while(fscanf(wc, "%*[^\n]\n")!=EOF)
		numSt++;
	fclose(wc);
	threadsAlive = 1;
	pthread_t * threads=(pthread_t *)malloc((N+1)*sizeof(pthread_t));
	buffers = malloc(sizeof(buffer_t)*N);
	pthread_cond_t* can_consume = malloc(sizeof(pthread_cond_t));
	*can_consume = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	
	for(int i = 0; i < N; i++){
		int *x = (int*)(malloc(sizeof(int)));
		*x = i;
		pthread_create(&threads[i], NULL, producer, (void*)x);
		buffers[i].students = malloc(sizeof(student)*bufferSize);
		buffers[i].len = bufferSize;
		buffers[i].head = buffers[i].tail = 0;
        buffers[i].can_produce = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
		buffers[i].can_consume = can_consume;
		buffers[i].mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	}
	pthread_create(&threads[N], NULL, consumer, can_consume);
	for(int i = 0; i < N; i++)
		pthread_join(threads[i], NULL);
	//printf("waited producers\n");
	pthread_mutex_lock(&consMutex);
	threadsAlive = 0;
	pthread_mutex_unlock(&consMutex);
	pthread_cond_signal(can_consume);
	pthread_join(threads[N], NULL);
	//printf("waited consumer..\n");
	return 0;
}