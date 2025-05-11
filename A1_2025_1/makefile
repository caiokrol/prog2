# Nome do executável
EXEC = vinac

# Diretório de saída
OUTDIR = login

# Fontes
SRCS = main.c arquivo.c diretorio.c util.c lz.c
OBJS = $(SRCS:.c=.o)

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Regra padrão
all: $(OUTDIR)/$(EXEC)

# Cria o diretório de saída e compila o executável
$(OUTDIR)/$(EXEC): $(OBJS)
	@mkdir -p $(OUTDIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Remove arquivos objetos e o executável
clean:
	rm -f $(OBJS)
	rm -f $(OUTDIR)/$(EXEC)

# Recompila tudo
rebuild: clean all

# Regras auxiliares
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
