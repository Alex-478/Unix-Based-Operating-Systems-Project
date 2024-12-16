#include "util.h"
#include "manager_files/manager_util.h"

UTILIZADOR utilizadores[MAX_USERS];
int num_users = 0;

TOPICO topicos[MAX_TOPICOS];
int num_topicos = 0;

pthread_mutex_t mutex_topicos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_msg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_utilizadores = PTHREAD_MUTEX_INITIALIZER;


int main(){
    TDATA pdados[3];
    pthread_t thread_id[3];
    int continuar = 1;
    int fd;

    if( access(FIFO_SRV, F_OK ) == 0){
        printf("[ERRO] Ja existe um servidor!\n");
        exit(3);
    }

    printf("[INFO] Manager Iniciado.\n");
    mkfifo(FIFO_SRV,0600);
    fd = open(FIFO_SRV,O_RDWR); // ?? posso passar isto para dentro da thread

    carregar_mensagens("manager_files/msg.txt");

    

    //Thread le os comandos admin
    pdados[0].pfd = &fd;
    pdados[0].pcontinuar = &continuar; //?? porque usar referencia
    pthread_create(&thread_id[0], NULL, thread_admin, (void *) &pdados[0]);

    //Thread le o pipe
    pdados[1].pfd = &fd;
    pdados[1].pcontinuar = &continuar;
    pthread_create(&thread_id[1], NULL, thread_le_pipe, (void *) &pdados[1] );

    //Contar o tempo
    pdados[2].pfd = &fd;
    pdados[2].pcontinuar = &continuar;
    pthread_create(&thread_id[2], NULL, thread_monitorar_mensagens, (void *) &pdados[2]); 

    pthread_join(thread_id[0], NULL); //Aguarda Thread admin
    pthread_join(thread_id[1], NULL); //Aguarda Thread Pipe
    pthread_join(thread_id[2], NULL); //Aguarda Thread Tempo

    armazena_mensagens("manager_files/msg.txt");

    close (fd);
    unlink(FIFO_SRV);
    printf("FIM\n");
    exit(0);
}
//Funçoes------------------------------------------------
//Thread verificar mensagens persistentes
void *thread_monitorar_mensagens(void *pdata) {
    TDATA *data = (TDATA *)pdata;
    while (*(data->pcontinuar)) {
        atualizar_mensagens();
        sleep(1); 
        //printf("[DEBUG]Atuliza Tempo mensagens\n");  
    }
    printf("[DEBUG]Termina thread tempo/mensagens[DEBUG]\n");  
    pthread_exit(NULL);
}
//Thread Espera comandos dos Utilizadores
void *thread_le_pipe(void *pdata){
    TDATA *data = (TDATA *)pdata;
    int res;
    char fifo[40];
    PEDIDO p;
    bool aux = false;


    do{ 
       if(aux){ 
        printf("ADMIN> ");
        fflush(stdout);   
        }
        aux = true;

        res = read (*(data->pfd), &p, sizeof(PEDIDO) );
        if(res == sizeof(PEDIDO) && strcmp(p.str, "terminarLePipe") !=0 ){
            printf("RECEBI... '%s' - %d (%d)\n", p.str, p.user.pid, res);
            //Tratar mensagem Cliente
            processar_palavras_utilizador(p, fifo);
        }

    }while (*(data->pcontinuar)); //espera pelo quit da thread admin

    printf("[DEBUG] Termina thread ler pipe [DEBUG]\n");  
    pthread_exit(NULL);
}
//Thread Espera comandos do admin
void *thread_admin(void *pdata){
    TDATA *data = (TDATA *)pdata;
    char str[TAM];
    char fifo[40];
    PEDIDO p;

    do{
        printf("ADMIN> ");
        fflush(stdout);

        if (fgets(str, sizeof(str), stdin)) {
            str[strcspn(str, "\n")] = '\0'; 
            if (strlen(str) > 0) { 
                processar_palavras_admin(str, fifo);
            }
        } else {
        printf("[ERRO] Problema ao ler entrada ou EOF atingido.\n");
        break; 
        }
    }while ( strcmp(str, "close") !=0);

    *(data->pcontinuar) = 0;
    //atualiza o pipe para avançar a thread le pipe
    snprintf(p.str, sizeof(p.str), "terminarLePipe");
    write (*(data->pfd), &p,sizeof(PEDIDO)); //envia para o manager

    printf("[DEBUG] Termina thread Admin-Teclado [DEBUG]\n");
    pthread_exit(NULL);
}
// Processo comnando admin
void processar_palavras_admin(char str[TAM], char fifo[40]){
    MSGSTRUCT msgs;
    msgs.type = TIPO_RESPOSTA;   
    int res, fd_cli;

            char* tmpWords[10]= {NULL}; 
            tmpWords[0] = strtok(str," ");
            tmpWords[1] = strtok(NULL," ");

            //comando users -- lista users
            if(strcmp(tmpWords[0], "users") == 0){
                listar_users();
            }
            //comando topics -- lista topicos
            if(strcmp(tmpWords[0], "topics") == 0){
                listar_topicos();
            }
            //comando remove "user"
            if(strcmp(tmpWords[0], "remove") == 0){
                if(tmpWords[1] == NULL) {return;}
                remover_user(tmpWords[1]);
            }
            //comando lock "topico"
            if(strcmp(tmpWords[0], "lock") == 0){
                if(tmpWords[1] == NULL) {return;}
                bloquear_topico(tmpWords[1]);
            }
            //comando unlock "topico"
            if(strcmp(tmpWords[0], "unlock") == 0){
                if(tmpWords[1] == NULL) {return;}
                desbloquear_topico(tmpWords[1]);
            }
            //comando show "topico"
            if(strcmp(tmpWords[0], "show") == 0){
                if(tmpWords[1] == NULL) {return;}
                listar_mensagens_topico(tmpWords[1]); //?? incompleto, receber diferentes tipos de dados
            }
            //comando quit -- termina todos os clientes
            if(strcmp(str, "close") == 0){ 
                for(int i = 0; i<MAX_USERS; ++i){ 
                        if(utilizadores[i].ativo){
                            sprintf(fifo, FIFO_CLI, utilizadores[i].pid);
                            fd_cli = open(fifo, O_WRONLY);
                            strcpy(msgs.conteudo.resposta.str, "Servidor Desligado\n");
                            res = write( fd_cli, &msgs, sizeof(MSGSTRUCT));
                            strcpy(msgs.conteudo.resposta.str, "exit");
                            res = write( fd_cli, &msgs, sizeof(MSGSTRUCT));
                            close(fd_cli);
                            //printf("ENVIEI... '%s' (%d)\n", r.str,res);
                            printf("[INFO] (%d) Mensagem enviada para o %s: '%s'\n", res, utilizadores[i].nome, msgs.conteudo.resposta.str);
                        }
                    }
            }
            
    return;
}
// Processa comando utilizador
void processar_palavras_utilizador(PEDIDO p, char fifo[40]){
    int fd_cli;
                   
    char comando_aux[300]; //so interesa as primeiras 3 palavras
    strncpy(comando_aux, p.str, sizeof(comando_aux) - 1); 
    comando_aux[sizeof(comando_aux) - 1] = '\0';

    char* palavras[4] = {NULL, NULL, NULL, NULL}; 
   
    palavras[0] = strtok(comando_aux, " "); //comando_aux fica so com a primeira palavra??
    palavras[1] = palavras[0] ? strtok(NULL, " ") : NULL;
    palavras[2] = palavras[1] ? strtok(NULL, " ") : NULL;
    palavras[3] = palavras[2] ? strtok(NULL, " ") : NULL;
   
    // Verifica o comando (primeira palavra)
    if (palavras[0] == NULL) {
        printf("[ERRO] Comando vazio.\n");
        return;
    }
                 //-------------------

                    if (strcmp(palavras[0], "msg") == 0) {
                        if(palavras[1] == NULL || palavras[2] == NULL || palavras[3] == NULL) {return;}
                        processar_messagem_utilizador(p);
                    }
                    if (strcmp(palavras[0], "topics") == 0) {
                        sprintf(fifo, FIFO_CLI, p.user.pid);
                        fd_cli = open(fifo, O_WRONLY);
                        listar_topicos_para_cliente(fd_cli);
                        close(fd_cli);
                        }
                    if (strcmp(palavras[0], "registar") == 0) {
                        adicionar_user(p.user.nome, p.user.pid);
                        }
                    if (strcmp(palavras[0], "subscribe") == 0) { 
                        if(palavras[1] == NULL) {
                            printf("[ERRO] Comando 'subscribe' requer um tópico.\n");
                            return;
                            }
                        criarTopico(palavras[1]);
                        subscreveTopico(palavras[1], p.user.pid);
                    }
                     if (strcmp(palavras[0], "unsubscribe") == 0) { 
                        if(palavras[1] == NULL) {
                            
                            printf("[ERRO] Comando 'unsubscribe' requer um tópico.\n");
                            return;
                            }
                        remove_subscricao_topico(palavras[1], p.user.pid);
                    }  
                    if(strcmp(palavras[0], "exit") == 0){
                        remover_user(p.user.nome);
                    }
                    return;
}
