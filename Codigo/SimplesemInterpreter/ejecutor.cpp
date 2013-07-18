#include "ejecutor.h"
#include <QProcess>
#include <QCoreApplication>
#include <QDebug>

#define PERL_PATH "perl"

Ejecutor::Ejecutor() : QObject()
{
    columnaDatos = 1;
}

/**
  * @brief Interpreta una instruccion que llega en un arreglo de 3 QStrings que contienen: instruccion, destino, origen
  * @param instruccion_destino_origen Es el arreglo de 3 elementos que contiene los datos en el orden indicado, como QStrings
  * @param tabla Es la tabla que contiene el mapa de memoria del interprete, para poder escribir sobre el segmento de datos el resultado
  * @return Devuelve el nuevo valor que debe tomar el IP luego de procesar la instruccion. Si no debe cambiarse, se devuelve -1.
  *         Si la instrucción es HALT, se devuelve -2.
  */
int Ejecutor::procesar(QString instruccion_destino_origen[3], QTableWidget *tabla){
    this->tabla = tabla;

    //Primero obtengo la columna donde están los datos.
    for(columnaDatos = 1; columnaDatos < tabla->columnCount(); columnaDatos++)
        if(!tabla->horizontalHeaderItem(columnaDatos)->text().compare("Datos"))
            break;

    /*------------ Reduccion del destino ------------*/
    int dest;
    QString destino = instruccion_destino_origen[1];
    dest = reducirExpresion(destino);

    /*------------ Reduccion del origen ------------*/
    int source;
    QString origen = instruccion_destino_origen[2];
    source = reducirExpresion(origen);

    //Ahora tenemos tanto la expresión original (para saber si era un read o un write, o cualquier otra cosa,
    //como la expresión resuelta, en el caso de que tengamos que reducirla a un valor, asi que ya podemos ejecutar

    /* Instruccion SET */
    if(!instruccion_destino_origen[0].compare("set", Qt::CaseInsensitive)){
        //Primero tengo que ver si el origen o el destino es read o write, respectivamente, ya que de ser así­ tengo
        //que pedir entrada de usuario, o escribir en la consola.
        instruccion_destino_origen[1].compare("write", Qt::CaseInsensitive) ? \
            instruccion_destino_origen[1] = QString::number(dest) : instruccion_destino_origen[1] = "write";

        //Si en el origen hay una cadena con comillas, es porque tenemos que escribir en pantalla el mensaje de origen,
        //por lo que no deberí­amos modificarlo, por eso antes de modificarlo, preguntamos si el origen tiene comillas.
        //Es decir que el origen solo cambia si era una expresión, en cuyo caso me pone el resultado de la expresión.
        if(!instruccion_destino_origen[2].contains("\"")){
            instruccion_destino_origen[2].compare("read", Qt::CaseInsensitive) ? \
                instruccion_destino_origen[2] = QString::number(source) : instruccion_destino_origen[2] = "read";
        }
        set(instruccion_destino_origen[1], instruccion_destino_origen[2]);

        return -1;
    }

    /* Instruccion JUMP */
    if(!instruccion_destino_origen[0].compare("jump", Qt::CaseInsensitive)){
        return jump(QString::number(dest));
    }

    /* Instruccion JUMPT */
    //JUMPT es de la forma jumpt destino, condicion. Cuando se hace la reducción del origen (en este caso condicion)
    //se obtiene en "origen" el resultado de la expresión lógica, ya que luego de reducirse la expresión de que tiene
    //partes de la forma D[indice], se evalua la operacion lógica o matemática resultante, por lo que en orí­gen
    //quedará true(!=0) si la condición es verdadera, o false(0) en caso contrario.
    if(!instruccion_destino_origen[0].compare("jumpt", Qt::CaseInsensitive)){
        int retorno = jumpt(dest, source);
        return retorno;
    }

    /* Instruccion HALT */
    if(!instruccion_destino_origen[0].compare("halt", Qt::CaseInsensitive)){
        return -2;
    }

    return 0;
}

/**
  * @brief Implementa la función SET de simplesem
  * @param destino QString que debe ser evaluado para obtener el destino de la instruccion
  * @param origen QString que debe ser evaluado para obtener el origen de la instruccion
  */
void Ejecutor::set(QString destino, QString origen){
    int indiceDS;
    QString datoAManipular;

    /*--------- Trabajo con el ORIGEN ---------*/
    //Si solicito entrada del usuario, tengo que crear una ventanita para que introduzca el valor.
    if(!origen.compare("read"))
        datoAManipular = QInputDialog::getText(tabla, "Solicitud de entrada de usuario", "Numero", QLineEdit::Normal);
    else
        datoAManipular = origen;

    /*--------- Trabajo con el DESTINO ---------*/
    //Si el destino es write, tengo que imprimir el dato en pantalla, sino tengo que guardar el dato en el lugar correspondiente.
    if(!destino.compare("write")){
        emit print(datoAManipular);
        //qDebug("%s", datoAManipular.toAscii().data());
    }
    else{
        indiceDS = destino.toInt();
        tabla->setItem(indiceDS, columnaDatos, new QTableWidgetItem(datoAManipular));
    }

}

/**
  * @brief Implementa la función JUMP de simplesem
  * @param destino QString que debe ser evaluado para obtener el destino de la instruccion
  */
int Ejecutor::jump(QString destino){
    return destino.toInt();
}

/**
  * @brief Implementa la función JUMPT de simplesem
  * @param destino Entero que representa el valor que tomará el IP si la condición es verdadera
  * @param condicion Entero que representa el resultado de la condición
  */
int Ejecutor::jumpt(int destino, int condicion){
    //Si la condición es verdadera, devolvemos el valor de destino en un entero, que será el nuevo valor que tenga
    //que tomar el IP, sino, devolvemos -1, que es el valor para indicar que no se debe cambiar el IP
    if(condicion)
        return destino;
    else
        return -1;
}

