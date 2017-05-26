#include <QCoreApplication>

#include "DeviceManager.h"
#include "DeviceHandler.h"

DeviceManager * DeviceManager::m_instance = NULL;

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
{

}

DeviceManager::~DeviceManager()
{
}

DeviceManager *DeviceManager::instance()
{
    if(m_instance == NULL)
        m_instance = new DeviceManager(qApp);

    return m_instance;
}

int DeviceManager::getDeviceStatus(int deviceID)
{
    return m_statusMap.value(deviceID).getCurrentStatus();
}

int DeviceManager::getDeviceCount(void) const
{
    return m_statusMap.keys().count();
}

int DeviceManager::getDeviceID(int index) const
{
    return m_statusMap.keys().value(index);
}

void DeviceManager::setDeviceStatus(int deviceID, int status)
{
    DeviceStatus newStatus(status, QDateTime::currentDateTime());

    if(m_statusMap.value(deviceID).getCurrentStatus() != status)
    {
        qDebug(QString("!!!!!!!!!!!!!!!SET DEVICE STATUS %1 to %2").arg(deviceID).arg(status).toLatin1());
        m_statusMap[deviceID] = newStatus;
        emit deviceStatusChanged(deviceID, status);
    }
}

void DeviceManager::addDeviceHandler(ClassEnum classCode, DeviceHandler *handler)
{
    m_deviceMap[classCode] = handler;
}

void DeviceManager::removeDeviceHandler(ClassEnum classCode)
{
    m_deviceMap.remove(classCode);
}
