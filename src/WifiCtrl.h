#ifndef WIFI_DAEMOM
#define WIFI_DAEMON

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WiFiType.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#elif ESP32
#include <WiFi.h>
#include <WiFiType.h>
#include <HTTPUpdateServer.h>
#include <ESPmDNS.h>
#endif

#define SEC_MS 1000l

#ifndef TIMEOUT_WIFI_CHECK_STATE
#define TIMEOUT_WIFI_CHECK_STATE SEC_MS * 10
#endif

#ifndef TIMEOUT_WIFI_CONNECT
#define TIMEOUT_WIFI_CONNECT SEC_MS * 10
#endif

#ifndef PARAM_SSID_LENGTH
#define PARAM_SSID_LENGTH 25
#endif

#ifndef PARAM_SSID_PASS_LENGTH
#define PARAM_SSID_PASS_LENGTH 25
#endif

#ifndef PARAM_DEVICE_LENGTH
#define PARAM_DEVICE_LENGTH 30
#endif

class WifiCtrl
{
    private:
        bool _useAPMode;
        unsigned long _tsReconnect;
        unsigned long _connectionsAP;
        
        #ifdef ESP32
        WiFiClass *_wifi;
        #else
        ESP8266WiFiClass *_wifi;
        #endif

        char _ssidSTA[PARAM_SSID_LENGTH];
        char _passSTA[PARAM_SSID_PASS_LENGTH];
        char _ssidDefault[PARAM_SSID_LENGTH]; 
        char _passDefault[PARAM_SSID_PASS_LENGTH];
        char _deviceName[PARAM_DEVICE_LENGTH];
            
        bool _hasAPConnection();
        bool _connectTo(const char *ssid, const char *psw);
        void _checkWifiConnection();
        void _createAP(const char *ssid, const char *psw);
        void _wifiCtrl(bool useAPMode, 
                const char *ssid, const char *pass);   
        void _printDebugInfo();
    public:
        #ifdef ESP32
            WifiCtrl(WiFiClass* wifi, bool useAPMode, 
                const char *ssid, const char *pass);
        #else
            WifiCtrl(ESP8266WiFiClass* wifi, bool useAPMode, 
                const char *ssid, const char *pass);
        #endif


        void begin(const char *deviceName,  const char *ssid, 
            const char *pass);
        
        const char* getIp(char* buf);
        const char* getSSID(char* buf);
        
        time_t getTime(const char* host);

        bool loop();
};

#endif
