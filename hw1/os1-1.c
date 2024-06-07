#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

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
    //struct list_head list;
} code_tuple;

typedef struct{
    int pid;
    int arrival_time;
    int code_bytes;
    code_tuple *operations; //?
    struct list_head job, ready, wait;
} process;


#define print(cur) \
	printf("PID: %03d\tARRIVAL: %03d\tCODESIZE: %03d\n", cur->pid, cur->arrival_time, cur->code_bytes); \
			for(int i = 0; i < cur->code_bytes / 2; i++){ \
                printf("%d %d\n", cur->operations[i].op, cur->operations[i].len); \
            }


int main(int argc, char* argv[]){
    LIST_HEAD(job_q);

    process *cur, *next;
	
        while((cur = (process*)malloc(sizeof(process))) && fread(cur, sizeof(int) * 3, 1, stdin)){ 
            //printf("PID: %03d\tARRIVAL: %03d\tCODESIZE: %03d\n", cur->pid, cur->arrival_time, cur->code_bytes); //디버깅용
            cur->operations = (code_tuple*)malloc(cur->code_bytes); 
            fread(cur->operations, cur->code_bytes, 1, stdin); ///////////음 codetuple 사이즈 때문에 안 읽힘;;;-> 주소전달로 해결
			/* //디버깅용
			for(int i = 0; i < cur->code_bytes / 2; i++){
                printf("%d %d\n", cur->operations[i].op, cur->operations[i].len);
            }
			*/ 
			//cur 데이터로 연결리스트 생성
            INIT_LIST_HEAD(&cur->job);  //초기화
            list_add_tail(&cur->job, &job_q); //끝에 넣고
        } 		
		list_for_each_entry_reverse(cur, &job_q, job){
			print(cur);
		}
		list_for_each_entry_safe(cur, next, &job_q, job){
			/*
            printf("remove PID: %03d\tARRIVAL: %03d\tCODESIZE: %03d\n", cur->pid, cur->arrival_time, cur->code_bytes);
			for(int i = 0; i < cur->code_bytes / 2; i++){
                printf("%d %d\n", cur->operations[i].op, cur->operations[i].len);
            }
            */
			list_del(&cur->job);
			free(cur->operations);
			free(cur);
		}
		/*
		printf("\n 삭제후 \n");
		list_for_each_entry(cur, &job_q, job){ //cur->job != (head) 왜 안멈춤?
			print(cur);
		}
		*/		
    return 0;
}
