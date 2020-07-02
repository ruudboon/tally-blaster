#include <FS.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h>
#include <WebSocketsServer.h>
#include <WiFiClient.h>
#include <Ticker.h>


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

#define CONFIG_FILENAME "/tally-config.cfg"
#define VERSION         "0.0.2"

// optional arguments fuction need to be defined
void setLedColor(uint32_t color, bool ignoreDisabledLeds=false);

// Init services
Ticker ticker;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);
WiFiClient vmixConnection;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(TOTAL_PIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t status[6] = {
    pixels.Color(0,0,0), //STATUS_CONNECTED: Black is connected
    pixels.Color(0,255,0), //STATUS_PREVIEW: Green = Preview
    pixels.Color(255,0,0), //STATUS_PROGRAM: Red = Program
    pixels.Color(255,255,255), //STATUS_LOCATE: White Blinking = Identify / Call
    pixels.Color(128,0,128), //STATUS_CONNECTWIFI: Purple Blinkning = Connecting to wifi
    pixels.Color(255,140,0) //STATUS_CONNECTVMIX: Orange Blinkning = Conecting to vmix
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
  
  //Enable wifi led
  digitalWrite(BUILTIN_LED, LOW);

  if (MDNS.begin(hostname)) {
    Serial.println("mDNS started: vmixtally");  
    if (MDNS.addService("vmix-tally", "tcp", 81)) {
       Serial.println("mDNS registered as _vmix-tally._tcp"); 
    } else {
      Serial.println("mDNS failed registering");
    }
  }
  MDNS.update();

  startWebSocket(); 
  
  server.on("/", handleSettings);
  server.begin();
  Serial.println("Server listening");
  ledState = STATUS_CONNECTVMIX;
  updateLedColor();
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
        return;
    }
    if (settingKey == "vmixHost") {
        settingValue.toCharArray(settings.vmixHost, 63);

        String message = getSettingAsString("vmixHost");
        webSocket.broadcastTXT(message);
        vmixConnection.stop();
        return;
    }
    if (settingKey == "tallyNumber") {
        settings.tallyNumber = settingValue.toInt();
        vmixConnection.stop();
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
    if (ledState > STATUS) {
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

// Handle vmix data
void handleVmix(String data)
{
  // Check if server data is tally data
  if (data.indexOf("TALLY") == 0)
  {
    Serial.println(data);
    char newState = data.charAt(settings.tallyNumber + 8);

    // Check if tally state has changed
    // if (currentState != newState)
    {
    //   currentState = newState;

      switch (newState)
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
  }
  else
  {
    Serial.print("Response from vMix: ");
    Serial.println(data);
  }
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

 if (vmixConnection.connect(settings.vmixHost, settings.vmixPort))
 {
   Serial.println(" Connected!");
   Serial.println("------------");
   stopPulsating();
   ledState = STATUS_CONNECTED;
   updateLedColor();
   setLedBrightness(settings.brightness);
   vmixConnection.println("SUBSCRIBE TALLY");
 }
 else
 {
   Serial.println(" Not found!");
 }
}

void handleSettings() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  html += "html { font-family: Helvetica; background-color: #000; color:#fff;}";
  html += "body { text-align: center;}";
  html += "form { width: 500px; text-align: left;}";
  html += "h1 { text-size: 20px; }";
  html += "</style>";
  html += "<script>var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);connection.onopen = function () {  connection.send('Connect ' + new Date()); }; connection.onerror = function (error) {    console.log('WebSocket Error ', error);};connection.onmessage = function (e) {  console.log('Server: ', e.data);};function sendRGB() {  var r = parseInt(document.getElementById('r').value).toString(16);  var g = parseInt(document.getElementById('g').value).toString(16);  var b = parseInt(document.getElementById('b').value).toString(16);  if(r.length < 2) { r = '0' + r; }   if(g.length < 2) { g = '0' + g; }   if(b.length < 2) { b = '0' + b; }   var rgb = '#'+r+g+b;    console.log('RGB: ' + rgb); connection.send(rgb); }</script>";
  html += "</head><body><form><fieldset>";
  html += "<h1>vMix Tally settings for " + chipId +"</h1>";
  html += "<input type=\"range\" min=\"0\" max=\"255\" name=\"intensity\" value=\"" + String(currentBrightness) +"\">";
  html += "<input type=\"number\" name=\"tally\" min=\"1\" max=\"1000\">";
  html += "</fieldset></form></body></html>";
  
  String message = "Number of args received:";
  message += server.args();            //Get number of parameters
  message += "\n";                            //Add a new line
  
  for (int i = 0; i < server.args(); i++) {
    message += "Arg nº" + (String)i + " –> ";   //Include the current iteration value
    message += server.argName(i) + ": ";     //Get the name of the parameter
    message += server.arg(i) + "\n";              //Get the value of the parameter
  } 
  
  server.send(200, "text/html", html);       //Response to the HTTP request
}
void loop() {
  MDNS.update();
  webSocket.loop();
  server.handleClient();

  while (vmixConnection.available())
  {
    String data = vmixConnection.readStringUntil('\r\n');
    handleVmix(data);
  }

  if (!vmixConnection.connected())
  {
    vmixConnection.stop();

    connectTovMix();
    // lastCheck = millis();
  }
}
