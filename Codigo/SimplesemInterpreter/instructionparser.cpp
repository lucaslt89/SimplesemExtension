#include "instructionparser.h"
#include <QRegExp>

InstructionParser::InstructionParser(QObject *parent) :
    QObject(parent)
{
}

/**
  * @brief Separa una instruccion y devuelve en instruccionSeparada un arreglo de la forma {Instruccion, Destino, Origen}.
  * @param instruccion Instruccion que debemos parsear.
  * @param instruccionParseada Array donde se va a guardar el arreglo
  * @param hiloEjecutor Hilo que contiene la instrucción que se desea ejecutar.
  * @param hilos Lista que contiene todos los hilos de la máquina Simplesem.
  */
void InstructionParser::separarInstruccion(QString instruccion, QString instruccionSeparada[3], int hiloEjecutor, QList<Hilo> hilos){

    /* Configuro la expresión regular para encontrar la operación */
    QString pattern = "\\w*";
    QRegExp operacion(pattern);

    //Matcheo el nombre de la operacion
    //QString::number(operacion.indexIn(instruccion));
    operacion.indexIn(instruccion);
    instruccionSeparada[0] = operacion.cap(0);


    /* Configuro la expresion regular para encontrar el destino */
    if(!instruccionSeparada[0].compare("jump") ||
       !instruccionSeparada[0].compare("processors") ||
       !instruccionSeparada[0].compare("wait")){
        pattern = "\\s.*";
    }
    else{
        pattern = "\\s.*,";
    }
    QRegExp destino(pattern);

    //Matcheo el destino y le remuevo la , del final, porque encuentra algo de la forma "destino,"
    destino.indexIn(instruccion);
    instruccionSeparada[1] = destino.cap(0).remove(QRegExp(",")).remove(0,1);


    /* Configuro la expresión regular para encontrar el orí­gen, que será buscado si no es un jump o un halt. */
    if(instruccionSeparada[0].compare("jump") || instruccionSeparada[0].compare("halt")){
        pattern = ",.*"; //Encuentro todo lo que esta despues de la coma
        QRegExp origen(pattern);

        //Matcheo el origen y remuevo la coma, los espacios y el \n del final.
        origen.indexIn(instruccion);
        instruccionSeparada[2] = origen.cap(0).remove(0,1).remove("\\n").remove(" ");
    }

    //Imprime la instruccion separada en el cuadro de traza.
    //for(int i = 0; i <= 2; i++)
    //    ui->traza->append(instruccionSeparada[i]);

    //Reemplazo las palabras ip y numHilo en el caso de que hagan falta.
    instruccionSeparada[1].replace("ip",QString::number(hilos[hiloEjecutor].getIP()), Qt::CaseInsensitive);
    instruccionSeparada[2].replace("ip",QString::number(hilos[hiloEjecutor].getIP()), Qt::CaseInsensitive);
    instruccionSeparada[1].replace("numHilo",QString::number(hilos[hiloEjecutor].getThreadID()), Qt::CaseInsensitive);
    instruccionSeparada[2].replace("numHilo",QString::number(hilos[hiloEjecutor].getThreadID()), Qt::CaseInsensitive);

    return;
}
