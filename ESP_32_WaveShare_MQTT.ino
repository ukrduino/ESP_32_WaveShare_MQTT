#include <WiFi.h>
#include <Credentials\Credentials.h>
#include "EEPROM.h" //For storing MD5 for OTA
#include <Update.h>
#include <PubSubClient.h>



WiFiClient espClient;
unsigned long reconnectionPeriod = 10000; //miliseconds
unsigned long lastWifiConnectionAttempt = 0;
bool HOME = true;
const char* _SSID;
const char* _PSWD;
String host;


PubSubClient client(espClient);
unsigned long lastBrokerConnectionAttempt = 0;
unsigned long lastSensorMsg = 0;
char msg[50];
int sensorRequestPeriod = 10; // seconds
const char* mqtt_server;


int flow;
const int SETTINGS_SWITCH_1_PIN = 16;
const int SETTINGS_SWITCH_2_PIN = 17;
const int SETTINGS_SWITCH_3_PIN = 18;
const int SETTINGS_SWITCH_4_PIN = 19;

//-----------------HTTP_OTA------------------------

/* Over The Air automatic firmware update from a web server.  ESP32 will contact the
*  server on every boot and check for a firmware update.  If available, the update will
*  be downloaded and installed.  Server can determine the appropriate firmware for this
*  device from combination of HTTP_OTA_FIRMWARE and firmware MD5 checksums.
*/

// Name of firmware
#define HTTP_OTA_FIRMWARE String(String(__FILE__).substring(String(__FILE__).lastIndexOf('\\')) + ".bin").substring(1)

// Variables to validate response
int contentLength = 0;
bool isValidContentType = false;
bool isNewFirmware = false;
int port = HTTP_OTA_PORT;
String binPath = String(HTTP_OTA_PATH) + HTTP_OTA_FIRMWARE;

String MD5;
int EEPROM_SIZE = 1024;
int MD5_address = 0; // in EEPROM

int sleepPeriod = 60; // Seconds




// mapping suggestion for ESP32, e.g. LOLIN32, see .../variants/.../pins_arduino.h for your board
// NOTE: there are variants with different pins for SPI ! CHECK SPI PINS OF YOUR BOARD
// BUSY -> 4, RST -> 16, DC -> 17, CS -> SS(5), CLK -> SCK(18), DIN -> MOSI(23), GND -> GND, 3.3V -> 3.3V

// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_3C.h>
#include <../Adafruit_GFX_Library/Fonts/FreeMono9pt7b.h>
#include <../Adafruit_GFX_Library/Fonts/FreeMonoBold9pt7b.h>

GxEPD2_3C<GxEPD2_290c, GxEPD2_290c::HEIGHT> display(GxEPD2_290c(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));

#include "bitmaps/Bitmaps3c128x296.h" // 2.9"  b/w/r

RTC_DATA_ATTR int variable;

void setup()
{
	Serial.begin(115200);
	pinMode(SETTINGS_SWITCH_1_PIN, INPUT);
	pinMode(SETTINGS_SWITCH_2_PIN, INPUT);
	pinMode(SETTINGS_SWITCH_3_PIN, INPUT);
	pinMode(SETTINGS_SWITCH_4_PIN, INPUT);
	flow = getSettings();
	switch (flow) {
	case 1://(1.0.0.0)
		Serial.println("Show temp/hum and sent to MQTT");
		display.init(115200);
		//getSensorData();
		//updateScreen();
		display.powerOff();
		delay(100);
		if (HOME)
		{
			_SSID = SSID;
			_PSWD = PASSWORD;
			host = SERVER_IP;
			mqtt_server = SERVER_IP;
		}
		else
		{
			_SSID = SSID_1;
			_PSWD = PASSWORD_1;
			host = SERVER_IP_1;
			mqtt_server = SERVER_IP_1;
		}
		setup_wifi();
		client.setServer(mqtt_server, 1883);
		client.setCallback(callback);
		connectToBroker();		
		//sendToBrocker();
		sleep(sleepPeriod);
		break;

	case 2://(0.1.0.0)
		Serial.println("OTA");
		delay(100);		
		setup_wifi();
		checkEEPROM();
		delay(100);
		execOTA();		
		break;

	case 3://(0.0.1.0)
		Serial.println("Free slot");
		break;

	case 4://(0.0.0.1)
		Serial.println("Free slot");
		break;

	default:
		Serial.println("Error in settings");
		break;
	}
}

