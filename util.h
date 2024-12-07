#ifndef UTIL_H
#define UTIL_H

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
#define TYPE_RESPOSTA = 1;

typedef struct {
    int *pfd;
    int *pcontinuar; 
} TDATA;

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
        const int type; // type 1
        char str[TAM_MSG]; //TAM
}RESPOSTA;

typedef struct{
    const int type; // type 2
    char nome_topico[TAM]; 
    char utilizador[TAM];
    char corpo[TAM_MSG];
}MSG_USER;

typedef struct {
    char utilizador[TAM];
    char corpo[TAM_MSG];
    int duracao;        // Em segundos
    time_t timestamp;  // Momento em que a mensagem foi enviada
} MENSAGEM;

typedef struct {
    char utilizador[TAM];
    char nome_topico[TAM];   
    char corpo[TAM_MSG];        
    int duracao;            
    time_t timestamp;       
} MENSAGEM_FICH;

typedef struct {
    char nome[TAM];                     // Nome do tópico
    MENSAGEM mensagens[MAX_MSG_PERSISTENTES];  // Mensagens persistentes
    int num_mensagens;                  // Quantidade de mensagens armazenadas
    int bloqueado;                      // 1 = bloqueado, 0 = desbloqueado
    int subscritores[MAX_USERS];        // PIDs dos usuários subscritos
    int num_subscritores;               // Número de subscritores
} TOPICO;



#endif

