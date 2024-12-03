#include "util.h"
/*
Trocar pid_usuario por pid_user
funçao para receber msg cliente
Fazer sistema de tempo



*/


int main(int argc, char *argv[]){
    char str[TAM];
    //char temp[10];
    int fd, res;
    PEDIDO p;
    RESPOSTA r;
    char fifo[40];
    int fd_cli, n;
    fd_set fds;

    // Verificar se o nome do usuário foi passado como argumento
    if (argc != 2) {
        printf("[ERRO] Uso: %s <nome_usuario>\n", argv[0]);
        exit(1);
    }
    

    printf("[INFO] Feed Iniciado. \n");
    if(access(FIFO_SRV, F_OK) !=0 ){
        printf("[ERRO] Servidor nao esta a correr!\n");
        exit(3);
    }
    
    fd = open(FIFO_SRV,O_WRONLY);
    sprintf(fifo,FIFO_CLI,getpid() );
    mkfifo(fifo, 0600);
    fd_cli = open (fifo, O_RDWR);

    // Enviar o nome do usuário ao manager
    strcpy(p.user.nome, argv[1]);
    strcpy(p.str, "registar");
    p.user.pid = getpid();
    res = write(fd, &p, sizeof(PEDIDO));
    if (res != sizeof(PEDIDO)) {
        printf("[ERRO] Falha ao enviar nome de usuário ao servidor.\n");
        close(fd);
        close(fd_cli);
        //unlink(fifo); ??
        exit(3);
    }
    printf("[INFO] Usuário %s conectado ao servidor.\n", p.user.nome);
   
   //ciclo ler teclado/ler pipe
    do{
        printf("%s> ", p.user.nome);
        fflush(stdout);  
        // select 
        FD_ZERO(&fds);
        FD_SET(0, &fds);        
        //stdin teclado
        FD_SET(fd_cli,&fds);  // npipe(fd)

        
        n = select( fd_cli + 1, &fds, NULL, NULL, NULL);
        if (n == -1 ) {
            printf("[ERRO] Select!\n");
        }
        else if(n>0){ //ha dados.. onde?
            if(FD_ISSET(0, &fds)){      //select teclado
                //scanf("%s",str);
                fgets(str, sizeof(str), stdin);
                str[strcspn(str, "\n")] = '\0';  // Remove o '\n' se presente
                //printf("Dbug 1: '%s'\n", str);  // Debug ?? faz um \n a mais??
                strcpy(p.str, str);
        
 
                res = write (fd, &p,sizeof(PEDIDO)); //envia para o manager
                if(res == sizeof(PEDIDO) ){
                    printf("ENVIEI... '%s' (%d)\n", p.str, res);  //Debug
                }
                
            }
            else if(FD_ISSET(fd_cli, &fds)){        //select pipe
                res = read(fd_cli, &r, sizeof(RESPOSTA));
                printf("RECEBI...\n '%s' (%d)\n", r.str, res);
            }
        }     
    }while(strcmp(r.str, "fim")!=0 && strcmp(p.str,"fim")!=0 ); //adicionar o fim do teclado do user
    
    close(fd);
    close(fd_cli);
    printf("FIM\n");
    exit(0);
}


                    