#ifndef EJEMPLOS_H
#define EJEMPLOS_H

#include <QObject>
#include <QString>

class Ejemplos : public QObject
{
    Q_OBJECT
public:
    explicit Ejemplos(QObject *parent = 0);
    static QString getEjemploParallelFor();
    static QString getEjemploNumerosPrimos();
    static QString getEjemploFactorial();
    static QString getEjemploMCD();
    static QString getEjemploMultiplesParametros();
    static QString getEjemploFilosofos();

signals:
    
public slots:
    
};

#endif // EJEMPLOS_H
