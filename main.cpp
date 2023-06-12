#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <ctime>
#include <queue>
#include <stdio.h>
#include <semaphore.h>
#include "sleep.cpp"
using std::cout;
using std::cin;
using std::string;
using std::queue;
using std::endl;
typedef struct voter_info{
    int v_id;
    int polling_station_count;
    pthread_cond_t* voter_cond;
    
}voter_info;
time_t initial_time;
FILE *fptr;
pthread_mutex_t* voting_mutex[100];
pthread_mutex_t* queue_mutex[100];
pthread_cond_t* next_voter_cond[100];
pthread_t voter_t;
queue<voter_info*> voter_queue[100];
queue<voter_info*> pr_voter_queue[100];
int voter_id = 0;
pthread_mutex_t john_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t anna_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t marry_mutex = PTHREAD_MUTEX_INITIALIZER;
int voted_count[100];
int john_count = 0;
int anna_count = 0;
int marry_count = 0;
voter_info* next_voter_info[100];
int simulation_time;
int polling_station_count = 1;
int snapshot_time;
bool is_joined = false;
float arrival_probability;
float fail_probability = 0.0;
int waiting_time = 1; // t sec.
bool iswaiting = false;


void display_queue(int p) {
    //Printing ordinary people queue.
    cout << "Queue: ";
    queue<voter_info*> q = voter_queue[p];
    while (!q.empty()) {
        printf("%d ",q.front()->v_id);
        q.pop();
    }
    printf("\n");
}
void display_queue_pr(int p) {
    //Printing special people queue.
    cout << "PR Queue: ";
    queue<voter_info*> q = pr_voter_queue[p];
    while (!q.empty()) {
        printf("%d ",q.front()->v_id);
        q.pop();
    }
    printf("\n");
}
int least_busy_station(){
    //Returns the index of least busy station.
    int min  = 999999;
    int index;
    for(int i=0;i<polling_station_count;i++){
        pthread_mutex_lock(queue_mutex[i]);
        queue<voter_info*> q = voter_queue[i];
        pthread_mutex_unlock(queue_mutex[i]);
        if(q.size() < min){
            index = i;
            min = q.size();
        }
    }
    return index;
}
int least_busy_station_pr(){
    //Returns the index of least busy station for special people.
    int min  = 999999;
    int index;
    for(int i=0;i<polling_station_count;i++){
        pthread_mutex_lock(queue_mutex[i]);
        queue<voter_info*> q = pr_voter_queue[i];
        pthread_mutex_unlock(queue_mutex[i]);
        if(q.size() < min){
            index = i;
            min = q.size();
        }
    }
    return index;
}

