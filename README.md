# Métodos Numéricos para EDO

| | |
|---|---|
| **Autor** | Nattan Silva Montel |
| **Professor** | Laudo Claumir |
| **Matéria** | EDO - Equações Diferenciais Ordinárias |
| **Data** | 23/11/2026 |
| **Feito em** | C |

## O que faz

Resolve numericamente o problema de valor inicial

```
y' = f(x, y),   y(a) = y0,   x em [a, b]
```

com `n` pontos e compara 5 métodos com a solução exata. Os dados são digitados numa **interface web**
(intervalo, `y(a)`, `n`, `f(x,y)` e `df(x,y)`, cada campo com instruções de formato); o cálculo é feito pelo
**programa em C**, e a tabela aparece na página com botão **Exportar PDF**.

| # | Método | Fórmula (passo `h = (b-a)/(n-1)`) |
|---|---|---|
| 1 | Taylor 1ª ordem | `y(i+1) = y + h f` |
| 2 | Taylor 2ª ordem | `y(i+1) = y + h f + (h²/2) df` |
| 3 | Euler Melhorado (Heun) | `y(i+1) = y + (h/2) [ f(x,y) + f(x+h, y + h f(x,y)) ]` |
| 4 | Euler Modificado (ponto médio) | `y(i+1) = y + h f(x + h/2, y + (h/2) f(x,y))` |
| 5 | Runge-Kutta 4ª ordem | `y(i+1) = y + (h/6)(k1 + 2k2 + 2k3 + k4)` |
| 6 | Solução exata | calculada em forma fechada |

`df` é a derivada total de `f`: `df = ∂f/∂x + ∂f/∂y · f` (usada só no Taylor 2).
Bibliotecas: `stdio.h`, `stdlib.h`, `string.h`, `ctype.h` e `math.h`. O PDF sai pelo próprio navegador (`window.print()`).

## Como usar

```bash
gcc edo.c -o edo -lm
python3 server.py        # abre http://localhost:8765
```

Abra http://localhost:8765, preencha os campos (ou escolha um exemplo) e clique em **Calcular**.
Depois, **Exportar PDF** → "Salvar como PDF".

Regras de digitação (também explicadas sob cada campo da página):
- Decimais com **ponto** (`0.5`), nunca vírgula. `n` é inteiro de 2 a 10000.
- `f` e `df`: variáveis `x` e `y`; operadores `+ - * / ^` e parênteses; multiplicação sempre com `*` (`2*x*y`);
  funções `sin cos tan asin acos atan exp log ln sqrt abs sinh cosh tanh`; constantes `pi` e `e`.
- Solução exata (opcional, só em `x`): se vazia, a coluna mostra uma referência RK4 com passo `h/100`.
- A última linha da tabela é o **erro máximo** `|y_método - y_exata|` de cada método.

Também dá para usar só o C, no terminal:

```bash
./edo --txt 0 2 0.5 11 "y - x^2 + 1" "-2*x + y - x^2 + 1" "(x+1)^2 - 0.5*exp(x)"
```

As expressões são lidas por um parser próprio em C (sem `eval`); o servidor chama o programa sem shell.

## Exemplos prontos (do mais fácil ao mais difícil)

Disponíveis no menu "Carregar um exemplo" da página:
`y'=x`, `y'=y`, `y'=x+y`, `y'=y-x²+1`, `y'=-2y+x`, `y'=y cos x`, `y'=2xy`, `y'=y/x+x` (a > 0),
logística `y'=y(1-y)` e `y'=-50(y-cos x)` (**rígida**: com `h` grande os métodos explícitos divergem).

## Testes

`./testes.sh` compila (com `-Wall -Wextra`, sem avisos) e roda 10 casos. Erro máximo por método:

| Caso | Taylor 1 | Taylor 2 | Melhorado | Modificado | RK4 |
|---|---|---|---|---|---|
| 1 `0 2 0 11` | 2.0e-01 | 4.4e-16 | 4.4e-16 | 4.4e-16 | 4.4e-16 |
| 2 `0 1 1 11` | 1.2e-01 | 4.2e-03 | 4.2e-03 | 4.2e-03 | 2.1e-06 |
| 3 `0 2 0.5 21` | 9.9e-01 | 3.4e-02 | 3.4e-02 | 3.4e-02 | 1.7e-05 |
| 4 `0 2 0.5 11` | 4.4e-01 | 4.2e-02 | 7.2e-02 | 1.5e-02 | 1.1e-04 |
| 5 `0 2 1 21` | 5.0e-02 | 3.6e-03 | 3.6e-03 | 3.6e-03 | 7.2e-06 |
| 6 `0 3 1 31` | 8.3e-02 | 7.9e-03 | 5.3e-03 | 1.4e-03 | 1.1e-06 |
| 7 `0 1 1 21` | 2.1e-01 | 8.3e-03 | 2.3e-03 | 5.3e-03 | 7.4e-07 |
| 8 `1 3 2 21` | 3.2e-01 | 1.1e-14 | 9.4e-03 | 4.8e-03 | 3.7e-06 |
| 9 `0 5 0.1 51` | 1.3e-02 | 1.9e-04 | 4.2e-04 | 1.7e-04 | 1.2e-07 |
| 10 `0 2 0 11` | 3.5e+09 | 1.3e+16 | 1.3e+16 | 1.3e+16 | 4.4e+24 |

Leitura dos resultados:
- Taylor 1 (ordem 1) é sempre o pior; Taylor 2, Melhorado e Modificado (ordem 2) ficam na mesma faixa; RK4 (ordem 4) é de longe o melhor.
- Casos 1 e 8: a solução exata é um polinômio de grau 2, por isso os métodos de ordem 2 acertam até o erro de arredondamento.
- Caso 10 (rígido): `h = 0.2` está acima do limite de estabilidade (`h < 2/50 = 0.04` para Euler), então todos divergem. Com `n` maior (por exemplo 201, `h = 0.01`) voltam a convergir.

## Arquivos

- `edo.c`: programa em C (parser de expressões + os 5 métodos + solução exata)
- `index.html`: interface (formulário e tabela)
- `server.py`: servidor local que liga a página ao programa em C
- `testes.sh`: 10 casos de teste + casos de entrada inválida
