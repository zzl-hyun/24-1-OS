#include <stdio.h>
#include <stdlib.h>
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

//202246109 김기현 과제1 
//과제 1-3(최종)의 코드

#define print(cur) \
	printf("PID: %03d\tARRIVAL: %03d\tCODESIZE: %03d\n", cur->pid, cur->arrival_time, cur->code_bytes); \
			for(int i = 0; i < cur->code_bytes / 2; i++){ \
                printf("%d %d\n", cur->operations[i].op, cur->operations[i].len); \
            }


typedef struct{
    unsigned char op; //cpu인지 io인지
    unsigned char len; //수행시간
} code_tuple;

typedef struct{
    int pid;
    int arrival_time;
    int code_bytes;
    int PC;                 //Program Counter   operations[PC] 
    int ioEnd;              //IO 끝나는 시간
    code_tuple *operations; //code_tuple이 저장된 위치를 가리킴
    struct list_head job, ready, wait;
} process;

    
int jobQnum = 0;        //job_q에 있는 프로세스 수
int clock = 0;          
int opEndTime = 0;     //operation이 끝나는 시간
int terminateNum = 0;   //종료된 프로세스 수
int idleTime = 0;       //idle시 카운트
int prevPid = 0;        //이전 프로세스 pid
bool doIdle = false;    //idleTime++ 용 플래그
bool reschedule = false;    //리스케줄 플래그


bool opCpu(process *cur){
    opEndTime = cur->operations[cur->PC].len + clock;    //명령 종료시간 
    //printf("%04d CPU: OP_CPU START \tPID: %03d len: %03d ends at: %04d PC: %3d\n", clock, cur->pid, cur->operations[cur->PC].len, opEndTime, cur->PC);
    cur->PC++;
    return false;
}

bool opIO(process *cur, struct list_head *wait_q){
    cur->ioEnd = cur->operations[cur->PC].len + clock;
    list_del_init(&cur->ready);             //ready_q -> wait_q
    list_add_tail(&cur->wait, wait_q);
    reschedule = true; 
    //printf("%04d CPU: OP_IO START len: %03d ends at: %04d\n", clock, cur->operations[cur->PC].len, cur->ioEnd);
    cur->PC++;                    //ready_q의 상태가 변했으니 리스케쥴 요청
    return false;  //cpu는 일을 했으니 clock 소모
}

bool terminateProcess(process *cur){
    //printf("%04d CPU: Process is terminated PID: %03d PC: %03d\n", clock, cur->pid, cur->PC);
    prevPid = cur->pid;             //pid 기록
    list_del_init(&cur->ready);     //readyQ에 프로세스 삭제
    terminateNum++;                 //종료된 프로세스 수 카운트
    reschedule = true;              //list_del로 인해 ready_q 의 상태가 변했으니 리스케쥴 요청
    free(cur->operations);    
    return true;
}

bool opIdle(process *cur, struct list_head *ready_q){
    list_del_init(&cur->ready);  
    
    if(list_empty(ready_q)){       //ready_q가 비었으면
                                    //1. 모든 프로세스가 종료           //(main의 while에서 처리 됨)
                                    //2. 프로세스가 아직 도착하지 않음   // O
        list_add_tail(&cur->ready, ready_q);
        if(doIdle == false){       //본격적인 idle 타임
            doIdle = true;
            //printf("%04d CPU: OP_IDLE START idleTIME : %d\n", clock, idleTime);
        }
        return false;               //clock 소모 해도 됨
    }else if(!list_empty(ready_q)){       //ready_q가 안비어있으면 작업이 남아있다는 소리
        
        list_add_tail(&cur->ready, ready_q);    //idle을 ready_q 맨 뒤로 보냄
        reschedule = true;      //리스케쥴 요청;
        if(doIdle){             //idle 처리하다가 끝나는 상황
            doIdle = false;
            idleTime++;         //true를 보내 continue를 할 예정이기 때문에
            clock++;            //continue 때문에 여기서 처리
            //printf("%04d CPU: OP_IDLE END  idleTime : %d\n", clock, idleTime);
        }
        return true;   
    }
    return;
}