void loop()
{
}



int getSettings() {
	int switch_1 = digitalRead(SETTINGS_SWITCH_1_PIN);
	int switch_2 = digitalRead(SETTINGS_SWITCH_2_PIN);
	int switch_3 = digitalRead(SETTINGS_SWITCH_3_PIN);
	int switch_4 = digitalRead(SETTINGS_SWITCH_4_PIN);
	Serial.println("Setting switches:");
	Serial.println(switch_1);
	Serial.println(switch_2);
	Serial.println(switch_3);
	Serial.println(switch_4);
	if (switch_1 == 1 && switch_2 == 0 && switch_3 == 0 && switch_4 == 0)
	{
		return 1;
	}
	else if (switch_1 == 0 && switch_2 == 1 && switch_3 == 0 && switch_4 == 0)
	{
		return 2;
	}
	else if (switch_1 == 0 && switch_2 == 0 && switch_3 == 1 && switch_4 == 0)
	{
		return 3;
	}
	else if (switch_1 == 0 && switch_2 == 0 && switch_3 == 0 && switch_4 == 1)
	{
		return 4;
	}
	else
	{
		return 0;
	}
}

void setup_wifi() {
	// We start by connecting to a WiFi network

	Serial.print(F("Connecting to "));
	Serial.println(_SSID);

	WiFi.begin(_SSID, _PSWD);
	delay(3000);

	if (WiFi.waitForConnectResult() != WL_CONNECTED) {

		Serial.println(F("Connection Failed!"));
		return;
	}
}

// Utility to extract header value from headers
String getHeaderValue(String header, String headerName) {
	return header.substring(strlen(headerName.c_str()));
}

// Used for storing of MD5 hash
void checkEEPROM() {
	if (!EEPROM.begin(EEPROM_SIZE)) {

		Serial.println("Failed to initialise EEPROM");
		Serial.println("Restarting...");

		delay(1000);
		ESP.restart();
	}
}

void saveMD5toEEPROM() {

	Serial.println("Writing MD5 to EEPROM : " + MD5);

	EEPROM.writeString(MD5_address, MD5);
	EEPROM.commit();

	if (EEPROM.readString(MD5_address) == MD5)
	{
		Serial.println("Successfully written MD5 to EEPROM : " + EEPROM.readString(MD5_address));
	}
	else
	{
		Serial.println("Failed to write MD5 to EEPROM : " + MD5);
		Serial.println("MD5 in EEPROM : " + EEPROM.readString(MD5_address));
	}

}

String loadMD5FromEEPROM() {

	Serial.println("Loaded MD5 from EEPROM : " + EEPROM.readString(MD5_address));

	return EEPROM.readString(MD5_address);
}

