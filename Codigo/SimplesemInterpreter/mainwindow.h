#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <filemanager.h>
#include <ejecutor.h>
#include <QRegExp>
#include <hilo.h>
#include <QDebug>
#include <QProcess>
#include <ejemplos.h>
#include <instructionparser.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_botonEjecutar_clicked();
    void on_botonPaso_clicked();
    void on_portada_clicked();
    void on_botonReiniciar_clicked();
    void on_botonHilo0_clicked();
    void on_botonHilo1_clicked();
    void on_botonHilo2_clicked();
    void on_botonHilo3_clicked();
    void on_botonCodigo_clicked();
    void on_botonCompilar_clicked();
    void on_botonLimpiar_clicked();

    void cargarEjParallel_For();
    void cargarEjNumerosPrimos();
    void cargarEjFactorial();
    void cargarEjMCD();
    void cargarEjMultiplesParametros();
    void cargarEjFilosofos();

    void print(QString dato);

private:
    Ui::MainWindow *ui;
    FileManager fileManager;
    bool esperarHilos;
    QList<Hilo> hilos;
    Ejecutor ejecutor;
    InstructionParser parser;

    void ejecutarPaso();
    void ejecutarPasoHilo(int hiloID);
    void ejecutar();
    void detenerEjecucion();
    void crearHilos(int cantHilos);
    void finalizarHilo(int hiloID);
    void mostrarBotonHilo(int botonHilo, bool mostrar);
    int getColumnaDeHilo(int hiloID);
    void waitBarrier();
    void mostrarCodigo(bool mostrarCodigo);
};

#endif // MAINWINDOW_H