void* next_voter(void* polling_station){
    time_t current_time = time(NULL); // get the current time in seconds
    time_t snapshot_time = time(NULL);
    time_t end_time = initial_time + simulation_time;
    int p = *(int *)(&polling_station);
    while(voter_queue[p].size() == 0 && pr_voter_queue[p].size() == 0){ //Until first voter arrives.
        continue;
    }
    int vt= 0; //to avoid starvation in pr queue.
    while(time(NULL) < end_time){
        if(!voter_queue[p].empty() || !pr_voter_queue[p].empty()){
            int f_prob = (rand() % 100) +1;
            pthread_mutex_lock(voting_mutex[p]);
            if(voter_queue[p].size() >=5 && vt<=4){ // Allow max of 4 consequent ordinary people to avoid starvation.
                next_voter_info[p] = voter_queue[p].front();
                vt++;
            }
            else if (!pr_voter_queue[p].empty()){
                next_voter_info[p] = pr_voter_queue[p].front();
                vt = 0;
            }
            else{
                next_voter_info[p] = voter_queue[p].front();
            }
            int a = pthread_cond_signal(next_voter_info[p]->voter_cond); //Signaling the voter.
            printf("Next voter is: %d in St.%d\n",next_voter_info[p]->v_id,p);
            display_queue(p);
            display_queue_pr(p);
            pthread_cond_wait(next_voter_cond[p],voting_mutex[p]); //Waiting signal from voter to vote their vote.
            pthread_mutex_unlock(voting_mutex[p]);
            if(time(NULL) - current_time > 10*waiting_time){
                if(f_prob <= fail_probability*100){
                    printf("Maintenance in St. %d\n",p);
                    char log[512]; //Logging to maintenance.
                    int request_time = time(NULL) - initial_time;
                    sprintf(log,"%d.%-19d %-12c %-17d %-23d %-19d",p,-1,'M',request_time,request_time+5*waiting_time,5*waiting_time);
                    fprintf(fptr,"%s\n",log);
                    pthread_sleep(5*waiting_time);
                }
                current_time = time(NULL);
            }
        }
        //pthread_sleep(1);
    }
    printf("QUEUE's are empty.\n");
    
}
void* voter(void * v_info){
    int request_time = time(NULL) - initial_time;
    char is_pr = 'O';
    struct voter_info *voter_info_ptr = (*(struct voter_info**) v_info);
    int voter_id = voter_info_ptr->v_id;
    int p = voter_info_ptr->polling_station_count;
    pthread_cond_t* cond = (voter_info_ptr->voter_cond);
    int prob = (rand() % 100) +1;
    int a = pthread_mutex_lock(voting_mutex[p]);
    while(voter_id != next_voter_info[p]->v_id){
        printf("%d is waiting.\n",voter_id);
        int a = pthread_cond_wait(cond,voting_mutex[p]); //Waiting until station signals.

    }
    pthread_sleep(2*waiting_time);
    pthread_mutex_lock(queue_mutex[p]);
    bool deleted =false;
    if(voter_queue[p].size() > 0){
        if(voter_queue[p].front()->v_id == voter_id){
            voter_queue[p].pop();
            deleted = true;
        }
    }
    if(!deleted){
        is_pr = 'S';
        pr_voter_queue[p].pop();
    }
    pthread_mutex_unlock(queue_mutex[p]);
    voted_count[p]+=1;
    if(prob <= 40){
        printf("Voter %d has voted for Mary in St. %d.Total Votes: %d\n",voter_id,p,voted_count[p]);
        pthread_mutex_lock(&marry_mutex);
        marry_count+=1;
        pthread_mutex_unlock(&marry_mutex);
    }
    else if(prob <= 55){
        printf("Voter %d has voted for John in St. %d.Total Votes: %d\n",voter_id,p,voted_count[p]);
        pthread_mutex_lock(&john_mutex);
        john_count+=1;
        pthread_mutex_unlock(&john_mutex);
    }
    else{
        printf("Voter %d has voted for Anna in St. %d.Total Votes: %d\n",voter_id,p,voted_count[p]);
        pthread_mutex_lock(&anna_mutex);
        anna_count+=1;
        pthread_mutex_unlock(&anna_mutex);
    }
    int polling_station_time = time(NULL) - initial_time;
    int turnaround_time = polling_station_time - request_time;
    char log[512]; //Logging.
    sprintf(log,"%d.%-19d %-12c %-17d %-23d %-19d",p,voter_id,is_pr,request_time,polling_station_time,turnaround_time);
    fprintf(fptr,"%s\n",log);
    pthread_mutex_unlock(voting_mutex[p]);
    pthread_cond_signal(next_voter_cond[p]); //Signaling the polling station.
    pthread_exit(NULL);

}

