#include "BlockDevice.h"
#include "NetworkManager.h"
#include "UDPMessage.h"

BlockDevice::BlockDevice()
	: m_downloadConfig(false), m_last(0), m_current(0), m_currentState(BlockUnknown), m_currentTimeout(false)
{
}


BlockDevice::~BlockDevice()
{
}

void BlockDevice::process(ModuleData &data)
{
	BlockState newState;
	byte value;
	byte pin;
	if (getPort() < 8)
	{
		value = data.getByteA();
		pin = getPort();
	}
	else
	{
		value = data.getByteB();
		pin = getPort() - 8;
	}

	long t = millis();
	if (value == m_last && m_last != m_current && (t - m_currentTimeout) > TIMEOUT_INTERVAL)
	{
		m_currentTimeout = t;
		m_current = value;

		if (bitRead(value, pin) == LOW)
			newState = BlockOccupied;
		else
			newState = BlockClear;
		if (m_currentState != newState)
		{
			m_currentState = newState;
			sendStatusMessage();
		}
	}
	m_last = value;
}

void BlockDevice::setup(int deviceID, byte port)
{
	DEBUG_PRINT("BlockDevice::setup: deviceID: %d  Port: %d\n", deviceID, port);
	setID(deviceID);
	setPort(port);

	m_currentState = BlockClear;

	String json = loadConfig();
	DEBUG_PRINT("%s\n", json.c_str());
	if (json.length() == 0)
		m_downloadConfig = true;
	else
		m_downloadConfig = (parseConfig(json, false) == false);
}

void BlockDevice::processPin(byte pin, byte value)
{
	if (pin == getPort())
	{
		DEBUG_PRINT("BlockDevice::processPin  DeviceID %d  Pin %d  value %d\n", getID(), pin, value);
		BlockState newState;
		long t = millis();
		if (value == m_last && m_last != m_current && (t - m_currentTimeout) > TIMEOUT_INTERVAL)
//		if (value != m_current)
		{
			m_currentTimeout = t;
			m_current = value;

			if (value == PinOff)
				newState = BlockOccupied;
			else
				newState = BlockClear;
			if (m_currentState != newState)
			{
				m_currentState = newState;
				sendStatusMessage();
			}
		}
		m_last = value;
	}
}


bool BlockDevice::parseConfig(String &jsonText, bool setVersion)
{
	DEBUG_PRINT("BlockDevice::parseConfig\n");
	StaticJsonBuffer<1024> jsonBuffer;
	JsonObject &json = jsonBuffer.parseObject(jsonText);

	if (setVersion)
	{
		json["version"] = CONFIG_VERSION;
	}
	else if (json["version"] != (int)CONFIG_VERSION)
	{
		DEBUG_PRINT("parseConfig  WRONG VERSION.\n");
		return false;
	}

	if (setVersion)
	{
		jsonText = "";
		json.printTo(jsonText);
	}
	return true;
}

void BlockDevice::sendStatusMessage(void)
{
	DEBUG_PRINT("BlockDevice::sendStatusMessage: %d\n", m_currentState);

	UDPMessage message;
	message.setMessageID(BLK_STATUS);
	message.setID(getID());
	message.setField(0, m_currentState);
	NetManager.sendUdpMessage(message, true);
}

void BlockDevice::serverFound(void)
{
	if (m_downloadConfig)
	{
		String json = NetManager.getDeviceConfig(getID());
		if (json.length() > 0)
		{
			parseConfig(json, true);
			saveConfig(json);
			m_downloadConfig = false;
			setup(getID(), getPort());
		}
	}
	sendStatusMessage();
}

void BlockDevice::processUDPMessage(ModuleData &moduleData, const UDPMessage &message)
{
	if (message.getMessageID() == SYS_RESET_DEVICE_CONFIG)
	{
		if (message.getID() == getID())
		{
			DEBUG_PRINT("processUDPMessage RESET DEVICE CONFIG\n", message.getID());
			String json = NetManager.getDeviceConfig(getID());
			parseConfig(json, true);
			saveConfig(json);
			setup(getID(), getPort());
		}
	}
}