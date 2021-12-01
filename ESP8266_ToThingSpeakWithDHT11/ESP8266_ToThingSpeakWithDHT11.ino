#include<EthernetClient.h> 
#include<ESP8266WiFi.h> 

#include "DHT.h"
#define DHTTYPE DHT11 // DHT 11
#define DHTPIN 2
DHT dht(DHTPIN, DHTTYPE);


char jsonBuffer[500] = "["; // Initialize the jsonBuffer to hold data

char ssid[] = "63 Beaucourt"; //  Your network SSID (name)
char pass[] = "gohavefun2020"; // Your network password
WiFiClient client; // Initialize the WiFi client library

char server[] = "api.thingspeak.com"; // ThingSpeak Server

/* Collect data once every 15 seconds and post data to ThingSpeak channel once every 2 minutes */
unsigned long lastConnectionTime = 0; // Track the last connection time
unsigned long lastUpdateTime = 0; // Track the last update time
const unsigned long postingInterval = 20L * 1000L; // Post data every 2 minutes
const unsigned long updateInterval = 15L * 1000L; // Update once every 15 seconds


void setup() {
  Serial.begin(9600);
  dht.begin();

  
  // Attempt to connect to WiFi network
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
    delay(10000);  // Wait 10 seconds to connect
  }
  
  Serial.println("Connected to wifi");
  printWiFiStatus(); // Print WiFi connection information
}


void loop() {
  // If update time has reached 1 second, then update the jsonBuffer
  if (millis() - lastUpdateTime >=  updateInterval) {
    updatesJson(jsonBuffer);
  }
    
}


// Updates the jsonBuffer with data
void updatesJson(char* jsonBuffer){
  /* JSON format for updates paramter in the API
   *  This examples uses the relative timestamp as it uses the "delta_t". 
   *  You can also provide the absolute timestamp using the "created_at" parameter
   *  instead of "delta_t".
   *   "[{\"delta_t\":0,\"field1\":-70},{\"delta_t\":3,\"field1\":-66}]"
   */
  // Format the jsonBuffer as noted above
  strcat(jsonBuffer,"{\"delta_t\":");               // {"delta_t
  unsigned long deltaT = (millis() - lastUpdateTime)/1000;
  size_t lengthT = String(deltaT).length();
  char temp[4];                                    /* Change this for Kenyan Location Temp (use LoRa input from Edge Device) */
//  Serial.println("=====> Temperature =" + String(temp));
  String(deltaT).toCharArray(temp,lengthT+1);
  strcat(jsonBuffer,temp);
  strcat(jsonBuffer,",");
  float rssi = dht.readTemperature(); 
  strcat(jsonBuffer, "\"field1\":");
  lengthT = String(rssi).length();
  String(rssi).toCharArray(temp,lengthT+1);
  strcat(jsonBuffer,temp);

  // more fields:
  char humidity[4];
  strcat(jsonBuffer, ",");
  strcat(jsonBuffer, "\"field2\":");
  float hum = dht.readHumidity();
  lengthT = String(hum).length();
  String(hum).toCharArray(humidity,lengthT+1);
  strcat(jsonBuffer,humidity);

  
  strcat(jsonBuffer,"},");                         // "}
  // If posting interval time has reached 2 minutes, update the ThingSpeak channel with your data
  if (millis() - lastConnectionTime >=  postingInterval) {
        size_t len = strlen(jsonBuffer);
        jsonBuffer[len-1] = ']';
        httpRequest(jsonBuffer);
  }
  lastUpdateTime = millis(); // Update the last update time
}


// Updates the ThingSpeakchannel with data
void httpRequest(char* jsonBuffer) {
  /* JSON format for data buffer in the API
   *  This examples uses the relative timestamp as it uses the "delta_t". 
   *  You can also provide the absolute timestamp using the "created_at" parameter
   *  instead of "delta_t".
   *   "{\"write_api_key\":\"YOUR-CHANNEL-WRITEAPIKEY\",
   *   \"updates\":[{\"delta_t\":0,\"field1\":-60},
   *   {\"delta_t\":15,\"field1\":200},{\"delta_t\":15,
   *   \"field1\":-66}]
   */
  // Format the data buffer as noted above
  char data[500] = "{\"write_api_key\":\"B59JRRWWVXMTAUK6\",\"updates\":"; // Replace YOUR-CHANNEL-WRITEAPIKEY with your ThingSpeak channel write API key
  strcat(data,jsonBuffer);
  strcat(data,"}");
  // Close any connection before sending a new request
  client.stop();
  String data_length = String(strlen(data)+1); //Compute the data buffer length
  Serial.println(data);
  // POST data to ThingSpeak
  if (client.connect(server, 80)) {
    client.println("POST /channels/1587902/bulk_update.json HTTP/1.1"); // Replace YOUR-CHANNEL-ID with your ThingSpeak channel ID
    client.println("Host: api.thingspeak.com");
    client.println("User-Agent: mw.doc.bulk-update (Arduino ESP8266)");
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("Content-Length: "+data_length);
    client.println();
    client.println(data);
  }
  else {
    Serial.println("Failure: Failed to connect to ThingSpeak");
  }
  delay(250); //Wait to receive the response
  client.parseFloat();
  String resp = String(client.parseInt());
  Serial.println("Response code:"+resp); // Print the response code. 202 indicates that the server has accepted the response
  jsonBuffer[0] = '['; //Reinitialize the jsonBuffer for next batch of data
  jsonBuffer[1] = '\0';
  lastConnectionTime = millis(); //Update the last conenction time
}


void printWiFiStatus() {
  // Print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // Print your device IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // Print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
