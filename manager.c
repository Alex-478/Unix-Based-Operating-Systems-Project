#include "util.h"

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

    carregar_mensagens("msg.bin");

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
    pthread_create(&thread_id[2], NULL, monitorar_mensagens, (void *) &pdados[2]);

    pthread_join(thread_id[0], NULL); //Aguarda Thread admin
    pthread_join(thread_id[1], NULL); //Aguarda Thread Pipe
    pthread_join(thread_id[2], NULL); //Aguarda Thread Tempo

    armazena_mensagens("msg.bin");

    close (fd);
    unlink(FIFO_SRV);
    printf("FIM\n");
    exit(0);
}
//Funçoes------------------------------------------------

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
void *monitorar_mensagens(void *pdata) {
    TDATA *data = (TDATA *)pdata;
    while (*(data->pcontinuar)) {
        atualizar_mensagens();
        sleep(1); 
        //printf("[DEBUG]Atuliza Tempo mensagens\n");  
    }
    printf("[DEBUG]Termina thread tempo\n");  
    pthread_exit(NULL);
}

void *thread_le_pipe(void *pdata){
    TDATA *data = (TDATA *)pdata;
    int res;
    char fifo[40];
    PEDIDO p;

    do{
        printf("ADMIN> ");
        fflush(stdout);    

        res = read (*(data->pfd), &p, sizeof(PEDIDO) );
        if(res == sizeof(PEDIDO) && strcmp(p.str, "terminarLePipe") !=0 ){
            printf("RECEBI... '%s' - %d (%d)\n", p.str, p.user.pid, res);
            //Tratar mensagem Cliente
            processar_palavras_utilizador(p, fifo);
        }
        printf("[DEBUG] Ciclo thread pipe\n");

    }while (*(data->pcontinuar)); //espera pelo quit da thread admin

    printf("[DEBUG]Termina thread pipe\n");  
    pthread_exit(NULL);
}
void *thread_admin(void *pdata){
    TDATA *data = (TDATA *)pdata;
    char str[TAM];
    char fifo[40];
    PEDIDO p;
    int res;

    do{
        printf("ADMIN> ");
        fflush(stdout);

        fgets(str, sizeof(str), stdin);
        str[strcspn(str, "\n")] = '\0'; 

        processar_palavras_admin(str, fifo);
        //printf("[DEBUG]Ciclo thread admin.\n");    
    }while ( strcmp(str, "quit") !=0);

    *(data->pcontinuar) = 0;
    //atualiza o pipe para avançar a thread le pipe
    snprintf(p.str, sizeof(p.str), "terminarLePipe");
    res = write (*(data->pfd), &p,sizeof(PEDIDO)); //envia para o manager

    printf("[DEBUG] Termina thread admin\n");
    pthread_exit(NULL);
}
void processar_palavras_admin(char str[TAM], char fifo[40]){
    RESPOSTA r;
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
                if(tmpWords[1] == NULL) {return;;}
                remover_user(tmpWords[1]);
            }
            //comando lock "topico"
            if(strcmp(tmpWords[0], "lock") == 0){
                if(tmpWords[1] == NULL) {return;;}
                bloquear_topico(tmpWords[1]);
            }
            //comando unlock "topico"
            if(strcmp(tmpWords[0], "unlock") == 0){
                if(tmpWords[1] == NULL) {return;;}
                desbloquear_topico(tmpWords[1]);
            }
            //comando show "topico"
            if(strcmp(tmpWords[0], "show") == 0){
                if(tmpWords[1] == NULL) {return;;}
                listar_mensagens_topico(tmpWords[1]); //?? incompleto, receber diferentes tipos de dados
            }
            //comando quit -- termina todos os clientes
            if(strcmp(str, "quit") == 0){ 
                for(int i = 0; i<MAX_USERS; ++i){ 
                        if(utilizadores[i].ativo){
                            sprintf(fifo, FIFO_CLI, utilizadores[i].pid);
                            fd_cli = open(fifo, O_WRONLY);
                            strcpy(r.str, "fim");
                            res = write( fd_cli, &r, sizeof(RESPOSTA));
                            close(fd_cli);
                            //printf("ENVIEI... '%s' (%d)\n", r.str,res);
                            printf("[INFO] (%d) Mensagem enviada para o %s: '%s'\n", res, utilizadores[i].nome, r.str);
                        }
                    }
            }
            
    return;
}

