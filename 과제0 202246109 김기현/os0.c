#include <stdio.h>
#include <stdlib.h>
//202246109 김기현
typedef struct{
    unsigned char op;
    unsigned char len;
} code_tuple;

typedef struct{
    int pid;
    int arrival_time;
    int code_bytes;
} process;

int main(int argc, char* argv[]){
    process cur; //고정크기이기 때문에 malloc이 필요가 없다!
    code_tuple *codes;

        while(fread(&cur, sizeof(process), 1, stdin)){ 
            // fread()의 리턴값 
            // 읽기 성공 : 읽은 전체 항목의 수   true
            // 읽기 실패 : count 보다 작음      false
            
            printf("%d %d %d\n", cur.pid, cur.arrival_time, cur.code_bytes);
            codes = (code_tuple*)malloc(cur.code_bytes); 
            //가변크기이기 때문에 malloc으로 code_bytes의 값 만큼 메모리 할당
            
            fread(codes, cur.code_bytes, 1, stdin);
            for(int i = 0; i < cur.code_bytes / 2; i++){
                //tuple의 수는 code_bytes의 절반 (code_bytes가 10이다 -> 튜플은 5개)
                printf("%d %d\n", codes[i].op, codes[i].len);
            }
            free(codes); //혹시모를 free
        } 
    return 0;
}