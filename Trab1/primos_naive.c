#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <math.h>

// Função matemática otimizada
int primo(int n) {
    if (n <= 1) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    
    int limite = (int)sqrt(n);
    for (int i = 3; i <= limite; i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

void escolhe_send(char a, void *buf, int count, MPI_Datatype type, int dest, int tag, MPI_Request *req) {
    switch (a) {
        case '1':
            MPI_Send(buf, count, type, dest, tag, MPI_COMM_WORLD);
            if (req != NULL) *req = MPI_REQUEST_NULL; 
            break;
        case '2':
            MPI_Isend(buf, count, type, dest, tag, MPI_COMM_WORLD, req);
            break;
        case '3': 
            MPI_Rsend(buf, count, type, dest, tag, MPI_COMM_WORLD);
            if (req != NULL) *req = MPI_REQUEST_NULL;
            break;
        case '4':
            MPI_Bsend(buf, count, type, dest, tag, MPI_COMM_WORLD);
            if (req != NULL) *req = MPI_REQUEST_NULL; 
            break;
        case '5':
            MPI_Ssend(buf, count, type, dest, tag, MPI_COMM_WORLD);
            if (req != NULL) *req = MPI_REQUEST_NULL; 
            break;
        default:
            break;
    }
}

void escolhe_receive(char b, void *buf, int count, MPI_Datatype type, int source, int tag, MPI_Status *status, MPI_Request *req) {
    switch (b) {
        case '1':
            MPI_Recv(buf, count, type, source, tag, MPI_COMM_WORLD, status);
            if (req != NULL) *req = MPI_REQUEST_NULL;
            break;
        case '2': 
            MPI_Irecv(buf, count, type, source, tag, MPI_COMM_WORLD, req);
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    
    double t_inicial, t_final;
    int cont = 0, total = 0;
    int i, n;
    int meu_ranque, num_procs;
    int inicio, fim, tamanho_bloco;
    void *buffer_bsend = NULL;

    MPI_Comm_rank(MPI_COMM_WORLD, &meu_ranque);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (argc < 4) {
        if (meu_ranque == 0) printf("Uso: %s <limite> <modo_send> <modo_recv>\n", argv[0]);
        MPI_Finalize();
        return 0;
    }
    
    n = atoi(argv[1]);
    char m_send = argv[2][0];
    char m_recv = argv[3][0];

    if (num_procs < 2) {
        if (meu_ranque == 0) printf("Este programa exige pelo menos 2 processos.\n");
        MPI_Finalize();
        return 1;
    }

    if (m_send == '4') {
        int size_int;
        int buffer_size;
        MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &size_int);
        
        if (meu_ranque == 0) {
            buffer_size = 0; 
        } else {
            buffer_size = 1 * (size_int + MPI_BSEND_OVERHEAD);
        }
        
        if (buffer_size > 0) {
            buffer_bsend = malloc(buffer_size);
            MPI_Buffer_attach(buffer_bsend, buffer_size);
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    t_inicial = MPI_Wtime();

    // ================== CÁLCULO DOS BLOCOS (INTERVALO ABERTO) ==================
    tamanho_bloco = n / num_procs;
    inicio = meu_ranque * tamanho_bloco;
    
    // O último processo assume até o limite 'n', mas o laço usará '< fim'
    if (meu_ranque == num_procs - 1) {
        fim = n; 
    } else {
        fim = inicio + tamanho_bloco;
    }

    // Ajuste de segurança para o início
    if (inicio < 3) inicio = 3;
    if (inicio % 2 == 0) inicio++;

    // O coração do programa: Usa < fim (garante que n nunca será testado)
    for (i = inicio; i < fim; i += 2) {
        if (primo(i)) cont++;
    }

    // ================== CÓDIGO DO MESTRE ==================
    if (meu_ranque == 0) {
        MPI_Status estado;
        MPI_Request req_recv = MPI_REQUEST_NULL;
        
        total += cont; // O mestre soma a própria carga
        
        for (int j = 1; j < num_procs; j++) {
            int cont_parcial;
            escolhe_receive(m_recv, &cont_parcial, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, &estado, &req_recv);
            
            if (req_recv != MPI_REQUEST_NULL) {
                MPI_Wait(&req_recv, &estado);
            }
            total += cont_parcial;
        }
    }
    // ================== CÓDIGO DO ESCRAVO ==================
    else { 
        MPI_Request req_envio = MPI_REQUEST_NULL;
        
        escolhe_send(m_send, &cont, 1, MPI_INT, 0, 1, &req_envio);
        
        if (req_envio != MPI_REQUEST_NULL) {
            MPI_Wait(&req_envio, MPI_STATUS_IGNORE);
        }
    }

    // ================== FINALIZAÇÃO ==================
    if (meu_ranque == 0) {
        t_final = MPI_Wtime();
        
        // CORREÇÃO: No intervalo aberto (0, n[, o número '2' só está contido se n > 2.
        if (n > 2) total += 1; 
        
        printf("Quant. de primos entre 1 e %d: %d \n", n, total);
        printf("Tempo de execucao: %1.3f s\n", t_final - t_inicial);      
    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    if (m_send == '4' && meu_ranque != 0) {
        int size_extra;
        MPI_Buffer_detach(&buffer_bsend, &size_extra);
        free(buffer_bsend);
    }
    
    MPI_Finalize();
    return(0);
}
