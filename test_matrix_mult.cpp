#include <iostream>
#include <vector>
#include <string>

void matrix_multiply(int* A, int* B, int* C, int rowsA, int colsA, int colsB);

void test_matrix_multiplication(int rowsA, int colsA, int colsB) {
    std::vector<int> A(rowsA * colsA);
    std::vector<int> B(colsA * colsB);
    std::vector<int> C(rowsA * colsB, 0);
    
    // Initialize matrices A and B with some values
    for (int i = 0; i < rowsA * colsA; ++i) A[i] = i + 1;
    for (int i = 0; i < colsA * colsB; ++i) B[i] = i + 1;
    
    // Call matrix multiplication function
    matrix_multiply(A.data(), B.data(), C.data(), rowsA, colsA, colsB);
    
    // Print result matrix C
    std::cout << "Result matrix C (" << rowsA << "x" << colsB << "):" << std::endl;
    for (int i = 0; i < rowsA; ++i) {
        for (int j = 0; j < colsB; ++j) {
            std::cout << C[i * colsB + j] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    // Test case 1: 2x2 * 2x2
    test_matrix_multiplication(2, 2, 2);
    
    // Test case 2: 4x4 * 4x4
    test_matrix_multiplication(4, 4, 4);
    
    // Test case 3: 3x4 * 4x2
    test_matrix_multiplication(3, 4, 2);
    
    return 0;
}

// This function will be replaced by the pPIM compiler-generated code
void matrix_multiply(int* A, int* B, int* C, int rowsA, int colsA, int colsB) {
    for (int i = 0; i < rowsA; ++i) {
        for (int j = 0; j < colsB; ++j) {
            int sum = 0;
            for (int k = 0; k < colsA; ++k) {
                sum += A[i * colsA + k] * B[k * colsB + j];
            }
            C[i * colsB + j] = sum;
        }
    }
}
