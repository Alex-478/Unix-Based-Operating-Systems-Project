#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <string.h>

#define TAM 30
#define FIFO_SRV "/tmp/tubo"
#define FIFO_CLI "/tmp/f%d"
#define MAX_USERS 10
#define MAX_TOPICOS 20
#define MAX_MSG_PERSISTENTES 5
#define TAM_MSG 300

//Declarar Funçoes
void subscreveTopico(const char* nome_topico, int pid_usuario);
void criarTopico(const char* nome);
void listar_topicos();

int adicionar_usuario(const char* nome_usuario, int pid);
void listar_usuarios();
int remover_usuario(const char* nome_usuario);




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

typedef struct {
    char corpo[TAM_MSG];
    int duracao;        // Em segundos
    time_t timestamp;  // Momento em que a mensagem foi enviada
} MENSAGEM;

typedef struct {
    char nome[TAM];                     // Nome do tópico
    MENSAGEM mensagens[MAX_MSG_PERSISTENTES];  // Mensagens persistentes
    int num_mensagens;                  // Quantidade de mensagens armazenadas
    int bloqueado;                      // 1 = bloqueado, 0 = desbloqueado
    int subscritores[MAX_USERS];               // PIDs dos usuários subscritos
    int num_subscritores;               // Número de subscritores
} TOPICO;
