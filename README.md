![tally-blaster-logo-small](https://user-images.githubusercontent.com/7444246/87219490-e8005e80-c35b-11ea-96ca-02b44e2f3a28.png)
# Tally Blaster ![Build](https://github.com/ruudboon/tally-blaster/workflows/Build/release/badge.svg)
WiFi based Tally using NodeMCU and 2 NeoPixel Mini PCB. Currently supporting vMix, planning to support more systems like Blackmagic, Roland etc.

![tally-blaster](https://user-images.githubusercontent.com/7444246/87221383-f8b8d080-c36b-11ea-8cfc-20c5f82e6d98.gif)

Case by Elvin Media

## Features
- Camera tally LED & Host tally LED
- Brightness control
- Locate function
- Save settings permanent (App not needed during production)
- Autodiscover tally nodes using Zerconf/Bonjour
- Configure app to control and monitor all nodes
- vMix support


### How It Works
- When your ESP starts up, it sets it up in Station mode and tries to connect to a previously saved Access Point
- if this is unsuccessful (or no previous network saved) it moves the ESP into Access Point mode and spins up a DNS and WebServer (default ip 192.168.4.1)
- using any wifi enabled device with a browser (computer, phone, tablet) connect to the newly created Access Point (ESP*)
- because of the Captive Portal and the DNS server you will either get a 'Join to network' type of popup or get any domain you try to access redirected to the configuration portal
- choose the access points that connects your vMix (must have DHCP), enter password, click save
- start Configuration App and scan for Tally Nodes
- configure your nodes with the vMix IP and tally number.


### Needed Hardware
- NodeMCU v3
- 2 NeoPixels Mini PCB
- Usb cable
- USB powersupply or Powerbank
- WiFi connection to your vMix computer

### Flashing NodeMCU (V3)
- Download latest release
- Unpack zip
- Open vmix-tally.ino with arduino IDE
- Make sure you have the NodeMCU board [installed](https://github.com/esp8266/Arduino#installing-with-boards-manager)
- Install the extra libraries (WiFiManager by [tzapu 2.0.3-alpha](https://github.com/tzapu/WiFiManager/releases/tag/2.0.3-alpha), Adafruit NeoPixel by Adafruit 1.6.0, WebSockets by Markus Sattler 2.2.0, [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP/archive/master.zip), [Atem](https://github.com/kasperskaarhoj/SKAARHOJ-Open-Engineering/blob/master/ArduinoLibs.zip))
- ** For the atem library, download and extract library and select atem folder using `Include Library -> Add .ZIP Library` ** 
- Select board (NodeMCU 1.0)
- Flash NodeMCU

## NodeMCU pin layout
- Connect 1 NeoPixel input to 1 NeoPixel output (+,signal,-)
- Connect + of first NeoPixel to pin VU
- Connect - of first NeoPixel to ground
- Connect signal of first NeoPixel to D2
- Power using USB

## Current plans
- Add 3d printer case design
- Stabilize, optimise and improve software
- Improve documentation
- Configure using webinterface
- [Ask vMix for ZeroConf broadcast to allow auto connect](https://forums.vmix.com/posts/t23873-Zeroconf---Bonjour)
- Create demonstration video

# Led colors
- CONNECTED: LEDS off
- PREVIEW: Green
- PROGRAM: Red
- LOCATE: White Blinking
- CONNECTING WIFI: Purple Blinkning
- CONNECTING VMIX: Orange Blinkning

# Sponsor
This project will grow faster with your help. Donations and sponsorships allow me to spend more time on this project and help me paying for licenses and hardware to test, coffee, debugging pizza and release beers. Want your logo on this page? Please contact me.

[Github Sponsor](https://github.com/sponsors/ruudboon). One-time donations are welcome through [PayPal](https://www.paypal.me/ruudboon).

# Tally Blaster Control app
![Tally Blaster Control app](https://user-images.githubusercontent.com/7444246/86400131-613aea00-bca8-11ea-897d-dd0e05ca8eaa.png)


### Inspired by
* [Arduino-vMix-tally](https://github.com/ThomasMout/Arduino-vMix-tally)
* [WiFiManager](https://github.com/tzapu/WiFiManager)
* [Automatic Update Server](https://www.instructables.com/id/Set-Up-an-ESP8266-Automatic-Update-Server/)
