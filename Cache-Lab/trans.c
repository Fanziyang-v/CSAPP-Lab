/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void trans_helper32(int M, int N, int A[N][M], int B[M][N], int startx, int starty);
/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
 
void trans_helper32(int M, int N, int A[N][M], int B[M][N], int startx, int starty)
{
	int i, j;
	int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	for (j = startx; j < startx + 32; j += 8) {
		for (i = starty; i < starty + 32; i++) {
			tmp0 = A[i][j];
			tmp1 = A[i][j+1];
			tmp2 = A[i][j+2];
			tmp3 = A[i][j+3];
			tmp4 = A[i][j+4];
			tmp5 = A[i][j+5];
			tmp6 = A[i][j+6];
			tmp7 = A[i][j+7];
			B[j][i] = tmp0;
			B[j+1][i] = tmp1;
			B[j+2][i] = tmp2;
			B[j+3][i] = tmp3;
			B[j+4][i] = tmp4;
			B[j+5][i] = tmp5;
			B[j+6][i] = tmp6;
			B[j+7][i] = tmp7;
		}
	}
}

void trans_helper4(int M, int N, int A[N][M], int B[M][N], int size)
{
	int i, j;
	int tmp0, tmp1, tmp2, tmp3;
	for (j = 0; j < size; j += 4) {
		for (i = 0; i < size; i++) {
			tmp0 = A[i][j];
			tmp1 = A[i][j+1];
			tmp2 = A[i][j+2];
			tmp3 = A[i][j+3];
			B[j][i] = tmp0;
			B[j+1][i] = tmp1;
			B[j+2][i] = tmp2;
			B[j+3][i] = tmp3;
		}
	}
}

void copy_symmetric_block(int M, int N, int A[N][M], int B[M][N], int sx, int sy)
{
	int i, j, k, l;
	
	for (i = sx, k = 0; i < sx + 8; i++, k++) {
		for (j = sy, l = 0; j < sy + 8; j++, l++) {
			B[i][j] = A[sy + k][sx + l];
		}
	}
}

void trans_submatrix(int M, int N, int B[M][N], int sx, int sy)
{
	int i, j;
	int tmp1, tmp2;
	for (i = sx; i < sx + 8; i++) {
		for (j = sy; j < sy + (i - sx); j++) {
			tmp1 = B[i][j];
			tmp2 = B[j - sy + sx][i - sx + sy];
			B[i][j] = tmp2;
			B[j - sy + sx][i - sx + sy] = tmp1;
		}
	}
}

void trans_advanced_helper(int M, int N, int A[N][M], int B[M][N])
{
	int i, j;
	int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;

	for (j = 0; j < 8 * (M / 8); j += 8) {
		for (i = 0; i < 4 * (N / 4); i++) {
			tmp0 = A[i][j];
			tmp1 = A[i][j + 1];
			tmp2 = A[i][j + 2];
			tmp3 = A[i][j + 3];
			tmp4 = A[i][j + 4];
			tmp5 = A[i][j + 5];
			tmp6 = A[i][j + 6];
			tmp7 = A[i][j + 7];
			B[j][i] = tmp0;
			B[j + 1][i] = tmp1;
			B[j + 2][i] = tmp2;
			B[j + 3][i] = tmp3;

			if (i + 4 < 4 * (M / 4)) {
				B[j][i + 4] = tmp4;
				B[j + 1][i + 4] = tmp5;
				B[j + 2][i + 4] = tmp6;
				B[j + 3][i + 4] = tmp7;
			}

			if (i % 4 == 3 && j + 8 <= 8 * (N / 8)) {
				/* sx, sy */
				tmp0 = j + 4;
				tmp1 = i - 3;

				for (tmp2 = j, tmp3 = i + 1; tmp2 < j + 4; tmp2++) {
					tmp4 = B[tmp2][tmp3];
					tmp5 = B[tmp2][tmp3 + 1];
					tmp6 = B[tmp2][tmp3 + 2];
					tmp7 = B[tmp2][tmp3 + 3];

					B[tmp0 + tmp2 - j][tmp1] = tmp4;
					B[tmp0 + tmp2 - j][tmp1 + 1] = tmp5;
					B[tmp0 + tmp2 - j][tmp1 + 2] = tmp6;
					B[tmp0 + tmp2 - j][tmp1 + 3] = tmp7;
				}
			}

		}

		/* Process special case */
		for (tmp0 = i - 4, tmp1 = j + 4; tmp0 < 4 * (M / 4); tmp0++) {

			tmp2 = A[tmp0][tmp1];
			tmp3 = A[tmp0][tmp1 + 1];
			tmp4 = A[tmp0][tmp1 + 2];
			tmp5 = A[tmp0][tmp1 + 3];

			B[tmp1][tmp0] = tmp2;
			B[tmp1 + 1][tmp0] = tmp3;
			B[tmp1 + 2][tmp0] = tmp4;
			B[tmp1 + 3][tmp0] = tmp5;
		}
		
	}
}

