#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define PAGESIZE (32)
#define PAS_FRAMES (256)
#define PAS_SIZE (PAGESIZE*PAS_FRAMES) // 8kb
#define VAS_PAGES (64)
#define VAS_SIZE (PAGESIZE*VAS_PAGES) // 2kb
#define PTE_SIZE (4)
#define PAGETABLE_FRAMES (VAS_PAGES*PTE_SIZE/PAGESIZE) // 8b
#define PAGE_INVALID (0)
#define PAGE_VALID (1)
#define MAX_REFERENCES (256)

typedef struct{
    unsigned char b[PAGESIZE];
} frame;

typedef struct{
    unsigned char frame;
    unsigned char vflag;
    unsigned char ref;
    unsigned char pad;
} pte; 

typedef struct{
    int pid;
    int ref_len;
    unsigned char *references;
    int allocatedFrame;
    int pfCount;
    int refCount;
} process;

process *psArray[10];
int psNum = 0;
int usedFrame = 0;

void loadProcess(){
    printf("load_process() start\n");
    for(; (psArray[psNum] = (process*)malloc(sizeof(process))) && fread(psArray[psNum], sizeof(int) * 2, 1, stdin); psNum++){
        //ref_len 만큼 메모리 할당
        psArray[psNum]->references = (unsigned char*)malloc(psArray[psNum]->ref_len);
        fread(psArray[psNum]->references, psArray[psNum]->ref_len, 1, stdin);
        psArray[psNum]->allocatedFrame = 1; //L1PT용 프레임
        usedFrame++; 
        psArray[psNum]->pfCount = 0;
        psArray[psNum]->refCount = 0;
    }
    
    // debug
    for(int i = 0; i < psNum; i++){
        printf("%d %d\n", psArray[i]->pid, psArray[i]->ref_len);
        for(int j = 0; j < psArray[i]->ref_len; j++){
            printf("%d ", psArray[i]->references[j]);
        }
        printf("\n");
    }
    printf("load_process() end\n");
    
    
    return;
}

void init(frame* pas){
    pte* cur_pte = (pte*)&pas[0];
    for(int i = 0; i < VAS_SIZE; i++){
        //초기화
        cur_pte->vflag = PAGE_INVALID;
        cur_pte->ref = 0;
        cur_pte++; // 다음 pte
    }    
}

bool accessMemory(frame* pas, process* cur, int pos){
    /*
    {
        • 동작 예 : 10번 페이지 접근 시
        • L1 PT
            • 10/8 = 1번 PTE를 확인하고, page fault 인 경우, frame 할당 후, valid 설정
        • L2 PT
            • L1 1번 PTE를 확인 후, 해당 PTE 가 가리키는 frame 을 확인
            • 해당 frame 은 다시 8개의 PTE 로 구성되어 있고,
            • 10%8 = 2 번 PTE를 확인하고,
                • Page fault 인 경우, frame 할당 후, valid 설정 후 아래와 동일하게 처리
                • PF가 아닌 경우, 해당 frame 으로 접근, references count 증가
        • 출력: L1 PT를 순회하며, valid PTE에 대해 L2 PT 의 내용을 3-1과 같이 출력
            • L1 PT: Index -> Frame
            • L2 PT: Page -> Frame REF=해당 페이지에 대한 references count
    }
    */
    pte* L1PT = (pte*)&pas[cur->pid];
    int L1_index = cur->references[pos] / PAGETABLE_FRAMES;
    int L2_index = cur->references[pos] % PAGETABLE_FRAMES;
    
    // valid pte 일 때
    if(L1PT[L1_index].vflag == PAGE_VALID){
        //[PID 00 REF:001] Page access 052: (L1PT) Frame 002, (L2PT) Frame 003
        printf("[PID %02d REF:%03d] Page access %03d: (L1PT) Frame %03d,", cur->pid, cur->refCount, cur->references[pos],  L1PT[L1_index].frame);
    }
    //L1PT 할당
    else if(L1PT[L1_index].vflag == PAGE_INVALID){
        // L1_index page fault
        if(usedFrame >= PAS_FRAMES){
            //printf("break"); 
            return true;
        }
        L1PT[L1_index].frame = usedFrame++;
        L1PT[L1_index].vflag = PAGE_VALID;
        cur->pfCount++;
        cur->allocatedFrame++;
        printf("[PID %02d REF:%03d] Page access %03d: (L1PT) PF,Allocated Frame %03d -> %03d,", cur->pid, cur->refCount, cur->references[pos], L1_index, L1PT[L1_index].frame);
    }
        
    pte* L2PT = (pte*)&pas[L1PT[L1_index].frame];
    // valid pte 일 때
    if(L2PT[L2_index].vflag == PAGE_VALID){
        L2PT[L2_index].ref++;
        cur->refCount++;
        //(L2PT) Frame 003
        printf("(L2PT) Frame %03d\n", L2PT[L2_index].frame);
    }
    // unvalid pte 일 때 -> page fault    
    else if(L2PT[L2_index].vflag == PAGE_INVALID){
        if(usedFrame >= PAS_FRAMES){
            //printf("break"); 
            return true;
        }
        L2PT[L2_index].frame = usedFrame++;
        L2PT[L2_index].vflag = PAGE_VALID;
        cur->pfCount++;
        cur->allocatedFrame++;

        //PF,Allocated Frame 006
        printf("(L2PT) PF,Allocated Frame %03d\n", L2PT[L2_index].frame);
        L2PT[L2_index].ref++;
        cur->refCount++;
    }
    return false;
}

