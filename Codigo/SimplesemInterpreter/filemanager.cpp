#include "filemanager.h"

FileManager::FileManager(QObject *parent) :
    QObject(parent)
{

}

/**
 * @brief Lee un archivo con las instrucciones simplesem y las coloca en la tabla
 * @param archivo Archivo que contiene las instrucciones simplesem
 * @param tabla Tabla donde se colocarán las instrucciones.
 */
void FileManager::fileRead(QFile* archivo, QTableWidget* tabla){
    if(!archivo->open(QIODevice::ReadOnly)){
        qDebug("Error al abrir el archivo de lectura, verifique que el archivo exista");
        return;
    }
    tabla->clearContents();
    QTextStream stream(archivo);
    QString linea;
    int i = 0;

    while(!stream.atEnd()){
        linea = stream.readLine();
        tabla->setItem(i,0, new QTableWidgetItem(linea));
        i++;
    }
}

/**
 * @brief Escribe una cadena de texto en un archivo
 * @param archivo Destino donde se escribirá la cadena de texto.
 * @param contenido Cadena de texto que se desea guardar.
 */
void FileManager::fileWrite(QFile* archivo, QString contenido)
{
    archivo->open(QIODevice::ReadWrite);
    QTextStream outStream(archivo);
    outStream << contenido;
    archivo->close();
    return;
}
