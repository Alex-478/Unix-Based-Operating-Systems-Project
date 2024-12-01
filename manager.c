#include "util.h"

//Globais
TOPICO topicos[MAX_TOPICOS]; 
int num_topicos = 0;

USUARIO usuarios[MAX_USERS];  // Array para armazenar usuários
int num_usuarios = 0;  //posso utilizar uma static na estrutura??

int main(){
    char str[TAM], fifo[40];
    int fd, res, fd_cli, n;
    PEDIDO p;
    RESPOSTA r;
    fd_set fds;
    struct timeval tempo;


    if( access(FIFO_SRV, F_OK ) == 0){
        printf("[ERRO] Ja existe um servidor!\n");
        exit(3);
    }

    printf("INICIO...\n");
    mkfifo(FIFO_SRV,0600);
    fd = open(FIFO_SRV,O_RDWR);
do{
    printf("ADMIN> ");
    fflush(stdout);

    //seletc();
    FD_ZERO(&fds);
    FD_SET(0, &fds);   //stdin teclado
    FD_SET(fd,&fds);  // npipe(fd)
    tempo.tv_sec = 30;
    tempo.tv_usec = 0;

    n = select( fd + 1, &fds, NULL, NULL, &tempo);
    if(n > 0){  //ha dados... onde?
        if(FD_ISSET(0, &fds)){  //stdin teclado
            scanf("%s", str);
            printf("Comando '%s' introduzido...\n", str );

            //comando users -- lista users
            if(strcmp(str, "users") == 0){
                listar_usuarios();
            }
            
            //comando topics -- lista topicos
            if(strcmp(str, "topics") == 0){
                 listar_topicos();
            }

            if(strcmp(str, "remove") == 0){
                 //remover utilizador pedido.
            }
            //comando quit -- termina todos os clientes
            if(strcmp(str, "quit") == 0){ 
                for(int i = 0; i<MAX_USERS; ++i){ 
                        if(usuarios[i].ativo){
                            sprintf(fifo, FIFO_CLI, usuarios[i].pid);
                            fd_cli = open(fifo, O_WRONLY);
                            strcpy(r.str, "fim");
                            res = write( fd_cli, &r, sizeof(RESPOSTA));
                            close(fd_cli);
                            printf("ENVIEI... '%s' (%d)\n", r.str,res);
                        }
                    }
            }
        }
        else if(FD_ISSET(fd, &fds)){  //pide fd
                res = read (fd, &p, sizeof(PEDIDO) );
                if(res == sizeof(PEDIDO) ){
                    printf("RECEBI... '%s' - %d (%d)\n", p.str, p.user.pid, res);
                    
                    //Tratar mensagem Cliente
                    
                    char* tmpTopico[10];
                    tmpTopico[0] = strtok(p.str," ");
                    tmpTopico[1] = strtok(NULL," ");

                    if (strcmp(tmpTopico[0], "registar") == 0) {
                        adicionar_usuario(p.user.nome, p.user.pid);
                        }

                    //se o comando for subscribe-INCOMPLETO
                    if (strcmp(tmpTopico[0], "subscribe") == 0) { //se o comando for subscribe
                        criarTopico(tmpTopico[1]);
                        subscreveTopico(tmpTopico[1], p.user.pid);
                    }

                    //Remover se receber fim do user    
                    if(strcmp(tmpTopico[0], "fim") == 0){
                        remover_usuario(p.user.nome);
                    }
                    /*else{  //envia para todos os clientes o que tiver recebido de um cliente
                        for(int i = 0; i<num_usuarios; ++i){ 
                            if(usuarios[i].ativo){
                                sprintf(fifo, FIFO_CLI, usuarios[i].pid);
                                fd_cli = open(fifo, O_WRONLY);
                                strcpy(r.str, p.str);
                                res = write( fd_cli, &r, sizeof(RESPOSTA));  // res=-1 ??
                                close(fd_cli);
                                printf("ENVIEI... '%s' (%d)\n", r.str,res);
                    
                            }
                        }
                    }*/
                }
            }
    }    
}while ( strcmp(str, "quit") !=0);

    close (fd);
    unlink(FIFO_SRV);
    printf("FIM\n");
    exit(0);
}


