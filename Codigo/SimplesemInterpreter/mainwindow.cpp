#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->Tabla->hide();
    ui->traza->hide();
    ui->Codigo->hide();
    ui->botonEjecutar->hide();
    ui->botonPaso->hide();
    ui->botonReiniciar->hide();
    ui->botonCodigo->hide();
    ui->botonHilo0->hide();
    ui->botonHilo1->hide();
    ui->botonHilo2->hide();
    ui->botonHilo3->hide();
    ui->botonCompilar->hide();
    ui->Output->hide();
    ui->OutputLabel->hide();
    ui->botonLimpiar->hide();
    ui->menuBar->hide();
    ui->traza_o_codigo_Label->hide();
    ui->menuEjemplos->setEnabled(false);

    hilos.append(Hilo(0,0)); //Agrego el hilo principal, que se va a iniciliazar con IP = 0 e ID = 0;
    this->showMaximized(); //Maximizo la ventana
    esperarHilos = false;

    connect(ui->actionParallel_for, SIGNAL(triggered()), this, SLOT(cargarEjParallel_For()));
    connect(ui->actionFactorial_2, SIGNAL(triggered()), this, SLOT(cargarEjFactorial()));
    connect(ui->actionM_ltiples_par_metros, SIGNAL(triggered()), this, SLOT(cargarEjMultiplesParametros()));
    connect(ui->actionM_ximo_Com_n_Divisor, SIGNAL(triggered()), this, SLOT(cargarEjMCD()));
    connect(ui->actionNumeros_primos, SIGNAL(triggered()), this, SLOT(cargarEjNumerosPrimos()));
    connect(ui->actionProblema_de_los_fil_sofos, SIGNAL(triggered()), this, SLOT(cargarEjFilosofos()));
    connect(&ejecutor, SIGNAL(print(QString)), this, SLOT(print(QString)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief Ejecuta las instrucciones hasta que se encuentra un halt o una instrucción en blanco (error).
 */
void MainWindow::ejecutar(){
    while(ui->botonPaso->isEnabled())
        ejecutarPaso();
}

/**
  * @brief Ejecuta solo una instrucción de cada hilo
  */
void MainWindow::ejecutarPaso(){

    int hiloID = 0;
    //Para cada hilo de la lista de hilos ejecuto una instrucción.

    while(hiloID < hilos.size()){
        ejecutarPasoHilo(hiloID);
        hiloID++;
    }

}

/**
  * @brief Ejecuta una instrucción del hilo dado.
  * @param hiloID ID del hilo del cual se quiere ejecutar una instrucción
  */
void MainWindow::ejecutarPasoHilo(int hiloID){
    QString instruccionSeparada[3] = {""};

    //En columna voy a guardar la columna donde está el código que debo ejecutar. getColumnaDeHilo devuelve -1 si
    //el hilo terminó de ejecutar y no está mas la columna.
    int columna = getColumnaDeHilo(hiloID);

    //Si no hay mas instrucciones para ejecutar, o si tengo que esperar a que terminen los otros hilos no hago nada.
    if(ui->Tabla->item(hilos[hiloID].getIP(), columna) &&
       !(hilos[hiloID].getThreadID() == 0 && esperarHilos) &&
       (columna != -1)
      ){
        //Desmarco la ultima instruccion ejecutada
        ui->Tabla->item(hilos[hiloID].getUltimaInstruccion(), columna)->setSelected(false);

        //Marco la instrucción a ejecutar en la interfaz gráfica.
        ui->Tabla->item(hilos[hiloID].getIP(),columna)->setSelected(true);
        ui->Tabla->scrollToItem(ui->Tabla->item(hilos[hiloID].getIP(),columna));

        //Obtengo la instruccion
        QString instruccion = ui->Tabla->item(hilos[hiloID].getIP(),columna)->text();

        //Incremento el IP
        hilos[hiloID].setUltimaInstruccion(hilos[hiloID].getIP());
        hilos[hiloID].setIP(hilos[hiloID].getIP() + 1);

        //y ejecuto la instrucción
        parser.separarInstruccion(instruccion, instruccionSeparada, hiloID, hilos);

        //Imprimo la traza de instrucciones que ejecuté y lo paro al final para que no aparezcan instrucciones fuera de pantalla.
        ui->traza->append(instruccion);
        QTextCursor c =  ui->traza->textCursor();
        c.movePosition(QTextCursor::End);
        ui->traza->setTextCursor(c);

        //Verifico si debo crear hilos, y si debo hacerlo los creo.
        if(!instruccionSeparada[0].compare("processors")){
            crearHilos(instruccionSeparada[1].toInt());
            return;
        }

        //Verifico si es una instrucción parallel_for, y si lo es no hago nada
        if(!instruccionSeparada[0].compare("parallel_for"))
            return;

        //Verifico si es una instrucción barrier, y si lo es, espero que los otros hilos terminen.
        //El IP del hilo actual en este momento apunta a la instrucción siguiente al barrier. Lo que hacemos es decrementarlo
        //para que apunte a la instrucción barrier, y si TODOS los hilos están apuntando a la instrucción barrier,
        //incrementamos el IP de todos los hilos a la vez.
        if(!instruccionSeparada[0].compare("barrier")){
            hilos[hiloID].setIP(hilos[hiloID].getIP()-1);
            waitBarrier();
            return;
        }

        //Verifico si es una instrucción wait, para tomar las acciones necesarias.
        if(!instruccionSeparada[0].compare("wait")){
            //Veo si la variable por la que espera vale 0. Si vale 0, disminuyo en 1 el IP para mantenerme
            //en la misma instrucción. Si vale más de 0, disminuyo el valor de la variable.
            int fila = ejecutor.reducirExpresion(instruccionSeparada[1]);
            if(ui->Tabla->item(fila, ui->Tabla->columnCount()-1)->text().toInt() == 0){
                hilos[hiloID].setIP(hilos[hiloID].getIP()-1);
            }
            else
                ui->Tabla->setItem(
                                    fila, //Fila
                                    ui->Tabla->columnCount()-1,     //Columna
                                    new QTableWidgetItem(               //Dato
                                                        QString::number(ui->Tabla->item(
                                                                                        fila,
                                                                                        ui->Tabla->columnCount()-1
                                                                                       )->text().toInt() - 1)
                                                        )
                                  );
            return;
        }

        //Verifico si debo sacar el hilo porque termino de ejecutar la parte paralela.
        if(!instruccionSeparada[0].compare("end_par_for")){
            finalizarHilo(hilos[hiloID].getThreadID());
            return;
        }

        int newIP = ejecutor.procesar(instruccionSeparada, ui->Tabla);
        //Si procesar devuelve un valor mayor o igual a 0, ese valor indica el nuevo valor que debe tomar el IP.
        //Si devuelve -2, es porque se ejecutó la instrucción halt y debe detenerse la ejecución del programa.
        if(newIP >= 0)
            hilos[hiloID].setIP(newIP);

        else if(newIP == -2) //instrucción HALT ejecutada
            detenerEjecucion();
    }
}

/**
  * @brief Detiene la ejecución del programa, evitando presionar nuevamente los botones de ejecución
  */
void MainWindow::detenerEjecucion(){
    ui->botonEjecutar->setEnabled(false);
    ui->botonPaso->setEnabled(false);
    ui->botonHilo0->setEnabled(false);
    ui->botonHilo1->setEnabled(false);
    ui->botonHilo2->setEnabled(false);
    ui->botonHilo3->setEnabled(false);
}

/**
  * @brief Crea hilos que se representan como columnas en la interfaz gráfica, y los agrega a la lista de hilos.
  * @param cantHilos Cantidad de hilos que deben ser creados.
  */
void MainWindow::crearHilos(int cantHilos){
    for(int columna = 1; columna < cantHilos; columna++){
        //Creo el nuevo hilo que se ejecutará y lo agrego a la lista de hilos en ejecución.
        hilos.append(Hilo(columna, hilos[0].getIP()));

        //Creo la nueva columna que representará al nuevo hilo.
        ui->Tabla->insertColumn(columna);
        ui->Tabla->setHorizontalHeaderItem(columna, new QTableWidgetItem(QString("Hilo ").append(QString::number(columna))));

        //Habilito el boton correspondiente
        mostrarBotonHilo(columna, true);

        //Lleno la columna con las instrucciones del segmento de código.
        for(int fila = 0; ui->Tabla->item(fila,0); fila++){
            QTableWidgetItem* nuevoItem = new QTableWidgetItem(*ui->Tabla->item(fila,0));
            ui->Tabla->setItem(fila, columna, nuevoItem);
        }
    }
}

/**
  * @brief Finaliza un hilo, elimina la columna de la interfaz gráfica, y reacomoda el identificador de los hilos restantes
  * @param Hilo que debe finalizarse. No puede finalizarse el hilo principal (0), por lo que si se intenta esto, no se hace nada.
  */
void MainWindow::finalizarHilo(int hiloID){

    //Si se intenta finalizar el hilo principal porque se alcanzó la instrucción end_par_for, se verifica si hay otros
    //hilos en ejecución. Si no los hay (solo hay 2 columnas, la de código del hilo principal y la de datos), solo vuelvo,
    //pero si hay al menos un hilo en ejecución debo esperar a que termine.
    if(hiloID == 0){

        //Si ya terminaron todos los hilos en paralelo, solo dejamos el hilo principal.
        if(ui->Tabla->columnCount() == 2){
            for(int i = 1; i < hilos.size(); i++)
                hilos.removeAt(i);
            return;
        }
        else{
            esperarHilos = true;
            return;
        }
    }

    //Remuevo la columna y el boton de la GUI;
    ui->Tabla->removeColumn(getColumnaDeHilo(hiloID));
    mostrarBotonHilo(hiloID, false);

    //Verifico si el que finalizó era el ultimo hilo
    if(ui->Tabla->columnCount() == 2){
        esperarHilos = false;
    }
}

/**
  * @brief Muestra u oculta un boton de los que permiten la ejecución del código de un hilo.
  * @param botonHilo Hilo del cual se desea mostrar u ocultar el boton.
  * @param mostrar Determina si el boton va a mostrarse (true) u ocultarse (false)
  */
void MainWindow::mostrarBotonHilo(int botonHilo, bool mostrar){
    switch(botonHilo){
        case 0:
            ui->botonHilo0->setVisible(mostrar);
            break;

        case 1:
            ui->botonHilo1->setVisible(mostrar);
            break;

        case 2:
            ui->botonHilo2->setVisible(mostrar);
            break;

        case 3:
            ui->botonHilo3->setVisible(mostrar);
            break;
    }
}

/**
  * @brief Dado un hiloID, obtiene la columna que contiene el código del hilo en la interfaz gráfica
  * @param hiloID ID del hilo del cual se desea obtener la columna que contiene el código
  * @return Columna que contiene el código del hilo correspondiente. Devuelve -1 si no se encontra la columna del hilo, lo cual puede ocurrir si el hilo ya finalizó la ejecución.
  */
int MainWindow::getColumnaDeHilo(int hiloID){
    QString cabecera;

    for(int i = 0; i < ui->Tabla->columnCount() - 1; i++){
        cabecera = ui->Tabla->horizontalHeaderItem(i)->text();
        cabecera.remove("Hilo ");
        if(!cabecera.compare(QString::number(hiloID)))
            return i;
    }

    return -1;
}

/**
  * @brief Chequea si todos los hilos están en la instrucción barrier. Mientras un hilo esté en la instrucción\
  * barrier, su IP apuntará a la instrucción barrier. Cuando todos estén en barrier, se incrementa el IP de todos los hilos.
  */
void MainWindow::waitBarrier(){
    bool todosBarrier = true;
    QString instruccion;
    for(int i = 0; i < hilos.size(); i++){
        instruccion = ui->Tabla->item(hilos[i].getIP(),getColumnaDeHilo(i))->text();
        if(instruccion.compare("barrier")){
            todosBarrier = false;
            break;
        }
    }

    //Si todos están en barrier, incremento el IP de todos los hilos en 1 para que apunten a la siguiente instrucción.
    if(todosBarrier){
        for(int i = 0; i < hilos.size(); i++)
            hilos[i].setIP(hilos[i].getIP() + 1);
    }
    return;
}

/**
  * @brief Intercambia el entorno para mostrar el código fuente o la traza.
  * @param mostrar Determina si se muestra el entorno para escribir código fuente (true) o para ver la traza (false
  */
void MainWindow::mostrarCodigo(bool mostrarCodigo){
    if(mostrarCodigo){
        ui->traza->hide();
        ui->Codigo->show();
        ui->botonCompilar->show();
        ui->botonCodigo->setText("Traza");
        ui->traza_o_codigo_Label->setText(QString::fromLatin1("Código:"));
    }
    else{
        ui->traza->show();
        ui->Codigo->hide();
        ui->botonCompilar->hide();
        ui->botonCodigo->setText("Codigo");
        ui->traza_o_codigo_Label->setText("Traza:");
    }

}

/************************** SLOTS ******************************/

void MainWindow::on_botonEjecutar_clicked(){
    ejecutar();
}

void MainWindow::on_botonPaso_clicked(){
    ejecutarPaso();
}

void MainWindow::on_portada_clicked()
{
    ui->Tabla->show();
    mostrarCodigo(true);
    ui->botonEjecutar->show();
    ui->botonPaso->show();
    ui->botonReiniciar->show();
    ui->botonHilo0->show();
    ui->botonCodigo->show();
    ui->Output->show();
    ui->OutputLabel->show();
    ui->botonLimpiar->show();
    ui->menuBar->show();
    ui->traza_o_codigo_Label->show();
    ui->menuEjemplos->setEnabled(true);

    ui->portada->hide();
}

/**
  * @brief Reinicia el intérprete a un estado inicial.
  */
void MainWindow::on_botonReiniciar_clicked()
{
    //Primero deberí­a eliminar todas las columnas que representan a los hilos que no son el principal.
    for(int hilo = 1; hilo < ui->Tabla->columnCount() + 1; hilo++){
        ui->Tabla->removeColumn(hilo);
    }

    //Luego vacio la lista de hilos, y creo un nuevo hilo principal.
    hilos.clear();
    hilos.append(Hilo(0,0));

    //Vací­o el segmento de datos
    ui->Tabla->removeColumn(1);
    ui->Tabla->insertColumn(1);
    if(ui->Tabla->columnCount() == 3)
        ui->Tabla->removeColumn(2);

    //Cambio la cabecera y creo en todos los items de la columna de datos, un item vací­o.
    ui->Tabla->setHorizontalHeaderItem(1, new QTableWidgetItem("Datos"));
    for(int fila = 0; fila < 256; fila++){
        ui->Tabla->setItem(fila, 1, new QTableWidgetItem());
    }

    //Habilito los botones, por si estaban deshabilitados
    ui->botonEjecutar->setEnabled(true);
    ui->botonPaso->setEnabled(true);
    ui->botonHilo0->setEnabled(true);
    ui->botonHilo1->setEnabled(true);
    ui->botonHilo2->setEnabled(true);
    ui->botonHilo3->setEnabled(true);

    //Oculto todos los botones de los hilos por si estaban habilitados.
    ui->botonHilo1->hide();
    ui->botonHilo2->hide();
    ui->botonHilo3->hide();
}

/**
  * @brief Ejecuta una instrucción del hilo 0.
  */
void MainWindow::on_botonHilo0_clicked(){
    ejecutarPasoHilo(0);
}

/**
  * @brief Ejecuta una instrucción del hilo 1.
  */
void MainWindow::on_botonHilo1_clicked(){
    ejecutarPasoHilo(1);
}

/**
  * @brief Ejecuta una instrucción del hilo 2.
  */
void MainWindow::on_botonHilo2_clicked(){
    ejecutarPasoHilo(2);
}

/**
  * @brief Ejecuta una instrucción del hilo 3.
  */
void MainWindow::on_botonHilo3_clicked(){
    ejecutarPasoHilo(3);
}

/**
  * @brief Al presionar el boton Código, se muestra el área para escribir código fuente, y el boton para compilarlo
  */
void MainWindow::on_botonCodigo_clicked()
{
    //Si estamos viendo la traza y queremos cambiar al código.
    if(!ui->botonCodigo->text().compare("Codigo")){
        mostrarCodigo(true);
    }
    //Si estamos viendo el Código y queremos cambiar a la traza.
    else if(!ui->botonCodigo->text().compare("Traza")){
        mostrarCodigo(false);
    }

}

void MainWindow::on_botonCompilar_clicked()
{
    //Guardo el código en un archivo

#ifdef Q_OS_MAC
    QFile fileOutput(QCoreApplication::applicationDirPath().append("/source.c3p"));
#else
    QFile fileOutput("source.c3p");
#endif
    fileManager.fileWrite(&fileOutput, ui->Codigo->toPlainText());

    //Proceso el archivo con el parser
    ejecutor.compilar();

    //Pongo el código simplesem en la tabla.
#ifdef Q_OS_MAC
    fileManager.fileRead(new QFile(QCoreApplication::applicationDirPath().append("/SimpleSemSource.txt")), ui->Tabla);
#else
    fileManager.fileRead(new QFile("SimpleSemSource.txt"), ui->Tabla);
#endif

    //Reinicio el intérprete
    on_botonReiniciar_clicked();
}

/**
  * @brief Carga el código para ver el ejemplo del parallel_for
  */
void MainWindow::cargarEjParallel_For(){
    //Cargo el entorno para ver el código fuente.
    mostrarCodigo(true);

    ui->Codigo->setText(Ejemplos::getEjemploParallelFor());
}

/**
  * @brief Carga el código para ver el ejemplo de Numeros Primos
  */
void MainWindow::cargarEjNumerosPrimos(){
    //Cargo el entorno para ver el código fuente.
    mostrarCodigo(true);

    ui->Codigo->setText(Ejemplos::getEjemploNumerosPrimos());
}

/**
  * @brief Carga el código para ver el ejemplo de Factorial
  */
void MainWindow::cargarEjFactorial(){
    //Cargo el entorno para ver el código fuente.
    mostrarCodigo(true);

    ui->Codigo->setText(Ejemplos::getEjemploFactorial());
}

/**
  * @brief Carga el código para ver el ejemplo de Máximo Comíºn Divisor
  */
void MainWindow::cargarEjMCD(){
    //Cargo el entorno para ver el código fuente.
    mostrarCodigo(true);

    ui->Codigo->setText(Ejemplos::getEjemploMCD());
}

/**
  * @brief Carga el código para ver el ejemplo de una función que recibe míºltiples parámetros.
  */
void MainWindow::cargarEjMultiplesParametros(){
    //Cargo el entorno para ver el código fuente.
    mostrarCodigo(true);

    ui->Codigo->setText(Ejemplos::getEjemploMultiplesParametros());
}

/**
  * @brief Carga el código para ver el ejemplo de los filósofos con 4 filósofos.
  */
void MainWindow::cargarEjFilosofos(){
    //Cargo el entorno para ver el código fuente.
    mostrarCodigo(true);

    ui->Codigo->setText(Ejemplos::getEjemploFilosofos());
}

/**
  * @brief Imprime en el cuadro de salida
  * @param dato Cadena que se va a imprimir por el cuadro de salida.
  */
void MainWindow::print(QString dato){
    ui->Output->append(dato);
}

/**
  * @brief Limpia la consola de salida.
  */
void MainWindow::on_botonLimpiar_clicked()
{
    ui->Output->clear();
}
