#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdbool.h>

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next) {
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head) {
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head) {
	__list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head * prev, struct list_head * next) {
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry) {
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

static inline void list_del_init(struct list_head *entry) {
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

static inline void list_move(struct list_head *list, struct list_head *head) {
        __list_del(list->prev, list->next);
        list_add(list, head);
}

static inline void list_move_tail(struct list_head *list,
				  struct list_head *head) {
        __list_del(list->prev, list->next);
        list_add_tail(list, head);
}

static inline int list_empty(const struct list_head *head) {
	return head->next == head;
}

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head);	\
       pos = pos->next)

#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; prefetch(pos->prev), pos != (head); \
        	pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = list_entry((head)->prev, typeof(*pos), member),	\
		n = list_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.prev, typeof(*n), member))

#if 0    //DEBUG
#define debug(fmt, args...) fprintf(stderr, fmt, ##args)
#else
#define debug(fmt, args...)
#endif



typedef struct{
    unsigned char op; //cpu인지 io인지
    unsigned char len; //수행시간
} code_tuple;

typedef struct{
    int pid;
    int arrival_time;
    int code_bytes;
    int PC;     //operations[PC] 로 쓰는 거 같음
    code_tuple *operations; 
    struct list_head job, ready, wait;
} process;

#define print(cur) \
	printf("PID: %03d\tARRIVAL: %03d\tCODESIZE: %03d\n", cur->pid, cur->arrival_time, cur->code_bytes); \
			for(int i = 0; i < cur->code_bytes / 2; i++){ \
                printf("%d %d\n", cur->operations[i].op, cur->operations[i].len); \
            }


bool isIdle(process *cur, struct list_head *ready_q, int *idleTime, int *clock){
    
    if(cur->pid == 100){    

        list_del_init(&cur->ready);     //지웠을 때

        if(!list_empty(ready_q)){       //ready_q가 안비어있으면 작업이 남아있다는 소리
            list_add_tail(&cur->ready, ready_q);    //idle을 ready_q 맨 뒤로 보냄
            return true;    //clock 소모 방지
        }else if (list_empty(ready_q)){ //ready_q가 비었으면
                                        //1. 모든 프로세스가 종료 (main의 while에서 처리 됨)
                                        //2. 프로세스가 아직 도착하지 않음
            list_add_tail(&cur->ready, ready_q);
            (*idleTime)++;
            return false;   //clock 소모 해도 됨
        }
    }

    return false;

}

void switchProcess(int *idleTime, int *clock, int *prevpid, process *cur, process *next, struct list_head *job_q , struct list_head *ready_q){
    (*idleTime) += 10;
    (*clock) += 10;
    //printf("%04d CPU: Switchedfrom: %03d to: %03d\n", *clock, *prevpid, cur->pid);
    (*prevpid) = cur->pid;
        
    //스위칭 중에는 다른 일을 하지 못함
    //스위치 도중 프로세스가 도착 했을 때
    for(int i = 0; i < 10; i++){
        list_for_each_entry(next, job_q, job){
            if(next->arrival_time == (*clock) - 10 + i){    //clock 10 증가시켜놔서
                list_add_tail(&next->ready, ready_q);
                printf("%04d CPU: Loaded PID: %03d\tArrival: %03d\tCodesize: %03d\tPC: %03d\n", (*clock), next->pid, next->arrival_time, next->code_bytes, next->PC);
            }
        }
    }
}

bool excuteProcess(int *clock, int *processEnd, process *cur, int *idleTime, int *prevpid, int *terminateNum){
    if(*clock >= *processEnd){  //프로세스 처리 중 에는 작동 안함
        
        if(*clock != 0 && *clock == *processEnd){
            //printf("%04d CPU: Increase PC \tPID:%03d PC:%03d\n", *clock, cur->pid, cur->PC);
            cur->PC++;
        }

        if(cur->PC < (cur->code_bytes / 2)){
            if(cur->operations[cur->PC].op == 0){
                //op가 0 이면 cpu
                *processEnd = cur->operations[cur->PC].len + *clock;
                //printf("%04d CPU: OP_CPU START \tPID: %03d len: %03d ends at: %04d PC: %3d\n", *clock, cur->pid, cur->operations[cur->PC].len, *processEnd, cur->PC);
                return false;
            }
            else if(cur->operations[cur->PC].op == 1){
                //op가 1 이면 io
                *processEnd = cur->operations[cur->PC].len + *clock;
                *idleTime += cur->operations[cur->PC].len;
                printf("%04d CPU: OP_IO START len: %03d ends at: %04d\n", *clock, cur->operations[cur->PC].len, *processEnd);
                //"%04d CPU: OP_IO START \tPID: %03d len: %03d ends at: %04d PC: %3d\n", clock, cur->pid, cur->operations[cur->PC].len, processEnd, cur->PC
                return false;
            }       
        } else{
            //printf("%04d CPU: Process is terminated PID: %03d PC: %03d\n", clock, cur->pid, cur->PC);
            //printf("%04d CPU: Reschedule PID: %03d Status: 04\n", clock, cur->pid);
            *prevpid = cur->pid;
            list_del_init(&cur->ready);     //readyQ에 작업 삭제
            (*terminateNum)++;              // 종료된 작업 수 카운트
            
            //printf("terminateNum++\n");
            //sleep(1);
            return true;
        }
    }return false;
}


