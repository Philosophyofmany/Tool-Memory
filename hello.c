#include <stdio.h>

#define MATRIX_SIZE 512

int main() {
    volatile int matrixA[MATRIX_SIZE][MATRIX_SIZE];
    volatile int matrixB[MATRIX_SIZE][MATRIX_SIZE];
    volatile int result[MATRIX_SIZE][MATRIX_SIZE];

    // Initialize matrices
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            matrixA[i][j] = i + j;
            matrixB[i][j] = i - j;
            result[i][j] = 0;
        }
    }

    // Matrix multiplication
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            for (int k = 0; k < MATRIX_SIZE; k++) {
                result[i][j] += matrixA[i][k] * matrixB[k][j];
            }
        }
    }

    // Print a sample result
    printf("Result[0][0]: %d\n", result[0][0]);
}
