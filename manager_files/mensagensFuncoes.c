#include "../util.h"
#include "manager_util.h"
void carregar_mensagens(const char* nome_ficheiro) {
    FILE* ficheiro = fopen(nome_ficheiro, "rb");
    if (!ficheiro) {
        printf("[INFO] Ficheiro '%s' não encontrado. Nenhuma mensagem recuperada.\n", nome_ficheiro);
        return;
    }


    char linha[512];
    while (fgets(linha, sizeof(linha), ficheiro)) {
        TOPICO tAux;

        // Lê os dados formatados
        sscanf(linha, "%s %s %d %[^\n]", 
        tAux.nome,      
        tAux.mensagens[0].utilizador,  
        &tAux.mensagens[0].duracao,    
            tAux.mensagens[0].corpo);    

        //printf("[DEBUG]CARREGAR.MESAGEM: %s\n", tAux.mensagens[0].corpo);

        // Cria o tópico, se necessário
        criarTopico(tAux.nome);

        printf("[INFO] Mensagem recuperada do ficheiro [%s]%s: %s\n",
               tAux.nome, tAux.mensagens[0].utilizador, tAux.mensagens[0].corpo);

        // Adiciona a mensagem ao tópico
        guardar_mensagem(tAux.mensagens[0].utilizador, tAux.nome, tAux.mensagens[0].corpo, tAux.mensagens[0].duracao);

        
    }

    fclose(ficheiro);
    printf("[INFO] Mensagens persistentes carregadas de '%s'.\n", nome_ficheiro);
}

void armazena_mensagens(const char* nome_ficheiro) {
    FILE* ficheiro = fopen(nome_ficheiro, "wb");
    if (!ficheiro) {
        printf("[ERRO] Não foi possível abrir o ficheiro '%s' para escrita.\n", nome_ficheiro);
        return;
    }

    for (int i = 0; i < num_topicos; i++) {
        for (int j = 0; j < topicos[i].num_mensagens; j++) {
            
            int duracao;    
            // Preenche os dados
            time_t agora = time(NULL);
            duracao = topicos[i].mensagens[j].duracao - (agora - topicos[i].mensagens[j].timestamp);

            // Escreve em formato texto
            fprintf(ficheiro, "%s %s %d %s\n", topicos[i].nome,
                    topicos[i].mensagens[j].utilizador, duracao, 
                    topicos[i].mensagens[j].corpo);
        }
    }

    fclose(ficheiro);
    printf("[INFO] Mensagens armazenadas com sucesso: '%s'.\n", nome_ficheiro);
}

void atualizar_mensagens() {
    time_t agora = time(NULL); // Tempo atual

    // Iterar sobre todos os tópicos
    for (int i = 0; i < num_topicos; i++) {
        for (int j = 0; j < topicos[i].num_mensagens; j++) {
            // Verifica se a mensagem expirou
            if (agora - topicos[i].mensagens[j].timestamp >= topicos[i].mensagens[j].duracao) {
                printf("[INFO] Mensagem expirada no tópico '%s': %s\n", topicos[i].nome, topicos[i].mensagens[j].corpo);
                pthread_mutex_lock(&mutex_msg);
    
                // Remove a mensagem (movendo as seguintes para a esquerda)
                for (int k = j; k < topicos[i].num_mensagens - 1; k++) {
                    topicos[i].mensagens[k] = topicos[i].mensagens[k + 1];
                }

                topicos[i].num_mensagens--; // Decrementa o contador de mensagens
                j--; // Ajusta o índice para verificar a nova mensagem na posição
                pthread_mutex_unlock(&mutex_msg);
            }
        }
    }
  return;  
}

void guardar_mensagem(const char* nome_user, const char* topico, const char* mensagem, int duracao) {
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, topico) == 0) { 
            pthread_mutex_lock(&mutex_msg);

            // Verifica se o limite de msg
            if (topicos[i].num_mensagens >= 100) {
                printf("[ERRO] O tópico '%s' atingiu o limite de mensagens.\n", topico);
                return;
            }
            
            

            // Armazena 
            int idx = topicos[i].num_mensagens;
            strcpy(topicos[i].mensagens[idx].utilizador, nome_user);
            strncpy(topicos[i].mensagens[idx].corpo, mensagem, sizeof(topicos[i].mensagens[idx].corpo) - 1);
            topicos[i].mensagens[idx].corpo[sizeof(topicos[i].mensagens[idx].corpo) - 1] = '\0';
            topicos[i].mensagens[idx].duracao = duracao;
            topicos[i].mensagens[idx].timestamp = time(NULL); 
            topicos[i].num_mensagens++;

            printf("[INFO] Mensagem guardada no tópico [%s]: %s (Duração: %d segundos)\n", topico, mensagem, duracao);
            pthread_mutex_unlock(&mutex_msg);
            return;
        }
    }

    // Se o tópico não foi encontrado
    printf("[ERRO] O tópico '%s' não existe. Mensagem não armazenada.\n", topico);
}