//Subscreve Topico
void subscreveTopico(const char* nome_topico, int pid_usuario){
    // Verifica se o tópico existe
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, nome_topico) == 0) {
            // Verifica se o usuário já está subscrito
            for (int j = 0; j < topicos[i].num_subscritores; j++) {
                if (topicos[i].subscritores[j] == pid_usuario) {
                    printf("[INFO] Usuário (PID: %d) já está subscrito no tópico '%s'.\n", pid_usuario, nome_topico);
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
int adicionar_usuario(const char* nome_usuario, int pid) {
    int fd_cli, res;
    char fifo[40];
    RESPOSTA r;


    //verifica se o num de usarios atingio o MAX
    if (num_usuarios >= MAX_USERS) {
        printf("[ERRO] Limite de usuários atingido.\n");
        return -1;
    }
    //Verifica se usuario já existe, se já alter o PID
    for (int i = 0; i < MAX_USERS; i++) {
        if (usuarios[i].ativo && strcmp(usuarios[i].nome, nome_usuario) == 0) {  //podemos verificar se esta ativo 
            usuarios[i].pid = pid;                                                  
            printf("[INFO] Usuário com PID %d já está registrado.\n", pid);
            return -1;
        }
    }

    //Posso usar o valor de num_usuarios diretamente
        strcpy(usuarios[num_usuarios].nome, nome_usuario);
            usuarios[num_usuarios].pid = pid;
            usuarios[num_usuarios].ativo = 1;
            num_usuarios++;

     //Adiciona a primeira vaga inativo       
    /*for (int i = 0; i < MAX_USERS; i++) { 
        if (!usuarios[i].ativo) {
            strcpy(usuarios[i].nome, nome_usuario);
            usuarios[i].pid = pid;
            usuarios[i].ativo = 1;
            num_usuarios++;
            printf("[INFO] Usuário '%s' com PID %d adicionado.\n", nome_usuario, pid);
            //return 0;
        }
    } */

    for (int j = 0; j < num_usuarios; j++){
                if (usuarios[j].ativo){
                    sprintf(fifo, FIFO_CLI, usuarios[j].pid);  // Formata o nome do pipe do cliente
                    fd_cli = open(fifo, O_WRONLY);             // Abre o pipe para o cliente
                    if (fd_cli != -1) {
                        snprintf(r.str, sizeof(r.str), "O usuário '%s' conectou.", nome_usuario);
                        res = write(fd_cli, &r, sizeof(RESPOSTA));  // Envia a mensagem
                        close(fd_cli);
                        printf("[INFO] (%d) Mensagem enviada para o %s: '%s'\n", res, usuarios[j].nome, r.str);
                    } else {
                        printf("[ERRO] Não foi possível abrir o pipe para o %s.\n", usuarios[j].nome);
                    }
                }
            }

    return 0;
}
//Listar Usuarios
void listar_usuarios() {
    printf("Usuários ativos:\n");
    for (int i = 0; i < MAX_USERS; i++) {
        if (usuarios[i].ativo) {
            printf("Nome: %s -PID: %d\n", usuarios[i].nome, usuarios[i].pid);
        }
    }
    if (num_usuarios == 0) {
        printf("[INFO] Nenhum usuário ativo.\n");
    }
}
//Remover Usuario
int remover_usuario(const char* nome_usuario) {
    int fd_cli, res;
    char fifo[40];
    RESPOSTA r;
    //Verifica se esta registado
    for (int i = 0; i < num_usuarios; i++) {
        if (usuarios[i].ativo && strcmp(usuarios[i].nome, nome_usuario) == 0) {
            usuarios[i].ativo = 0;
            usuarios[i].pid = 0;
            num_usuarios--;  
            printf("[INFO] Usuário '%s' removido.\n", nome_usuario);


            //Informar todos os users conectados sobre a desconexao
            for (int j = 0; j < num_usuarios; j++){
                if (usuarios[j].ativo){
                    sprintf(fifo, FIFO_CLI, usuarios[j].pid);  // Formata o nome do pipe do cliente
                    fd_cli = open(fifo, O_WRONLY);             // Abre o pipe para o cliente
                    if (fd_cli != -1) {
                        snprintf(r.str, sizeof(r.str), "O usuário '%s' desconectou.", nome_usuario);
                        res = write(fd_cli, &r, sizeof(RESPOSTA));  // Envia a mensagem
                        close(fd_cli);
                        printf("[INFO] (%d) Mensagem enviada para o %s: '%s'\n", res, usuarios[j].nome, r.str);
                    } else {
                        printf("[ERRO] Não foi possível abrir o pipe para o %s.\n", usuarios[j].nome);
                    }
                }
            }

            return 0; 
        }
    }

    
    printf("[ERRO] Usuário '%s' não encontrado.\n", nome_usuario);
    return -1; 
}