#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

int is_prime(long long n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if ((n & 1LL) == 0) return 0;

    long long limit = (long long)sqrt((double)n);

    for (long long d = 3; d <= limit; d += 2) {
        if (n % d == 0) return 0;
    }

    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <num_threads> <N>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_threads = atoi(argv[1]);
    long long N = atoll(argv[2]);

    if (num_threads <= 0 || N < 2) {
        fprintf(stderr, "Erro: use num_threads > 0 e N >= 2.\n");
        return EXIT_FAILURE;
    }

    omp_set_num_threads(num_threads);

    long long count = 1;

    double start = omp_get_wtime();

    #pragma omp parallel for schedule(dynamic, 10) reduction(+:count)
    for (long long i = 3; i <= N; i += 2) {
        count += is_prime(i);
    }

    double end = omp_get_wtime();

    printf("Algoritmo: Naive com Escalonamento Dinâmico\n");
    printf("Threads: %d\n", num_threads);
    printf("N: %lld\n", N);
    printf("Primos encontrados: %lld\n", count);
    printf("Tempo: %.6f segundos\n", end - start);

    return EXIT_SUCCESS;
}
