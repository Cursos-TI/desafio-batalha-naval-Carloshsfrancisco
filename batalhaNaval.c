#include <stdio.h>
#include <stdlib.h> // Necessário para usar a função abs()


#define TAMANHO 10   // tabuleiro 10x10
#define NAVIO 3      // cada navio tem 3 posições

// Valores no tabuleiro:
// 0 -> água
// 3 -> navio
// 5 -> área de habilidade (sem navio)
// 8 -> navio + área de habilidade (sobreposição)

void inicializarTabuleiro(int tab[TAMANHO][TAMANHO]) {
    for (int i = 0; i < TAMANHO; i++)
        for (int j = 0; j < TAMANHO; j++)
            tab[i][j] = 0;
}

void exibirTabuleiro(int tab[TAMANHO][TAMANHO]) {
    printf("\n    ");
    for (int c = 0; c < TAMANHO; c++) printf("%c ", 'A' + c);
    printf("\n");
    for (int r = 0; r < TAMANHO; r++) {
        printf("%2d  ", r + 1);
        for (int c = 0; c < TAMANHO; c++) {
            printf("%d ", tab[r][c]);
        }
        printf("\n");
    }
    printf("\nLegenda: 0=agua  3=navio  5=area  8=navio+area\n\n");
}

// ----------------- Posicionamento de navios (reuso das funções anteriores) -----------------

int posicionarNavioHorizontal(int tab[TAMANHO][TAMANHO], int linha, int coluna) {
    if (linha < 0 || linha >= TAMANHO || coluna < 0 || coluna >= TAMANHO) return 0;
    if (coluna + NAVIO > TAMANHO) return 0;
    for (int i = 0; i < NAVIO; i++)
        if (tab[linha][coluna + i] != 0) return 0;
    for (int i = 0; i < NAVIO; i++)
        tab[linha][coluna + i] = 3;
    return 1;
}

int posicionarNavioVertical(int tab[TAMANHO][TAMANHO], int linha, int coluna) {
    if (linha < 0 || linha >= TAMANHO || coluna < 0 || coluna >= TAMANHO) return 0;
    if (linha + NAVIO > TAMANHO) return 0;
    for (int i = 0; i < NAVIO; i++)
        if (tab[linha + i][coluna] != 0) return 0;
    for (int i = 0; i < NAVIO; i++)
        tab[linha + i][coluna] = 3;
    return 1;
}

int posicionarNavioDiagonalPrincipal(int tab[TAMANHO][TAMANHO], int linha, int coluna) {
    if (linha < 0 || coluna < 0) return 0;
    if (linha + NAVIO > TAMANHO || coluna + NAVIO > TAMANHO) return 0;
    for (int i = 0; i < NAVIO; i++)
        if (tab[linha + i][coluna + i] != 0) return 0;
    for (int i = 0; i < NAVIO; i++)
        tab[linha + i][coluna + i] = 3;
    return 1;
}

int posicionarNavioDiagonalSecundaria(int tab[TAMANHO][TAMANHO], int linha, int coluna) {
    if (linha < 0 || coluna < 0) return 0;
    if (linha + NAVIO > TAMANHO || coluna - (NAVIO - 1) < 0) return 0;
    for (int i = 0; i < NAVIO; i++)
        if (tab[linha + i][coluna - i] != 0) return 0;
    for (int i = 0; i < NAVIO; i++)
        tab[linha + i][coluna - i] = 3;
    return 1;
}

// ----------------- Construção dinâmica das matrizes de habilidade -----------------
// Usamos tamanho FIXO para as matrizes de habilidade: 5x5 (padrão razoável para visualização)

#define HSIZE 5

// Gera uma matriz "cone" (topo apontando para baixo).
// Exemplo visual (5x5), mas cone ocupa apenas 3 linhas (topo + expansão):
// 0 0 1 0 0
// 0 1 1 1 0
// 1 1 1 1 1
// 0 0 0 0 0
// 0 0 0 0 0
void gerarCone(int m[HSIZE][HSIZE]) {
    // inicializa zeros
    for (int r = 0; r < HSIZE; r++)
        for (int c = 0; c < HSIZE; c++)
            m[r][c] = 0;

    int mid = HSIZE / 2; // centro = 2 (0..4)
    // definimos um cone de altura 3: linhas 0,1,2 do bloco 5x5
    for (int r = 0; r <= 2; r++) {
        // largura do cone na linha r: de mid-r até mid+r
        for (int c = mid - r; c <= mid + r; c++) {
            if (c >= 0 && c < HSIZE) m[r][c] = 1;
        }
    }
}

