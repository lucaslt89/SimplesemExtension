#ifndef INSTRUCTIONPARSER_H
#define INSTRUCTIONPARSER_H

#include <QObject>
#include <hilo.h>

class InstructionParser : public QObject
{
    Q_OBJECT
public:
    explicit InstructionParser(QObject *parent = 0);
    void separarInstruccion(QString instruccion, QString instruccionSeparada[3], int hiloEjecutor, QList<Hilo> hilos);
    
signals:
    
public slots:
    
};

#endif // INSTRUCTIONPARSER_H
