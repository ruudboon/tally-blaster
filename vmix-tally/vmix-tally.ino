#include <FS.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h>
#include <WebSocketsServer.h>
#include <ESPAsyncTCP.h>
#include <Ticker.h>
#include <stdint.h>

#define PIXEL_PIN       D2
#define RESET_PIN       4
#define TOTAL_PIXELS    2
#define CAMERA_LED      0
#define VIEWER_LED      1


#define STATUS_CONNECTED    0
#define STATUS_PREVIEW      1
#define STATUS_PROGRAM      2
#define STATUS_LOCATE       3
#define STATUS_CONNECTWIFI  4
#define STATUS_CONNECTVMIX  5
#define STATUS_UPGRADE      6

#define CONFIG_FILENAME "/tally-config.cfg"
#define VERSION         "0.3.2"

// optional arguments fuction need to be defined
void setLedColor(uint32_t color, bool ignoreDisabledLeds=false);

// Init services
Ticker ticker;
WebSocketsServer webSocket(81);
AsyncClient* vmixClient = new AsyncClient();
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(TOTAL_PIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t status[7] = {
    pixels.Color(0,0,0), //STATUS_CONNECTED: Black is connected
    pixels.Color(0,255,0), //STATUS_PREVIEW: Green = Preview
    pixels.Color(255,0,0), //STATUS_PROGRAM: Red = Program
    pixels.Color(255,255,255), //STATUS_LOCATE: White Blinking = Identify / Call
    pixels.Color(128,0,128), //STATUS_CONNECTWIFI: Purple Blinkning = Connecting to wifi
    pixels.Color(255,140,0), //STATUS_CONNECTVMIX: Orange Blinkning = Conecting to vmix
    pixels.Color(0,255,255) //STATUS_UPGRADE: Aqua Blinkning = Upgrading firmware
};

// Settings object
struct Settings
{
  char vmixHost[64]="172.20.0.193";
  int vmixPort=8099;
  int tallyNumber=1;
  bool viewerLedEnabled=true;
  bool cameraLedEnabled=true;
  int brightness = 255;
};

// Setup default vars
int ledState = 0;
int oldLedState = -1;
int tickerCountdown = 0;
bool fadingUp = false;
String chipId = String(ESP.getChipId());
String hostname = String("vmix-tally-" + chipId);
int currentBrightness = 255;
Settings settings;

void tick()
{
  if (currentBrightness < 255) {
    currentBrightness = 255;
  } else {
    currentBrightness = 100;
  }
  pixels.setBrightness(currentBrightness);
  pixels.show();
  if (tickerCountdown != 0) {
      tickerCountdown--;
      if (tickerCountdown == 0) {
          stopPulsating();
          if (oldLedState != -1) {
              ledState = oldLedState;
              oldLedState = -1;
              updateLedColor();
          }
      }
  }
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  Serial.println("vMix Tally NodeMCU v" + String(VERSION));
  SPIFFS.begin();
  readConfig();
  setupLeds();
  setLedColor(status[STATUS_CONNECTWIFI]);
  ledState = STATUS_CONNECTWIFI;
  updateLedColor();
  startPulsating();

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  Serial.println("Setup WIFI");
  WiFiManager wifiManager;
  //reset settings - for testing
//  wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  WiFi.hostname(hostname);
  Serial.println("Hostname: " + hostname);
  
//  wifiManager.startConfigPortal("OnDemandAP");

  //Enable wifi led
  digitalWrite(BUILTIN_LED, LOW);

  if (MDNS.begin(hostname)) {
    Serial.println("mDNS started: vmixtally");  
    if (MDNS.addService("vmix-tally", "tcp", 81)) {
       Serial.println("mDNS registered as _vmix-tally._tcp"); 
    } else {
      Serial.println("mDNS failed registering");
    }
  } else {
    Serial.println("Unable to start MDNS");
  }
  MDNS.update();

  startWebSocket(); 
  
  Serial.println("Websocket Server listening");

  vmixClient->onData(&vmixHandleData, vmixClient);
  vmixClient->onConnect(&onVmixConnect, vmixClient);
  vmixClient->onDisconnect(&onVmixDisconnect, vmixClient);
  vmixClient->onError(&handleVmixError, vmixClient);
  vmixClient->onTimeout(&handleTimeOut, vmixClient);
  connectTovMix();
}

void saveConfig()
{
    File file = SPIFFS.open(CONFIG_FILENAME, "w");
    if (file) {
        Serial.println("Writing config"); 
        file.write((char*) &settings, sizeof(settings));
        file.close();
    } else {
        Serial.println("Opening config for saved failed"); 
    }
}

void readConfig()
{
    File file = SPIFFS.open(CONFIG_FILENAME, "r");
    if (file) {
        Serial.println("Reading config"); 
        file.readBytes((char*) &settings, sizeof(settings));
        file.close();
    } else {
        Serial.println("Opening config for read [failed"); 
    }
}

void startPulsating()
{
  ticker.attach(0.5, tick);
}

void stopPulsating()
{
  setLedBrightness(settings.brightness);
  ticker.detach();
}

void setupLeds()
{
  pinMode(BUILTIN_LED, OUTPUT);
  pixels.begin();
  pixels.setBrightness(currentBrightness);
}

void startWebSocket() {
  webSocket.begin();                    
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started.");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        sendAllSettings(num);
      }
      break;
    case WStype_TEXT:
      updateSetting((char*)payload);
      Serial.printf("[%u] get Text: %s\n", num, payload);
      break;
  }
}