void demandPaging(frame* pas){
    int pos = 0;
    printf("start() start\n");

    while(1){
        // reference sequence 끝에 도달
        if(pos >= MAX_REFERENCES){
            return;
        }
        // 프로세스 순회하면서 페이지 접근
        for(int i = 0; i < psNum; i++){
            process* cur = psArray[i];
            // 프레임 부족하면
            if(usedFrame >= PAS_FRAMES){
                //cur = psArray[i % psNum];
                printf("[PID %02d REF:%03d] Page access %03d: (L1PT) Frame ,(L2PT) PF,",cur->pid, cur->refCount, cur->references[pos]);
                printf("Out of memory!!\n");
                return;
            } 
            // 프레임 부족하면 break;
            if(cur->ref_len > pos && accessMemory(pas, cur, pos)){
                break;
            }
        }
        pos++;
    }   
}

void finalReport(frame* pas){
    int totalFrame = 0;
    int totalPF = 0;
    int totalRef = 0;
    printf("start() end\n");

    for(int i = 0; i < psNum; i++){
        // process
        printf("** Process %03d: Allocated Frames=%03d PageFaults/References=%03d/%03d\n",
         psArray[i]->pid, psArray[i]->allocatedFrame, psArray[i]->pfCount, psArray[i]->refCount);
        
        totalFrame += psArray[i]->allocatedFrame;
        totalPF += psArray[i]->pfCount;
        totalRef += psArray[i]->refCount;

        pte* L1PT = (pte*)&pas[i];
        for(int j = 0; j < PAGETABLE_FRAMES; j++){
            // valid L1PT
            if(L1PT[j].vflag == PAGE_VALID){
                //(L1PT) 002 -> 012
                printf("(L1PT) %03d -> %03d\n", j, L1PT[j].frame);
                pte* L2PT = (pte*)&pas[L1PT[j].frame];
                // valid L2PT
                for(int k = 0; k < PAGETABLE_FRAMES; k++){
                    if(L2PT[k].vflag == PAGE_VALID){
                        //(L2PT) 017 -> 013 REF=001
                        //printf("page access %d",psArray[i]->references[k]);
                        printf("(L2PT) %03d -> %03d REF=%03d\n", k + (j * PAGETABLE_FRAMES), L2PT[k].frame, L2PT[k].ref);
                        //printf("%d %d %d\n",i,j,k);
                        //printf("%d %d\n",L1PT[j].frame, L2PT[L1PT[j].frame].frame);
                    }
                }
            }
        }
    }
    printf("Total: Allocated Frames=%03d Page Faults/References=%03d/%03d\n", totalFrame, totalPF, totalRef);
}

int main(){
    frame* pas = (frame*)malloc(PAS_SIZE);

    // LOAD
    loadProcess();
    //printf("%d", usedFrame);
    
    // INIT
    init(pas);

    // Demand Paging
    demandPaging(pas);

    // Final Report
    finalReport(pas);

    /*
    pte* PTE = (pte*)&pas[0];
    for (int i = 0; i < VAS_SIZE; i++) {
        if(PTE->vflag) printf("i=%03d\tframe=%03d\tref=%03d\t\n", i, PTE->frame,  PTE->ref);
        //printf("i=%03d\tframe=%03d\tref=%03d\n", i, PTE->frame,  PTE->ref);
        PTE++;
    }
    */
    
    // FREE
    free(pas);
    for(int i = 0; i < psNum; i++){
        free(psArray[i]->references);
        free(psArray[i]);
    }
    return 0;
}