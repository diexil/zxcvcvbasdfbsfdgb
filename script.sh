#!/bin/bash

# Limpa o terminal
clear

# Verifica se o argumento foi passado
if [ -z "$1" ]; then
    echo "Uso: ./script.sh <1 a 4>"
    exit 1
fi

x=$1

# Checa o valor de x
case $x in
    1)
        echo "Executando comandos para o jogador 1"
        ./exec 1 3050 3051
        ;;
    2)
        echo "Executando comandos para o jogador 2"
        ./exec 2 3051 3052
        ;;
    3)
        echo "Executando comandos para o jogador 3"
        ./exec 3 3052 3053
        ;;
    4)
        echo "Executando comandos para o jogador 4"
    	if gcc -O0 -g maquinas.c -o exec; then
        	./exec 4 3053 3050
    	else
        	echo "Erro: Falha na compilação. Corrija os erros no código."
    	fi
        ;;
    *)
        echo "Número inválido. Use um número de 1 a 4."
        exit 1
        ;;
esac

