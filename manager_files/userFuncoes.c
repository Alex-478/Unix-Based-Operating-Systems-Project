#include "../util.h"
#include "manager_util.h"
//Adicionar User
int adicionar_user(const char* nome_user, int pid) {
    int fd_cli, res;
    char fifo[40];
    RESPOSTA r = {.type = 1};       
    printf("[DEBUG]--Resposta type inteiro: %d", r.type);  
    

    //verifica se o num de usarios atingio o MAX
    if (num_users >= MAX_USERS) {
        printf("[ERRO] Limite de usuários atingido.\n");
        return -1;
    }
    //Verifica se usuario já existe, se já alter o PID
    for (int i = 0; i < MAX_USERS; i++) {
        if (utilizadores[i].ativo && strcmp(utilizadores[i].nome, nome_user) == 0) {  //podemos verificar se esta ativo 
            utilizadores[i].pid = pid;                             //alterar PID                 
            printf("[INFO] Usuário com PID %d já está registrado.\n", pid);
            return -1;
        }
    }
    pthread_mutex_lock(&mutex_utilizadores);
    //Posso usar o valor de num_users diretamente
        strcpy(utilizadores[num_users].nome, nome_user);
            utilizadores[num_users].pid = pid;
            utilizadores[num_users].ativo = 1;
            num_users++;
    pthread_mutex_unlock(&mutex_utilizadores);
     //Adiciona a primeira vaga inativo       
    /*for (int i = 0; i < MAX_USERS; i++) { 
        if (!utilizadores[i].ativo) {
            strcpy(utilizadores[i].nome, nome_usuario);
            utilizadores[i].pid = pid;
            utilizadores[i].ativo = 1;
            num_users++;
            printf("[INFO] Usuário '%s' com PID %d adicionado.\n", nome_usuario, pid);
            //return 0;
        }
    } */

    for (int j = 0; j < MAX_USERS; j++){
                if (utilizadores[j].ativo){
                    sprintf(fifo, FIFO_CLI, utilizadores[j].pid);  // Formata o nome do pipe do cliente
                    fd_cli = open(fifo, O_WRONLY);             // Abre o pipe para o cliente
                    if (fd_cli != -1) {
                        snprintf(r.str, sizeof(r.str), "O usuário '%s' conectou.", nome_user);
                        res = write(fd_cli, &r, sizeof(RESPOSTA));  // Envia a mensagem
                        close(fd_cli);
                        printf("[INFO] (%d) Mensagem enviada para o %s: '%s'\n", res, utilizadores[j].nome, r.str);
                    } else {
                        printf("[ERRO] Não foi possível abrir o pipe para o %s.\n", utilizadores[j].nome);
                    }
                }
            }
    
    return 0;
}
//Listar Utilizadores
void listar_users() {
    printf("Usuários ativos:\n");
    for (int i = 0; i < MAX_USERS; i++) {
        if (utilizadores[i].ativo) {
            printf("Nome: %s -PID: %d\n", utilizadores[i].nome, utilizadores[i].pid);
        }
    }
    if (num_users == 0) {
        printf("[INFO] Nenhum usuário ativo.\n");
    }
return;
}
//Remover User
int remover_user(const char* nome_user) {
    int fd_cli, res;
    char fifo[40];
    RESPOSTA r = {.type = 1};       
    printf("[DEBUG]--Resposta type inteiro: %d", r.type);  


    //Verifica se esta registado
    for (int i = 0; i < num_users; i++) {
        if (utilizadores[i].ativo && strcmp(utilizadores[i].nome, nome_user) == 0) {
            //Enviar fim para terminar user
            sprintf(fifo, FIFO_CLI, utilizadores[i].pid);
            fd_cli = open(fifo, O_WRONLY);
            strcpy(r.str, "fim");
            res = write( fd_cli, &r, sizeof(RESPOSTA));
            close(fd_cli);
            //printf("ENVIEI... '%s' (%d)\n", r.str,res);
            printf("[INFO] Disconectar: '%s'\n", utilizadores[i].nome);  
            //remover User
            pthread_mutex_lock(&mutex_utilizadores); //mutex lock
            utilizadores[i].ativo = 0;
            utilizadores[i].pid = 0;
              
            printf("[INFO] Usuário '%s' removido.\n", nome_user);       
            
            for (int j = i; j < num_users - 1; j++) {
                utilizadores[j] = utilizadores[j + 1];
            }
            num_users--;  
            pthread_mutex_unlock(&mutex_utilizadores); // mutex unlock
            //Informar todos os users conectados sobre a desconexao
            for (int j = 0; j < num_users; j++){
                if (utilizadores[j].ativo){
                    sprintf(fifo, FIFO_CLI, utilizadores[j].pid);  // Formata o nome do pipe do cliente
                    fd_cli = open(fifo, O_WRONLY);             // Abre o pipe para o cliente
                    if (fd_cli != -1) {
                        snprintf(r.str, sizeof(r.str), "O usuário '%s' desconectou.", nome_user);
                        res = write(fd_cli, &r, sizeof(RESPOSTA));  // Envia a mensagem
                        close(fd_cli);
                        printf("[INFO] (%d) Mensagem enviada para o %s: '%s'\n", res, utilizadores[j].nome, r.str);
                    } else {
                        printf("[ERRO] Não foi possível abrir o pipe para o %s.\n", utilizadores[j].nome);
                    }
                }
            }
            return 0; 
        }
    }
    printf("[ERRO] Usuário '%s' não encontrado.\n", nome_user);
    return 0;
}