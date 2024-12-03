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
        char str[1024]; //TAM
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
    int subscritores[MAX_USERS];        // PIDs dos usuários subscritos
    int num_subscritores;               // Número de subscritores
} TOPICO;

//Globais
TOPICO topicos[MAX_TOPICOS]; 
int num_topicos = 0;

UTILIZADOR utilizadores[MAX_USERS];  // Array para armazenar usuários
int num_users = 0;  //posso utilizar uma static na estrutura??



//Declarar Funçoes

//Mensagens
void enviar_mensagem_cliente(int pid, const char* mensagem);

//Threads
void *thread_le_pipe(void *pdata);
void *thread_admin(void *pdata);

//Processar as palavras
void processar_palavras_admin(char str[TAM], char fifo[40]);
void processar_palavras_utilizador(PEDIDO p, char fifo[40]);

//Topicos
void listar_topicos_para_cliente(int fd_cliente); //imcompleto?? Diferentes tamanho de msg
void listar_mensagens_topico(const char* nome_topico);
void bloquear_topico(const char* nome_topico);
void desbloquear_topico(const char* nome_topico);
void eliminar_topico(const char* nome_topico);
void remove_subscricao_topico(const char* nome_topico, int pid_usuario);
void subscreveTopico(const char* nome_topico, int pid_usuario);
void criarTopico(const char* nome);
void listar_topicos();

//USERS
int adicionar_user(const char* nome_usuario, int pid);
void listar_users();
int remover_user(const char* nome_usuario);