void* voter_arrives(void* arg){
    time_t end_time = initial_time + simulation_time;
    while(time(NULL) < end_time){
        pthread_sleep(waiting_time);
        int prob1 = (rand() %100) +1;
        int prob2 = (rand() %100) +1;
        if(prob1 <= arrival_probability*100){
            voter_info *v_info = new voter_info;
            v_info->v_id = voter_id;
            pthread_cond_t* p_cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
            v_info->voter_cond = p_cond;
            pthread_cond_init(v_info->voter_cond, NULL);
            int p = least_busy_station();
            v_info->polling_station_count = p;
            pthread_mutex_lock(queue_mutex[p]);
            voter_queue[p].push(v_info);
            pthread_mutex_unlock(queue_mutex[p]);
            printf("Voter %d has arrived at the station %d.\n",v_info->v_id,p);
            pthread_create(&voter_t,NULL,voter,&v_info);
            voter_id++;
        }
        if(prob2 >= arrival_probability*100){
            voter_info *v_info = new voter_info;
            v_info->v_id = voter_id;
            pthread_cond_t* p_cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
            v_info->voter_cond = p_cond;
            pthread_cond_init(v_info->voter_cond, NULL);
            int p = least_busy_station_pr();
            v_info->polling_station_count = p;
            pthread_mutex_lock(queue_mutex[p]);
            pr_voter_queue[p].push(v_info);
            pthread_mutex_unlock(queue_mutex[p]);
            printf("Voter %d (pr)has arrived at the station %d.\n",v_info->v_id,p);
            pthread_create(&voter_t,NULL,voter,&v_info);
            voter_id++;
        }
    }
    pthread_exit(NULL);
}
void* logging_thread(void* arg){
    time_t end_time = initial_time + simulation_time;
    time_t current_time = time(NULL);
    while(time(NULL) < end_time){
        if(time(NULL) - current_time >= snapshot_time){
            snapshot_time = 1;
            current_time = time(NULL);
            for(int i=0;i<polling_station_count;i++){
                printf("At %d sec, polling station %d, elderly/pregnant:",time(NULL)- initial_time,i);
                display_queue_pr(i);
                printf("At %d sec, polling station %d, ordinary:",time(NULL)- initial_time,i);
                display_queue(i);
            }
            printf("At %d sec total votes: (Marry: %d, John:%d, Anna: %d)\n",time(NULL)- initial_time,marry_count,john_count,anna_count);
        }
    }
}


int main(int argc, char** argv) {
    for(int i=1; i<argc; i++){
        if(!strcmp(argv[i], "-t")) {simulation_time = atoi(argv[++i]);}
        else if (!strcmp(argv[i], "-p")) {arrival_probability = atof(argv[++i]);}
        else if (!strcmp(argv[i], "-f")) {fail_probability = atof(argv[++i]);}
        else if (!strcmp(argv[i], "-c")) {polling_station_count = atoi(argv[++i]);}
        else if (!strcmp(argv[i], "-n")) {snapshot_time = atoi(argv[++i]);}
    }
    srand(10);
    queue<voter_info> voter_queue;
    queue<voter_info> pr_voter_queue;
    pthread_t logging_t,voter_arrives_t,next_voter_t[polling_station_count];
    char fileName[256];
    initial_time = time(NULL);
    sprintf(fileName,"%ld-voters.log",initial_time);
    fptr = fopen(fileName,"w");
    fprintf(fptr,"StationID.VoterID | Category | Request Time | Polling Station Time | Turnaround Time\n");
    fprintf(fptr,"-------------------------------------------------------------------------------------\n");
    for (int i = 0; i < polling_station_count; i++) {
        voting_mutex[i] = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(voting_mutex[i], NULL);
        queue_mutex[i] = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(queue_mutex[i], NULL);

        next_voter_cond[i] = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
        pthread_cond_init(next_voter_cond[i],NULL);
    }   
    pthread_create(&voter_arrives_t,NULL,voter_arrives,NULL);
    pthread_create(&logging_t,NULL,logging_thread,NULL);
    for(int i = 0;i<polling_station_count;i++){
        pthread_create(&next_voter_t[i],NULL,next_voter,(void *) i);
    }
    pthread_join(voter_arrives_t,NULL);
    for(int i = 0;i<polling_station_count;i++){
        pthread_join(next_voter_t[i],NULL);
    }
    is_joined = true;
    pthread_join(logging_t,NULL);
    for(int i= 0;i<polling_station_count;i++){
        printf("Total Votes in Station %d is : %d\n",i,voted_count[i]);
    }
    fclose(fptr);
    printf("Anna Count: %d\n",anna_count);
    printf("John Count: %d\n",john_count);
    printf("Marry Count: %d\n",marry_count);

    return 0;
}