void trans_helper_auto(int M, int N, int A[N][M], int B[M][N])
{
	int i, j;
	int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	
	for (j = 0; j < 8 * (M / 8); j+=8) {
		for (i = 0; i < 8 * (N / 8); i++) {
			tmp0 = A[i][j];
			tmp1 = A[i][j+1];
			tmp2 = A[i][j+2];
			tmp3 = A[i][j+3];
			tmp4 = A[i][j+4];
			tmp5 = A[i][j+5];
			tmp6 = A[i][j+6];
			tmp7 = A[i][j+7];
			B[j][i] = tmp0;
			B[j+1][i] = tmp1;
			B[j+2][i] = tmp2;
			B[j+3][i] = tmp3;
			B[j+4][i] = tmp4;
			B[j+5][i] = tmp5;
			B[j+6][i] = tmp6;
			B[j+7][i] = tmp7;
		}
	}
	if (M == 61) {
		j = 56;
		for (i = 0; i < 67; i++) {
			tmp0 = A[i][j];
			tmp1 = A[i][j+1];
			tmp2 = A[i][j+2];
			tmp3 = A[i][j+3];
			tmp4 = A[i][j+4];
			B[j][i] = tmp0;
			B[j+1][i] = tmp1;
			B[j+2][i] = tmp2;
			B[j+3][i] = tmp3;
			B[j+4][i] = tmp4;
		}
		
		for (j = 0; j < 56; j += 8) {
			for (i = 64; i < 67; i++) {
				tmp0 = A[i][j];
				tmp1 = A[i][j+1];
				tmp2 = A[i][j+2];
				tmp3 = A[i][j+3];
				tmp4 = A[i][j+4];
				tmp5 = A[i][j+5];
				tmp6 = A[i][j+6];
				tmp7 = A[i][j+7];
				B[j][i] = tmp0;
				B[j+1][i] = tmp1;
				B[j+2][i] = tmp2;
				B[j+3][i] = tmp3;
				B[j+4][i] = tmp4;
				B[j+5][i] = tmp5;
				B[j+6][i] = tmp6;
				B[j+7][i] = tmp7;
			}
		}
		
	}
	
}



char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
	//int i, j;
	//int tmp;
	if (M == 32) {
		//trans_advanced_helper(M, N, A, B);
		trans_helper32(M, N, A, B, 0, 0);
	} else if (M == 64) {
		//trans_advanced_helper(M, N, A, B);
		trans_helper4(M, N, A, B, 64);
	} else if (M == 61) {
		trans_helper_auto(M, N, A, B);
	}
}





char trans_advanced_desc[] = "Transpose Advanced blocking";
void trans_advanced_blocking(int M, int N, int A[N][M], int B[M][N])
{
	if (M == 32 || M == 64) {
		trans_advanced_helper(M, N, A, B);
	}
}


char trans_blocking_desc[] = "Transpose using blocking";
void trans_blocking(int M, int N, int A[N][M], int B[M][N])
{
	int bsize = 8;
	int en = bsize * (N / bsize);
	int kk, jj;
	/* Using Blocking transpose matrix */
	for (kk = 0; kk < en; kk += bsize) {
		for (jj = 0; jj < en; jj += bsize) {
			copy_symmetric_block(M, N, A, B, kk, jj);
			/* Transpose every blocking matrix */
			trans_submatrix(M, N, B, kk, jj);
		}
	}
}



char trans_help_desc[] = "Transpose Using helper function";
void trans_help(int M, int N, int A[N][M], int B[M][N])
{
	int kk, jj, j, k;
	
	int en;
	int tmp;
	int bsize;
	if (M == 32)  bsize = 8;
	if (M == 64)  bsize = 4;
	en = bsize * (M / bsize);
	
	for (kk = 0; kk < en; kk += bsize) {
		for (jj = 0; jj < en; jj += bsize) {
			for (j = jj; j < jj + bsize; j++) {
				for (k = kk; k < kk + bsize; k++) {
					tmp = A[k][j];
					B[j][k] = tmp;
				}
			}
		}
	}
}






/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 



char trans_col_wise_desc[] = "Column-wise scan tranpose";
void trans_col_wise(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, tmp;
	
	for (j = 0; j < M; j++) {
		for (i = 0; i < N; i++) {
			tmp = A[i][j];
			B[j][i] = tmp;
		}
	}
}

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
    
    registerTransFunction(trans_col_wise, trans_col_wise_desc); 
	
	registerTransFunction(trans_help, trans_help_desc);
	
	registerTransFunction(trans_blocking, trans_blocking_desc);
	
	registerTransFunction(trans_advanced_blocking, trans_advanced_desc);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

