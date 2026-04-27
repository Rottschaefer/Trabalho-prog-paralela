#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <math.h>

#define TAMANHO 500000

int primo(int n) {
    int i;

    for (i = 3; i < (int)(sqrt(n) + 1); i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int calcula_bloco(int inicio, int n) {
    int i;
    int cont = 0;

    if (inicio % 2 == 0) {
        inicio++;
    }

    for (i = inicio; i < inicio + TAMANHO && i < n; i += 2) {
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
        /*
         * MPI_Rsend não foi usado no Bag of Tasks porque ele exige
         * que o receive correspondente já esteja postado antes do envio.
         */
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

void mestre_envia_tarefa(
    char modo_send,
    int *send_buffers,
    MPI_Request *send_requests,
    int dest,
    int valor,
    int tag
) {
    espera_request(&send_requests[dest]);

    send_buffers[dest] = valor;
    inicia_send_int(modo_send, &send_buffers[dest], dest, tag, &send_requests[dest]);
}

int main(int argc, char *argv[]) {
    double t_inicial, t_final;
    int cont = 0, total = 0;
    int n;
    int meu_ranque, num_procs;
    int inicio, dest, stop = 0;
    void *buffer_bsend = NULL;
    MPI_Status estado;

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

    if (num_procs < 2) {
        if (meu_ranque == 0) {
            printf("Este programa exige pelo menos 2 processos.\n");
        }

        MPI_Finalize();
        return 1;
    }

    if (m_send != '1' && m_send != '2' && m_send != '4' && m_send != '5') {
        if (meu_ranque == 0) {
            printf("Modo de envio inválido para Bag of Tasks. Use 1, 2, 4 ou 5.\n");
            printf("Observação: MPI_Rsend não foi usado no Bag of Tasks.\n");
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

    MPI_Barrier(MPI_COMM_WORLD);
    t_inicial = MPI_Wtime();

    if (meu_ranque == 0) {
        int *send_buffers = calloc(num_procs, sizeof(int));
        MPI_Request *send_requests = malloc(num_procs * sizeof(MPI_Request));

        if (send_buffers == NULL || send_requests == NULL) {
            printf("Erro de alocação no processo mestre.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        for (int i = 0; i < num_procs; i++) {
            send_requests[i] = MPI_REQUEST_NULL;
        }

        if (m_recv == '2') {
            int ativos = 0;
            int indice_finalizado;
            int *resultados = calloc(num_procs, sizeof(int));
            MPI_Request *recv_requests = malloc(num_procs * sizeof(MPI_Request));

            if (resultados == NULL || recv_requests == NULL) {
                printf("Erro de alocação no processo mestre.\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            for (int i = 0; i < num_procs; i++) {
                recv_requests[i] = MPI_REQUEST_NULL;
            }

            inicio = 3;

            for (dest = 1; dest < num_procs; dest++) {
                if (inicio < n) {
                    mestre_envia_tarefa(m_send, send_buffers, send_requests, dest, inicio, 1);
                    inicio += TAMANHO;

                    MPI_Irecv(&resultados[dest], 1, MPI_INT, dest, 1,
                              MPI_COMM_WORLD, &recv_requests[dest]);

                    ativos++;
                } else {
                    mestre_envia_tarefa(m_send, send_buffers, send_requests, dest, inicio, 99);
                }
            }

            while (ativos > 0) {
                MPI_Waitany(num_procs, recv_requests, &indice_finalizado, &estado);

                if (indice_finalizado == MPI_UNDEFINED) {
                    break;
                }

                total += resultados[indice_finalizado];
                dest = indice_finalizado;

                if (inicio < n) {
                    mestre_envia_tarefa(m_send, send_buffers, send_requests, dest, inicio, 1);
                    inicio += TAMANHO;

                    MPI_Irecv(&resultados[dest], 1, MPI_INT, dest, 1,
                              MPI_COMM_WORLD, &recv_requests[dest]);
                } else {
                    mestre_envia_tarefa(m_send, send_buffers, send_requests, dest, inicio, 99);
                    recv_requests[dest] = MPI_REQUEST_NULL;
                    ativos--;
                }
            }

            free(resultados);
            free(recv_requests);
        } else {
            inicio = 3;

            for (dest = 1; dest < num_procs; dest++) {
                if (inicio < n) {
                    mestre_envia_tarefa(m_send, send_buffers, send_requests, dest, inicio, 1);
                    inicio += TAMANHO;
                } else {
                    mestre_envia_tarefa(m_send, send_buffers, send_requests, dest, inicio, 99);
                    stop++;
                }
            }

            while (stop < num_procs - 1) {
                recebe_int_bloqueante(m_recv, &cont, MPI_ANY_SOURCE, MPI_ANY_TAG, &estado);

                total += cont;
                dest = estado.MPI_SOURCE;

                if (inicio < n) {
                    mestre_envia_tarefa(m_send, send_buffers, send_requests, dest, inicio, 1);
                    inicio += TAMANHO;
                } else {
                    mestre_envia_tarefa(m_send, send_buffers, send_requests, dest, inicio, 99);
                    stop++;
                }
            }
        }

        for (int i = 1; i < num_procs; i++) {
            espera_request(&send_requests[i]);
        }

        free(send_buffers);
        free(send_requests);
    } else {
        if (m_recv == '2') {
            MPI_Request recv_request;
            MPI_Request send_request = MPI_REQUEST_NULL;

            MPI_Irecv(&inicio, 1, MPI_INT, 0, MPI_ANY_TAG,
                      MPI_COMM_WORLD, &recv_request);
            MPI_Wait(&recv_request, &estado);

            while (estado.MPI_TAG != 99) {
                cont = calcula_bloco(inicio, n);

                inicia_send_int(m_send, &cont, 0, 1, &send_request);

                MPI_Irecv(&inicio, 1, MPI_INT, 0, MPI_ANY_TAG,
                          MPI_COMM_WORLD, &recv_request);

                MPI_Wait(&recv_request, &estado);
                espera_request(&send_request);
            }

            espera_request(&send_request);
        } else {
            MPI_Request send_request = MPI_REQUEST_NULL;

            while (1) {
                recebe_int_bloqueante(m_recv, &inicio, 0, MPI_ANY_TAG, &estado);
                espera_request(&send_request);

                if (estado.MPI_TAG == 99) {
                    break;
                }

                cont = calcula_bloco(inicio, n);
                inicia_send_int(m_send, &cont, 0, 1, &send_request);
            }
            espera_request(&send_request);
        }
    }

    if (meu_ranque == 0) {
        t_final = MPI_Wtime();
        total += 1;
        printf("Quant. de primos entre 1 e %d: %d \n", n, total);
        printf("Tempo de execucao: %1.3f \n", t_final - t_inicial);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (m_send == '4') {
        int size_extra;
        MPI_Buffer_detach(&buffer_bsend, &size_extra);
        free(buffer_bsend);
    }

    MPI_Finalize();
    return 0;
}