#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <math.h>
#define TAMANHO 500000

int primo (int n) {
int i;
	int limite = (int)(sqrt(n));
    for (i = 3; i <= limite; i += 2) {
			if(n%i == 0) return 0;
	}
	return 1;
}

void escolhe_send(char a, void *buf, int count, MPI_Datatype type, int dest, int tag, MPI_Request *req) {

    switch (a)
    {
    // SEND
    case '1':
        MPI_Send(buf, count, type, dest, tag, MPI_COMM_WORLD);
        if (req != NULL) *req = MPI_REQUEST_NULL; // Marca que não há espera pendente
        break;
    // ISEND
    case '2':
        MPI_Isend(buf, count, type, dest, tag, MPI_COMM_WORLD, req);
        break;
    // RSEND
    case '3':
        // Não é apropriado para o bag of tasks
        break;
    // BSEND
    case '4':
        MPI_Bsend(buf, count, type, dest, tag, MPI_COMM_WORLD);
        if (req != NULL) *req = MPI_REQUEST_NULL; // Marca que não há espera pendente
        break;
    // SSEND
    case '5':
        MPI_Ssend(buf, count, type, dest, tag, MPI_COMM_WORLD);
        if (req != NULL) *req = MPI_REQUEST_NULL; // Marca que não há espera pendente
        break;
    default:
        break;
    }
}

void escolhe_receive(char b, void *buf, int count, MPI_Datatype type, int source, int tag, MPI_Status *status, MPI_Request *req) {

    switch (b)
    {
        
    // RECV
    case '1':
        MPI_Recv(buf, count, type, source, tag, MPI_COMM_WORLD, status);
        if (req != NULL) *req = MPI_REQUEST_NULL;
        break;

    // IRECV
    case '2': {
        MPI_Irecv(buf, count, type, source, tag, MPI_COMM_WORLD, req);
        break;
    }

    default:
        break;
    }
}

int main(int argc, char *argv[]) {
MPI_Init(&argc, &argv);
double t_inicial, t_final;
int cont, total = 0;
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
        int buffer_size;
        
        // Descobre o tamanho em bytes de 1 inteiro no MPI
        MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &size_int);
        
        // Apenas o processo 0 precisa de um buffer, pois envia mensagens para 3 processos, os outros enviam apenas para o processo 0
        if (meu_ranque == 0) {
            buffer_size = (num_procs - 1) * (size_int + MPI_BSEND_OVERHEAD);
        } else {
            buffer_size = size_int + MPI_BSEND_OVERHEAD;
        }
        buffer_bsend = malloc(buffer_size);
        MPI_Buffer_attach(buffer_bsend, buffer_size);
    }
    
