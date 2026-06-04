#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>

int is_prime(long N) {
    if (N < 2) return 0;
    if (N == 2) return 1;
    if ((N & 1L) == 0) return 0;

    long limit = (long)sqrt((double)N);

    for (long div = 3; div <= limit; div += 2) {
        if (N % div == 0) return 0;
    }

    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <num_threads> <N> <static|dynamic|guided>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_threads = atoi(argv[1]);
    long N = atol(argv[2]);
    char *escalonamento = argv[3];
    int chunk = 10;

    if (num_threads <= 0 || N < 0) {
        fprintf(stderr, "Erro: use num_threads > 0 e N >= 0.\n");
        return EXIT_FAILURE;
    }

    omp_set_num_threads(num_threads);

    long count = 0;

    if (N >= 2) {
        count = 1;
    }

    double start = omp_get_wtime();

    if (strcmp(escalonamento, "static") == 0) {
        #pragma omp parallel for schedule(static, chunk) reduction(+:count)
        for (long i = 3; i <= N; i += 2) {
            count += is_prime(i);
        }

    } else if (strcmp(escalonamento, "dynamic") == 0 || strcmp(escalonamento, "dinamic") == 0) {
        #pragma omp parallel for schedule(dynamic, chunk) reduction(+:count)
        for (long i = 3; i <= N; i += 2) {
            count += is_prime(i);
        }

    } else if (strcmp(escalonamento, "guided") == 0) {
        #pragma omp parallel for schedule(guided, chunk) reduction(+:count)
        for (long i = 3; i <= N; i += 2) {
            count += is_prime(i);
        }

    } else {
        fprintf(stderr, "Erro: política inválida. Use static, dynamic ou guided.\n");
        return EXIT_FAILURE;
    }

    double end = omp_get_wtime();

    printf("Algoritmo: Naive\n");
    printf("Política de escalonamento: %s\n", escalonamento);
    printf("Chunk do escalonamento OpenMP: %d\n", chunk);
    printf("Threads: %d\n", num_threads);
    printf("N: %ld\n\n", N);
    printf("Primos encontrados: %ld\n", count);
    printf("Tempo: %.6f segundos\n", end - start);

    return EXIT_SUCCESS;
}