#include "util.h"
/* NOTAS: Projeto!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
[DEBUG]

VARIAVEIS GLOBAIS....(ganhar coragem)

por vezes a dar quit (servidor) nao termina as threads todas, mas nao consigo replicar
bug complicado possivelmente mutex - quando o user sai e depois o servidor termina

*/


int main(int argc, char *argv[]){
    char str[TAM];
    //char temp[10];
    int fd, res;
    PEDIDO p;
    MSGSTRUCT mensagem_recebida;
    char fifo[40];
    int fd_cli, n;
    fd_set fds;
   
    // Verificar se o nome do user foi passado como argumento
    if (argc != 2) {
        printf("[ERRO] Uso: %s <nome_user>\n", argv[0]);
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

    // Enviar o nome do user ao manager
    strcpy(p.user.nome, argv[1]);
    strcpy(p.str, "registar");
    p.user.pid = getpid();
    res = write(fd, &p, sizeof(PEDIDO));
    if (res != sizeof(PEDIDO)) {
        printf("[ERRO] Falha ao enviar nome de utilizador ao servidor.\n");
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
        else if(n>0){ //ha dados? Pode ser Teclado ou Pipe
            if(FD_ISSET(0, &fds)){      //select teclado
                //scanf("%s",str);
                fgets(str, sizeof(str), stdin);
                str[strcspn(str, "\n")] = '\0';  // Remove o '\n' se presente
                //printf("Dbug 1: '%s'\n", str);  // Debug ?? faz um \n a mais??
                strcpy(p.str, str);
        
 
                res = write (fd, &p,sizeof(PEDIDO)); //envia para o manager
                if(res == sizeof(PEDIDO) ){
                    printf("[SEND] '%s' (%d)\n", p.str, res);  //Debug
                }
                
            }
            else if(FD_ISSET(fd_cli, &fds)){
                /*res = read(fd_cli, &type, sizeof(int));        //select pipe
                if (res == sizeof(int)) {
                printf("[DEBUG] leu inteiro: %d\n", type);
                if(type == 1){
                     res = read(fd_cli, &r, sizeof(RESPOSTA));
                }
                 if(type == 2){
                     res = read(fd_cli, &m, sizeof(MSG_USER));
                }} */
               /*
                char buffer[2048];
                res = read(fd_cli, buffer, sizeof(buffer)); 
                printf("[DEBUG] leu bufer\n");

                if (res >= sizeof(int)) {
                int type = *(int*)buffer; // Extraia o inteiro
                printf("[DEBUG] leu inteiro: %d\n", type);
                if (type == 1) {
                    memcpy(&r, buffer, sizeof(RESPOSTA)); // Copia todos os bytes para a estrutura RESPOSTA
                    printf("Recebi RESPOSTA: %s\n", r.str);
                } else if (type == 2) {
                    memcpy(&m, buffer, sizeof(MSG_USER)); // Copia todos os bytes para a estrutura MSG_USER
                    printf("Recebi MSG_USER: %s\n", m.corpo);
                    printf("[DEBUG] MSG_USER:\n");
                    printf("  Nome Tópico: %s\n", m.nome_topico);
                    printf("  Utilizador: %s\n", m.utilizador);
                    printf("  Corpo: %s\n", m.corpo);
                            //printf("RECEBI...\n '%s' (%d)\n", r.str, res);
                        }
                }
                 buffer[0] = '\0';*/

                //---------------------
                 
                res = read(fd_cli, &mensagem_recebida, sizeof(MSGSTRUCT));
                if (res > 0) {
                if(mensagem_recebida.type == TIPO_RESPOSTA ){
                    printf("\n%s\n", mensagem_recebida.conteudo.resposta.str);
                }
               if(mensagem_recebida.type == TIPO_MSG_USER ){
                    printf("\n[%s] %s: %s\n", 
                    mensagem_recebida.conteudo.msg_user.nome_topico,
                    mensagem_recebida.conteudo.msg_user.utilizador,
                    mensagem_recebida.conteudo.msg_user.corpo);
                    //printf("Recebi uma MSG_USER:\n");
                    //printf("Topico: %s\n", mensagem_recebida.conteudo.msg_user.nome_topico);
                    //printf("Utilizador: %s\n", mensagem_recebida.conteudo.msg_user.utilizador);
                    //printf("Corpo: %s\n", mensagem_recebida.conteudo.msg_user.corpo);
                }
                 } else {
                        perror("[FEED]Erro na leitura do pipe\n");
                    }
        } 
        }    
    }while(strcmp(mensagem_recebida.conteudo.resposta.str, "exit")!=0 && strcmp(p.str,"exit")!=0 ); //adicionar o fim do teclado do user
    
    close(fd);
    close(fd_cli);
    //printf("FIM\n");
    exit(0);
}


                    