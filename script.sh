#!/bin/bash

# Configurações
LIMITE=500000000
PROGRAMA="./primos_bag"
ARQUIVO_SAIDA="resultados_bag_irecv.csv"
TEMPO_1_PROC=676.828
NUM_PROCS=4

echo "modo_send,modo_recv,tempo_em_segundos,speedup,eficiencia" > $ARQUIVO_SAIDA

mpicc -o primos_bag primos_bag.c -lm

echo "Programa Compilado"
echo "----------------------"
echo "Iniciando os testes"

for SEND_MODE in 1 2 4 5
do  
    for RECV_MODE in 2
    do
        echo "Rodando com modo Send-$SEND_MODE e Recv-$RECV_MODE"
        SAIDA=$(mpirun -n 4 --oversubscribe $PROGRAMA $LIMITE $SEND_MODE $RECV_MODE)

        echo "$SAIDA"
        
        TEMPO=$(echo "$SAIDA" | grep "Tempo de execucao:" | awk '{print $4}')
        
        # Só calcula se o valor de TEMPO for válido/existir
        if [[ ! -z "$TEMPO" ]]; then
            SPEEDUP=$(awk "BEGIN {printf \"%.3f\", $TEMPO_1_PROC / $TEMPO}")
            EFICIENCIA=$(awk "BEGIN {printf \"%.3f\", $SPEEDUP / $NUM_PROCS}")
        else
            SPEEDUP="-"
            EFICIENCIA="-"
        fi
        
        echo "$SEND_MODE,$RECV_MODE,$TEMPO,$SPEEDUP,$EFICIENCIA" >> $ARQUIVO_SAIDA
        echo "--------------------------------------"
    done
done

echo "Fim dos testes"