void enviar_msg_subscritos(const char* nome_user, const char* topico, const char* mensagem) {
    char fifo[40];
    int fd;
    MSG_USER msg;
    
    
    
    strncpy(msg.corpo, mensagem, sizeof(msg.corpo) - 1);
    msg.corpo[sizeof(msg.corpo) - 1] = '\0'; 
    strcpy(msg.nome_topico, topico);
    strcpy(msg.utilizador, nome_user);
    
    MSGSTRUCT msgs;
    msgs.type = TIPO_MSG_USER;
    msgs.conteudo.msg_user = msg;

    // Percorre todos os tópicos para encontrar o correspondente
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, topico) == 0) { // Verifica se é o tópico correto
            // Enviar mensagem para todos os subscritores do tópico
            for (int j = 0; j < topicos[i].num_subscritores; j++) {
                int pid_user = topicos[i].subscritores[j];

                // Verifica se o User está ativo
                for (int k = 0; k < MAX_USERS; k++) {
                    if (utilizadores[k].ativo && utilizadores[k].pid == pid_user) {
                        // Abre o FIFO do cliente
                        snprintf(fifo, sizeof(fifo), FIFO_CLI, pid_user);
                        fd = open(fifo, O_WRONLY);
                        if (fd < 0) {
                            printf("[ERRO] Não foi possível enviar mensagem para %s.\n", utilizadores[k].nome);
                        }

                        // Escreve a mensagem no FIFO
                        if (write(fd, &msgs, sizeof(MSGSTRUCT)) < 0) {
                            printf("[ERRO] Falha ao enviar mensagem para %s.\n", utilizadores[k].nome);
                        } else {
                            printf("[INFO] Mensagem enviada para %s: %s\n", utilizadores[k].nome, msg.corpo);
                        }

                        // Fecha o FIFO
                        close(fd);
                    }
                }
            }
            return; // Sai da função após enviar para todos os subscritores
        }
    }
}

void processar_messagem_utilizador(PEDIDO p) {
    char comando_copia[TAM_MSG] ;
    strncpy(comando_copia, p.str, sizeof(comando_copia) - 1);
    comando_copia[sizeof(comando_copia) - 1] = '\0'; 
    char aviso[TAM_MSG];
    char* utilizador = p.user.nome;
    char* topico = NULL;       
    char* duracao_str = NULL;  
    char* mensagem = NULL;     

    // Captura o tópico
    strtok(comando_copia, " ");// ignora palavra 'msg'
    topico = strtok(NULL, " ");
    if (topico == NULL) {
        printf("[ERRO] Comando inválido. Tópico não especificado.\n");
        return;
    }
    // Captura a duração
    duracao_str = strtok(NULL, " ");
    if (duracao_str == NULL) {
        printf("[ERRO] Comando inválido. Duração não especificada.\n");
        return;
    }
    // Captura a mensagem (todo o restante da string)
    mensagem = strtok(NULL, "\0");
    if (mensagem == NULL) {
        printf("[ERRO] Comando inválido. Mensagem não especificada.\n");
        return;
    }

    //duração para inteiro
    int duracao = atoi(duracao_str);
    if (duracao < 0) {
        printf("[ERRO] Duração inválida.\n");
        return;
    }
    //Verificar se o topico existe
    if (verificar_topico(topico) == 0) {
        printf("[ERRO] O tópico '%s' não existe.\n", topico);
        return;
    }    


    printf("\n--[DEBUG]Mensagem Separada----\n");
    printf("Utilizador: %s\n", utilizador);
    printf("Tópico: %s\n", topico);
    printf("Duração: %d\n", duracao);
    printf("Mensagem: %s\n", mensagem);

    bool permitirMSG = false;
    //verifica subscricao e bloqueio
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, topico) == 0) {
            for (int j = 0; j < topicos[i].num_subscritores; j++) {
                if (topicos[i].subscritores[j] == p.user.pid) {  
                    if(topicos[i].bloqueado == 0)    
                        permitirMSG = true;  
                }
            }   
        }
    }

    

    //Envia para Utilizadores subscritos nos topicos
    if(permitirMSG){
        enviar_msg_subscritos(utilizador,topico, mensagem);
        //Guarda msg
        if(duracao != 0){
            guardar_mensagem(utilizador,topico, mensagem, duracao);
        }
    }
    if(!permitirMSG){
        snprintf(aviso, sizeof(aviso),  "[INFO] Não se encontra subscrito ou Topico '%s' bloqueado .\n", topico);
        enviar_resposta_cliente(p.user.pid, aviso);
    }


   return;     
}
void enviar_resposta_cliente(int pid, const char* mensagem) { // !! alterar nome
    char fifo[40];        
    int fd_cli;              
    MSGSTRUCT msgs;
    msgs.type = TIPO_RESPOSTA;
 
    sprintf(fifo, FIFO_CLI, pid);
    fd_cli = open(fifo, O_WRONLY);

    strncpy(msgs.conteudo.resposta.str, mensagem, sizeof(msgs.conteudo.resposta.str) -1 );
    msgs.conteudo.resposta.str[sizeof(msgs.conteudo.resposta.str) - 1] = '\0';
    
    if (write(fd_cli, &msgs, sizeof(MSGSTRUCT)) < 0) {
        printf("[ERRO] Falha ao enviar mensagem para o cliente com PID %d.\n", pid);
    } else {
        printf("[INFO] Mensagem enviada para o cliente com PID %d: %s\n", pid, mensagem);
    }
    close(fd_cli);
    return;
}