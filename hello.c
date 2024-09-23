#include <stdio.h>

#define MATRIX_SIZE 512

int main() {
    volatile int matrixA[MATRIX_SIZE][MATRIX_SIZE];
    volatile int matrixB[MATRIX_SIZE][MATRIX_SIZE];  // Will store the transpose of matrixA

    // Initialize matrixA
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            matrixA[i][j] = i + j;
        }
    }

    // Transpose matrixA into matrixB
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            matrixB[j][i] = matrixA[i][j];
        }
    }

    // Print a sample result from the transpose matrix
    printf("Transposed Result[0][0]: %d\n", matrixB[0][0]);
    printf("Transposed Result[1][0]: %d\n", matrixB[1][0]);  // Transposed element from matrixA[0][1]

    return 0;
}
