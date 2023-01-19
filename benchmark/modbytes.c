
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
    FILE * fp = fopen(argv[1], "r");
    uint32_t amount = atoi(argv[2]);
    uint32_t changed = 0;
    uint64_t file_size;

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);

    while(changed != amount) {
        uint32_t size = 1 + rand() % (amount - changed);
        uint32_t offset = rand() % (file_size - size);
        fseek(fp, offset, SEEK_SET);
        for(uint32_t i = 0; i < size; i++)
            fputc(rand() % 256, fp);
        changed += size;
    }

    fflush(fp);
    fclose(fp);
}