// OTA Logic ESP-32
void execOTA() {

	Serial.println("Connecting to: " + String(host));

	// Connect to S3
	if (espClient.connect(host.c_str(), port)) {
		// Connection Succeed.
		// Fecthing the bin

		Serial.println("Fetching Bin: " + String(binPath));

		// Get the contents of the bin file
		espClient.print(String("GET ") + binPath + " HTTP/1.1\r\n" +
			"Host: " + host + "\r\n" +
			"Cache-Control: no-cache\r\n" +
			"User-agent: esp-32\r\n" +
			"MD5: " + loadMD5FromEEPROM() + "\r\n" +
			"Connection: close\r\n\r\n");

		unsigned long timeout = millis();
		while (espClient.available() == 0) {
			if (millis() - timeout > 5000) {

				Serial.println("Client Timeout !");

				espClient.stop();
				return;
			}
		}

		while (espClient.available()) {
			// read line till /n
			String line = espClient.readStringUntil('\n');
			// remove space, to check if the line is end of headers
			line.trim();

			Serial.println(line);

			// if the the line is empty,
			// this is end of headers
			// break the while and feed the
			// remaining `client` to the
			// Update.writeStream();
			if (!line.length()) {
				//headers ended
				break; // and get the OTA started
			}

			// Check if the HTTP Response is 200
			// else break and Exit Update
			if (line.startsWith("HTTP/1.1")) {
				if (line.indexOf("200") < 0) {

					Serial.println("Got a non 200 status code from server. Exiting OTA Update.");

					break;
				}
			}

			// extract headers here
			// Start with content length
			if (line.startsWith("Content-Length: ")) {
				contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str());

				Serial.println("Got " + String(contentLength) + " bytes from server");

			}

			// Next, the content type
			if (line.startsWith("Content-Type: ")) {
				String contentType = getHeaderValue(line, "Content-Type: ");

				Serial.println("Got " + contentType + " payload.");

				if (contentType == "application/octet-stream") {
					isValidContentType = true;
				}
			}
			// Get MD5 from response and compare with stored MD5
			if (line.startsWith("md5: ")) {
				MD5 = getHeaderValue(line, "md5: ");

				Serial.println("Got md5 from response : " + MD5);
				Serial.print("Size of md5 : ");
				Serial.println(sizeof(MD5));

				if (!MD5.equals(loadMD5FromEEPROM()) && sizeof(MD5) > 10) {
					isNewFirmware = true;
				}
				else
				{
					isNewFirmware = false;
				}
			}
		}
	}
	else {
		// Connect to S3 failed
		// May be try?
		// Probably a choppy network?

		Serial.println("Connection to " + String(host) + " failed. Please check your setup");

		// retry??
		// execOTA();
	}

	// Check what is the contentLength and if content type is `application/octet-stream`

	Serial.println("contentLength : " + String(contentLength));
	Serial.println("isValidContentType : " + String(isValidContentType));
	Serial.println("isNewFirmware : " + String(isNewFirmware));

	// check contentLength and content type
	if (contentLength && isValidContentType) {
		if (isNewFirmware)
		{
			// Check if there is enough to OTA Update
			bool canBegin = Update.begin(contentLength);

			// If yes, begin
			if (canBegin) {

				Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");

				// No activity would appear on the Serial monitor
				// So be patient. This may take 2 - 5mins to complete
				size_t written = Update.writeStream(espClient);

				if (written == contentLength) {
					Serial.println("Written : " + String(written) + " successfully");
				}
				else {
					Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
					// retry??
					// execOTA();
				}

				if (Update.end()) {

					Serial.println("OTA done!");

					if (Update.isFinished()) {

						Serial.println("Update successfully completed. Rebooting.");

						saveMD5toEEPROM();
						ESP.restart();
					}
					else {

						Serial.println("Update not finished? Something went wrong!");

					}
				}
				else {

					Serial.println("Error Occurred. Error #: " + String(Update.getError()));

				}
			}
			else {
				// not enough space to begin OTA
				// Understand the partitions and
				// space availability

				Serial.println("Not enough space to begin OTA");

				espClient.flush();
			}
		}
		else
		{

			Serial.println("There is no new firmware");

			espClient.flush();
		}
	}
	else {

		Serial.println("There was no content in the response");

		espClient.flush();
	}
}

void callback(char* topic, byte* payload, unsigned int length) {

	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println("");

	if (strcmp(topic, "Battery/restart") == 0) {
		//Restart ESP to update flash
		ESP.restart();
	}
	if (strcmp(topic, "Battery/sensorRequestPeriod") == 0) {
		String myString = String((char*)payload);
		sensorRequestPeriod = myString.toInt();

		Serial.println(myString);
		Serial.print("Sensor request period set to :");
		Serial.print(sensorRequestPeriod);
		Serial.println(" seconds");
	}
	if (strcmp(topic, "Battery/sleepPeriod") == 0) {
		String myString = String((char*)payload);
		Serial.println(myString);
		sleepPeriod = myString.toInt();
		String sleepPeriodMessage = String() + "Battery(ESP32) sleep period set to : " + sleepPeriod + " seconds";
		Serial.println(sleepPeriodMessage);
		client.publish("Battery/status", sleepPeriodMessage.c_str());
	}
}

//Connection to MQTT broker
void connectToBroker() {

	Serial.print("Attempting MQTT connection...");

	// Attempt to connect
	if (client.connect("Battery")) {

		Serial.println("ESP32 сonnected to MQTT broker");

		// Once connected, publish an announcement...
		client.publish("Battery/status", "Battery(ESP32) connected");
		// ... and resubscribe
		client.subscribe("Battery/sensorRequestPeriod");
		client.subscribe("Battery/restart");
		client.subscribe("Battery/sleepPeriod");
	}
	else {
		Serial.print("failed, rc=");
		Serial.print(client.state());
		Serial.println(" try again in 60 seconds");
	}
}

