#include "util.h"
#include "../util.h"
//Estruturas Manager 
typedef struct {
    int *pfd;
    int *pcontinuar; 
} TDATA;

typedef struct {
    char utilizador[TAM];
    char nome_topico[TAM];   
    char corpo[TAM_MSG];        
    int duracao;            
    time_t timestamp;       
} MENSAGEM_FICH;

typedef struct {
    char utilizador[TAM];
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


//Declarar Funçoes

//Mensagens
void carregar_mensagens(const char* nome_ficheiro);
void armazena_mensagens(const char* nome_ficheiro);
void atualizar_mensagens();
void guardar_mensagem(const char* nome_user, const char* topico, const char* mensagem, int duracao);
void enviar_msg_subscritos(const char* nome_user, const char* topico, const char* mensagem);
void processar_messagem_utilizador(PEDIDO p);
void enviar_resposta_cliente(int pid, const char* mensagem);

//Threads
void *thread_monitorar_mensagens(void *pdata);
void *thread_le_pipe(void *pdata);
void *thread_admin(void *pdata);

//Processar as palavras
void processar_palavras_admin(char str[TAM], char fifo[40]);
void processar_palavras_utilizador(PEDIDO p, char fifo[40]);

//Topicos
int verificar_topico(const char* nome_topico); //implementar esta funçao onde possivel
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

// Declaração das variáveis globais
extern UTILIZADOR utilizadores[MAX_USERS];
extern int num_users;
extern TOPICO topicos[MAX_TOPICOS];
extern int num_topicos;

extern pthread_mutex_t mutex_topicos; 
extern pthread_mutex_t mutex_utilizadores;
extern pthread_mutex_t mutex_msg;

//pthread_mutex_lock(&mutex_utilizadores);
//pthread_mutex_unlock(&mutex_topicos);