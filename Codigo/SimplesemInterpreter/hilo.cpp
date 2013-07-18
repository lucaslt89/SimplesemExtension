#include "hilo.h"

Hilo::Hilo()
{
    IP = 0;
    threadID = 0;
    ultimaInstruccion = 0;
}

Hilo::Hilo(int threadID, int startIP){
    setThreadID(threadID);
    setIP(startIP);
    setUltimaInstruccion(startIP);
}
