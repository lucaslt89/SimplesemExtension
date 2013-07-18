#include "ejemplos.h"

Ejemplos::Ejemplos(QObject *parent) :
    QObject(parent)
{
}

/**
  * @brief Ejemplo que muestra el uso de la estructura Parallel For
  * @return Cadena que contiene el ejemplo para colocar en un campo de texto.
  */
QString Ejemplos::getEjemploParallelFor()
{
    return QString("var: a range 1..8, b range 1..8, suma, i range 1..4, j;\n"
                   "program{\n"
                       "\tfor(j=0;j <= 7; j++){\n"
                           "\t\ta[j] = 1 + j;\n"
                           "\t\tb[j] = 2 + j;\n"
                       "\t}\n"

                       "\tpar_for 4 (i = 0;i <= 7;i++){\n"
                           "\t\tsuma = suma + a[i];\n"
                           "\t\tbarrier;\n"
                           "\t\tsuma = suma + b[i];\n"
                       "\t}\n"
                       "\tend_par_for;\n"
                       "\twrite(suma);\n"
                   "}");
}

/**
  * @brief Ejemplo que calcula los números primos entre 0 y un número ingresado por el usuario desde 0 a 200.
  * @return Cadena que contiene el ejemplo para colocar en un campo de texto.
  */
QString Ejemplos::getEjemploNumerosPrimos(){
    return QString( "var: esPrimo range 1..200,j, i, max;\n"
                    "program{\n"
                        "\tread(max);\n"
                        "\tmax--;\n"
                        "\tfor(j=0;j < max; j++){\n"
                            "\t\tesPrimo[j] = 1;\n"
                        "\t}\n\n"

                        "\tfor(j=2;j < max; j++){\n"
                            "\t\tif(esPrimo[j]==1){\n"
                                "\t\t\tfor(i=2*j;i < max; i=i+j){\n"
                                    "\t\t\t\tesPrimo[i]=0;\n"
                                "\t\t\t}\n"
                            "\t\t}\n"
                        "\t}\n"

                        "\tfor(j=0;j < max; j++){\n"
                            "\t\tif(esPrimo[j]==1){\n"
                                "\t\t\twrite(j);\n"
                            "\t\t}\n"
                        "\t}\n"
                    "}");
}

/**
  * @brief Ejemplo que calcula el factorial de un número.
  * @return Cadena que contiene el ejemplo para colocar en un campo de texto.
  */
QString Ejemplos::getEjemploFactorial(){
    return QString( "var: n;\n"
                    "fact()\n"
                    "var: loc;\n"
                    "{\n"
                        "\tif (n > 1){\n"
                            "\t\tloc = n;\n"
                            "\t\tn = n - 1;\n"
                            "\t\tloc = loc * fact();\n"
                            "\t\treturn loc;\n"
                        "\t}\n"
                        "\telse {\n"
                            "\t\treturn 1;\n"
                        "\t}\n"
                    "}\n\n"

                    "program{\n"
                        "\tread(n);\n"
                        "\tif (n >= 0){\n"
                            "\t\twrite(fact());\n"
                        "\t}\n"
                        "\telse{\n"
                            "\t\twrite(0);\n"
                        "\t}\n"
                    "}");
}

/**
  * @brief Ejemplo que calcula el máximo común divisor entre dos números.
  * @return Cadena que contiene el ejemplo para colocar en un campo de texto.
  */
QString Ejemplos::getEjemploMCD(){
    return QString( "var: numMay, numMin, resto;\n"
                    "program {\n"
                        "\tresto = 1;\n"
                        "\tread(numMay);\n"
                        "\tread(numMin);\n"
                        "\twhile(resto <> 0){\n"
                            "\t\twhile(numMay >= 0){\n"
                                "\t\t\tnumMay = numMay - numMin;\n"
                            "\t\t}\n"
                            "\t\tnumMay = numMay + numMin;\n"
                            "\t\tresto = numMay;\n"
                            "\t\tnumMay = numMin;\n"
                            "\t\tnumMin = resto;\n"
                        "\t}\n"
                        "\twrite(numMay);\n"
                    "}");
}

/**
  * @brief Ejemplo que permite ver el manejo de una función que recibe múltiples parámetros.
  * @return Cadena que contiene el ejemplo para colocar en un campo de texto.
  */
QString Ejemplos::getEjemploMultiplesParametros(){
    return QString( "var: n, j;\n"
                    "suma(num1, num2, num3, num4)\n"
                    "var: loc;\n"
                    "{\n"
                        "\tloc = -4;\n"
                        "\tnum1 = num1 + num2 + num3 + num4 + loc;\n"
                        "\treturn num1;\n"
                    "}\n\n"

                    "program{\n"
                        "\tn = 5;\n"
                        "\tj = 6;\n"
                        "\twrite(suma(n, 4, j, 3));\n"
                    "}");
}

/**
  * @brief Ejemplo que muestra el problema de los filósofos implementado en lenguaje C3P.
  * @return Cadena que contiene el ejemplo para colocar en un campo de texto.
  */
QString Ejemplos::getEjemploFilosofos(){
    return QString( "var: tenedor range 1..4, filosofo range 1..4, j;\n\n"

                    "program{\n"
                    "//Problema de los filosofos implementado con semaforos.\n\n"

                        "\t//Inicializacion de los semaforos (tenedores).\n"
                        "\tfor(j=0;j <= 3; j++){\n"
                            "\t\ttenedor[j] = 1;\n"
                        "\t}\n\n"

                        "\t//Cada filosofo toma el tenedor que esta a su izquierda y a su derecha.\n"
                        "\t//Estados:\n"
                        "\t//		x1 = Filosofo x Pensando\n"
                        "\t//		x0 = Filosofo x Comiendo\n"
                        "\tpar_for 4 (filosofo = 0;filosofo <= 3; filosofo++){\n"
                            "\t\twhile(1 > 0){\n"
                                "\t\t\tif(filosofo == 3){\n"
                                    "\t\t\t\t//Pensando\n"
                                    "\t\t\t\twrite(filosofo*10+1);\n\n"

                                    "\t\t\t\t//Toma los recursos\n"
                                    "\t\t\t\twait(tenedor[0]);\n"
                                    "\t\t\t\twait(tenedor[3]);\n\n"

                                    "\t\t\t\t//Comiendo\n"
                                    "\t\t\t\twrite(filosofo*10+0);\n\n"

                                    "\t\t\t\t//Libera los recursos\n"
                                    "\t\t\t\tnotify(tenedor[3]);\n"
                                    "\t\t\t\tnotify(tenedor[0]);\n"
                                "\t\t\t}\n"
                                "\t\t\telse{\n"
                                    "\t\t\t\t//Pensando\n"
                                    "\t\t\t\twrite(filosofo*10+1);\n\n"

                                    "\t\t\t\t//Toma los recursos\n"
                                    "\t\t\t\twait(tenedor[filosofo]);\n"
                                    "\t\t\t\twait(tenedor[filosofo+1]);\n\n"

                                    "\t\t\t\t//Comiendo\n"
                                    "\t\t\t\twrite(filosofo*10+0);\n\n"

                                    "\t\t\t\t//Libera los recursos\n"
                                    "\t\t\t\tnotify(tenedor[filosofo+1]);\n"
                                    "\t\t\t\tnotify(tenedor[filosofo]);\n"
                                "\t\t\t}\n"
                            "\t\t}\n"
                        "\t}\n"
                        "\tend_par_for;\n"
                    "}");
}
