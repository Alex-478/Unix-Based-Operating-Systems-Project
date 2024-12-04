#include "../util.h"
#include "manager_util.h"
void carregar_mensagens(const char* nome_ficheiro) {
    FILE* ficheiro = fopen(nome_ficheiro, "rb");
    if (!ficheiro) {
        printf("[INFO] Ficheiro '%s' não encontrado. Nenhuma mensagem recuperada.\n", nome_ficheiro);
        return;
    }

    MENSAGEM_FICH msg_fich;
    while (fread(&msg_fich, sizeof(MENSAGEM_FICH), 1, ficheiro) == 1) {
        // Cria o tópico 
        criarTopico(msg_fich.nome_topico);
        // Adiciona a mensagem ao tópico
        guardar_mensagem(msg_fich.nome_topico, msg_fich.corpo, msg_fich.duracao);
        printf("[INFO] Mensagem recuperada no tópico '%s': %s\n", msg_fich.nome_topico, msg_fich.corpo);
        
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
            MENSAGEM_FICH msg_fich;

            //Preenche os dados
            strncpy(msg_fich.nome_topico, topicos[i].nome, sizeof(msg_fich.nome_topico) - 1);
            msg_fich.nome_topico[sizeof(msg_fich.nome_topico) - 1] = '\0';

            strncpy(msg_fich.corpo, topicos[i].mensagens[j].corpo, sizeof(msg_fich.corpo) - 1);
            msg_fich.corpo[sizeof(msg_fich.corpo) - 1] = '\0';

            time_t agora = time(NULL);
            msg_fich.duracao = topicos[i].mensagens[j].duracao;
            msg_fich.duracao = msg_fich.duracao - (agora - topicos[i].mensagens[j].timestamp);

            //msg_fich.duracao = topicos[i].mensagens[j].duracao;
            //msg_fich.timestamp = topicos[i].mensagens[j].timestamp;

            //Escreve no ficheiro
            fwrite(&msg_fich, sizeof(MENSAGEM_FICH), 1, ficheiro);

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

                // Remove a mensagem (movendo as seguintes para a esquerda)
                for (int k = j; k < topicos[i].num_mensagens - 1; k++) {
                    topicos[i].mensagens[k] = topicos[i].mensagens[k + 1];
                }

                topicos[i].num_mensagens--; // Decrementa o contador de mensagens
                j--; // Reajusta o índice para verificar a nova mensagem na posição
            }
        }
    }
}

void guardar_mensagem(const char* topico, const char* mensagem, int duracao) {
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, topico) == 0) { 
            // Verifica se o limite de msg
            if (topicos[i].num_mensagens >= 100) {
                printf("[ERRO] O tópico '%s' atingiu o limite de mensagens.\n", topico);
                return;
            }

            // Armazena 
            int idx = topicos[i].num_mensagens;
            strncpy(topicos[i].mensagens[idx].corpo, mensagem, sizeof(topicos[i].mensagens[idx].corpo) - 1);
            topicos[i].mensagens[idx].corpo[sizeof(topicos[i].mensagens[idx].corpo) - 1] = '\0';
            topicos[i].mensagens[idx].duracao = duracao;

            topicos[i].mensagens[idx].timestamp = time(NULL); 
            topicos[i].num_mensagens++;

            printf("[INFO] Mensagem armazenada no tópico '%s': %s (Duração: %d segundos)\n", topico, mensagem, duracao);
            return;
        }
    }

    // Se o tópico não foi encontrado
    printf("[ERRO] O tópico '%s' não existe. Mensagem não armazenada.\n", topico);
}

void enviar_msg_subscritos(const char* topico, const char* mensagem) {
    char fifo[40];
    MENSAGEM msg;
    int fd;

    // Formata a mensagem para incluir o tópico
    snprintf(msg.corpo, sizeof(msg.corpo), "[%s]: %s", topico, mensagem);
    // Percorre todos os tópicos para encontrar o correspondente
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, topico) == 0) { // Verifica se é o tópico correto
            // Enviar mensagem para todos os subscritores do tópico
            for (int j = 0; j < topicos[i].num_subscritores; j++) {
                int pid_usuario = topicos[i].subscritores[j];

                // Verifica se o usuário está ativo
                for (int k = 0; k < MAX_USERS; k++) {
                    if (utilizadores[k].ativo && utilizadores[k].pid == pid_usuario) {
                        // Abre o FIFO do cliente
                        snprintf(fifo, sizeof(fifo), FIFO_CLI, pid_usuario);
                        fd = open(fifo, O_WRONLY);
                        if (fd < 0) {
                            printf("[ERRO] Não foi possível enviar mensagem para %s.\n", utilizadores[k].nome);
                        }

                        // Escreve a mensagem no FIFO
                        if (write(fd, &msg, sizeof(MENSAGEM)) < 0) {
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

void processar_messagem_utilizador(char* comando) {
    char comando_copia[TAM_MSG] ;
    strncpy(comando_copia, comando, sizeof(comando_copia) - 1);
    comando_copia[sizeof(comando_copia) - 1] = '\0'; 

    char* topico = NULL;       
    char* duracao_str = NULL;  
    char* mensagem = NULL;     

     printf("---[DEBUG]Mensagem Recebida---- %s\n", topico);
    printf("MSG: %s\n", comando_copia);

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
    printf("Tópico: %s\n", topico);
    printf("Duração: %d\n", duracao);
    printf("Mensagem: %s\n", mensagem);

    //Envia para Utilizadores subscritos nos topicos
    enviar_msg_subscritos(topico, mensagem);
    
    //Guarda msg
    if(duracao != 0){
        guardar_mensagem(topico, mensagem, duracao);
    }

   return;     
}
void enviar_mensagem_cliente(int pid, const char* mensagem) { // !! alterar nome
    char fifo[40];        
    int fd_cli;              
    RESPOSTA resposta;       

    sprintf(fifo, FIFO_CLI, pid);
    fd_cli = open(fifo, O_WRONLY);
    snprintf(resposta.str, sizeof(resposta.str), "%s", mensagem);

    if (write(fd_cli, &resposta, sizeof(RESPOSTA)) < 0) {
        printf("[ERRO] Falha ao enviar mensagem para o cliente com PID %d.\n", pid);
    } else {
        printf("[INFO] Mensagem enviada para o cliente com PID %d: %s\n", pid, mensagem);
    }
    close(fd_cli);
    return;
}