int main(int argc, char* argv[]){
    
    LIST_HEAD(job_q);       //시작점 생성(head)
    LIST_HEAD(ready_q);     //시작점 생성(head)
    process *cur, *next;
    int jobQnum = 0;        //job_q에 있는 프로세스 수
    int clock = 0;          
    int processEnd = 0;     //operation이 끝나는 시간
    int terminateNum = 0;   //종료된 프로세스 수
    int idleTime = 0;       
    int prevpid = 0;        //이전 프로세스 pid

    while((cur = (process*)malloc(sizeof(process))) && fread(cur, sizeof(int) * 3, 1, stdin)){ 
        
        cur->PC = 0;
        cur->operations = (code_tuple*)malloc(cur->code_bytes); 
        fread(cur->operations, cur->code_bytes, 1, stdin); 
        INIT_LIST_HEAD(&cur->job);  //초기화
        INIT_LIST_HEAD(&cur->ready);  //초기화
        list_add_tail(&cur->job, &job_q); //끝에 넣고
        jobQnum++;
    } 

    
    {   //idle프로세스 추가
        cur = (process*)malloc(sizeof(process));
        cur->pid = 100;
        cur->arrival_time = 0;
        cur->code_bytes = 2;
        cur->operations = (code_tuple*)malloc(cur->code_bytes); 
        cur->operations[0].op = 0xff;
        cur->operations[0].len = 0; 
        cur->PC = 0;
        
        INIT_LIST_HEAD(&cur->job);  //초기화
        INIT_LIST_HEAD(&cur->ready);  //초기화
        list_add_tail(&cur->job, &job_q); //끝에 넣고
    }
        
    /* 파일 잘 읽어왔는지 확인 용
    list_for_each_entry(cur, &job_q, job){ 
		print(cur);
	}
    */
         
    while(jobQnum != terminateNum){ //jobQ에 들어있는거(idle때문에 -1) 랑 종료된 프로세스 수가 같아질 때 까지

        //ready_q에 넣는 코드
        list_for_each_entry(cur, &job_q, job){  //순회 해서
            if(clock == cur->arrival_time){     //도착시간이랑 시간이 같으면
                list_add_tail(&cur->ready, &ready_q);   //ready_q에 추가
                printf("%04d CPU: Loaded PID: %03d\tArrival: %03d\tCodesize: %03d\tPC: %03d\n", clock, cur->pid, cur->arrival_time, cur->code_bytes, cur->PC);
            }           
        }
        
        cur = list_entry(ready_q.next, process, ready); //cur의 위치 조정 ready_q의 맨 앞으로

        if(isIdle(cur, &ready_q, &idleTime, &clock)){    //현재 프로세스가 idle인지 확인하고 처리
            continue;
        }else{
            if(cur->pid != prevpid && cur->pid != 0){
                switchProcess(&idleTime, &clock, &prevpid, cur, next, &job_q, &ready_q);
            }
            if(excuteProcess(&clock, &processEnd, cur, &idleTime, &prevpid, &terminateNum)) continue;
        }
        clock++;
    }

    printf("*** TOTAL CLOCKS: %04d IDLE: %04d UTIL: %2.2f%%\n", clock, idleTime, (float)(clock - idleTime) * 100 / clock );
        
    list_for_each_entry_safe(cur, next, &job_q, job){
		list_del_init(&cur->job);
		free(cur->operations);
		free(cur);
	}
    list_for_each_entry(cur, &job_q, job){ 
		print(cur);
	}
    return 0;
}
