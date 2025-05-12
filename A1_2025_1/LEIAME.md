# LEIAME

**VINAc** – Arquivador com suporte a compressão

**Autor:** Caio Mendonça Krol  
**GRR:** GRR20245217

---

## Arquivos e Diretórios

- **main.c**: Função principal que interpreta os argumentos da linha de comando e invoca as operações correspondentes.
- **arquivo.c / arquivo.h**: Implementação das operações de inserção, extração, remoção, movimentação e inspeção de membros no arquivo compactado.
- **diretorio.c / diretorio.h**: Gerenciamento do diretório de membros, incluindo leitura, escrita e busca de membros.
- **util.c / util.h**: Funções auxiliares, como geração de UID e obtenção da data de modificação de arquivos.
- **lz.c / lz.h**: Implementação dos algoritmos de compressão e descompressão utilizando o algoritmo LZ (Terceiros).
- **makefile**: Script de compilação que define regras para compilar o projeto, limpar arquivos intermediários e reconstruir o executável.
- **.gitignore**: Lista de arquivos e diretórios a serem ignorados pelo sistema de controle de versão Git.

---

## Estrutura de Dados e Algoritmos

### Estrutura do Arquivo Compactado

O arquivo compactado segue a seguinte estrutura:

**Diretório**: Vetor de estruturas `membro_t`, cada uma representando um arquivo armazenado, com informações como nome, UID, tamanhos original e compactado, data de modificação, ordem e offset.
**Dados dos Arquivos**: Conteúdo dos arquivos armazenados.

### Estrutura `membro_t`

```c
typedef struct {
    char nome[1024];         // Nome do membro (sem espaços)
    __uid_t uid;             // Identificador único
    size_t tamanho_orig;     // Tamanho original
    size_t tamanho_disco;    // Tamanho após compressão (ou igual se não comprimido)
    time_t data_mod;         // Data de modificação
    int ordem;               // Ordem no arquivo
    long offset;             // Offset onde os dados começam no arquivo compactado
} membro_t;
```

### Algoritmos e Implementações

- **Compressão**: Utiliza o algoritmo LZ para reduzir o tamanho dos arquivos armazenados. A compressão é aplicada apenas se resultar em um tamanho menor que o original.
- **Inserção de Arquivos**: Ao adicionar um novo arquivo, o diretório é expandido e os dados existentes são realocados para acomodar o novo membro. Isso envolve mover os dados dos arquivos existentes para novos offsets, garantindo a integridade dos dados.
- **Extração de Arquivos**: Os arquivos podem ser extraídos individualmente ou em lote. Se estiverem compactados, são descompactados antes da escrita no sistema de arquivos.
- **Remoção de Arquivos**: Um membro pode ser removido do diretório, e o diretório é atualizado para refletir essa mudança.
- **Movimentação de Arquivos**: Permite alterar a ordem dos membros no diretório, o que pode afetar a ordem de extração ou listagem.
- **Inspeção de Arquivos**: Exibe informações detalhadas sobre um membro específico, incluindo os primeiros bytes de seu conteúdo.

---

## Decisões de Implementação

- **Gerenciamento de Diretório**: Optou-se por manter o diretório em memória como um vetor de `membro_t`, facilitando operações como busca, inserção e remoção.
- **Realocação de Dados**: Ao inserir novos membros, é necessário realocar os dados dos membros existentes para acomodar o crescimento do diretório. Essa operação foi cuidadosamente implementada para evitar corrupção de dados.
- **Compressão Condicional**: A compressão é aplicada apenas se resultar em economia de espaço, evitando sobrecarga desnecessária.
- **UID Único**: Cada membro recebe um identificador único (`uid`) para facilitar o rastreamento e evitar conflitos.
- **Inspecionar_membro**: Durante a implementação me deparei com um problema de implementação e precisei de uma ferramenta para ler o offset, tamanho e os Bytes de maneira crua por isso criei a função inspecionar_membro, acabei optando por adicionar ela ao programa com o comando -t, como uma ferramenta útil caso o usuário necessite.

---

## Dificuldades Encontradas

A principal dificuldade enfrentada foi a manipulação da realocação dos dados dos arquivos existentes ao inserir novos membros. Inicialmente, os dados eram sobrescritos ou corrompidos devido à não atualização correta dos offsets e à falta de movimentação real dos dados no arquivo compactado. A resolução envolveu a implementação de uma função de movimentação de dados que ajusta corretamente os offsets e move os dados para os novos locais, garantindo a integridade do arquivo compactado.

---

## Bugs Conhecidos

Atualmente, não há bugs conhecidos. Todas as funcionalidades foram testadas e estão operando conforme o esperado. No entanto, uma possível melhoria futura seria a implementação de uma reorganização dos arquivos depois de excluir um arquivo do .vc.
