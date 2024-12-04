# Nome dos executáveis
MANAGER_EXEC = manager
FEED_EXEC = feed

# Diretórios adicionais
MANAGER_DIR = manager_files

# Arquivos-fonte para cada executável
MANAGER_SRC = manager.c $(MANAGER_DIR)/mensagensFuncoes.c $(MANAGER_DIR)/topicosFuncoes.c $(MANAGER_DIR)/userFuncoes.c
FEED_SRC = feed.c

# Cabeçalhos compartilhados
HEADERS = util.h	$(MANAGER_DIR)/manager_util.h

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I. -I$(MANAGER_DIR)

# Regra padrão: compilar ambos os executáveis
all: $(MANAGER_EXEC) $(FEED_EXEC)

# Regra para compilar o manager
$(MANAGER_EXEC): $(MANAGER_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MANAGER_SRC) -o $(MANAGER_EXEC)

# Regra para compilar o feed
$(FEED_EXEC): $(FEED_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(FEED_SRC) -o $(FEED_EXEC)

# Limpar os arquivos gerados
clean:
	rm -f $(MANAGER_EXEC) $(FEED_EXEC)

# Regra "phony" para garantir que clean seja sempre executado
.PHONY: all clean