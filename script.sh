#!/bin/bash

# LIMITE = 500000000
LIMITE=10000
PROGRAMA="./main"
ARQUIVO_SAIDA="resultados_bag.csv"

echo "modo_send,tempo_em_segundos" > $ARQUIVO_SAIDA 

mpicc -o main main.c -lm

echo "Programa Compilado"
echo "----------------------"
echo "Iniciando os testes"

for SEND_MODE in 1 2 4 5
do  
    for RECV_MODE in 1 2
    do
        echo "Rodando com modo Send-$SEND_MODE e Recv-$RECV_MODE"
        SAIDA=$(mpirun -n 4 --oversubscribe $PROGRAMA $LIMITE $SEND_MODE $RECV_MODE)
        
        TEMPO=$(echo "$SAIDA" | grep "Tempo de execucao:" | awk '{print $4}')
        echo "$SEND_MODE,$TEMPO" >> $ARQUIVO_SAIDA
        echo "--------------------------------------"
    done
done

echo "Fim dos testes"