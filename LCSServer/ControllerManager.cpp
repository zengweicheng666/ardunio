#include <QCoreApplication>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaMethod>
#include <QTimer>
#include <QPointer>

#include "ControllerManager.h"
#include "Database.h"
#include "NotificationServer.h"
#include "MessageBroadcaster.h"

class ControllerEntry
{
public:
    ControllerEntry(void) : m_serialNumber(-1), m_controllerID(-1), m_status(ControllerStatusUnknown) { }
    ControllerEntry(const ControllerEntry &other) { copy(other); }

    int getSerialNumber(void) const { return m_serialNumber; }
    void setSerialNumber(int value) { m_serialNumber = value; }
    int getControllerID(void) const { return m_controllerID; }
    void setControllerID(int value) { m_controllerID = value; }
    ControllerStatusEnum getStatus(void) const { return m_status; }
    void setStatus(ControllerStatusEnum value) { m_status = value; }
    QString getVersion(void) const { return m_version; }
    void setVersion(int major, int minor, int build)
    {
        m_version = QString("%1.%2.%3").arg(major).arg(minor).arg(build);
    }
    void operator = (const ControllerEntry &other) { copy(other); }

private:
    void copy(const ControllerEntry &other)
    {
        m_serialNumber = other.m_serialNumber;
        m_controllerID = other.m_controllerID;
        m_status = other.m_status;
    }

    int m_serialNumber;
    int m_controllerID;
    ControllerStatusEnum m_status;
    QString m_version;
};

ControllerManager * ControllerManager::m_instance = NULL;

ControllerManager::ControllerManager(QObject *parent)
    : QObject(parent),
      m_server(new QWebSocketServer(QStringLiteral("Controller Server"), QWebSocketServer::NonSecureMode, this)),
      m_pingTimer(NULL)
{
    connect(this, &ControllerManager::sendNotificationMessage, NotificationServer::instance(), &NotificationServer::sendNotificationMessage);
    connect(MessageBroadcaster::instance(), SIGNAL(newMessage(UDPMessage)), this, SLOT(newUDPMessage(UDPMessage)));
    if (m_server->listen(QHostAddress::Any, UdpPort + 1))
    {
        qDebug() << "Controller listening on port" << (UdpPort + 1);
        connect(m_server, &QWebSocketServer::newConnection,this, &ControllerManager::onNewConnection);
        connect(m_server, &QWebSocketServer::closed, this, &ControllerManager::connectionClosed);
        m_pingTimer = new QTimer(this);
        connect(m_pingTimer, &QTimer::timeout, this, &ControllerManager::pingSlot);
        m_pingTimer->setInterval(30000);
        m_pingTimer->start();
    }
}

ControllerManager::~ControllerManager()
{

}

ControllerManager *ControllerManager::instance()
{
    if(m_instance == NULL)
        m_instance = new ControllerManager(qApp);

    return m_instance;
}

QString ControllerManager::getControllerIPAddress(int serialNumber) const
{
    QString ipAddress;
    for(int x = 0; x < m_socketList.count(); x++)
    {
        if(m_socketList.value(x)->property("serialNumber").toInt() == serialNumber)
            ipAddress = m_socketList.value(x)->peerAddress().toString();
    }
    return ipAddress;
}

bool ControllerManager::sendMessage(const ControllerMessage &message)
{
    QJsonDocument doc;
    bool ret = true;
    QJsonObject obj(message.getObject());
    obj["transactionID"] = message.getTransactionID();
    doc.setObject(obj);
    m_messageMap[message.getTransactionID()] = message;

    QByteArray normalizedSignature = QMetaObject::normalizedSignature("sendMessageSlot(int, int, QString)");
    int methodIndex = this->metaObject()->indexOfMethod(normalizedSignature);
    QMetaMethod method = this->metaObject()->method(methodIndex);
    method.invoke(this,
                  Qt::QueuedConnection,
                  Q_ARG(int, message.getTransactionID()),
                  Q_ARG(int, message.getSerialNumber()),
                  Q_ARG(QString, QString(doc.toJson())));
    return ret;
}

int ControllerManager::getConnectionSerialNumber(int index) const
{
    int serialNumber(0);
    QWebSocket *socket = m_socketList.value(index);
    if(socket)
        serialNumber = socket->property("serialNumber").toInt();

    return serialNumber;
}

