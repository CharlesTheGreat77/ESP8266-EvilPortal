# ESP8266-EvilPortal
Easily upload captive portal templates from flipper zeros captive portal repo, scan SSIDs, and 
change AP and MAC on the fly! 😈

# Prerequisites
```
Arduino IDE
File > Preferences > Additional Board Manager URL > 
"http://arduino.esp8266.com/stable/package_esp8266com_index.json"
Board Manager > esp8266 (generic)
```
Tutorial: 
https://help.ubidots.com/en/articles/928408-program-the-esp8266-with-the-arduino-ide-in-3-simple-steps

# Download INO
Download and flash the INO file and look for ESP8266AP as a softAP

# Setup
To access the web interface, go to http://192.168.4.1/admin in your favorite web browser.
Web interface username and password is admin:admin

# Note 
Upon initial connect to the AP opens a captive portal that states "No HTML tempate uploaded", 
when inputing an html template from online, it will update the captive portal automatically.
I did not want to hardcode different templates for each portal.. why not just copy and paste 
existing portals!!! 🤔📝
