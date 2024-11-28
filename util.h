#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <string.h>

#define TAM 20
#define FIFO_SRV "/tmp/tubo"
#define FIFO_CLI "/tmp/f%d"
#define MAX_USERS 10

typedef struct {
    char nome[TAM]; //nome usuario quando executa o feed
    int pid;    // PID do usuário
    int ativo;  // Flag: 1 = ativo, 0 = não ativo
} USUARIO;

typedef struct{
        char str[TAM];
        USUARIO user;
}PEDIDO;

typedef struct{
        char str[TAM];
}RESPOSTA;


#define MAX_TOPICOS 20
#define MAX_MSG_PERSISTENTES 5
#define TAM_MSG 300


typedef struct {
    char corpo[TAM_MSG];
    int duracao;        // Em segundos
    time_t timestamp;  // Momento em que a mensagem foi enviada
} Mensagem;

typedef struct {
    char nome[TAM];                     // Nome do tópico
    Mensagem mensagens[MAX_MSG_PERSISTENTES];  // Mensagens persistentes
    int num_mensagens;                  // Quantidade de mensagens armazenadas
    int bloqueado;                      // 1 = bloqueado, 0 = desbloqueado
    int subscritores[10];               // PIDs dos usuários subscritos
    int num_subscritores;               // Número de subscritores
} Topico;
