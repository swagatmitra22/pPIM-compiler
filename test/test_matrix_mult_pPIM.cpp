// test_matrix_mult_pPIM.cpp
// Test cases for pPIM compiler with different matrix dimensions

// Test case 1: 2x2 matrices
matrix A 2 2 [1, 2, 3, 4];
matrix B 2 2 [5, 6, 7, 8];
matrix C 2 2;
multiply A B C;

// Test case 2: 4x4 matrices
matrix A 4 4 [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];
matrix B 4 4 [16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1];
matrix C 4 4;
multiply A B C;

// Test case 3: 3x4 * 4x2 matrices
matrix A 3 4 [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
matrix B 4 2 [1, 2, 3, 4, 5, 6, 7, 8];
matrix C 3 2;
multiply A B C;
