#!/bin/bash
# 10 casos, do mais facil ao mais dificil. Mostra o erro maximo de cada metodo.
# Colunas: Taylor1 Taylor2 Melhorado Modificado RK4
gcc -Wall -Wextra edo.c -o edo -lm || exit 1
t() { echo "== $1"; shift; ./edo --txt "$@" | grep '^erro max'; }
t "1  y'=x"            0 2 0   11 "x"                  "1"                              "x^2/2"
t "2  y'=y"            0 1 1   11 "y"                  "y"                              "exp(x)"
t "3  y'=x+y"          0 2 0.5 21 "x+y"                "1+x+y"                          "1.5*exp(x)-x-1"
t "4  y'=y-x^2+1"      0 2 0.5 11 "y - x^2 + 1"       "-2*x + y - x^2 + 1"             "(x+1)^2 - 0.5*exp(x)"
t "5  y'=-2y+x"        0 2 1   21 "-2*y + x"           "1 - 2*(-2*y + x)"               "x/2 - 0.25 + 1.25*exp(-2*x)"
t "6  y'=y cos x"      0 3 1   31 "y*cos(x)"           "-y*sin(x) + y*cos(x)^2"         "exp(sin(x))"
t "7  y'=2xy"          0 1 1   21 "2*x*y"              "2*y + 4*x^2*y"                  "exp(x^2)"
t "8  y'=y/x+x"        1 3 2   21 "y/x + x"            "-y/x^2 + 1 + (y/x + x)/x"       "x*(1 + x)"
t "9  logistica"       0 5 0.1 51 "y*(1-y)"            "(1-2*y)*y*(1-y)"                "1/(1 + 9*exp(-x))"
t "10 rigida"          0 2 0   11 "-50*(y - cos(x))"   "-50*sin(x) - 50*(-50*(y - cos(x)))" "(2500*cos(x) + 50*sin(x))/2501 - 2500/2501*exp(-50*x)"
echo "== erros de entrada (devem recusar)"
for args in '0,5 2 0 11 y 1' '0 2 0 11.5 y 1' '0 2 0 1 y 1' '2 0 0 11 y 1' '0 2 0 11 2y 1' '0 2 0 11 y foo(x)' '0 2 0 11 (y+1 1' '0 2 0 11 y 1 y'; do
    echo "-- $args"; ./edo --txt $args 2>&1 | head -1
done
echo "== sem exata (referencia RK4 h/100), problema 4"
./edo --txt 0 2 0.5 11 "y - x^2 + 1" "-2*x + y - x^2 + 1" | grep '^erro max'
