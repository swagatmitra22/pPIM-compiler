#include <iostream>
using namespace std;

int main() {
    const int r1 = 2, c1 = 3, r2 = 3, c2 = 2;
    
    int A[r1][c1] = {{1, 2, 3},
                     {4, 5, 6}};
    
    int B[r2][c2] = {{7, 8},
                     {9, 10},
                     {11, 12}};
    
    int C[r1][c2];
    
    for (int i = 0; i < r1; i++) {
        for (int j = 0; j < c2; j++) {
            C[i][j] = 0;
        }
    }
    
    for (int i = 0; i < r1; i++) {
        for (int j = 0; j < c2; j++) {
            for (int k = 0; k < c1; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    
    cout << "Resultant Matrix:" << endl;
    for (int i = 0; i < r1; i++) {
        for (int j = 0; j < c2; j++) {
            cout << C[i][j] << " ";
        }
        cout << endl;
    }
    
    return 0;
}
