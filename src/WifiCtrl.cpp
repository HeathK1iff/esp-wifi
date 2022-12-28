#include "WifiCtrl.h"

#define USE_SERIAL_DEBUG
#define NTP_PACKET_SIZE 48

#ifdef ESP8266
#define MODE_STA WIFI_STA
#define MODE_AP WIFI_AP
#else
#define MODE_STA WIFI_MODE_STA
#define MODE_AP WIFI_MODE_AP
#endif

#ifdef ESP8266
WiFiEventHandler wifiDisconnectHandler;
WiFiEventHandler wifiStationConnectedHandler;
WiFiEventHandler wifiStationDisconnectedHandler;
#endif

const char* WifiCtrl::getIp(char* buf){
    buf[0] = '\0';
        
    switch (_wifi->getMode())
    {
    case MODE_STA:
        strcat(buf, _wifi->localIP().toString().c_str());
        break;
    case MODE_AP:
        strcat(buf, _wifi->softAPIP().toString().c_str());
        break;
    default:
        break;
    } 
    return buf;
}

const char* WifiCtrl::getSSID(char* buf){
    buf[0] = '\0';

    switch (_wifi->getMode())
    {
    case MODE_STA:
        strcat(buf, WiFi.SSID().c_str());
        break;
    case MODE_AP:
        strcat(buf, WiFi.softAPSSID().c_str());
        break;
    } 

    return buf;
}

bool WifiCtrl::_hasAPConnection(){
    return _connectionsAP > 0;
}

bool WifiCtrl::_connectTo(const char *ssid, const char *psw)
{            
    if (strlen(ssid) == 0)
        return false;

    _wifi->disconnect();
    _wifi->enableSTA(true);
    _wifi->setHostname(_deviceName);
    _wifi->begin(ssid, psw);

    unsigned long timeout = millis() + TIMEOUT_WIFI_CONNECT;
    while ((timeout > millis()) && (!_wifi->isConnected())){
        delay(50);
    };

    return _wifi->isConnected();
}

void WifiCtrl::_wifiCtrl(bool useAPMode, const char *ssid, const char *pass){
    _useAPMode = useAPMode;

    memset(_ssidDefault, '\0', PARAM_SSID_LENGTH);
    memset(_passDefault, '\0', PARAM_SSID_PASS_LENGTH);
    memset(_ssidSTA, '\0', PARAM_SSID_LENGTH);
    memset(_passSTA, '\0', PARAM_SSID_PASS_LENGTH);

    strcat(_ssidDefault, ssid);    
    strcat(_passDefault, pass);
}   

#ifdef ESP8266
WifiCtrl::WifiCtrl(ESP8266WiFiClass* wifi, bool useAPMode, const char *ssid, const char *pass)
{
    _wifi = wifi;
    _wifiCtrl(useAPMode, ssid, pass);
}
#elif ESP32
WifiCtrl::WifiCtrl(WiFiClass* wifi, bool useAPMode, const char *ssid, const char *pass)
{
    _wifi = wifi;
    _wifiCtrl(useAPMode, ssid, pass);
}
#endif
 
void WifiCtrl::begin(const char *deviceName,  const char *ssid, const char *pass)
{    
    _tsReconnect = 0;
    strcat(_ssidSTA, ssid);    
    strcat(_passSTA, pass);
    strcat(_deviceName, deviceName);

    _wifi->setHostname(_deviceName);
    _wifi->setAutoReconnect(false);
    _wifi->setAutoConnect(false);
    _wifi->persistent(false);

    #ifdef ESP8266
    wifiDisconnectHandler = _wifi->onStationModeDisconnected([&](const WiFiEventStationModeDisconnected &event){
        _wifi->disconnect();
        _tsReconnect = 0;
    });
    
    wifiStationConnectedHandler = _wifi->onSoftAPModeStationConnected([&](WiFiEventSoftAPModeStationConnected event){
        _connectionsAP++;
    });

    wifiStationDisconnectedHandler = _wifi->onSoftAPModeStationDisconnected([&](WiFiEventSoftAPModeStationDisconnected event){
        _connectionsAP--;
    });

    #elif ESP32
    _wifi->onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info){
        _connectionsAP++;
    }, WiFiEvent_t::SYSTEM_EVENT_AP_STACONNECTED);

    _wifi->onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info){
        _connectionsAP--;
    }, WiFiEvent_t::SYSTEM_EVENT_AP_STADISCONNECTED);
    
    _wifi->onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info){
        _wifi->disconnect();
        _tsReconnect = 0;
    }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
    
    #endif

    _wifi->setHostname(_deviceName);

    MDNS.begin(_deviceName);
    MDNS.addService("http", "tcp", 80);
    
    _checkWifiConnection();
}

void WifiCtrl::_createAP(const char *ssid, const char *psw)
{
    if (strlen(ssid) == 0)
        return;

    _wifi->softAPdisconnect();
    _wifi->enableAP(true);
    _wifi->softAP(ssid, psw);             
    _wifi->softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 0, 0));
}

void WifiCtrl::_printDebugInfo(){
    char buf[25];
    char bufSSID[25];
    
    Serial.print("[");
    Serial.print(_wifi->getMode() == MODE_STA ? "STA," : "AP,");
    Serial.print(getSSID(bufSSID));
    Serial.print(",");
    Serial.print(getIp(buf));
    Serial.println("]");
}

void WifiCtrl::_checkWifiConnection()
{ 
    if (_connectTo(_ssidSTA, _passSTA)){
        _printDebugInfo();
        return;
    }        

    if (_useAPMode) {   
        _createAP(_ssidDefault, _passDefault);      
        _printDebugInfo();
              
    } else {
        if (_connectTo(_ssidDefault, _passDefault))
           _printDebugInfo();
    }   
}


bool WifiCtrl::loop(){
    if ((_tsReconnect == 0) || (millis() > _tsReconnect))
    { 
        if ((!_hasAPConnection()) && (!_wifi->isConnected())){
            _checkWifiConnection();        
        }

        _tsReconnect = millis() + TIMEOUT_WIFI_CHECK_STATE; 
    }
    
    #ifdef ESP8266
    if (_wifi->isConnected()){
        MDNS.update();
    }
    #endif

    return _wifi->isConnected();
}

time_t WifiCtrl::getTime(const char* host){
    time_t result = 0;
    unsigned long time_ms = 0;
    WiFiUDP *udp = new WiFiUDP();
    udp->begin(321);

    byte packetBuf[NTP_PACKET_SIZE];
    memset(packetBuf, 0, NTP_PACKET_SIZE);
    packetBuf[0] = 0b11100011;
    packetBuf[1] = 0;
    packetBuf[2] = 6;
    packetBuf[3] = 0xEC;
    packetBuf[12] = 49;
    packetBuf[13] = 0x4E;
    packetBuf[14] = 49;

    packetBuf[15] = 52;

    IPAddress timeServerIP;
    WiFi.hostByName(host, timeServerIP);

    udp->beginPacket(timeServerIP, 123);
    udp->write(packetBuf, NTP_PACKET_SIZE);
    udp->endPacket();
    delay(1000);

    int respond = udp->parsePacket();
    if (respond > 0)
    {
        udp->read(packetBuf, NTP_PACKET_SIZE);
        unsigned long highWord = word(packetBuf[40], packetBuf[41]);
        unsigned long lowWord = word(packetBuf[42], packetBuf[43]);
        time_t secsSince1900 = highWord << 16 | lowWord;
        result = secsSince1900 - 2208988800UL + 3 * 3600L;
    }
    delete udp;
    return result;
}