// Gera uma matriz "cruz" centrada
// 0 0 1 0 0
// 0 0 1 0 0
// 1 1 1 1 1
// 0 0 1 0 0
// 0 0 1 0 0
void gerarCruz(int m[HSIZE][HSIZE]) {
    for (int r = 0; r < HSIZE; r++)
        for (int c = 0; c < HSIZE; c++)
            m[r][c] = 0;

    int mid = HSIZE / 2;
    // linha central inteira
    for (int c = 0; c < HSIZE; c++) m[mid][c] = 1;
    // coluna central inteira
    for (int r = 0; r < HSIZE; r++) m[r][mid] = 1;
}

// Gera um "octaedro" visto de frente -> forma de losango (diamond)
// condição: abs(r-mid) + abs(c-mid) <= mid  => dentro do losango
void gerarOctaedro(int m[HSIZE][HSIZE]) {
    for (int r = 0; r < HSIZE; r++)
        for (int c = 0; c < HSIZE; c++)
            m[r][c] = 0;

    int mid = HSIZE / 2;
    for (int r = 0; r < HSIZE; r++) {
        for (int c = 0; c < HSIZE; c++) {
            if ( (abs(r - mid) + abs(c - mid)) <= mid ) {
                m[r][c] = 1;
            }
        }
    }
}

// ----------------- Sobreposição de uma matriz de habilidade ao tabuleiro -----------------
// origemLinha, origemColuna são as coordenadas no tabuleiro onde o centro da matriz (mid,mid) será alinhado.
// Se uma célula do skill == 1 e estiver dentro do tabuleiro, marca:
//  - se célula estava 0 -> seta 5
//  - se célula estava 3 (navio) -> seta 8 (navio+area)
//  - se célula já era 5 ou 8 -> mantém
void aplicarHabilidadeAoTabuleiro(int tab[TAMANHO][TAMANHO], int skill[HSIZE][HSIZE], int origemLinha, int origemColuna) {
    int mid = HSIZE / 2;
    for (int r = 0; r < HSIZE; r++) {
        for (int c = 0; c < HSIZE; c++) {
            if (skill[r][c] == 1) {
                int tr = origemLinha + (r - mid);
                int tc = origemColuna + (c - mid);
                // checa limites do tabuleiro
                if (tr >= 0 && tr < TAMANHO && tc >= 0 && tc < TAMANHO) {
                    if (tab[tr][tc] == 0) {
                        tab[tr][tc] = 5; // área de habilidade
                    } else if (tab[tr][tc] == 3) {
                        tab[tr][tc] = 8; // navio + área
                    } else {
                        // se já for 5 ou 8, mantem (não faz nada)
                    }
                }
            }
        }
    }
}

int main() {
    int tab[TAMANHO][TAMANHO];
    inicializarTabuleiro(tab);

    // --- Posicionamento de quatro navios (tamanho 3) ---
    // coordenadas em index 0-based
    // Navio horizontal: linha 2 (3ª linha), coluna 3 (D) -> ocupa D3,E3,F3
    if (!posicionarNavioHorizontal(tab, 2, 3)) printf("Falha ao posicionar navio horizontal\n");

    // Navio vertical: linha 6 (7ª linha), coluna 1 (B) -> ocupa B7,B8,B9
    if (!posicionarNavioVertical(tab, 6, 1)) printf("Falha ao posicionar navio vertical\n");

    // Navio diagonal principal (↘): começa em 0,0 -> A1,B2,C3
    if (!posicionarNavioDiagonalPrincipal(tab, 0, 0)) printf("Falha ao posicionar navio diagonal principal\n");

    // Navio diagonal secundária (↙): começa em 1,9 -> J2,I3,H4
    if (!posicionarNavioDiagonalSecundaria(tab, 1, 9)) printf("Falha ao posicionar navio diagonal secundaria\n");

    // --- Geração dinâmica das matrizes de habilidade ---
    int cone[HSIZE][HSIZE];
    int cruz[HSIZE][HSIZE];
    int octaedro[HSIZE][HSIZE];

    gerarCone(cone);
    gerarCruz(cruz);
    gerarOctaedro(octaedro);

    // --- Definição dos pontos de origem (centros) no tabuleiro (0-based) ---
    // Escolhas arbitrárias dentro do board (pode ajustar conforme quiser)
    int origemConeLinha = 1, origemConeColuna = 2;   // perto do canto superior (alinha o topo do cone)
    int origemCruzLinha = 5, origemCruzColuna = 5;   // centro do tabuleiro
    int origemOctaLinha = 8, origemOctaColuna = 3;   // perto da parte inferior

    // Aplica as habilidades ao tabuleiro (com verificação de limites dentro da função)
    aplicarHabilidadeAoTabuleiro(tab, cone, origemConeLinha, origemConeColuna);
    aplicarHabilidadeAoTabuleiro(tab, cruz, origemCruzLinha, origemCruzColuna);
    aplicarHabilidadeAoTabuleiro(tab, octaedro, origemOctaLinha, origemOctaColuna);

    // Exibe o tabuleiro final
    exibirTabuleiro(tab);

    return 0;
}
