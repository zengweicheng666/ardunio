#ifndef NCEINTERFACE_H
#define NCEINTERFACE_H

#include <QObject>
#include <QSerialPort>
#include <QThread>

#include "NCEMessage.h"

const int CS_ACCY_MEMORY = 0xEC00;
const int NUM_BLOCK = 16;
const int BLOCK_LEN = 16;
const int REPLY_LEN = 16;

struct AddressStruct {
    unsigned char byteL;
    unsigned char byteH;
};

union AddressUnion {
    unsigned int addressInt;
    AddressStruct addressStruct;
};

class SerialPortThread : public QThread
{
    Q_OBJECT
public:

    SerialPortThread(QObject *parent);
    ~SerialPortThread(void);
    virtual void run(void) Q_DECL_OVERRIDE;

    void setQuit(void)
    {
        quit();
        this->wait();
    }

signals:
    void newMessage(const NCEMessage &message);

public slots:
    void sendMessage(const NCEMessage &message);
    void timerProc(void);

private:
    void openPort(void);
    void pollRouteChanges(void);
    void processRouteBlock(const quint8 data, int blockIndex, int byteIndex);
    void sendMessageInternal(NCEMessage &message);

    QSerialPort *m_serialPort;
    quint8 m_nceBuffer[NUM_BLOCK * BLOCK_LEN]; // Copy of NCE CS accessory memory
    quint8 m_pollBuffer[NUM_BLOCK * BLOCK_LEN]; // place to store reply messages
    bool m_firstTime;
};

class NCEInterface : public QObject
{
    Q_OBJECT
public:
    explicit NCEInterface(QObject *parent = 0);
    ~NCEInterface(void);

    void setup(void);
    void postMessage(const NCEMessage &message);
    void sendMessage(const NCEMessage &message);

    static NCEInterface *instance(void);

signals:
    void sendMessageSignal(const NCEMessage &message);

public slots:
    void newMessageSlot(const NCEMessage &message);
    void routeStatusChanged(int routeID, bool isActive);
    void deviceStatusChanged(int deviceID, int status);

private:
    static NCEInterface *m_instance;

    SerialPortThread *m_pollThread;
};

#endif // NCEINTERFACE_H