String getSettingAsString(String settingKey)
{
    if (settingKey == "viewerLedEnabled") {
        String boolAsString = String("false");
        if (settings.viewerLedEnabled) {
            boolAsString = "true";
        }
        return settingKey + ":" + boolAsString;
    }
    if (settingKey == "cameraLedEnabled") {
        String boolAsString = String("false");
        if (settings.cameraLedEnabled) {
            boolAsString = "true";
        }
        return settingKey + ":" + boolAsString;
    }
    if (settingKey == "vmixPort") {
        return settingKey + ":" + String(settings.vmixPort);
    }
    if (settingKey == "vmixHost") {
        return settingKey + ":" + String(settings.vmixHost);
    }
    if (settingKey == "tallyNumber") {
        return settingKey + ":" + String(settings.tallyNumber);
    }
    if (settingKey == "brightness") {
        return settingKey + ":" + String(settings.brightness);
    }
    if (settingKey == "ledState") {
        return settingKey + ":" + String(ledState);
    }
    if (settingKey == "version") {
        return settingKey + ":" + String(VERSION);
    }
    return "";
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void updateSetting(String payload)
{
    String settingKey = getValue(payload, ':', 0);
    String settingValue = getValue(payload, ':', 1);
    if (settingKey == "viewerLedEnabled") {
        if (settingValue == "true") {
            settings.viewerLedEnabled = true;
        } else {
            settings.viewerLedEnabled = false;
        }
        updateLedColor();
        String message = getSettingAsString("viewerLedEnabled");
        webSocket.broadcastTXT(message);
        return;
    }
    if (settingKey == "cameraLedEnabled") {
        if (settingValue == "true") {
            settings.cameraLedEnabled = true;
        } else {
            settings.cameraLedEnabled = false;
        }
        setLedColor(status[ledState]);
        String message = getSettingAsString("cameraLedEnabled");
        webSocket.broadcastTXT(message);
        return;
    }
    if (settingKey == "vmixPort") {
        settings.vmixPort = settingValue.toInt();
        String message = getSettingAsString("vmixPort");
        webSocket.broadcastTXT(message);
        connectTovMix();
        return;
    }
    if (settingKey == "vmixHost") {
        settingValue.toCharArray(settings.vmixHost, 63);

        String message = getSettingAsString("vmixHost");
        webSocket.broadcastTXT(message);
        connectTovMix();
        return;
    }
    if (settingKey == "tallyNumber") {
        settings.tallyNumber = settingValue.toInt();
        connectTovMix();
        return;
    }
    if (settingKey == "brightness") {
        settings.brightness = settingValue.toInt();
        setLedBrightness(settings.brightness);
        return;
    }
    if (settingKey == "locate") {
        tickerCountdown = settingValue.toInt();
        oldLedState = ledState;
        ledState = STATUS_LOCATE;
        startPulsating();
        updateLedColor();
        return;
    }
    if (settingKey == "reboot") {
        ESP.reset();
    }
    if (settingKey == "save") {
        saveConfig();
    }
}

void sendAllSettings(uint8_t webSocketClientID)
{
    String reply = "";
    reply = getSettingAsString("viewerLedEnabled");
    webSocket.sendTXT(webSocketClientID, reply);

    reply = getSettingAsString("cameraLedEnabled");
    webSocket.sendTXT(webSocketClientID, reply);

    reply = getSettingAsString("vmixPort");
    webSocket.sendTXT(webSocketClientID, reply);

    reply = getSettingAsString("vmixHost");
    webSocket.sendTXT(webSocketClientID, reply);

    reply = getSettingAsString("tallyNumber");
    webSocket.sendTXT(webSocketClientID, reply);

    reply = getSettingAsString("brightness");
    webSocket.sendTXT(webSocketClientID, reply);

    reply = getSettingAsString("ledState");
    webSocket.sendTXT(webSocketClientID, reply);

    reply = getSettingAsString("version");
    webSocket.sendTXT(webSocketClientID, reply);
}

void setLedColor(uint32_t color, bool ignoreDisabledLeds)
{
  if (ignoreDisabledLeds || settings.cameraLedEnabled) {
    pixels.setPixelColor(CAMERA_LED, color);  
  } else {
    pixels.setPixelColor(CAMERA_LED, pixels.Color(0,0,0));
  }
  if (ignoreDisabledLeds || settings.viewerLedEnabled) {
    if (status[STATUS_PREVIEW] == color) {
        pixels.setPixelColor(VIEWER_LED, pixels.Color(0,0,0));
    } else {
        pixels.setPixelColor(VIEWER_LED, color);  
    }
  } else {
    pixels.setPixelColor(VIEWER_LED, pixels.Color(0,0,0));
  }
  pixels.show();
}

void updateLedColor() {
    String message = getSettingAsString("ledState");
    webSocket.broadcastTXT(message);
    if (ledState > STATUS_PROGRAM) {
        setLedColor(status[ledState], true);
    } else {
        setLedColor(status[ledState]);
    }
}

// Set led intensity from 0 to 255;
void setLedBrightness(int intensity)
{
  pixels.setBrightness(intensity);
  pixels.show();
}

// static void replyToServer(void* arg) {
// 	AsyncClient* vmixClient = reinterpret_cast<AsyncClient*>(arg);

// 	// send reply
// 	if (vmixClient->space() > 32 && vmixClient->canSend()) {
// 		char message[32];
// 		sprintf(message, "this is from %s", WiFi.localIP().toString().c_str());
// 		vmixClient->add(message, strlen(message));
// 		vmixClient->send();
// 	}
// }

// /* event callbacks */
// static void handleData(void* arg, AsyncClient* client, void *data, size_t len) {
// 	Serial.printf("\n data received from %s \n", client->remoteIP().toString().c_str());
// 	Serial.write((uint8_t*)data, len);

// 	os_timer_arm(&intervalTimer, 2000, true); // schedule for reply to server at next 2s
// }

static void replyToVmix(void* arg) {
	AsyncClient* client = reinterpret_cast<AsyncClient*>(arg);

	// send reply
	if (client->space() > 32 && client->canSend()) {
		char message[32];
		sprintf(message, "this is from %s", WiFi.localIP().toString().c_str());
		client->add(message, strlen(message));
		client->send();
	}
}

void onVmixConnect(void* arg, AsyncClient* client) {
	Serial.printf("\n client has been connected to %s on port %d \n", settings.vmixHost, settings.vmixPort);
    Serial.println(" Connected!");
    Serial.println("------------");

    stopPulsating();
    ledState = STATUS_CONNECTED;
    WiFi.mode(WIFI_STA);
    updateLedColor();
    setLedBrightness(settings.brightness);
    char message[18] = "SUBSCRIBE TALLY\r\n";
    Serial.println(message);
    vmixClient->add(message, strlen(message));
    vmixClient->send();
}

void onVmixDisconnect(void* arg, AsyncClient* c) {
  Serial.print("We're disconnected!\n");
  WiFi.mode(WIFI_AP_STA);
  connectTovMix();
}

static void handleTimeOut(void* arg, AsyncClient* client, uint32_t time) {
	Serial.printf("\n client ACK timeout ip: %s \n", client->remoteIP().toString().c_str());
}

// Handle vmix data
void vmixHandleData(void* arg, AsyncClient* client, void *data, size_t len)
{
  Serial.write("vmix Data:");
  Serial.write((uint8_t*)data, len);
  uint8_t* d = reinterpret_cast<uint8_t*>(data);
  String vmixData = (char*)data;
  Serial.println(vmixData);
  // Check if server data is tally data
  if (vmixData.indexOf("TALLY") == 0)
  {
    char vMixTallyState = vmixData.charAt(settings.tallyNumber + 8);

    switch (vMixTallyState)
    {
      case '0':
        Serial.println("tally off");
        ledState = STATUS_CONNECTED;
        updateLedColor();
        break;
      case '1':
        Serial.println("tally progam");
        ledState = STATUS_PROGRAM;
        updateLedColor();
        break;
      case '2':
        Serial.println("tally preview");
        ledState = STATUS_PREVIEW;
        updateLedColor();
        break;
      default:
        Serial.println("unknown - tally off");
        ledState = STATUS_CONNECTED;
        updateLedColor();
    }
  }
  else
  {
    Serial.print("Response from vMix: ");
    Serial.println(vmixData);
  }
}

 /* clients events */
static void handleVmixError(void* arg, AsyncClient* client, int8_t error) {
	Serial.printf("\n connection error %s from client %s \n", client->errorToString(error), client->remoteIP().toString().c_str());
}

//Connect to vMix instance
void connectTovMix()
{
 Serial.print("Connecting to vMix on ");
 Serial.print(String(settings.vmixHost) + ":" + settings.vmixPort);
 Serial.print("...");
 ledState = STATUS_CONNECTVMIX;
 updateLedColor();
 startPulsating();
 
 vmixClient->connect(settings.vmixHost, settings.vmixPort);
}

void loop() {
  MDNS.update();
  webSocket.loop();
}