bool executeProcess(process *cur, struct list_head *ready_q, struct list_head *wait_q){
    // cpu, io, idle 처리하는 함수
    // 프로세스가 종료되거나 idle이 종료될 때 true를 리턴 함으로써 continue가 됨

    if(clock >= opEndTime && cur->pid != 100){  //명령 처리 중(opEndTime > clock) 에는 작동 안함
        if(clock != 0 && clock == opEndTime){   //명령 처리가 끝났을 때 PC 증가
            //printf("%04d CPU: Increase PC \tPID:%03d PC:%03d\n", clock, cur->pid, cur->PC);
        }
        if(cur->PC < (cur->code_bytes / 2)){        //프로세스가 실행할 코드가 남아 있는지
            if(cur->operations[cur->PC].op == 0){   //op가 0 이면 cpu
                return opCpu(cur);
            }else if(cur->operations[cur->PC].op == 1){  //op가 1 이면 io
                return opIO(cur, wait_q);           
            }       
        }else{ //모든 명령이 끝났을 때 프로세스는 종료
            return terminateProcess(cur);
        }
    }else if(cur->pid == 100){          //프로세스가 idle 일 때
        return opIdle(cur, ready_q);
    }
    return false;
}

void checkIoEnd(process *cur, process *next,  struct list_head *wait_q, struct list_head *ready_q){
    //Io작업이 끝났는지 확인하는 함수;
    //io시작 io종료 겹칠때 io시작이 먼저 실행

    list_for_each_entry_safe(cur, next, wait_q, wait){  //wait_q를 순회해서
        if(clock >= cur->ioEnd){                 //IO 명령이 끝나면
            list_del_init(&cur->wait);              //wait_q -> ready_q
            list_add_tail(&cur->ready, ready_q);
            printf("%04d IO : COMPLETED! PID: %03d\tIOTIME: %03d\tPC: %03d\n", clock, cur->pid, cur->ioEnd, (cur->PC) - 1);
            //cur->PC++;                              //IO 작업이 끝났으니 PC
        }
    }
}

void switchProcess(process *cur, process *next, struct list_head *job_q, struct list_head *ready_q, struct list_head *wait_q){
    // Context Switching
    idleTime += 10;
    clock += 10;
    printf("%04d CPU: Switched\tfrom: %03d\tto: %03d\n", clock, prevPid, cur->pid);
    prevPid = cur->pid;
    
    //스위칭 중에는 다른 일을 하지 못함
    //스위치 도중 프로세스가 도착 했을 때
    for(int i = 0; i < 10; i++){
        list_for_each_entry(next, job_q, job){
            if(next->arrival_time == clock - 10 + i){    //clock 10 증가시켜놔서
                list_add_tail(&next->ready, ready_q);
                printf("%04d CPU: Loaded PID: %03d\tArrival: %03d\tCodesize: %03d\tPC: %03d\n", (clock), next->pid, next->arrival_time, next->code_bytes, next->PC);
            }
        }
    }
}


