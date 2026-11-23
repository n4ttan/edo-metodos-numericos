/* Autor: Nattan Silva Montel | Prof.: Laudo Claumir
   EDO - Equacoes Diferenciais Ordinarias | 23/11/2026 | Feito em C
   Compilar: gcc edo.c -o edo -lm
   Uso:  ./edo [--txt] a b y(a) n "f(x,y)" "df(x,y)" ["exata(x)"]
   Saida: JSON (usado pela interface web) ou, com --txt, tabela no terminal.
   f, df e exata sao expressoes em texto, lidas por um parser proprio (sem eval). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* ---------- parser de expressoes: x, y, numeros, + - * / ^, funcoes ---------- */
typedef struct N { char t; int fn; double v; struct N *l, *r; } N;
/* t: 'n' numero, 'x', 'y', '+' '-' '*' '/' '^', 'm' menos unario, 'f' funcao(l) */
static const char *src, *perr;
static char msgbuf[96];
static int allow_y;

static const struct { const char *nm; double (*fp)(double); } FN[] = {
    {"sin", sin}, {"cos", cos}, {"tan", tan}, {"asin", asin}, {"acos", acos}, {"atan", atan},
    {"exp", exp}, {"log", log}, {"ln", log}, {"sqrt", sqrt}, {"abs", fabs},
    {"sinh", sinh}, {"cosh", cosh}, {"tanh", tanh}};

static N *mk(char t, N *l, N *r) { N *n = calloc(1, sizeof *n); n->t = t; n->l = l; n->r = r; return n; } /* nunca liberado: processo curto */
static void skip(void) { while (isspace((unsigned char)*src)) src++; }
static void seterr(const char *m) { if (!perr) perr = m; }
static N *expr(void);
static N *unary(void);

static N *atom(void) {
    skip();
    if (perr) return NULL;
    if (*src == '(') {
        src++; N *e = expr(); skip();
        if (*src != ')') seterr("falta fechar parenteses"); else src++;
        return e;
    }
    if (isdigit((unsigned char)*src) || *src == '.') {
        char *e; N *n = mk('n', NULL, NULL); n->v = strtod(src, &e); src = e; return n;
    }
    if (isalpha((unsigned char)*src)) {
        char id[16]; int k = 0;
        while (isalnum((unsigned char)*src)) { if (k < 15) id[k++] = *src; src++; }
        id[k] = 0;
        if (!strcmp(id, "x")) return mk('x', NULL, NULL);
        if (!strcmp(id, "y")) {
            if (!allow_y) { seterr("a solucao exata so pode usar x"); return NULL; }
            return mk('y', NULL, NULL);
        }
        if (!strcmp(id, "pi") || !strcmp(id, "e")) { N *n = mk('n', NULL, NULL); n->v = id[0] == 'p' ? M_PI : M_E; return n; }
        skip();
        for (unsigned i = 0; i < sizeof FN / sizeof *FN; i++)
            if (!strcmp(id, FN[i].nm) && *src == '(') {
                src++; N *n = mk('f', expr(), NULL); n->fn = i; skip();
                if (*src != ')') seterr("falta fechar parenteses"); else src++;
                return n;
            }
        snprintf(msgbuf, sizeof msgbuf, "nome desconhecido '%s' (lembre: multiplicacao explicita com *)", id);
        seterr(msgbuf); return NULL;
    }
    if (!*src) seterr("expressao vazia ou incompleta");
    else {
        char c = (*src == '"' || *src == '\\' || *src < 32) ? '?' : *src;
        snprintf(msgbuf, sizeof msgbuf, "caractere inesperado '%c'", c); seterr(msgbuf);
    }
    return NULL;
}
static N *power(void) {
    N *b = atom(); skip();
    if (*src == '^') { src++; return mk('^', b, unary()); } /* associa a direita */
    return b;
}
static N *unary(void) {
    skip();
    if (*src == '-') { src++; return mk('m', unary(), NULL); }
    if (*src == '+') { src++; return unary(); }
    return power();
}
static N *term(void) {
    N *l = unary();
    for (;;) { skip(); if (*src == '*' || *src == '/') { char o = *src++; l = mk(o, l, unary()); } else return l; }
}
static N *expr(void) {
    N *l = term();
    for (;;) { skip(); if (*src == '+' || *src == '-') { char o = *src++; l = mk(o, l, term()); } else return l; }
}
static N *parse(const char *s, int com_y) {
    src = s; perr = NULL; allow_y = com_y;
    N *r = expr(); skip();
    if (!perr && *src) {
        char c = (*src == '"' || *src == '\\' || *src < 32) ? '?' : *src;
        snprintf(msgbuf, sizeof msgbuf, "caractere inesperado '%c' (falta operador? ex.: 2*x)", c); perr = msgbuf;
    }
    return r;
}
static double ev(const N *n, double x, double y) {
    switch (n->t) {
    case 'n': return n->v;
    case 'x': return x;
    case 'y': return y;
    case 'm': return -ev(n->l, x, y);
    case 'f': return FN[n->fn].fp(ev(n->l, x, y));
    case '+': return ev(n->l, x, y) + ev(n->r, x, y);
    case '-': return ev(n->l, x, y) - ev(n->r, x, y);
    case '*': return ev(n->l, x, y) * ev(n->r, x, y);
    case '/': return ev(n->l, x, y) / ev(n->r, x, y);
    case '^': return pow(ev(n->l, x, y), ev(n->r, x, y));
    }
    return 0;
}

