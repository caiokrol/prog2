# Nome do executável final que será gerado
EXEC = vinac

# Diretório onde o executável será armazenado após a compilação
OUTDIR = cmk24

# Lista de arquivos-fonte (.c) do projeto
SRCS = main.c arquivo.c diretorio.c util.c lz.c

# Lista de arquivos-objeto (.o) correspondentes aos fontes acima
# A substituição transforma cada .c em .o automaticamente
OBJS = $(SRCS:.c=.o)

# Compilador a ser utilizado
CC = gcc

# Flags de compilação:
#  -Wall     → habilita todos os avisos comuns
#  -Wextra   → habilita avisos extras
#  -O2       → otimizações de nível 2 para melhor desempenho
CFLAGS = -Wall -Wextra -Werror -O2

# Regra padrão: será executada quando você rodar apenas 'make'
# Gera o executável dentro do diretório de saída
all: $(OUTDIR)/$(EXEC)

# Regra que cria o diretório de saída (se necessário) e gera o executável
# $@ → representa o alvo (no caso, cmk24/vinac)
# $^ → representa as dependências (os objetos)
$(OUTDIR)/$(EXEC): $(OBJS)
	@mkdir -p $(OUTDIR)              # Cria o diretório cmk24, se não existir
	$(CC) $(CFLAGS) -o $@ $(OBJS)    # Compila o executável com os objetos

# Regra para limpar os arquivos gerados pela compilação
clean:
	rm -f $(OBJS)                   # Remove todos os arquivos .o
	rm -f $(OUTDIR)/$(EXEC)        # Remove o executável gerado
	rm -rf cmk24                   # Remove o diretório de saída inteiro

# Regra que recompila o projeto do zero
rebuild: clean all

# Regra de compilação genérica para arquivos .c → .o
# $< → representa o arquivo de origem (.c)
# $@ → representa o alvo da regra (.o)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
