# Nome dos executáveis
MANAGER_EXEC = manager
FEED_EXEC = feed

# Arquivos-fonte para cada executável
MANAGER_SRC = manager.c
FEED_SRC = feed.c

# Cabeçalhos compartilhados
HEADERS = util.h

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

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