/* ---------- utilitarios ---------- */
static int txt;
static void fail(const char *m) {
    if (txt) fprintf(stderr, "erro: %s\n", m); else printf("{\"ok\":false,\"erro\":\"%s\"}\n", m);
    exit(txt);
}
static void failf(const char *campo, const char *m) {
    char b[200]; snprintf(b, sizeof b, "%s: %s", campo, m); fail(b);
}
static double getd(const char *s, const char *campo) {
    char *e; double d = strtod(s, &e);
    while (isspace((unsigned char)*e)) e++;
    if (!*s || *e || !isfinite(d)) failf(campo, strchr(s, ',') ? "use ponto decimal, nao virgula (ex.: 0.5)" : "numero invalido");
    return d;
}
static N *getexpr(const char *s, const char *campo, int com_y) {
    N *r = parse(s, com_y);
    if (perr) failf(campo, perr);
    return r;
}
static void pj(double d) { if (isfinite(d)) printf("%.10g", d); else printf("null"); }

#define NM 6
static const char *nome[NM] = {"Taylor 1a ordem", "Taylor 2a ordem", "Euler Melhorado",
                               "Euler Modificado", "Runge-Kutta 4a", "Solucao Exata"};

static double rk4(const N *F, double x, double y, double h) {
    double k1 = ev(F, x, y), k2 = ev(F, x + h/2, y + h/2*k1), k3 = ev(F, x + h/2, y + h/2*k2), k4 = ev(F, x + h, y + h*k3);
    return y + h/6*(k1 + 2*k2 + 2*k3 + k4);
}

int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "--txt")) { txt = 1; argc--; argv++; }
    if (argc != 7 && argc != 8) fail("uso: edo [--txt] a b y(a) n f df [exata]");
    double a = getd(argv[1], "a"), b = getd(argv[2], "b"), y0 = getd(argv[3], "y(a)");
    char *e; long n = strtol(argv[4], &e, 10);
    if (!*argv[4] || *e) failf("n", "use um inteiro, sem virgula nem ponto (ex.: 11)");
    if (n < 2 || n > 10000) failf("n", "deve estar entre 2 e 10000");
    if (!(b > a)) failf("intervalo", "e preciso a < b");
    N *Fn = getexpr(argv[5], "f(x,y)", 1), *Dn = getexpr(argv[6], "df(x,y)", 1);
    int tem_ex = argc == 8 && *argv[7];
    N *En = tem_ex ? getexpr(argv[7], "solucao exata", 0) : NULL;

    double h = (b - a) / (n - 1);
    double (*v)[NM] = malloc(sizeof(double[NM]) * n);
    if (!v) fail("memoria");
    for (int m = 0; m < NM; m++) v[0][m] = y0;
    if (tem_ex) v[0][5] = ev(En, a, 0);

    for (int i = 0; i < n - 1; i++) {
        double x = a + i*h;
        double y1 = v[i][0], y2 = v[i][1], y3 = v[i][2], y4 = v[i][3];
        v[i+1][0] = y1 + h*ev(Fn, x, y1);
        v[i+1][1] = y2 + h*ev(Fn, x, y2) + h*h/2*ev(Dn, x, y2);
        double k = ev(Fn, x, y3);
        v[i+1][2] = y3 + h/2*(k + ev(Fn, x+h, y3 + h*k));
        v[i+1][3] = y4 + h*ev(Fn, x + h/2, y4 + h/2*ev(Fn, x, y4));
        v[i+1][4] = rk4(Fn, x, v[i][4], h);
        if (tem_ex) v[i+1][5] = ev(En, x + h, 0);
        else { /* referencia: RK4 com passo h/100 */
            double yr = v[i][5], hh = h/100;
            for (int s = 0; s < 100; s++) yr = rk4(Fn, x + s*hh, yr, hh);
            v[i+1][5] = yr;
        }
    }

    double emax[NM] = {0};
    for (int i = 0; i < n; i++)
        for (int m = 0; m < NM - 1; m++) {
            double er = fabs(v[i][m] - v[i][5]);
            if (!(er <= emax[m]) && !isnan(emax[m])) emax[m] = er; /* NaN/inf persistem */
        }

    if (txt) {
        printf("EDO | Nattan Silva Montel | Prof. Laudo Claumir | 23/11/2026\n");
        printf("[%g,%g] y(a)=%g n=%ld h=%g%s\n\n%-10s", a, b, y0, n, h, tem_ex ? "" : " (coluna exata = referencia RK4 h/100)", "x");
        for (int m = 0; m < NM; m++) printf("%-20s", nome[m]);
        puts("");
        for (int i = 0; i < n; i++) {
            printf("%-10.4f", a + i*h);
            for (int m = 0; m < NM; m++) printf("%-20.8g", v[i][m]);
            puts("");
        }
        printf("%-10s", "erro max");
        for (int m = 0; m < NM - 1; m++) printf("%-20.4e", emax[m]);
        puts("");
    } else {
        printf("{\"ok\":true,\"h\":"); pj(h); printf(",\"ref\":%d,\"emax\":[", !tem_ex);
        for (int m = 0; m < NM - 1; m++) { if (m) putchar(','); pj(emax[m]); }
        printf("],\"rows\":[");
        for (int i = 0; i < n; i++) {
            printf("%s[", i ? "," : ""); pj(a + i*h);
            for (int m = 0; m < NM; m++) { putchar(','); pj(v[i][m]); }
            putchar(']');
        }
        puts("]}");
    }
    free(v);
    return 0;
}
