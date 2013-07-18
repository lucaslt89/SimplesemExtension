#ifndef EJECUTOR_H
#define EJECUTOR_H

#include <QString>
#include <QRegExp>
#include <QTableWidget>
#include <QScriptEngine>
#include <QInputDialog>

class Ejecutor : public QObject
{
    Q_OBJECT
public:
    Ejecutor();
    int procesar(QString instruccion_destino_origen[3], QTableWidget* tabla);
    int reducirExpresion(QString expresion);
    void compilar();

private:
    void set(QString destino, QString origen);
    int jump(QString destino);
    int jumpt(int destino, int condicion);
    int DStoInt(QString expresionDS);

    QTableWidget* tabla; //Apunta al mapa de memoria con el que se va a trabajar
    int columnaDatos; //Contiene la columna donde estaran los datos

signals:
    void print(QString dato);
};

#endif // EJECUTOR_H
