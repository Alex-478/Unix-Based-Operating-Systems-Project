#include "util.h"

//Globais
Topico topicos[MAX_TOPICOS]; 
int num_topicos = 0;

USUARIO usuarios[MAX_USERS];  // Array para armazenar usuários
int num_usuarios = 0;  //posso utilizar uma static na estrutura??

//Declarar Funçoes
void criarTopico(const char* nome);

int adicionar_usuario(const char* nome_usuario, int pid);
void listar_usuarios();
int remover_usuario(const char* nome_usuario, int pid);



int main(int agrc, char *argv[]){
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
                 printf("Ainda nao implementado");
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
                    //adicionar users ativos
                    //usar flag para so adocionar quando conecta?? S
                    //Fazer funçao ou ficheirasado para tratar comandos
                    adicionar_usuario(p.user.nome, p.user.pid);

                    char tmpTopico[10];
                    tmpTopico[0] = strtok(p.str," ")   
                    tmpTopico[1] = strtok(NULL," ");

                    //se o comando for subscribe-INCOMPLETO
                    if (strcmp(tmpTopico[0], "subscribe") == 0) { //se o comando for subscribe
                      
                      criarTopico(tmpTopico[1]);
                      //adicionar ao utilizador.
                       
                       printf("Subscrito no topico: '%s'\n", tmpTopico[1]);
                    }

                    //Remover se receber fim do user    
                    if(strcmp(p.str, "fim") == 0){
                        remover_usuario(p.user.nome,p.user.pid);
                    }else{  //envia para todos os clientes
                        for(int i = 0; i<MAX_USERS; ++i){ 
                            if(usuarios[i].ativo){
                                sprintf(fifo, FIFO_CLI, usuarios[i].pid);
                                fd_cli = open(fifo, O_WRONLY);
                                strcpy(r.str, p.str);
                                res = write( fd_cli, &r, sizeof(RESPOSTA));
                                close(fd_cli);
                                printf("ENVIEI... '%s' (%d)\n", r.str,res);
                    
                            }
                        }
                    }
                }
            }
    }    
}while ( strcmp(str, "quit") !=0);

    close (fd);
    unlink(FIFO_SRV);
    printf("FIM\n");
    exit(0);
}

//Criar Topico
void criarTopico(const char* nome){
    if(num_topicos == MAX_TOPICOS){
         printf("[ERRO] Limite de tópicos atingido.\n");
            return;
    }
            // Verifica se o nome do tópico já existe
    for (int i = 0; i < num_topicos; i++) {
        if (strcmp(topicos[i].nome, nome) == 0) {
            printf("[ERRO] O tópico '%s' já existe.\n", nome);
            return;
        }
    }  
    topicos[num_topicos].nome == nome 
    topicos[num_topicos].num_mensagens = 0;
    topicos[num_topicos].bloqueado = 0;
    topicos[num_topicos].num_subscritores = 0;
   
    num_topicos++;
    printf("[INFO] Tópico '%s' criado com sucesso.\n", nome);
    return;
}


//Fazer Funçao Listar Topicos
//Fazer funçao Adicionar utilizador ao topico



//Adicionar Usuario
int adicionar_usuario(const char* nome_usuario, int pid) {
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
    //Adiciona a primeira vaga inativo
    for (int i = 0; i < MAX_USERS; i++) { 
        if (!usuarios[i].ativo) {
            strcpy(usuarios[i].nome, nome_usuario);
            usuarios[i].pid = pid;
            usuarios[i].ativo = 1;
            num_usuarios++;
            printf("[INFO] Usuário '%s' com PID %d adicionado.\n", nome_usuario, pid);
            return 0;
        }
    }
    return -1;
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
int remover_usuario(const char* nome_usuario, int pid) {
    //Verifica nome e ativo para 0
      for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(usuarios[i].nome, nome_usuario) == 0 && usuarios[i].ativo) {  //podemos verificar se esta ativo usuarios[i].ativo &&
            usuarios[i].ativo = 0;
            printf("[INFO] Usuário com PID %d removido.\n", pid);
            return 0;
        }
      }
    return -1;
}
