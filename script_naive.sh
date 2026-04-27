#!/bin/bash

set -euo pipefail

LIMITE=500000000
PROGRAMA="./primos_naive"
ARQUIVO_SAIDA="resultados_naive_completo.csv"
TEMPO_1_PROC=676.828
NUM_PROCS=4

echo "modo_send,modo_recv,tempo_em_segundos,speedup,eficiencia" > "$ARQUIVO_SAIDA"

if ! mpicc -o primos_naive primos_naive.c -lm; then
    echo "Erro na compilação do primos_naive.c"
    exit 1
fi

echo "Programa Compilado"
echo "----------------------"
echo "Iniciando os testes"

for SEND_MODE in 1 2 3 4 5
do
    for RECV_MODE in 1 2
    do
        echo "Rodando com modo Send-$SEND_MODE e Recv-$RECV_MODE"

        SAIDA=$(mpirun -n "$NUM_PROCS" --oversubscribe "$PROGRAMA" "$LIMITE" "$SEND_MODE" "$RECV_MODE")

        echo "$SAIDA"

        TEMPO=$(echo "$SAIDA" | grep "Tempo de execucao:" | awk '{print $4}' || true)

        if [[ -n "$TEMPO" ]]; then
            SPEEDUP=$(awk "BEGIN {printf \"%.3f\", $TEMPO_1_PROC / $TEMPO}")
            EFICIENCIA=$(awk "BEGIN {printf \"%.3f\", $SPEEDUP / $NUM_PROCS}")
        else
            TEMPO="-"
            SPEEDUP="-"
            EFICIENCIA="-"
        fi

        echo "$SEND_MODE,$RECV_MODE,$TEMPO,$SPEEDUP,$EFICIENCIA" >> "$ARQUIVO_SAIDA"
        echo "--------------------------------------"
    done
done

echo "Fim dos testes"