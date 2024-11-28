#include "util.h"

typedef struct{
    char str[TAM], fifo[40];
    int num;
    char letra;
    int res, fd_cli;
    PEDIDO p;
    RESPOSTA r;
    int *pcontinuar;
}TDATA;

//Globais
Topico topicos[MAX_TOPICOS]; 
int num_topicos = 0;

USUARIO usuarios[MAX_USERS];  // Array para armazenar usuários
int num_usuarios = 0;  //posso utilizar uma static na estrutura??

//Declarar Funçoes
int adicionar_usuario(const char* nome_usuario, int pid);
void listar_usuarios();
int remover_usuario(const char* nome_usuario, int pid);


void* processa_admin(void* arg) {
    TDATA *ptd = (TDATA *)pdata;
   
do{
    printf("ADMIN> ");
    fflush(stdout);
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
                *td->pcontinuar = 0;
            }

}while (*ptd->continuar)
    printf("Termina thread ADMIN\n");  
    pthread_exit(NULL);
}

void* processa_cliente(void *pdata) {
    
    
    do{
    res = read (fd, &p, sizeof(PEDIDO) );
                if(res == sizeof(PEDIDO) ){
                    printf("RECEBI... '%s' - %d (%d)\n", p.str, p.user.pid, res);
                    //adicionar users ativos
                    //usar flag para so adocionar quando conecta?? S
                    //Fazer funçao ou ficheirasado para tratar comandos
                    adicionar_usuario(p.user.nome, p.user.pid);

                    //se o comando for subscribe-INCOMPLETO
                    if (strcmp(strtok(p.str," ") , "subscribe") == 0) { //se o comando for subscribe
                      char tmpTopico[10];
                       //tmpTopico = strtok(NULL," ");
                       printf("Subscrito no topico: '%s'\n", tmpTopico);
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
       }while (*ptd->continuar)       
printf("Termina thread ADMIN\n");  
pthread_exit(NULL);
}


    


int main(int agrc, char *argv[]){
    
    int fd;
    struct timeval tempo;
    

    if( access(FIFO_SRV, F_OK ) == 0){
        printf("[ERRO] Ja existe um servidor!\n");
        exit(3);
    }

    printf("INICIO...\n");
    mkfifo(FIFO_SRV,0600);
    fd = open(FIFO_SRV,O_RDWR);

    pthread_t thread_id[2]; //informação diferente para cada thread
    int continuar = 1;  //informação igual para todas as thread, passado como ponteiro
    td[0].pcontinuar = &continuar;
    td[1].pcontinuar = &continuar;
    // Cria a thread para o administrador
    pthread_create(&thread_id[0], NULL, processa_admin, (void*) &td[0]);
    
    //Cria uma thread para lidar com o pipe
    pthread_create(&thread_id[1], NULL, processa_clientes, (void*) &td[1]);


    td[0].continuar = 0; // termina a thread
    pthread_join(thread_id[0], NULL); //espera que termine

    td[1].continuar = 0; // termina a thread
    pthread_join(thread_id[1], NULL); //espera que termine



    close (fd);
    unlink(FIFO_SRV);
    printf("FIM\n");
    exit(0);
}

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
