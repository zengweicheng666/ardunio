#ifndef API_H
#define API_H

#include <QObject>
#include <QWebSocket>
#include <QUdpSocket>
#include <QTimer>

#include "GlobalDefs.h"
#include "Entity.h"

class UDPMessage;

class API : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QString serverAddress READ getServerAddress WRITE setServerAddress NOTIFY serverAddressChanged)
    Q_PROPERTY(int serverPort READ getServerPort WRITE setServerPort NOTIFY serverPortChanged)
    Q_PROPERTY(bool apiReady READ getApiReady NOTIFY apiReady)

    explicit API(const QString &server, const int port, QObject *parent = nullptr);
    explicit API(QObject *parent = nullptr);
    ~API(void);

    static API *instance(void);

    QString getServerAddress(void) const { return m_server; }
    void setServerAddress(const QString &value) { m_server = value; emit serverAddressChanged(); emit apiReady(); }
    int getServerPort(void) const { return m_port; }
    void setServerPort(int value) { m_port = value; emit serverPortChanged(); }

signals:
    void controllerChanged(int serialNumber, ControllerStatus status, quint64 pingLength);
    void deviceChanged(int deviceID, int status);
    void apiReady(void);
    void serverAddressChanged(void);
    void serverPortChanged(void);
    void routeChanged(int routeID, bool isActive);

public slots:
    void activateTurnout(int deviceID, int newState);
    void activateRoute(int routeID);
    QString getControllerList(void);
    QString getControllerModuleList(int controllerID);
    QString getDeviceList(ClassEnum deviceType = ClassUnknown);
    QString getSignalAspectList(int deviceID);
    QString getSignalConditionList(int aspectID);
    QString getRouteList(void);
    QString getRouteEntryList(int routeID);

    Entity saveEntity(const Entity &entity, bool isNew = false);
    Entity deleteEntity(const Entity &entity);

    bool getApiReady(void);

    void restartController(int serialNumber);
    void sendControllerConfig(int serialNumber);

protected slots:
    void textMessageReceived(const QString &message);
    void notificationStateChanged(QAbstractSocket::SocketState state);
    void connectNotificationSocket(void);
    void newUDPMessage(const UDPMessage &message);
    void findServerSlot(void);

private:
    void setupNotificationSocket(void);
    void setupUDPSocket(void);
    QUrl buildNotifcationUrl(const QString &path);
    QUrl buildUrl(const QString &path);
    QString sendToServer(const QUrl &url, const QString &json, NetActionType netAction);

    bool m_connected;
    QString m_server;
    int m_port;
    QWebSocket *m_notificationSocket;
    QTimer m_findServerTimer;
    static API *m_instance;
};

#endif // API_H