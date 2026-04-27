#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <math.h>

int primo (long int n) { /* mpi_primos.c  */
	int i;
       
	for (i = 3; i < (int)(sqrt(n) + 1); i+=2) {
			if(n%i == 0) return 0;
	}
	return 1;
}


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
        MPI_Rsend(buf, count, type, dest, tag, MPI_COMM_WORLD);
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

    switch (b)
    {
    case '1':
        MPI_Recv(buf, count, type, source, tag, MPI_COMM_WORLD, status);
        break;

    case '2': {
        MPI_Request request;
        MPI_Irecv(buf, count, type, source, tag, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, status);
        break;
    }

    default:
        break;
    }
}

int main(int argc, char *argv[]) {
double t_inicial, t_final;
int cont = 0, total = 0;
long int i, n;
int meu_ranque, num_procs, inicio=3, fim, salto, tamanho_bloco;

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

	t_inicial = MPI_Wtime();

	tamanho_bloco = n/num_procs;
	inicio = 3 + meu_ranque*tamanho_bloco;
	fim = inicio + tamanho_bloco;

	if(inicio%2 == 0)inicio++; //Evitando que a contagem comece de um par


	for (i = inicio; i < fim && i<=n; i += 2) if(primo(i) == 1) cont++;
	
	// printf("Processo %d - inicio: %d - fim: %d - contagem:%d\n", meu_ranque, inicio, fim, cont); 

	if(meu_ranque == 0){

		MPI_Status status;

		total += cont;
		for (int j = 1; j < num_procs; j++)
		{
			escolhe_receive(m_recv, &cont, 1, MPI_INT, j, MPI_ANY_TAG, &status);
			total+=cont;
		}
		
	}

	else{ 
		escolhe_send(m_send, &cont, 1, MPI_INT, 0, 0);
	}

	
		
	
	t_final = MPI_Wtime();
	if (meu_ranque == 0) {
        total += 1;    /* Acrescenta o dois, que também é primo */
		printf("Quant. de primos entre 1 e n: %d \n", total);
		printf("Tempo de execucao: %1.3f \n", t_final - t_inicial);	 
	}
	MPI_Finalize();
	return(0);
}