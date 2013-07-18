#ifndef HILO_H
#define HILO_H

class Hilo
{
private:
    int IP;
    int ultimaInstruccion;
    int threadID;
public:
    Hilo();
    Hilo(int threadID, int startIP);
    void setIP(int newIP) {IP = newIP;}
    void setUltimaInstruccion(int oldIP) {ultimaInstruccion = oldIP;}
    void setThreadID(int newThreadID){threadID = newThreadID;}
    int getIP() {return IP;}
    int getUltimaInstruccion() {return ultimaInstruccion;}
    int getThreadID() {return threadID;}
};

#endif // HILO_H