void sleep(int sleepTimeInSeconds) {
	Serial.print("Go to deep sleep for ");
	Serial.print(sleepTimeInSeconds);
	Serial.println(" seconds");
	// Once connected, publish an announcement...
	client.publish("Battery/status", "Battery(ESP32) goes to sleep for " + sleepTimeInSeconds);
	delay(3000);
	esp_deep_sleep(sleepTimeInSeconds * 1000000);
}




















void helloWorld()
{
	//Serial.println("helloWorld");
	display.setRotation(1);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(GxEPD_RED);
	uint16_t x = (display.width() - 160) / 2;
	uint16_t y = display.height() / 2;
	display.setFullWindow();
	display.firstPage();
	do
	{
		display.fillScreen(GxEPD_WHITE);
		display.setCursor(x, y);
		display.println("Hello World!");
	} while (display.nextPage());
	Serial.println("helloWorld done");
}

void helloFullScreenPartialMode()
{
	Serial.println("helloFullScreenPartialMode");
	display.setPartialWindow(0, 0, display.width(), display.height());
	display.setRotation(1);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(GxEPD_BLACK);
	display.firstPage();
	do
	{
		uint16_t x = (display.width() - 160) / 2;
		uint16_t y = display.height() / 2;
		display.fillScreen(GxEPD_WHITE);
		display.setCursor(x, y);
		display.println("Hello World!");
		y = display.height() / 4;
		display.setCursor(x, y);
		display.println("full screen");
		y = display.height() * 3 / 4;
		if (display.width() <= 200) x = 0;
		display.setCursor(x, y);
		if (display.epd2.hasFastPartialUpdate)
		{
			display.println("fast partial mode");
		}
		else if (display.epd2.hasPartialUpdate)
		{
			display.println("slow partial mode");
		}
		else
		{
			display.println("no partial mode");
		}
	} while (display.nextPage());
	//Serial.println("helloFullScreenPartialMode done");
}

void helloArduino()
{
	//Serial.println("helloArduino");
	display.setRotation(1);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
	uint16_t x = (display.width() - 160) / 2;
	uint16_t y = display.height() / 4;
	display.setPartialWindow(0, y - 14, display.width(), 20);
	display.firstPage();
	do
	{
		display.fillScreen(GxEPD_WHITE);
		display.setCursor(x, y);
		display.println("Hello Arduino!");
	} while (display.nextPage());
	delay(1000);
	//Serial.println("helloArduino done");
}

void helloEpaper()
{
	//Serial.println("helloEpaper");
	display.setRotation(1);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
	uint16_t x = (display.width() - 160) / 2;
	uint16_t y = display.height() * 3 / 4;
	display.setPartialWindow(0, y - 14, display.width(), 20);
	display.firstPage();
	do
	{
		display.fillScreen(GxEPD_WHITE);
		display.setCursor(x, y);
		display.println("Hello E-Paper!");
	} while (display.nextPage());
	//Serial.println("helloEpaper done");
}

void showBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool partial)
{
	//Serial.println("showBox");
	display.setRotation(1);
	if (partial)
	{
		display.setPartialWindow(x, y, w, h);
	}
	else
	{
		display.setFullWindow();
	}
	display.firstPage();
	do
	{
		display.fillScreen(GxEPD_WHITE);
		display.fillRect(x, y, w, h, GxEPD_BLACK);
	} while (display.nextPage());
	//Serial.println("showBox done");
}

void drawCornerTest()
{
	display.setFullWindow();
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(GxEPD_BLACK);
	for (uint16_t r = 0; r <= 4; r++)
	{
		display.setRotation(r);
		display.firstPage();
		do
		{
			display.fillScreen(GxEPD_WHITE);
			display.fillRect(0, 0, 8, 8, GxEPD_BLACK);
			display.fillRect(display.width() - 18, 0, 16, 16, GxEPD_BLACK);
			display.fillRect(display.width() - 25, display.height() - 25, 24, 24, GxEPD_BLACK);
			display.fillRect(0, display.height() - 33, 32, 32, GxEPD_BLACK);
			display.setCursor(display.width() / 2, display.height() / 2);
			display.print(display.getRotation());
		} while (display.nextPage());
		delay(2000);
	}
}

