#!/bin/bash

LIMITE=500000000
PROGRAMA="./primos_naive"
ARQUIVO_SAIDA="resultados_naive.csv"

echo "modo_send,tempo_em_segundos" > $ARQUIVO_SAIDA 

mpicc -o primos_naive primos_naive.c -lm

echo "Programa Compilado"
echo "----------------------"
echo "Iniciando os testes"

for SEND_MODE in 1 2 3 4 5
do  
    for RECV_MODE in 1
    do
        echo "Rodando com modo Send-$SEND_MODE e Recv-$RECV_MODE"
        SAIDA=$(mpirun -n 4 --oversubscribe $PROGRAMA $LIMITE $SEND_MODE $RECV_MODE)

        echo "$SAIDA"
        
        TEMPO=$(echo "$SAIDA" | grep "Tempo de execucao:" | awk '{print $4}')
        echo "$SEND_MODE,$TEMPO" >> $ARQUIVO_SAIDA
        echo "--------------------------------------"
    done
done

echo "Fim dos testes"