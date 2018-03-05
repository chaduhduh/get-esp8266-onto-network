/*
 * Imports
 */
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include "FS.h"

/*
 * Objects
 */
struct Settings {
	String ssid = "";
	String key = "";
	String userId = "";
	String deviceId = "";
};

/*
 * Declarations
 */
ESP8266WebServer server(80);
IPAddress local_IP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);
WiFiClient client;
const char* ssid     = "TestNet";
const char* password = "1234";
const char* localDomain = "connect"; // connect.local
struct Settings ApSettings;
uint8_t useReset = 1;
int currentMode = 0;  // 0 - not setup
					            // 1 - ready
					            // 2 - connected
					

/*
 * Functions
 */
void getSettings() {
	File f = SPIFFS.open("/settings.txt", "r");
	if (!f){
		Serial.println("unable to read settings");
		return;
	}
	ApSettings.ssid = f.readStringUntil('\n');
	ApSettings.key = f.readStringUntil('\n');
	ApSettings.userId = f.readStringUntil('\n');
	ApSettings.deviceId = f.readStringUntil('\n');

	ApSettings.ssid.trim(); 
	ApSettings.key.trim();
	ApSettings.userId.trim();
	ApSettings.deviceId.trim(); 
	updateMode();
}


bool saveSettings(String ssid, String key, String userId, String deviceId) {
	File f = SPIFFS.open("/settings.txt", "w");
	if (!f){  return false;  }
	String ssidDefault = (ssid != "") ? ssid : (ApSettings.ssid != "") ? ApSettings.ssid : "";
	String keyDefault = (key != "") ? key : (ApSettings.key != "") ? ApSettings.key : "";
	String userIdDefault = (userId != "") ? userId : (ApSettings.userId != "") ? ApSettings.userId : "";
	String deviceIdDefault = (deviceId != "") ? deviceId : "";

	f.println(ssidDefault);
	f.println(keyDefault);
	f.println(userIdDefault);
	f.println(deviceIdDefault);
	f.close();

	ApSettings.ssid = ssidDefault;
	ApSettings.key = keyDefault;
	ApSettings.userId = userIdDefault;
	ApSettings.deviceId = deviceIdDefault;
	updateMode();

	return true;
}

void updateMode() {
	if(hasNetworkSettings()){
		setCurrentMode(1);
	}else{
		setCurrentMode(0);
	}
}

bool hasNetworkSettings() {
	return (ApSettings.ssid != "") ? true : false;
}

void getNetworkInfo() {
	String connectSsid = server.arg("ssid");
	String connectKey = server.arg("key");
	String connectUserid = server.arg("userid");
	saveSettings(connectSsid,connectKey,connectUserid,"");
	server.send(200, "text/html", "<h1>Settings Saved</h1>");
}

bool isReady() {
	return (currentMode == 1) ? true : false;  
}

bool validConnectionInfo() {
	return (currentMode == 2) ? true : false;  
}

void setCurrentMode(int e) {
	currentMode = e;
}

int getCurrentMode() {
	return currentMode;
}

bool WifiProcessing() {
	return (WiFi.status() != WL_CONNECTED &&
		WiFi.status() != WL_NO_SSID_AVAIL &&
		WiFi.status() != WL_CONNECT_FAILED && 
		WiFi.status() != WL_DISCONNECTED) ? true : false;
}

bool WifiFailed() {
	return (WiFi.status() == WL_NO_SSID_AVAIL ||
		WiFi.status() == WL_CONNECT_FAILED || 
		WiFi.status() == WL_DISCONNECTED) ? true : false;
}

bool WifiConnected() {
	return (WiFi.status() == WL_CONNECTED) ? true : false;
}



/*
 * Setup Proccesses
 */
void setup()
{
	Serial.begin(115200);
  Serial.println("Starting");
	WiFi.setAutoConnect(false);
	SPIFFS.begin();
	getSettings();

	Serial.println();
	Serial.print("\nSetting soft-AP configuration ... ");
	Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

	Serial.print("Setting soft-AP ... ");
	Serial.println(WiFi.softAP(ssid,password) ? "Ready" : "Failed!");

	Serial.print("Soft-AP IP address = ");
	Serial.println(WiFi.softAPIP());

	server.on("/", getNetworkInfo);
	server.begin();
	Serial.printf("Web server started, open %s in a web browser\n", WiFi.softAPIP().toString().c_str());
}


/*
 * Main Logic
 */
void loop() {
while(1)    {
	server.handleClient();
	if( !isReady() )
		continue;

	// if we were previously connected to AP then disconnect
	if(WiFi.status() == WL_CONNECTED && !validConnectionInfo()){
		WiFi.disconnect();
		Serial.print("Disconnecting");
		while(WiFi.status() == WL_CONNECTED){
	  		delay(500);
	  		Serial.print(".");
		}
		Serial.println();
	}

	// if wifi not connected then connect
	if( !WifiConnected() ){
		Serial.print("connecting to - ");
    Serial.println(ApSettings.ssid.c_str());
		WiFi.begin(ApSettings.ssid.c_str(), ApSettings.key.c_str());
		int count = 0;
		while(WiFi.status() != WL_CONNECTED && count < 20){
	  		server.handleClient();
	  		delay(500);
	 		Serial.print(".");
	  		count += 1;
		}
	}

	// if we are/were connected do something here
	if( WifiConnected() ) {
		Serial.println("Connected.");
    while(WifiConnected()){
     /*
     * YOUR DEVICE LOGIC HERE
     */
     Serial.print(".");
     delay(1000);
    }
	}

	// if connection failed do something TODO: implement some limit
	if( WifiFailed() ) {
		Serial.print(" - couldnt connect to - ");
		Serial.println(WiFi.SSID());
		delay(1000);
	}
}}