void showFont(const char name[], const GFXfont* f)
{
	display.setFullWindow();
	display.setRotation(0);
	display.setTextColor(GxEPD_BLACK);
	display.firstPage();
	do
	{
		drawFont(name, f);
	} while (display.nextPage());
}

void drawFont(const char name[], const GFXfont* f)
{
	//display.setRotation(0);
	display.fillScreen(GxEPD_WHITE);
	display.setTextColor(GxEPD_BLACK);
	display.setFont(f);
	display.setCursor(0, 0);
	display.println();
	display.println(name);
	display.println(" !\"#$%&'()*+,-./");
	display.println("0123456789:;<=>?");
	display.println("@ABCDEFGHIJKLMNO");
	display.println("PQRSTUVWXYZ[\\]^_");
	if (display.epd2.hasColor)
	{
		display.setTextColor(GxEPD_RED);
	}
	display.println("`abcdefghijklmno");
	display.println("pqrstuvwxyz{|}~ ");
}

void showPartialUpdate()
{
	// some useful background
	helloWorld();
	// use asymmetric values for test
	uint16_t box_x = 10;
	uint16_t box_y = 15;
	uint16_t box_w = 70;
	uint16_t box_h = 20;
	uint16_t cursor_y = box_y + box_h - 6;
	float value = 13.95;
	uint16_t incr = display.epd2.hasFastPartialUpdate ? 1 : 3;
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(GxEPD_BLACK);
	// show where the update box is
	for (uint16_t r = 0; r < 4; r++)
	{
		display.setRotation(r);
		display.setPartialWindow(box_x, box_y, box_w, box_h);
		display.firstPage();
		do
		{
			display.fillRect(box_x, box_y, box_w, box_h, GxEPD_BLACK);
			//display.fillScreen(GxEPD_BLACK);
		} while (display.nextPage());
		delay(2000);
		display.firstPage();
		do
		{
			display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
		} while (display.nextPage());
		delay(1000);
	}
	//return;
	// show updates in the update box
	for (uint16_t r = 0; r < 4; r++)
	{
		display.setRotation(r);
		display.setPartialWindow(box_x, box_y, box_w, box_h);
		for (uint16_t i = 1; i <= 10; i += incr)
		{
			display.firstPage();
			do
			{
				display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
				display.setCursor(box_x, cursor_y);
				display.print(value * i, 2);
			} while (display.nextPage());
			delay(500);
		}
		delay(1000);
		display.firstPage();
		do
		{
			display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
		} while (display.nextPage());
		delay(1000);
	}
}


void drawBitmaps()
{
	display.setFullWindow();
	drawBitmaps3c128x296();
}


struct bitmap_pair
{
	const unsigned char* black;
	const unsigned char* red;
};




void drawBitmaps3c128x296()
{
	bitmap_pair bitmap_pairs[] =
	{
		//{Bitmap3c128x296_1_black, Bitmap3c128x296_1_red},
		//{Bitmap3c128x296_2_black, Bitmap3c128x296_2_red},
		{ WS_Bitmap3c128x296_black, WS_Bitmap3c128x296_red }
	};

	if (display.epd2.panel == GxEPD2::GDEW029Z10)
	{
		for (uint16_t i = 0; i < sizeof(bitmap_pairs) / sizeof(bitmap_pair); i++)
		{
			display.firstPage();
			do
			{
				display.fillScreen(GxEPD_WHITE);
				display.drawInvertedBitmap(0, 0, bitmap_pairs[i].black, display.epd2.WIDTH, display.epd2.HEIGHT, GxEPD_BLACK);
				if (bitmap_pairs[i].red == WS_Bitmap3c128x296_red)
				{
					display.drawInvertedBitmap(0, 0, bitmap_pairs[i].red, display.epd2.WIDTH, display.epd2.HEIGHT, GxEPD_RED);
				}
				else display.drawBitmap(0, 0, bitmap_pairs[i].red, display.epd2.WIDTH, display.epd2.HEIGHT, GxEPD_RED);
			} while (display.nextPage());
			delay(2000);
		}
	}
}
