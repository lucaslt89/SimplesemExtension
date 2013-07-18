#ifndef FILEREADER_H
#define FILEREADER_H

#include <QObject>
#include <QFile>
#include <QTableWidget>
#include <QTextStream>
#include <QIODevice>

class FileManager : public QObject
{
    Q_OBJECT
public:
    explicit FileManager(QObject *parent = 0);
    void fileRead(QFile* archivo, QTableWidget* tabla);
    void fileWrite(QFile* archivo, QString contenido);
    
signals:
    
public slots:
    
};

#endif // FILEREADER_H