void ControllerManager::getConnectedInfo(int serialNumber, QString &version, ControllerStatusEnum &status)
{
    QList<ControllerEntry *> list = m_controllerMap.values();

    for(int x = 0; x < list.count(); x++)
    {
        if(list.value(x)->getSerialNumber() == serialNumber)
        {
            version = list.value(x)->getVersion();
            status = list.value(x)->getStatus();
            break;
        }
    }
//    for(int x = 0; x < m_socketList.count(); x++)
//    {
//        if(m_socketList.value(x)->property("serialNumber").toInt() == serialNumber)
//        {
//            version = m_socketList.value(x)->property("version").toInt();
//            status = ControllerStatusOnline;
//            break;
//        }
//    }
}

void ControllerManager::controllerResetting(long serialNumber)
{
    qDebug(QString("Controller %1 restarting!!!!!!!").arg(serialNumber).toLatin1());

    for(int x = 0; x < m_socketList.count(); x++)
    {
        QWebSocket *socket = m_socketList.value(x);
        if(socket->property("serialNumber").toInt() == serialNumber)
        {
            createAndSendNotificationMessage(serialNumber, ControllerStatusRestarting);
            socket->close();
        }
    }
    QList<ControllerEntry *> list = m_controllerMap.values();
    for(int x = 0; x < list.count(); x++)
    {
        if(list.value(x)->getSerialNumber() == serialNumber)
        {
            list.value(x)->setStatus(ControllerStatusRestarting);
            createAndSendNotificationMessage(serialNumber, ControllerStatusRestarting);
        }
    }
}

unsigned long ControllerManager::getSerialNumber(int controllerID)
{
    Database db;
    return db.getSerialNumber(controllerID);
}

void ControllerManager::sendMessageSlot(int transactionID, int serialNumber, const QString &data)
{
    for(int x = 0; x < m_socketList.count(); x++)
    {
        QWebSocket *socket = m_socketList.value(x);
        int socketSerialNumber = socket->property("serialNumber").toInt();
        if(socketSerialNumber == serialNumber)
        {
            int count = socket->sendTextMessage(data);
            if(socket->flush() == false)
            {
                m_messageMap.remove(transactionID);
                emit errorSendingMessage(m_messageMap.value(transactionID));
                socket->close();
            }
            qDebug(QString("ControllerManager::sendMessage length: %1  ret %2 ").arg(data.length()).arg(count).toLatin1());
        }
    }
}

void ControllerManager::onNewConnection(void)
{
    QWebSocket *socket = m_server->nextPendingConnection();
    connect(socket, &QWebSocket::textMessageReceived, this, &ControllerManager::processTextMessage);
    connect(socket, &QWebSocket::disconnected, this, &ControllerManager::connectionClosed);
    connect(this, &ControllerManager::pingSignal, socket, &QWebSocket::ping);
    connect(socket, &QWebSocket::pong, this, &ControllerManager::pongReply);
    m_socketList << socket;
    socket->setProperty("socketTimeout", QDateTime::currentDateTime().toTime_t());
}

void ControllerManager::connectionClosed(void)
{
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    int serialNumber = socket->property("serialNumber").toInt();
    if(serialNumber > 0)
    {
        bool found = false;
        for(int x = 0; x < m_socketList.count(); x++)
        {
            if(m_socketList.value(x) != socket && m_socketList.value(x)->property("serialNumber").toInt() == serialNumber)
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            qDebug(QString("Controller %1 disconnected.").arg(serialNumber).toLatin1());
            emit controllerRemoved(serialNumber);
            createAndSendNotificationMessage(serialNumber, ControllerStatusOffline);
        }
    }
    emit controllerDisconnected(m_socketList.indexOf(socket));
    m_socketList.removeAll(socket);
    socket->deleteLater();
}