void processar_palavras_utilizador(PEDIDO p, char fifo[40]){
    int fd_cli;
                   

    char comando_aux[300]; //so interesa as primeiras 3 palavras
    strncpy(comando_aux, p.str, sizeof(comando_aux) - 1); 
    comando_aux[sizeof(comando_aux) - 1] = '\0';

    char* palavras[4] = {NULL, NULL, NULL, NULL}; 
    //char* resto = NULL;                  
    printf("[DEBUG] Comando aux 1: %s\n", comando_aux);
    // Dividir em até 3 palavras
    palavras[0] = strtok(comando_aux, " "); //comando_aux fica so com a primeira palavra??
    palavras[1] = palavras[0] ? strtok(NULL, " ") : NULL;
    palavras[2] = palavras[1] ? strtok(NULL, " ") : NULL;
    palavras[3] = palavras[2] ? strtok(NULL, " ") : NULL;
    //resto = palavras[2] ? strtok(NULL, "") : NULL;
  //char* resto = NULL;                  
    printf("[DEBUG] Comando aux 2: %s\n", comando_aux);
    // Verifica o comando (primeira palavra)
    if (palavras[0] == NULL) {
        printf("[ERRO] Comando vazio.\n");
        return;
    }
                 //-------------------

                    if (strcmp(palavras[0], "msg") == 0) {
                        if(palavras[1] == NULL || palavras[2] == NULL || palavras[3] == NULL) {return;}
                        processar_messagem_utilizador(p.str);
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
                    if (strcmp(palavras[0], "subscribe") == 0) { //?? se o comando for subscribe sem 2ªpalavra da Falta de Segmentação
                        if(palavras[1] == NULL) {
                            printf("[DEBUG] Palavra 1: %s.\n", palavras[1]);
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
                    if(strcmp(palavras[0], "fim") == 0){
                        remover_user(p.user.nome);
                    }
                    return;
}
//procesar msg--
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

void listar_topicos_para_cliente(int fd_cliente) {
    char buffer[1024] = ""; 
    char estado[15];  
    RESPOSTA resposta;     

    if (num_topicos == 0) {
        //snprintf(buffer, sizeof(buffer), "[INFO] Não existem tópicos no momento.\n");
        //write(fd_cli, buffer, strlen(buffer)); // Envia a mensagem ao cliente

        snprintf(resposta.str, sizeof(resposta.str), "[INFO] Não existem tópicos no momento.\n");
        write(fd_cliente, &resposta, sizeof(RESPOSTA)); 
        return;
    }

    for (int i = 0; i < num_topicos; i++) {
        strcpy(estado, topicos[i].bloqueado ? "bloqueado" : "desbloqueado");

        // Concatena as informações no buffer
        /*snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                 "Tópico: %s | Mensagens: %d | Estado: %s\n",
                 topicos[i].nome, topicos[i].num_mensagens, estado);*/

        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                 "\nTópico: %s | Mensagens: %d | Estado: %s\n",
                 topicos[i].nome, topicos[i].num_mensagens, estado);


       
        if (strlen(buffer) >= sizeof(resposta.str) - 1 || i == num_topicos - 1) {
            strcpy(resposta.str, buffer);
            resposta.str[sizeof(resposta.str) - 1] = '\0';
            write(fd_cliente, &resposta, sizeof(RESPOSTA));
            buffer[0] = '\0'; // Limpa o buffer
        }
    }
    //write(fd_cli, buffer, strlen(buffer));
    write(fd_cliente, &resposta, sizeof(RESPOSTA));
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
void remove_subscricao_topico(const char* nome_topico, int pid_usuario) {
     char mensagem[200];    
    // Verifica se o tópico existe
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, nome_topico) == 0) {
            // Verifica se o usuário está subscrito
            for (int j = 0; j < topicos[i].num_subscritores; j++) {
                if (topicos[i].subscritores[j] == pid_usuario) {
                    // Remove o subscritor deslocando os elementos para preencher o espaço vazio
                    for (int k = j; k < topicos[i].num_subscritores - 1; k++) {
                        topicos[i].subscritores[k] = topicos[i].subscritores[k + 1];
                    }
                    
                    // Decrementa o número de subscritores
                    topicos[i].num_subscritores--;
                    printf("[INFO] Utilizador (PID: %d) removido do tópico '%s'.\n", pid_usuario, nome_topico);
                    snprintf(mensagem, sizeof(mensagem),  "[INFO] Subscrição removida do tópico '%s'.\n", nome_topico);
                    enviar_mensagem_cliente(pid_usuario, mensagem);
                    //Elimina topico se ja tiver sem users e msgs
                    eliminar_topico(nome_topico); 
                    return;
                }
            }

            // Se o usuário não está subscrito
            printf("[ERRO] Usuário (PID: %d) não está subscrito no tópico '%s'.\n", pid_usuario, nome_topico);
            return;
        }
    }
    printf("[ERRO] O tópico '%s' não existe.\n", nome_topico);
    return;
}
//Subscreve Topico
void subscreveTopico(const char* nome_topico, int pid_usuario){
    char mensagem[200];
    // Verifica se o tópico existe
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, nome_topico) == 0) {
            // Verifica se o usuário já está subscrito
            for (int j = 0; j < topicos[i].num_subscritores; j++) {
                if (topicos[i].subscritores[j] == pid_usuario) {
                    printf("[INFO] Usuário (PID: %d) já está subscrito no tópico '%s'.\n", pid_usuario, nome_topico);
                    snprintf(mensagem, sizeof(mensagem),  "[INFO] Já te encontras subscrito no tópico '%s'.\n", nome_topico);
                    enviar_mensagem_cliente(pid_usuario, mensagem);
                    return;
                }
            }

            // Verifica se há espaço para mais subscritores
            if (topicos[i].num_subscritores >= MAX_USERS) {
                printf("[ERRO] O tópico '%s' atingiu o limite de subscritores.\n", nome_topico);
                return; // Limite atingido
            }
            
            // Adiciona o usuário à lista de subscritores
            topicos[i].subscritores[topicos[i].num_subscritores] = pid_usuario;
            topicos[i].num_subscritores++;
            printf("[INFO] Usuário (PID: %d) subscrito ao tópico '%s' com sucesso.\n", pid_usuario, nome_topico);
            snprintf(mensagem, sizeof(mensagem), "[INFO] Subscrito ao tópico '%s' com sucesso.\n", nome_topico);
            enviar_mensagem_cliente(pid_usuario, mensagem);
            //Enviar mensagens Guardadas
            for(int k = 0; k < topicos[i].num_mensagens; k++ ){
                enviar_msg_subscritos(topicos[i].nome, topicos[i].mensagens[k].corpo);
            }

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
            printf("[ERRO] O tópico '%s' já existe.\n", nome);
            return;
        }
    }  
    //printf("[DEBUG] Variavel nome com: '%s'\n", nome);   
    strcpy(topicos[num_topicos].nome, nome);
    //printf("[DEBUG] Tópico '%s' criado com sucesso com num '%d'.\n", topicos[num_topicos].nome, num_topicos);
    topicos[num_topicos].num_mensagens = 0;
    topicos[num_topicos].bloqueado = 0;
    topicos[num_topicos].num_subscritores = 0;

    printf("[INFO] Tópico '%s' criado com sucesso.\n", topicos[num_topicos].nome);
    num_topicos++;
    
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
//Adicionar Usuario
int adicionar_user(const char* nome_usuario, int pid) {
    int fd_cli, res;
    char fifo[40];
    RESPOSTA r;


    //verifica se o num de usarios atingio o MAX
    if (num_users >= MAX_USERS) {
        printf("[ERRO] Limite de usuários atingido.\n");
        return -1;
    }
    //Verifica se usuario já existe, se já alter o PID
    for (int i = 0; i < MAX_USERS; i++) {
        if (utilizadores[i].ativo && strcmp(utilizadores[i].nome, nome_usuario) == 0) {  //podemos verificar se esta ativo 
            utilizadores[i].pid = pid;                                                  
            printf("[INFO] Usuário com PID %d já está registrado.\n", pid);
            return -1;
        }
    }

    //Posso usar o valor de num_users diretamente
        strcpy(utilizadores[num_users].nome, nome_usuario);
            utilizadores[num_users].pid = pid;
            utilizadores[num_users].ativo = 1;
            num_users++;

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
                        snprintf(r.str, sizeof(r.str), "O usuário '%s' conectou.", nome_usuario);
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
//Remover Usuario
int remover_user(const char* nome_usuario) {
    int fd_cli, res;
    char fifo[40];
    RESPOSTA r;

    //Verifica se esta registado
    for (int i = 0; i < num_users; i++) {
        if (utilizadores[i].ativo && strcmp(utilizadores[i].nome, nome_usuario) == 0) {
            //Enviar fim para terminar user
            sprintf(fifo, FIFO_CLI, utilizadores[i].pid);
            fd_cli = open(fifo, O_WRONLY);
            strcpy(r.str, "fim");
            res = write( fd_cli, &r, sizeof(RESPOSTA));
            close(fd_cli);
            //printf("ENVIEI... '%s' (%d)\n", r.str,res);
            printf("[INFO] Disconectar: '%s'\n", utilizadores[i].nome);  
            //remover usuario
            utilizadores[i].ativo = 0;
            utilizadores[i].pid = 0;
              
            printf("[INFO] Usuário '%s' removido.\n", nome_usuario);       
            
            for (int j = i; j < num_users - 1; j++) {
                utilizadores[j] = utilizadores[j + 1];
            }
            num_users--;  
            //Informar todos os users conectados sobre a desconexao
            for (int j = 0; j < num_users; j++){
                if (utilizadores[j].ativo){
                    sprintf(fifo, FIFO_CLI, utilizadores[j].pid);  // Formata o nome do pipe do cliente
                    fd_cli = open(fifo, O_WRONLY);             // Abre o pipe para o cliente
                    if (fd_cli != -1) {
                        snprintf(r.str, sizeof(r.str), "O usuário '%s' desconectou.", nome_usuario);
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
    printf("[ERRO] Usuário '%s' não encontrado.\n", nome_usuario);
    return 0;
}