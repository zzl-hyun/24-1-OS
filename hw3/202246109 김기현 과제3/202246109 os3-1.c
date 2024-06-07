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
        psArray[psNum]->allocatedFrame = 8; // page 하나는 8개의 프레임
        usedFrame += 8;
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
        // 초기화
        cur_pte->frame = 0;
        cur_pte->vflag = PAGE_INVALID;
        cur_pte->ref = 0;
        cur_pte++; // 다음 pte
    }    
}

bool accessMemory(frame* pas, process* cur, int pos){
    pte* cur_pte = (pte*)&pas[cur->pid * 8];
    // valid pte 일 때
    if(cur_pte[cur->references[pos]].vflag == PAGE_VALID){
        printf("[PID %02d REF:%03d] Page access %03d: Frame %03d\n", cur->pid, cur->refCount, cur->references[pos], cur_pte[cur->references[pos]].frame);
        cur_pte[cur->references[pos]].ref++;
        cur->refCount++;
    }
    // unvalid pte 일 때 -> page fault
    if(cur_pte[cur->references[pos]].vflag == PAGE_INVALID){
        //page fault
        if(usedFrame >= PAS_FRAMES){
            //printf("break"); 
            return true;
        }
        cur_pte[cur->references[pos]].frame = usedFrame++;
        cur_pte[cur->references[pos]].vflag = PAGE_VALID;
        cur->pfCount++;
        cur->allocatedFrame++;
        printf("[PID %02d REF:%03d] Page access %03d: PF,Allocated Frame %03d\n",cur->pid, cur->refCount, cur->references[pos], cur_pte[cur->references[pos]].frame);
        cur_pte[cur->references[pos]].ref++;
        cur->refCount++;
    }    
    return false;
}

void demandPaging(frame* pas){
    printf("start() start\n");
    int pos = 0;

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
                printf("[PID %02d REF:%03d] Page access %03d: PF,",cur->pid, cur->refCount, cur->references[pos]);
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
        //process
        printf("** Process %03d: Allocated Frames=%03d PageFaults/References=%03d/%03d\n",
         psArray[i]->pid, psArray[i]->allocatedFrame, psArray[i]->pfCount, psArray[i]->refCount);
        
        totalFrame += psArray[i]->allocatedFrame;
        totalPF += psArray[i]->pfCount;
        totalRef += psArray[i]->refCount;

        pte* cur_pte = (pte*)&pas[i * 8];
        // valid pte만 출력
        for(int j = 0; j < VAS_PAGES; j++){
            if(cur_pte[j].vflag == PAGE_VALID){
                printf("%03d -> %03d REF=%03d\n", j, cur_pte[j].frame, cur_pte[j].ref);
            }
        }
    }
    //printf("%d %d\n", usedFrame, totalFrame);
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
        if(PTE->vflag) printf("i=%03d\tframe=%03d\tref=%03d\n", i, PTE->frame,  PTE->ref);
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