#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

typedef struct {
    long long begin;
    long long end;
} Task;

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
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <num_threads> <N> <task_size>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_threads = atoi(argv[1]);
    long long N = atoll(argv[2]);
    long long task_size = atoll(argv[3]);

    if (num_threads <= 0 || N < 2 || task_size <= 0) {
        fprintf(stderr, "Erro: use num_threads > 0, N >= 2 e task_size > 0.\n");
        return EXIT_FAILURE;
    }

    omp_set_num_threads(num_threads);

    long long count = 1;

    // Conta quantidade de candidatos ímpares no intervalo [3,N]
    long long odd_candidates = 0;
    
    if (N >= 3) {
        odd_candidates = ((N - 3) / 2) + 1;
    }

    // Conta quantidade de tasks
    long long num_tasks = (odd_candidates + task_size - 1) / task_size;

    Task *tasks = NULL;

    // Aloca espaço para as tasks
    if (num_tasks > 0) {
        tasks = malloc(num_tasks * sizeof(Task));
        if (tasks == NULL) {
            fprintf(stderr, "Erro ao alocar memória para as tarefas.\n");
            return EXIT_FAILURE;
        }
    }

    // Calcula 'begin' e 'end' de cada task, isto é, seus intervalos. Exemplo:
    /* 
     * tasks[0] = [3,21]
     * tasks[1] = [23,41]
     * tasks[2] = [43,61]
     * ...
     */
    for (long long t = 0; t < num_tasks; t++) {
        long long first_index = t * task_size;
        long long last_index = first_index + task_size - 1;

        if (last_index >= odd_candidates) {
            last_index = odd_candidates - 1;
        }

        /*
         * Índice 0 representa o número 3.
         * Índice 1 representa o número 5.
         * Índice k representa 3 + 2k.
         */
        tasks[t].begin = 3 + 2 * first_index;
        tasks[t].end = 3 + 2 * last_index;
    }

    double start = omp_get_wtime();

    // Distribui as tasks de forma dinâmica entre as threads
    #pragma omp parallel for schedule(dynamic, 10) reduction(+:count)
    for (long long t = 0; t < num_tasks; t++) {
        // Contador interno para task
        long long local_count = 0;

        // Uma thread conta sequencialmente a quantidade de primos para uma task
        for (long long i = tasks[t].begin; i <= tasks[t].end; i += 2) {
            local_count += is_prime(i);
        }

        count += local_count;
    }

    double end = omp_get_wtime();

    printf("Algoritmo: Bag of Tasks com Escalonamento Dinâmico\n");
    printf("Threads: %d\n", num_threads);
    printf("N: %lld\n", N);
    printf("Tamanho de cada tarefa: %lld candidatos impares\n", task_size);
    printf("Quantidade de tarefas: %lld\n", num_tasks);
    printf("Primos encontrados: %lld\n", count);
    printf("Tempo: %.6f segundos\n", end - start);

    free(tasks);

    return EXIT_SUCCESS;
}
