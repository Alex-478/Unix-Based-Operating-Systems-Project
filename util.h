#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <string.h>
#include <pthread.h>

#define TAM 30
#define FIFO_SRV "/tmp/tubo"
#define FIFO_CLI "/tmp/f%d"
#define MAX_USERS 10
#define MAX_TOPICOS 20
#define MAX_MSG_PERSISTENTES 5
#define TAM_MSG 300
#define TYPE_RESPOSTA 1
#define TIPO_RESPOSTA 1
#define TIPO_MSG_USER 2


typedef struct {
    char nome[TAM]; //nome usuario quando executa o feed
    int pid;    // PID do usuário
    int ativo;  // Flag: 1 = ativo, 0 = não ativo
} UTILIZADOR;

typedef struct{
        char str[TAM_MSG];
        UTILIZADOR user;
}PEDIDO;

typedef struct{
        //const int type; // type 1
        char str[TAM_MSG]; //TAM
}RESPOSTA;

typedef struct{
    //const int type; // type 2
    char nome_topico[TAM]; 
    char utilizador[TAM];
    char corpo[TAM_MSG];
}MSG_USER;

typedef struct{
    int type;
    union{
        RESPOSTA resposta;
        MSG_USER msg_user;
    } conteudo;
}MSGSTRUCT;


#endif

