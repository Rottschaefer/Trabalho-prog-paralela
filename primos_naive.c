#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <math.h>

int primo(int n) {
    int i;

    for (i = 3; i < (int)(sqrt(n) + 1); i += 2) {
        if (n % i == 0) return 0;
    }

    return 1;
}

int calcula_primos_intervalo(int inicio, int fim, int n) {
    int i;
    int cont = 0;

    if (inicio % 2 == 0) {
        inicio++;
    }

    for (i = inicio; i < fim && i <= n; i += 2) {
        if (primo(i)) {
            cont++;
        }
    }

    return cont;
}

void inicia_send_int(char modo_send, int *buf, int dest, int tag, MPI_Request *request) {
    *request = MPI_REQUEST_NULL;

    switch (modo_send) {
    case '1':
        MPI_Send(buf, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
        break;

    case '2':
        MPI_Isend(buf, 1, MPI_INT, dest, tag, MPI_COMM_WORLD, request);
        break;

    case '3':
        MPI_Rsend(buf, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
        break;

    case '4':
        MPI_Bsend(buf, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
        break;

    case '5':
        MPI_Ssend(buf, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
        break;

    default:
        break;
    }
}

void espera_request(MPI_Request *request) {
    if (*request != MPI_REQUEST_NULL) {
        MPI_Wait(request, MPI_STATUS_IGNORE);
        *request = MPI_REQUEST_NULL;
    }
}

void recebe_int_bloqueante(char modo_recv, int *buf, int source, int tag, MPI_Status *status) {
    if (modo_recv == '1') {
        MPI_Recv(buf, 1, MPI_INT, source, tag, MPI_COMM_WORLD, status);
    } else {
        MPI_Request request;
        MPI_Irecv(buf, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, status);
    }
}

int main(int argc, char *argv[]) {
    double t_inicial, t_final;
    int cont = 0, total = 0;
    int i, n;
    int inicio = 3, fim, tamanho_bloco;
    int meu_ranque, num_procs;
    void *buffer_bsend = NULL;

    if (argc < 4) {
        printf("Uso: %s <limite> <modo_send> <modo_recv>\n", argv[0]);
        return 0;
    }

    n = atoi(argv[1]);
    char m_send = argv[2][0];
    char m_recv = argv[3][0];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &meu_ranque);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (m_send < '1' || m_send > '5') {
        if (meu_ranque == 0) {
            printf("Modo de envio inválido. Use 1, 2, 3, 4 ou 5.\n");
        }

        MPI_Finalize();
        return 1;
    }

    if (m_recv != '1' && m_recv != '2') {
        if (meu_ranque == 0) {
            printf("Modo de recebimento inválido. Use 1 para MPI_Recv ou 2 para MPI_Irecv.\n");
        }

        MPI_Finalize();
        return 1;
    }

    if (m_send == '4') {
        int size_int;
        MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &size_int);

        int buffer_size = num_procs * (size_int + MPI_BSEND_OVERHEAD);
        buffer_bsend = malloc(buffer_size);

        if (buffer_bsend == NULL) {
            if (meu_ranque == 0) {
                printf("Erro ao alocar buffer para MPI_Bsend.\n");
            }

            MPI_Finalize();
            return 1;
        }

        MPI_Buffer_attach(buffer_bsend, buffer_size);
    }

    tamanho_bloco = n / num_procs;
    inicio = 3 + meu_ranque * tamanho_bloco;

    if (meu_ranque == num_procs - 1) {
        fim = n + 1;
    } else {
        fim = inicio + tamanho_bloco;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    t_inicial = MPI_Wtime();

    if (m_recv == '2' && meu_ranque == 0) {
        int qtd_workers = num_procs - 1;
        int *parciais = malloc(qtd_workers * sizeof(int));
        MPI_Request *recv_requests = malloc(qtd_workers * sizeof(MPI_Request));
        MPI_Status *recv_statuses = malloc(qtd_workers * sizeof(MPI_Status));

        if (parciais == NULL || recv_requests == NULL || recv_statuses == NULL) {
            printf("Erro de alocação no processo raiz.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        /*
         * Aqui está o ponto importante:
         * o processo 0 posta todos os MPI_Irecv antes de calcular sua própria faixa.
         * Assim, os recebimentos já ficam ativos enquanto o processo 0 também calcula.
         */
        for (i = 1; i < num_procs; i++) {
            MPI_Irecv(&parciais[i - 1], 1, MPI_INT, i, MPI_ANY_TAG,
                      MPI_COMM_WORLD, &recv_requests[i - 1]);
        }

        cont = calcula_primos_intervalo(inicio, fim, n);
        total += cont;

        MPI_Waitall(qtd_workers, recv_requests, recv_statuses);

        for (i = 0; i < qtd_workers; i++) {
            total += parciais[i];
        }

        free(parciais);
        free(recv_requests);
        free(recv_statuses);
    } else {
        cont = calcula_primos_intervalo(inicio, fim, n);

        if (meu_ranque == 0) {
            MPI_Status status;

            total += cont;

            for (i = 1; i < num_procs; i++) {
                recebe_int_bloqueante(m_recv, &cont, i, MPI_ANY_TAG, &status);
                total += cont;
            }
        } else {
            MPI_Request send_request;

            inicia_send_int(m_send, &cont, 0, 0, &send_request);

            /*
             * O Wait não fica mais dentro da função de envio.
             * Aqui o processo espera apenas antes de finalizar, garantindo que o envio terminou.
             */
            espera_request(&send_request);
        }
    }

    t_final = MPI_Wtime();

    if (meu_ranque == 0) {
        total += 1;
        printf("Quant. de primos entre 1 e n: %d \n", total);
        printf("Tempo de execucao: %1.3f \n", t_final - t_inicial);
    }

    if (m_send == '4') {
        int size_extra;
        MPI_Buffer_detach(&buffer_bsend, &size_extra);
        free(buffer_bsend);
    }

    MPI_Finalize();
    return 0;
}