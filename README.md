# **Sistemas Operativos 24/25 – Plataforma de Mensagens**
Projeto de Sistema de Mensagens

Este projeto implementa um sistema de mensagens assíncrono em C que permite que diferentes clientes se conectem e interajam com tópicos e mensagens persistentes. O sistema é dividido em duas partes principais: o **Manager**, responsável pela coordenação de todas as mensagens e usuários, e o **Feed**, que é a interface de interação do usuário com o sistema.

## Manager

O **Manager** é o servidor central que coordena a comunicação entre os usuários, gerencia os tópicos de discussão, e armazena as mensagens persistentes. Ele também responde aos comandos administrativos para listar, bloquear ou desbloquear tópicos, além de gerenciar o ciclo de vida dos usuários.

### Ficheiros Relacionados ao Manager

1. **manager.c**:
   - Contém a lógica principal do servidor, incluindo a criação e inicialização do FIFO para comunicação, bem como a criação das threads que tratam dos comandos administrativos, do pipe dos usuários e da monitoração de mensagens.
   - Cria três threads: uma para leitura dos comandos do administrador, uma para leitura do pipe de comunicação, e outra para monitorar as mensagens.

2. **manager_util.h**:
   - Cabeçalho com as declarações das funções utilizadas pelo Manager. Define estruturas de dados comuns, como tópicos e usuários, além de variáveis globais e mutexes utilizados no projeto.

3. **mensagensFuncoes.c**:
   - Contém funções para carregar e armazenar mensagens persistentes. Também possui funções para atualizar as mensagens em tópicos, garantindo que mensagens expiradas sejam removidas.
   - Implementa a lógica de envio de mensagens para todos os subscritores de um tópico.

4. **topicosFuncoes.c**:
   - Contém as funções para manipulação dos tópicos, como criar, listar, bloquear, desbloquear e eliminar tópicos.
   - Também implementa a lógica para subscrever e remover usuários dos tópicos.

5. **userFuncoes.c**:
   - Contém as funções para gerenciar os usuários, como adicionar, remover e listar usuários ativos.
   - Também é responsável por notificar os usuários conectados sobre mudanças no sistema, como novas conexões ou desconexões.

## Feed

O **Feed** é a interface cliente que permite aos usuários interagirem com o sistema de mensagens. Através do Feed, os usuários podem se conectar ao servidor, enviar mensagens, subscrever tópicos, e visualizar mensagens de outros usuários em tópicos.

### Ficheiros Relacionados ao Feed

1. **feed.c**:
   - Contém a lógica principal do cliente. Permite ao usuário se conectar ao servidor especificando um nome e cria um FIFO exclusivo para comunicação com o servidor.
   - Utiliza a função `select()` para tratar tanto as entradas do teclado quanto as mensagens recebidas do servidor de forma assíncrona.
   - Envia comandos ao servidor para se registrar, enviar mensagens, subscrever ou sair de tópicos, entre outras interações.

## Como Utilizar o Projeto
## **Como Compilar e Executar o Projeto**

*   **Requisitos:** Ambiente Unix (Linux) e um compilador C.
*   **Compilação:** Utilize o `makefile` fornecido para compilar os programas. O `makefile` deve ter os targets "all", "feed", "manager" e "clean" [17].
    ```bash
    make all
    ```

### Execução
1. Primeiro, execute o **Manager** para iniciar o servidor:
   ```sh
   ./manager
   ```
   O Manager ficará aguardando conexões dos clientes e comandos do administrador.

2. Em seguida, execute o **Feed** em outro terminal para conectar um usuário:
   ```sh
   ./feed <nome_usuario>
   ```
   Onde `<nome_usuario>` é o nome que identifica o usuário no sistema.

## **Funcionalidades**
*   **Registo de utilizadores:** Os utilizadores registam-se com um username único [3].
*   **Tópicos:** Criação e gestão de tópicos de mensagens [4].
*   **Mensagens:** Envio de mensagens persistentes e não persistentes para tópicos [5]. As mensagens persistentes são armazenadas por um tempo limitado [6].
*   **Subscrição:** Os utilizadores podem subscrever tópicos para receber mensagens [7].
*   **Administração:** O administrador pode listar utilizadores, remover utilizadores e bloquear/desbloquear tópicos [8-10].

## Considerações
- **Comunicação**: A comunicação entre Manager e Feed é feita através de **FIFOs**, que permitem a troca de informações de maneira assíncrona.
- **Sincronização**: A sincronização entre as threads no Manager é feita com **mutexes** para evitar problemas de condição de corrida ao manipular estruturas globais como `topicos[]` e `utilizadores[]`.

## Estrutura do Projeto
```
/
|-- manager.c
|-- manager_util.h
|-- mensagensFuncoes.c
|-- topicosFuncoes.c
|-- userFuncoes.c
|-- feed.c
|-- Makefile (opcional)
```

## **Utilização**

*   **Cliente (Feed):**
    *   `topics`: Lista os tópicos existentes [18].
    *   `msg <topico> <duração> <mensagem>`: Envia uma mensagem para um tópico [18]. A duração define se a mensagem é persistente (duração > 0) [19].
    *   `subscribe <topico>`: Subscreve um tópico [19].
    *   `unsubscribe <topico>`: Remove a subscrição de um tópico [20].
    *   `exit`: Sai do programa `feed` [20].
*   **Administrador (Manager):**
    *   `users`: Lista os utilizadores conectados [21].
    *   `remove <username>`: Remove um utilizador [9].
    *   `topics`: Lista os tópicos existentes [9].
    *   `show <topico>`: Mostra as mensagens de um tópico [9].
    *   `lock <topico>`: Bloqueia um tópico [10].
    *   `unlock <topico>`: Desbloqueia um tópico [10].
    *   `close`: Encerra a plataforma [10].

## **Autores**
* Alexandre Moreira