/**
  * @brief Obtiene el valor numérico del segmento de datos a partir de una expresion que contiene algo de la forma D[indice]
  * @param expresionDS QString que contiene una expresion de la forma D[indice]
  * @return Devuelve el valor al que se convirtió la expresión de la forma D[indice]
  */
int Ejecutor::DStoInt(QString expresionDS){
    QString indice; //Guardo el í­ndice en un string, para despues evaluar la operacion matemática y obtener el número del indice.

    //Con esta expresión regular, obtengo solo lo de adentro, es decir la operacion que me da el í­ndice.
    QString pattern = "\\[.*\\]";
    QRegExp rx(pattern);
    rx.indexIn(expresionDS);
    indice = rx.cap(0);

    //Puede ocurrir que en la expresión tengamos 2 signos - seguidos, en cuyo caso se reemplaza por un signo +.
    indice.replace(QRegExp("\\-\\s*\\-"), "+");

    //Hasta ahora en í­ndice tengo una expresion matemática cuyo resultado es el verdadero í­ndice, asi que evaluamos esa
    //expresion matematica.
    QScriptEngine engine;
    indice = engine.evaluate(indice).toString();
    //Finalmente, trunco toda la parte decimal para que el método toInt() de la clase QString funcione correctamente.
    indice.remove(QRegExp("\\.\\w*"));

    return tabla->item(indice.toInt(), columnaDatos)->text().toInt();
}

/**
  * @brief Reduce una expresión logica o matematica que contiene referencias al segmento de datos, de la forma D[indice].
  * @param expresion Expresión que se desea reducir. La reducción consta de tomar una expresión que puede tener referencias
  * al segmento de datos (D[indice]) y devolver el resultado de dicha expresión.
  * @return Devuelve el resultado de la expresión en un entero.
  */
int Ejecutor::reducirExpresion(QString expresion){
    //Debe recibir la expresión y reducirla
    //El algoritmo es el siguiente: Supongamos el caso más complejo -> D[ D  [4 + 6] - 3 ] + 4
    //Necesitamos obtener primero el valor que está en D[10], luego el que esta en D[7], y finalmente a eso sumarle 4.
    //Resolviendo de adentro hacia afuera, usamos una expresión regular que encuentre:
    //D seguido de espacios en blanco, seguido de "[", seguido de un conjunto de simbolos no caracter, seguido de "]"
    //La expresion que resuelve eso es D\s*\[[^A-z]*\], lo que la primera vez nos devuelve "D  [4 + 6]".
    //Ahora buscamos el valor de D[10] con la funcion DStoInt, y suponiendo que ese valor sea 4, reemplazamos en la expresion
    //y vamos a tener ahora D[4-3] + 4, porque lo que nuevamente tenemos que matchear con D\s*\[[^A-z]*\] para obtener D[4-3]
    //Cuando ya no encontremo
    //A la salida del while tengo en destino una expresión matemáticas más matches de la regexp D\s*\[[^A-z]*\], evaluamos la expresion.

    int indiceDeMatcheo, longitudDeMatcheo, datoDelDS; //Variables utilizadas para reemplazar el QString D[indice] por el numero encontrado e ir reduciendo la expresión.

    QString pattern = "D\\s*\\[[^A-z]*\\]";
    QRegExp rx(pattern); //Expresion regular para buscar en el QString destino.

    //Busco si hay alguna expresion del tipo D[expresion_matematica]
    indiceDeMatcheo = rx.indexIn(expresion); //Devuelve -1 si no se encontro nada, y el indice si se encontró algo que coincida con la regexp
    longitudDeMatcheo = rx.matchedLength();
    //Mientras se encuentre algo de la forma D[operacion], resuelvo la operacion, obtengo el valor del segmento de datos,
    //y lo reemplazo en el string.
    while(indiceDeMatcheo != -1){
        datoDelDS = DStoInt(rx.cap(0));
        expresion.replace(indiceDeMatcheo, longitudDeMatcheo, QString::number(datoDelDS));
        indiceDeMatcheo = rx.indexIn(expresion);
        longitudDeMatcheo = rx.matchedLength();
    }

    //Puede ocurrir que en la expresión tengamos 2 signos - seguidos, en cuyo caso se reemplaza por un signo +.
    expresion.replace(QRegExp("\\-\\s*\\-"), "+");

    //A la salida del while tengo en destino una expresión matemática o logica, libre de expresiones del tipo D[indice],
    //asi que la resuelvo y obtengo en destino un número.
    QScriptEngine engine;
    expresion = engine.evaluate(expresion).toString();
    //Finalmente, trunco toda la parte decimal para que el método toInt() de la clase QString funcione correctamente.
    expresion.remove(QRegExp("\\.\\w*"));

    //Si es una operacion lógica, en expresion queda el resultado (true o false). La conversion de algo que no sea
    //numerico a entero, devuelve siempre 0, por lo que debemos verificar si es verdadera la condición, para devolver 1.
    if(!expresion.compare("true"))
        return 1;

    return expresion.toInt();
}

/**
 * @brief Metodo que ejecuta el compilador. Previamente debe crearse el archivo source.c3p en el mismo directorio donde se
 * encuentra el parser. Este archivo debe contener el codigo fuente en lenguaje C3P que desea ser compilado.
 */
void Ejecutor::compilar()
{
    QString program = PERL_PATH;
    QStringList argument;

#ifdef Q_OS_MAC
    argument << QCoreApplication::applicationDirPath().append("/parser.pl");
#else
    argument << "parser.pl";
#endif

    QProcess* process = new QProcess();
    process->start(program, argument);

    process->waitForFinished(30000);
}
