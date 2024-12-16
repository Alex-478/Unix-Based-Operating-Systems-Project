#include "../util.h"
#include "manager_util.h"

void listar_topicos_para_cliente(int fd_cliente) {
    char buffer[1024] = ""; 
    char estado[15];  
    MSGSTRUCT msgs;
    msgs.type = TIPO_RESPOSTA;
    

    if (num_topicos == 0) {
        snprintf(msgs.conteudo.resposta.str, sizeof(msgs.conteudo.resposta.str), "[INFO] Não existem tópicos no momento.\n");
        write(fd_cliente, &msgs, sizeof(MSGSTRUCT)); 
        return;
    }

    for (int i = 0; i < num_topicos; i++) {
        strcpy(estado, topicos[i].bloqueado ? "bloqueado" : "desbloqueado");

        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                 "\nTópico: %s | Mensagens: %d | Estado: %s\n",
                 topicos[i].nome, topicos[i].num_mensagens, estado);

        if (strlen(buffer) >= sizeof(msgs.conteudo.resposta.str) - 1 || i == num_topicos - 1) {
            strcpy(msgs.conteudo.resposta.str, buffer);
            msgs.conteudo.resposta.str[sizeof(msgs.conteudo.resposta.str) - 1] = '\0';
            write(fd_cliente, &msgs, sizeof(MSGSTRUCT));
            buffer[0] = '\0'; // Limpa o buffer
        }
    }
}
void listar_mensagens_topico(const char* nome_topico) {
    // Verifica se o tópico existe
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, nome_topico) == 0) {
            if (topicos[i].num_mensagens == 0) {
                printf("[INFO] O tópico '%s' não possui mensagens persistentes.\n", nome_topico);
                return;
            }
            // Lista as mensagens do tópico
            printf("=== Mensagens do Tópico '%s' ===\n", nome_topico);
            for (int j = 0; j < topicos[i].num_mensagens; j++) {
                printf("[%d] %s\n", j + 1, topicos[i].mensagens[j].corpo);
            }
            return;
        }
    }

    // Se o tópico não foi encontrado
    printf("[ERRO] O tópico '%s' não existe.\n", nome_topico);
}
//Desbloquear Topico
void desbloquear_topico(const char* nome_topico){
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, nome_topico) == 0) {
            if(!topicos[i].bloqueado){
                printf("[INFO] O tópico '%s' já se encontra desbloqueado.\n", nome_topico);
            }
            if(topicos[i].bloqueado){
                topicos[i].bloqueado = 0;
                printf("[INFO] O tópico '%s' foi desbloqueado.\n", nome_topico);
            }

        }
    }
    return;
}
//Bloquear Topico
void bloquear_topico(const char* nome_topico){
    int encontrado = 0;
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, nome_topico) == 0) {
            encontrado = 1;
            if(topicos[i].bloqueado){
                printf("[INFO] O tópico '%s' já se encontra bloqueado.\n", nome_topico);
            }
            if(!topicos[i].bloqueado){
                topicos[i].bloqueado = 1;
                printf("[INFO] O tópico '%s' foi bloqueado.\n", nome_topico);
            }

        }
    }
    if (!encontrado) {
        printf("[ERRO] O tópico '%s' não existe.\n", nome_topico);
    }
    return;
}
//Eliminar Topico Criado
void eliminar_topico(const char* nome_topico) {
   
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, nome_topico) == 0) {  // Verifica se o tópico existe
            
            // Verifica se há mensagens persistentes
            if (topicos[i].num_mensagens > 0) {  
                printf("[INFO] O tópico '%s' não pode ser removido porque contém mensagens persistentes.\n", nome_topico);
                return;
            }

            // Verifica se há subscritores
            if (topicos[i].num_subscritores > 0) {
                printf("[INFO] O tópico '%s' não pode ser removido porque ainda tem subscritores.\n", nome_topico);
                return;
            }

            // Desloca os topicos existentes
            for (int j = i; j < num_topicos - 1; j++) {
                topicos[j] = topicos[j + 1];
            }

            num_topicos--; 
            printf("[INFO] Tópico '%s' removido com sucesso.\n", nome_topico);
            return;
        }
    }

    printf("[ERRO] O tópico '%s' não existe.\n", nome_topico);
    return;
}
//remove subscrição Topico
void remove_subscricao_topico(const char* nome_topico, int pid_user) {
     //bloquear mutex
     char mensagem[200];    
    // Verifica se o tópico existe
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, nome_topico) == 0) {
            // Verifica se o User está subscrito
            for (int j = 0; j < topicos[i].num_subscritores; j++) {
                if (topicos[i].subscritores[j] == pid_user) {
                   pthread_mutex_lock(&mutex_topicos);
                    // Remove o subscritor deslocando os elementos para preencher o espaço vazio
                    for (int k = j; k < topicos[i].num_subscritores - 1; k++) {
                        topicos[i].subscritores[k] = topicos[i].subscritores[k + 1];
                    }
                    
                    // Decrementa o número de subscritores
                    topicos[i].num_subscritores--;
                    printf("[INFO] Utilizador (PID: %d) removido do tópico '%s'.\n", pid_user, nome_topico);
                    snprintf(mensagem, sizeof(mensagem),  "[INFO] Subscrição removida do tópico '%s'.\n", nome_topico);
                    enviar_resposta_cliente(pid_user, mensagem);
                    //Elimina topico se ja tiver sem users e msgs
                    eliminar_topico(nome_topico); 
                    pthread_mutex_unlock(&mutex_topicos);
                    return;
                }
            }
            snprintf(mensagem, sizeof(mensagem), "[INFO] Não se encontra subscrito no tópico '%s'.\n", nome_topico);
            enviar_resposta_cliente(pid_user, mensagem);
            // Se o User não está subscrito
            printf("[ERRO] User (PID: %d) não está subscrito no tópico '%s'.\n", pid_user, nome_topico);
            return;
        }
    }
    printf("[ERRO] O tópico '%s' não existe.\n", nome_topico);
    return;
}
//Subscreve Topico
void subscreveTopico(const char* nome_topico, int pid_user){
    char mensagem[200];
    MSGSTRUCT msgs;
    msgs.type = TIPO_MSG_USER;
    // Verifica se o tópico existe
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, nome_topico) == 0) {
            // Verifica se o User já está subscrito
            for (int j = 0; j < topicos[i].num_subscritores; j++) {
                if (topicos[i].subscritores[j] == pid_user) {
                    printf("[INFO] User (PID: %d) já está subscrito no tópico '%s'.\n", pid_user, nome_topico);
                    snprintf(mensagem, sizeof(mensagem),  "[INFO] Já te encontras subscrito no tópico '%s'.\n", nome_topico);
                    enviar_resposta_cliente(pid_user, mensagem);
                    return;
                }
            }

            // Verifica se há espaço para mais subscritores
            if (topicos[i].num_subscritores >= MAX_USERS) {
                printf("[ERRO] O tópico '%s' atingiu o limite de subscritores.\n", nome_topico);
                return; // Limite atingido
            }
            pthread_mutex_lock(&mutex_topicos);
            // Adiciona o User à lista de subscritores
            topicos[i].subscritores[topicos[i].num_subscritores] = pid_user;
            topicos[i].num_subscritores++;
            printf("[INFO] User (PID: %d) subscrito ao tópico '%s' com sucesso.\n", pid_user, nome_topico);
            snprintf(mensagem, sizeof(mensagem), "[INFO] Subscrito ao tópico '%s' com sucesso.", nome_topico);
            enviar_resposta_cliente(pid_user, mensagem);
            //Enviar mensagens Guardadas
            for(int k = 0; k < topicos[i].num_mensagens; k++ ){
                //enviar so para um user que subscreveu
                strcpy(msgs.conteudo.msg_user.nome_topico, topicos[i].nome);
                strcpy(msgs.conteudo.msg_user.utilizador, topicos[i].mensagens[k].utilizador);
                strcpy(msgs.conteudo.msg_user.corpo, topicos[i].mensagens[k].corpo);
                //printf("[DEBUG] MENSAGEM ANTES DE ENVIAR: %s \n", topicos[i].mensagens[k].corpo);
                char fifo[40];
                int fd;
                snprintf(fifo, sizeof(fifo), FIFO_CLI, pid_user);
                fd = open(fifo, O_WRONLY);
                write(fd, &msgs, sizeof(MSGSTRUCT));
                close(fd);
            }
            pthread_mutex_unlock(&mutex_topicos);
            return; // Sucesso
        }
    }
    // Se o tópico não foi encontrado
    printf("[ERRO] O tópico '%s' não existe.\n", nome_topico);
    return;
}
//Criar Topico
void criarTopico(const char* nome){
    if(num_topicos == MAX_TOPICOS){
         printf("[ERRO] Limite de tópicos atingido.\n");
            return;
    }
    // Verifica se o nome do tópico já existe ?? acho que nao ta funcional
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, nome) == 0) {
            printf("[INFO] O tópico '%s' já existe.\n", nome);
            return;
        }
    }  
    pthread_mutex_lock(&mutex_topicos); 
    strcpy(topicos[num_topicos].nome, nome);
    //printf("[DEBUG] Tópico '%s' criado com sucesso com num '%d'.\n", topicos[num_topicos].nome, num_topicos);
    topicos[num_topicos].num_mensagens = 0;
    topicos[num_topicos].bloqueado = 0;
    topicos[num_topicos].num_subscritores = 0;

    printf("[INFO] Tópico '%s' criado com sucesso.\n", topicos[num_topicos].nome);
    num_topicos++;
    pthread_mutex_unlock(&mutex_topicos);
    return;
}
int verificar_topico(const char* nome_topico) {
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, nome_topico) == 0) {
            return 1; 
        }
    }
    return 0; 
}
//Fazer Funçao Listar Topicos
void listar_topicos() {
    printf("=== Lista de Tópicos ===\n");
    if (num_topicos == 0) {
        printf("Nenhum tópico disponível.\n");
        return;
    }

    for (int i = 0; i < num_topicos; i++) {
        printf("Tópico: %s\n", topicos[i].nome);
        printf("  Mensagens armazenadas: %d\n", topicos[i].num_mensagens);
        printf("  Subscritores: %d\n", topicos[i].num_subscritores);
        printf("  Estado: %s\n", topicos[i].bloqueado ? "Bloqueado" : "Desbloqueado");
        printf("-----------------------\n");
    }
}

