# Nome do executável
TARGET = jogo

# Código fonte principal
SRC = main.c

# Flags do Allegro (usa pkg-config para facilitar)
CFLAGS = `pkg-config --cflags allegro-5 allegro_font-5 allegro_ttf-5 allegro_image-5 allegro_video-5 allegro_dialog-5 allegro_acodec-5 allegro_primitives-5`
LIBS   = `pkg-config --libs allegro-5 allegro_font-5 allegro_ttf-5 allegro_image-5 allegro_video-5 allegro_dialog-5 allegro_acodec-5 allegro_primitives-5`

# Regra principal
all:
	gcc $(SRC) -o $(TARGET) $(CFLAGS) $(LIBS) -lm

# Limpa os arquivos compilados
clean:
	rm -f $(TARGET)