int main(int argc, char* argv[]){
    LIST_HEAD(job_q);       //시작점 생성(head)
    LIST_HEAD(ready_q);     //시작점 생성(head)
    LIST_HEAD(wait_q);      //시작점 생성(head)
    process *cur, *next;
 
    while((cur = (process*)malloc(sizeof(process))) && fread(cur, sizeof(int) * 3, 1, stdin)){ 
        cur->PC = 0;
        cur->ioEnd = 0;
        cur->operations = (code_tuple*)malloc(cur->code_bytes); 
        fread(cur->operations, cur->code_bytes, 1, stdin); 
        INIT_LIST_HEAD(&cur->job);      //초기화
        INIT_LIST_HEAD(&cur->ready);    //초기화
        INIT_LIST_HEAD(&cur->wait);     //초기화
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
        cur->ioEnd = 0;
        INIT_LIST_HEAD(&cur->job);  //초기화
        INIT_LIST_HEAD(&cur->ready);  //초기화
        INIT_LIST_HEAD(&cur->wait);  //초기화
        list_add_tail(&cur->job, &job_q); //끝에 넣고
    }
        
    /*
    // 파일 잘 읽어왔는지 확인 매크로
    list_for_each_entry(cur, &job_q, job){ 
		print(cur);
	}
    printf("Start Processing. Loaded_procs = %d\n", jobQnum);
    */
    
        //jobQ에 들어있는거랑 종료된 프로세스 수가 같아질 때 까지
    while(jobQnum != terminateNum){ 
        //Load
        if(!reschedule){
            list_for_each_entry(cur, &job_q, job){          //job_q 순회 해서
                if(clock == cur->arrival_time && reschedule == false){             //도착시간이랑 시간이 같으면 && 리스케줄 요청시 스위칭으로 인한 로딩 중복 방지
                    list_add_tail(&cur->ready, &ready_q);   //ready_q에 추가
                    printf("%04d CPU: Loaded PID: %03d\tArrival: %03d\tCodesize: %03d\tPC: %03d\n", clock, cur->pid, cur->arrival_time, cur->code_bytes, cur->PC);
                }           
            }
            checkIoEnd(cur, next, &wait_q, &ready_q);   //Io작업이 끝났는지 확인하는 함수;
            /*
            list_for_each_entry_safe(cur, next, &wait_q, wait){  //wait_q를 순회해서
                if(clock >= cur->ioEnd && opEndTime != cur->ioEnd){                 //IO 명령이 끝나면
                    list_del_init(&cur->wait);              //wait_q -> ready_q
                    list_add_tail(&cur->ready, &ready_q);
                    printf("%04d IO : COMPLETED! PID: %03d\tIOTIME: %03d\tPC: %03d\n", clock, cur->pid, cur->ioEnd, (cur->PC) - 1);
                }
            }
            */
        }

        cur = list_entry(ready_q.next, process, ready);     //cur의 위치 조정 ready_q의 맨 앞으로

        if(reschedule){                                     //reschedule을 요청할 때(reschedule == true) 리스케쥴링
            reschedule = false;                             //flag 초기화;
            if(cur->pid == 100){                        //pid 000 -> 100 -> 001 으로 가는 케이스 방지
                                                        //    000 --------> 001 으로 가야 함
                list_del_init(&cur->ready);             //지웠을 때
                if(!list_empty(&ready_q)){          //ready_q가 안비어있으면 idle 외에 다른 작업이 남아있음
                    list_add_tail(&cur->ready, &ready_q);    //idle을 ready_q 맨 뒤로 보냄 idle은 가장 낮은 우선순위
                    reschedule = true;
                    continue;
                }else{                              //ready_q 비었을 때 (idle밖에 없을 때)
                    list_add_tail(&cur->ready, &ready_q);
                    doIdle = true;
                }
            }
            //printf("%04d Reschedule\tPID: %03d\n", clock, prevPid);
            switchProcess(cur, next, &job_q, &ready_q, &wait_q);   //Context Switching
            checkIoEnd(cur, next, &wait_q, &ready_q);   //clock + 10에 대한 체킹 
        }

        if(executeProcess(cur, &ready_q, &wait_q)){
            continue;
        } 
        // cpu, io, idle 처리하는 함수~
        // bool 형으로 return으로 true가 넘어올 때(ex. terminated, idle) 
        // clock 소모 방지로 continue;

        if(doIdle){         //doIdle 이 true일 때 idleTime 증가
            idleTime++;
        }
        
        clock++;
    }
    printf("*** TOTAL CLOCKS: %04d IDLE: %04d UTIL: %2.2f%%\n", clock, idleTime, (float)(clock - idleTime) * 100 / clock );
    //printf("DONE. Freeing to processes in job queue\n");
    list_for_each_entry_safe_reverse(cur, next, &job_q, job){
        //printf("PID: %03d\tARRIVAL: %03d\tCODESIZE: %03d\tPC: %3d\n", cur->pid, cur->arrival_time, cur->code_bytes, cur->PC); 
		list_del_init(&cur->job);
		free(cur);
	}
    return 0;
}