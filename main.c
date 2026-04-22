#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <math.h>
#define TAMANHO 500000

int primo (int n) {
int i;
	for (i = 3; i < (int)(sqrt(n) + 1); i+=2) {
			if(n%i == 0) return 0;
	}
	return 1;
}

//  switch (tipo_send)
//     {
//     case '1':
        
//         MPI_Send(buf_envio, count, MPI_INT, dest, tag, MPI_COMM_WORLD);
//         break;
//     case '2':
//         MPI_Request request;
//         MPI_Status status;
//         MPI_Isend(buf_envio, count, MPI_INT, dest, tag, MPI_COMM_WORLD, &request);
//         MPI_Wait(&request, &status);
//         break;
//     case '3':
//         MPI_Rsend(buf_envio, count, MPI_INT, dest, tag, MPI_COMM_WORLD);
//         break;

void escolhe_send(char a, void *buf, int count, MPI_Datatype type, int dest, int tag) {

    switch (a)
    {
    case '1':
        MPI_Send(buf, count, type, dest, tag, MPI_COMM_WORLD);
        break;
    case '2':
        MPI_Request request;
        MPI_Status status;
        MPI_Isend(buf, count, type, dest, tag, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);
        break;
    case '3':
        //Caso pro Rsend que não é apropriado para o bag of tasks
        break;
    case '4':
        MPI_Bsend(buf, count, type, dest, tag, MPI_COMM_WORLD);
        break;
    case '5':
        MPI_Ssend(buf, count, type, dest, tag, MPI_COMM_WORLD);
        break;
    default:
        break;
    }
}

void escolhe_receive(char b, void *buf, int count, MPI_Datatype type, int source, int tag, MPI_Status *status) {

    //MPI_Request request;
    switch (b)
    {
    case '1':
        MPI_Recv(buf, count, type, source, tag, MPI_COMM_WORLD, status);
        break;
    case '2':
        /* code */
        //MPI_Wait(&request, status);
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[]) {
double t_inicial, t_final;
int cont = 0, total = 0;
int i, n;
int meu_ranque, num_procs, inicio, dest, raiz=0, stop=0;
void *buffer_bsend = NULL;
MPI_Status estado;
/* Verifica o número de argumentos passados */
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
    
/* Se houver menos que dois processos aborta */
    if (num_procs < 2) {
        if (meu_ranque == 0) printf("Este programa exige pelo menos 2 processos.\n");
        MPI_Finalize();
        return 1;
    }
/* Preparação para buffer em caso BSend */
    if (m_send == '4') {
        int size_int;
        MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &size_int);
        int buffer_size = num_procs * (size_int + MPI_BSEND_OVERHEAD);
        buffer_bsend = malloc(buffer_size);
        MPI_Buffer_attach(buffer_bsend, buffer_size);
    }
    
/* Registra o tempo inicial de execução do programa */
    t_inicial = MPI_Wtime();
    
/* Envia pedaços com TAMANHO números para cada processo */
    if (meu_ranque == 0) { 
        for (dest = 1, inicio = 3; dest < num_procs; dest++) {
            if (inicio < n) {
                escolhe_send(m_send, &inicio, 1, MPI_INT, dest, 1);
                inicio += TAMANHO;
            } else {
                escolhe_send(m_send, &inicio, 1, MPI_INT, dest, 99);
                stop++;
            }
        }
        
/* Fica recebendo as contagens parciais de cada processo */
        while (stop < (num_procs-1)) {
		    escolhe_receive(m_recv, &cont, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, &estado);
            total += cont;
            dest = estado.MPI_SOURCE;
            
            if (inicio < n) {
                escolhe_send(m_send, &inicio, 1, MPI_INT, dest, 1);
                inicio += TAMANHO;
            } else {
                escolhe_send(m_send, &inicio, 1, MPI_INT, dest, 99);
                stop++;
            }
        }
    }       
    else { 
/* Cada processo escravo recebe o início do espaço de busca */
        while (1) {
            escolhe_receive(m_recv, &inicio, 1, MPI_INT, 0, MPI_ANY_TAG, &estado);
            
            if (estado.MPI_TAG == 99) break;
            
            cont = 0;
            for (i = inicio; i < (inicio + TAMANHO) && i < n; i+=2) 
                if (primo(i)) cont++;
/* Envia a contagem parcial para o processo mestre */
            escolhe_send(m_send, &cont, 1, MPI_INT, 0, 1);
        } 
    }
	if (meu_ranque == 0) {
		t_final = MPI_Wtime();
        total += 1;    /* Acrescenta o 2, que é primo */
		printf("Quant. de primos entre 1 e %d: %d \n", n, total);
		printf("Tempo de execucao: %1.3f \n", t_final - t_inicial);  	 
	}
/* Finaliza o programa */
	MPI_Barrier(MPI_COMM_WORLD);
    
    if (m_send == '4') {
        int size_extra;
        MPI_Buffer_detach(&buffer_bsend, &size_extra);
        free(buffer_bsend);
    }
    
	MPI_Finalize();
	return(0);
}
