#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define OP_LOAD  0x0
#define OP_STORE 0x1
#define OP_ADD   0x2
#define OP_MUL   0x3
#define OP_CMP   0x4
#define OP_BR    0x5
#define OP_PHI   0x6
#define OP_RET   0x7

typedef struct {
    uint32_t address;
    char name[50];
} LookupEntry;

LookupEntry lookup_table[20];
int lookup_count = 0;

void load_lookup_table() {
    FILE *file = fopen("lookup_table.txt", "r");
    if (!file) {
        perror("Failed to open lookup table");
        exit(1);
    }

    char line[100];
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file)) {
        char *addr_str = strtok(line, "\t");
        char *name = strtok(NULL, "\t\n");
        
        if (addr_str && name) {
            lookup_table[lookup_count].address = atoi(addr_str);
            strncpy(lookup_table[lookup_count].name, name, sizeof(lookup_table[0].name) - 1);
            lookup_count++;
        }
    }

    fclose(file);
}

uint32_t encode_instruction(uint8_t opcode, uint8_t ptr, uint8_t read, uint8_t write, uint16_t addr) {
    return ((opcode & 0x3) << 17) | 
           ((ptr & 0x3F) << 11) | 
           ((read & 0x1) << 10) | 
           ((write & 0x1) << 9) | 
           (addr & 0x1FF);
}

void generate_isa() {
    FILE *ir_file = fopen("matr.ir", "r");
    if (!ir_file) {
        perror("Failed to open IR file");
        exit(1);
    }

    FILE *isa_file = fopen("matrix_mul.isa", "w");
    if (!isa_file) {
        perror("Failed to create ISA file");
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), ir_file)) {
        uint32_t instr = 0;
        uint8_t ptr = 0; 
        uint8_t read = 0;
        uint8_t write = 0;
        uint16_t addr = 0;

        for (int i = 0; i < lookup_count; i++) {
            if (strstr(line, lookup_table[i].name)) {
                addr = lookup_table[i].address;
                break;
            }
        }

        if (strstr(line, "load")) {
            instr = encode_instruction(OP_LOAD, ptr, 1, 0, addr);
        } else if (strstr(line, "store")) {
            instr = encode_instruction(OP_STORE, ptr, 0, 1, addr);
        } else if (strstr(line, "add")) {
            instr = encode_instruction(OP_ADD, ptr, 1, 1, addr);
        } else if (strstr(line, "mul")) {
            instr = encode_instruction(OP_MUL, ptr, 1, 1, addr);
        } else if (strstr(line, "icmp")) {
            instr = encode_instruction(OP_CMP, ptr, 1, 0, addr);
        } else if (strstr(line, "br")) {
            instr = encode_instruction(OP_BR, ptr, 0, 0, addr);
        } else if (strstr(line, "phi")) {
            instr = encode_instruction(OP_PHI, ptr, 1, 1, addr);
        } else if (strstr(line, "ret")) {
            instr = encode_instruction(OP_RET, ptr, 0, 0, addr);
        }

        if (instr) {
            fprintf(isa_file, "0x%06x\n", instr);
        }
    }

    fclose(ir_file);
    fclose(isa_file);
}

int main() {
    load_lookup_table();
    generate_isa();
    printf("Generated matrix_mul.isa from IR\n");
    return 0;
}
