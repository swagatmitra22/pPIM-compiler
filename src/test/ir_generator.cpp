#include <stdio.h>
#include <stdlib.h>

#define ROWS_A 2
#define COLS_A 3
#define ROWS_B 3
#define COLS_B 2

int A[ROWS_A][COLS_A] = {{1, 2, 3}, {4, 5, 6}};
int B[ROWS_B][COLS_B] = {{7, 8}, {9, 10}, {11, 12}};

void generateIR() {
    FILE *ir_file = fopen("matr.ir", "w");
    if (!ir_file) {
        perror("Failed to create IR file");
        exit(1);
    }

    fprintf(ir_file, "@A = constant [%d x [%d x i32]] ", ROWS_A, COLS_A);
    fprintf(ir_file, "[");
    for (int i = 0; i < ROWS_A; i++) {
        fprintf(ir_file, "[%d x i32] [", COLS_A);
        for (int j = 0; j < COLS_A; j++) {
            fprintf(ir_file, "i32 %d%s", A[i][j], j == COLS_A - 1 ? "" : ", ");
        }
        fprintf(ir_file, "]%s", i == ROWS_A - 1 ? "" : ", ");
    }
    fprintf(ir_file, "]\n");

    fprintf(ir_file, "@B = constant [%d x [%d x i32]] ", ROWS_B, COLS_B);
    fprintf(ir_file, "[");
    for (int i = 0; i < ROWS_B; i++) {
        fprintf(ir_file, "[%d x i32] [", COLS_B);
        for (int j = 0; j < COLS_B; j++) {
            fprintf(ir_file, "i32 %d%s", B[i][j], j == COLS_B - 1 ? "" : ", ");
        }
        fprintf(ir_file, "]%s", i == ROWS_B - 1 ? "" : ", ");
    }
    fprintf(ir_file, "]\n");

    fprintf(ir_file, "@C = global [%d x [%d x i32]] zeroinitializer\n\n", ROWS_A, COLS_B);
    
    fprintf(ir_file, "define void @matrix_multiply() {\n");
    fprintf(ir_file, "entry:\n  br label %%outer_loop\n\n");
    fprintf(ir_file, "outer_loop:\n  %%i = phi i32 [0, %%entry], [%%i_next, %%outer_loop_end]\n");
    fprintf(ir_file, "  %%outer_cond = icmp slt i32 %%i, %d\n  br i1 %%outer_cond, label %%inner_loop, label %%exit\n\n", ROWS_A);
    fprintf(ir_file, "inner_loop:\n  %%j = phi i32 [0, %%outer_loop], [%%j_next, %%inner_loop_end]\n");
    fprintf(ir_file, "  %%inner_cond = icmp slt i32 %%j, %d\n  br i1 %%inner_cond, label %%innermost_loop, label %%outer_loop_end\n\n", COLS_B);
    fprintf(ir_file, "innermost_loop:\n  %%k = phi i32 [0, %%inner_loop], [%%k_next, %%innermost_loop_end]\n");
    fprintf(ir_file, "  %%innermost_cond = icmp slt i32 %%k, %d\n  br i1 %%innermost_cond, label %%loop_body, label %%inner_loop_end\n\n", COLS_A);
    fprintf(ir_file, "loop_body:\n  %%A_ptr = getelementptr [%d x [%d x i32]], [%d x [%d x i32]]* @A, i32 0, i32 %%i, i32 %%k\n", ROWS_A, COLS_A, ROWS_A, COLS_A);
    fprintf(ir_file, "  %%A_val = load i32, i32* %%A_ptr\n");
    fprintf(ir_file, "  %%B_ptr = getelementptr [%d x [%d x i32]], [%d x [%d x i32]]* @B, i32 0, i32 %%k, i32 %%j\n", ROWS_B, COLS_B, ROWS_B, COLS_B);
    fprintf(ir_file, "  %%B_val = load i32, i32* %%B_ptr\n");
    fprintf(ir_file, "  %%product = mul i32 %%A_val, %%B_val\n");
    fprintf(ir_file, "  %%C_ptr = getelementptr [%d x [%d x i32]], [%d x [%d x i32]]* @C, i32 0, i32 %%i, i32 %%j\n", ROWS_A, COLS_B, ROWS_A, COLS_B);
    fprintf(ir_file, "  %%current = load i32, i32* %%C_ptr\n");
    fprintf(ir_file, "  %%sum = add i32 %%current, %%product\n  store i32 %%sum, i32* %%C_ptr\n");
    fprintf(ir_file, "  %%k_next = add i32 %%k, 1\n  br label %%innermost_loop_end\n\n");
    fprintf(ir_file, "innermost_loop_end:\n  br label %%innermost_loop\n\n");
    fprintf(ir_file, "inner_loop_end:\n  %%j_next = add i32 %%j, 1\n  br label %%inner_loop\n\n");
    fprintf(ir_file, "outer_loop_end:\n  %%i_next = add i32 %%i, 1\n  br label %%outer_loop\n\n");
    fprintf(ir_file, "exit:\n  ret void\n}\n");
    fclose(ir_file);

    FILE *lookup_file = fopen("lookup_table.txt", "w");
    if (!lookup_file) {
        perror("Failed to create lookup table");
        exit(1);
    }
    fprintf(lookup_file, "Address\tVariable\n");
    fprintf(lookup_file, "0\t@A\n1\t@B\n2\t@C\n3\ti (outer loop counter)\n4\tj (inner loop counter)\n5\tk (innermost loop counter)\n");
    fclose(lookup_file);
}

int main() {
    generateIR();
    printf("Generated matr.ir and lookup_table.txt\n");
    return 0;
}