/* Registra o tempo inicial de execução do programa */
    MPI_Barrier(MPI_COMM_WORLD);
    t_inicial = MPI_Wtime();
    
    // ================== CÓDIGO DO MESTRE ==================
    if (meu_ranque == 0) { 
        MPI_Request *reqs_tarefas = NULL;
        int *buffer_tarefas = NULL;
        MPI_Request req_recv = MPI_REQUEST_NULL;
        
        // Aloca os arrays APENAS se o modo for ISend ('2') para evitar redundância com BSend e poupar RAM
        if (m_send == '2') {
            reqs_tarefas = malloc(num_procs * sizeof(MPI_Request));
            buffer_tarefas = malloc(num_procs * sizeof(int));
            for (i = 0; i < num_procs; i++) reqs_tarefas[i] = MPI_REQUEST_NULL;
        }
        
        // Envia pedaços com TAMANHO números para cada processo
        for (dest = 1, inicio = 3; dest < num_procs; dest++) {
            if (inicio <= n) {
                // SE FOR ISEND: Passa o buffer exclusivo do escravo e o ponteiro do ticket
                // Isso é importante, caso contrário o Mestre poderia sobrescrever 'inicio' antes mesmo do Escravo lê-lo
                if (m_send == '2') {
                    buffer_tarefas[dest] = inicio;
                    escolhe_send(m_send, &buffer_tarefas[dest], 1, MPI_INT, dest, 1, &reqs_tarefas[dest]);
                } 
                // SE NÃO FOR: Passa a variável simples e passa NULL no ticket
                else {
                    escolhe_send(m_send, &inicio, 1, MPI_INT, dest, 1, NULL);
                }
                inicio += TAMANHO;
            } else {
                if (m_send == '2') {
                    buffer_tarefas[dest] = inicio;
                    escolhe_send(m_send, &buffer_tarefas[dest], 1, MPI_INT, dest, 99, &reqs_tarefas[dest]);
                } else {
                    escolhe_send(m_send, &inicio, 1, MPI_INT, dest, 99, NULL);
                }
                stop++;
            }
        }
        
/* Fica recebendo as contagens parciais de cada processo */
        while (stop < (num_procs-1)) {
            escolhe_receive(m_recv, &cont, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, &estado, &req_recv);
            
            // GARANTIA DO RECEBIMENTO: Se o Mestre usou IRecv, ele deve esperar a rede preencher a variável 'cont'
            if (req_recv != MPI_REQUEST_NULL) {
                MPI_Wait(&req_recv, &estado);
            }
            
            total += cont;
            dest = estado.MPI_SOURCE;
            
            if (inicio <= n) {
                if (m_send == '2') {
                    // Chamada apenas para garantir que o ticket de controle (request) será liberado
                    MPI_Wait(&reqs_tarefas[dest], MPI_STATUS_IGNORE);
                    
                    buffer_tarefas[dest] = inicio;
                    escolhe_send(m_send, &buffer_tarefas[dest], 1, MPI_INT, dest, 1, &reqs_tarefas[dest]);
                } else {
                    escolhe_send(m_send, &inicio, 1, MPI_INT, dest, 1, NULL);
                }
                inicio += TAMANHO;
            } else {
                if (m_send == '2') {
                    // Chamada apenas para garantir que o ticket de controle (request) será liberado
                    MPI_Wait(&reqs_tarefas[dest], MPI_STATUS_IGNORE);
                    
                    buffer_tarefas[dest] = inicio;
                    escolhe_send(m_send, &buffer_tarefas[dest], 1, MPI_INT, dest, 99, &reqs_tarefas[dest]);
                } else {
                    escolhe_send(m_send, &inicio, 1, MPI_INT, dest, 99, NULL);
                }
                stop++;
            }
        }
        // Limpeza dos arrays caso o ISend tenha sido utilizado
        if (m_send == '2') {
            free(reqs_tarefas);
            free(buffer_tarefas);
        }
    }       
    else { 
        // ================== CÓDIGO DO ESCRAVO ==================
        int cont_calculo;
        int cont_envio = 0; // Buffer seguro para envio
        MPI_Request req_envio = MPI_REQUEST_NULL;
        MPI_Request req_recv = MPI_REQUEST_NULL;
        
        while (1) {
            escolhe_receive(m_recv, &inicio, 1, MPI_INT, 0, MPI_ANY_TAG, &estado, &req_recv);
            
            // Escravo aguarda recebimento 
            if (req_recv != MPI_REQUEST_NULL) {
                MPI_Wait(&req_recv, &estado);
            }
            
            if (estado.MPI_TAG == 99) break;
            
            // Para não sobrescrever um possível envio antigo que ainda não terminou
            cont_calculo = 0;
            
            for (i = inicio; i < (inicio + TAMANHO) && i < n; i+=2) 
                if (primo(i)) cont_calculo++;
            
            // Garante que o envio antigo terminou
            if (req_envio != MPI_REQUEST_NULL) {
                MPI_Wait(&req_envio, MPI_STATUS_IGNORE);
            }
            
            // Copia o valor para o buffer que será enviado sem risco de comprometer um resultado anterior
            cont_envio = cont_calculo;
            escolhe_send(m_send, &cont_envio, 1, MPI_INT, 0, 1, &req_envio);
        }
        
        // Antes de encerrar, garante que a última mensagem de fato saiu
        if (req_envio != MPI_REQUEST_NULL) {
            MPI_Wait(&req_envio, MPI_STATUS_IGNORE);
        }
    }
    
    // ================== FINALIZAÇÃO ==================
	if (meu_ranque == 0) {
		t_final = MPI_Wtime();
        if (n >= 2)
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