void ControllerManager::processTextMessage(QString message)
{
    qDebug(QString("PROCESS TEXT MESSAGE: %1").arg(message).toLatin1());
    if(message.startsWith("ACK_"))
    {
        QStringList parts = message.split('_');
        int transactionID = parts.value(1).toInt();
        ControllerMessage message(m_messageMap.value(transactionID));
        m_messageMap.remove(transactionID);
        emit messageACKed(message);
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(message.toLatin1());
    QJsonObject root = doc.object();
    QString uri = root["messageUri"].toString();
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    int serialNumber = socket->property("serialNumber").toInt();

    if(uri == "/controller/connect")
    {
        serialNumber = root["serialNumber"].toInt();
        int version = root["version"].toInt();
        socket->setProperty("serialNumber", serialNumber);
        socket->setProperty("version", version);
        bool found = false;
        for(int x = 0; x < m_socketList.count(); x++)
        {
            if(m_socketList.value(x) != socket && m_socketList.value(x)->property("serialNumber").toInt() == serialNumber)
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            emit controllerAdded(serialNumber);
            createAndSendNotificationMessage(serialNumber, ControllerStatusConected);
            createAndSendNotificationMessage(serialNumber, ControllerStatusOnline);
        }
        emit controllerConnected(m_socketList.indexOf(socket));
    }
    else if(uri == "/controller/multiConfig")
    {
        QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
        sendMultiControllerConfig(serialNumber, socket);
    }
    else
    {
        NetActionType actionType = (NetActionType)root["action"].toInt();
        int address = root["address"].toInt();
        DeviceClassEnum classCode = (DeviceClassEnum)(int)root["class"].toInt();

        emit newMessage(serialNumber, address, classCode, actionType, uri, root);
    }
}

void ControllerManager::sendControllerInfo(int serialNumber, QWebSocket *socket)
{
    qDebug(QString("handleController: controller %1").arg(serialNumber).toLatin1());
    QString controllerName;
    int controllerID;
    Database db;
    db.getControllerIDAndName(serialNumber, controllerID, controllerName);
    QJsonDocument jsonDoc;
    QJsonObject obj;
    obj["messageUri"] = "/controller/name";
    obj["action"] = (int)NetActionUpdate;
    obj["controllerID"] = controllerID;
    obj["controllerName"] = controllerName;
    QString sql;
    sql = QString("SELECT device.id as deviceID, deviceName FROM  device JOIN controllerModule ON device.controllerModuleID = controllerModule.id  WHERE controllerID = %1").arg(controllerID);
    QJsonArray a = db.fetchItems(sql);
    obj["devices"] = a;
    jsonDoc.setObject(obj);
    socket->sendTextMessage(jsonDoc.toJson());
    qDebug(jsonDoc.toJson());
}

void ControllerManager::sendMultiControllerConfig(int serialNumber, QWebSocket *socket)
{
    qDebug(QString("sendMultiControllerConfig: controller %1").arg(serialNumber).toLatin1());

    Database db;
    QString returnPayload = db.getMultiControllerConfig(serialNumber);
    socket->sendTextMessage(returnPayload);
    qDebug(returnPayload.toLatin1());
}

void ControllerManager::pongReply(quint64 length, const QByteArray &)
{
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    if(socket)
    {
        socket->setProperty("socketTimeout", QDateTime::currentDateTime().toTime_t());
        //        QString txt = QString("Pong reply from %1  Total time: %2").arg(socket->peerAddress().toString()).arg(length);
        //        qDebug(txt.toLatin1());
        int serialNumber = socket->property("serialNumber").toInt();
        createAndSendNotificationMessage(serialNumber, ControllerStatusOnline, length);
        emit controllerPing(m_socketList.indexOf(socket), length);
    }
}

void ControllerManager::pingSlot()
{
    for(int x = 0; x < m_socketList.count(); x++)
    {
        uint timeout = m_socketList.value(x)->property("socketTimeout").toUInt();
        timeout = QDateTime::currentDateTime().toTime_t() - timeout;
        if(timeout > 60)
            m_socketList.value(x)->close();
    }
    QByteArray data;
    emit pingSignal(data);
}

void ControllerManager::createAndSendNotificationMessage(int serialNumber, ControllerStatusEnum status, quint64 pingLength)
{
    QString uri("/api/notification/controller");
    QJsonObject obj;
    obj["serialNumber"] = QString("%1").arg(serialNumber);
    obj["status"] = QString("%1").arg(status);
    obj["pingLength"] = QString("%1").arg(pingLength);

    emit sendNotificationMessage(uri, obj);
}


void ControllerManager::newUDPMessage(const UDPMessage &message)
{
    if(message.getMessageID() == SYS_CONTROLLER_ONLINE)
    {
        int controllerID = message.getSerialNumber();
        int majorVersion = message.getField(5);
        int minorVersion = message.getField(6);
        int build = message.getField(7);
        int serialNumber = this->getSerialNumber(controllerID);

        ControllerEntry *entry = m_controllerMap.value(controllerID);
        if(entry == NULL)
           entry = new ControllerEntry;
        entry->setControllerID(controllerID);
        entry->setSerialNumber(serialNumber);
        entry->setStatus(ControllerStatusConected);
        entry->setVersion(majorVersion, minorVersion, build);
        m_controllerMap[controllerID] = entry;

        emit controllerAdded(serialNumber);
        createAndSendNotificationMessage(serialNumber, ControllerStatusConected);
    }
    int serialNumber = message.getSerialNumber();
    QList<ControllerEntry *> list = m_controllerMap.values();
    for(int x = 0; x < list.count(); x++)
    {
        if(list.value(x)->getSerialNumber() == serialNumber)
        {
            list.value(x)->setStatus(ControllerStatusRestarting);
            createAndSendNotificationMessage(serialNumber, ControllerStatusRestarting);
        }
    }
}
