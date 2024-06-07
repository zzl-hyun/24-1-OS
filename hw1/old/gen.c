#include <stdio.h>
#include <stdlib.h>

typedef struct {
    unsigned char op;
    unsigned char len;
} code_tuple;

typedef struct {
    int pid;
    int arrival_time;
    int code_bytes;
} process;

int main(int argc, char* argv[]) {
    FILE *fin = fopen("./in.txt", "r");
    FILE *fout = fopen("./out.bin", "wb");

    if (fin == NULL || fout == NULL) {
        printf("Error opening files.\n");
        return 1;
    }

    process cur;
    while (fscanf(fin, "%d %d %d", &cur.pid, &cur.arrival_time, &cur.code_bytes) == 3) {
        fwrite(&cur, sizeof(process), 1, fout);

        int num_tuples = cur.code_bytes / 2;
        code_tuple *codes = (code_tuple*)malloc(num_tuples * sizeof(code_tuple));

        for (int i = 0; i < num_tuples; ++i) {
            if (fscanf(fin, "%hhu %hhu", &codes[i].op, &codes[i].len) != 2) {
                printf("Error parsing code tuple.\n");
                break;
            }
        }

        fwrite(codes, sizeof(code_tuple), num_tuples, fout);
        free(codes);
    }

    fclose(fin);
    fclose(fout);
    return 0;
}
