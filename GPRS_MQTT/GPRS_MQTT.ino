#include "GPRS.h" // You have to add your APN here!
#include "MQTT.h" // You have to add your MQTT broker info here!

#include <driver/dac.h>
#include <ArduinoJson.h>
#include <Adafruit_MCP23X17.h>
#define JSON_BUFFER_SIZE       400     // Calculate the right number using:
                                       // https://arduinojson.org/v5/assistant/

#define SERIAL_MON_BAUD_RATE   115200  // 115.2K baud serial connection to computer
#define DATA_PUB_FREQUENCY     1000L  // ms -> 3 seconds

#define LED_PIN 0     // MCP23XXX pin LED is attached to
#define MCP_INPUTPIN 8

Adafruit_MCP23X17 mcp;

StaticJsonDocument<JSON_BUFFER_SIZE> DATA; // Json file that'll contain all data

unsigned long last_time_published = 0;
int packet_id = 0; // Variable used for demo purposes

void setup() {
  Serial.begin(SERIAL_MON_BAUD_RATE);
  // This function call sets up the module but in a blocking matter!
  GPRS_wake_up();  

  MQTT_setup(MQTT_callback);

  if (!mcp.begin_I2C()) {
    Serial.println("Error.");
    while (1);
  }

  // configure pins
  mcp.pinMode(LED_PIN, OUTPUT);
  mcp.pinMode(MCP_INPUTPIN,INPUT);  

  dac_output_enable(DAC_CHANNEL_1);
}


void communicate_(){

  // Container for the data to be sent
  char mqtt_buffer[JSON_BUFFER_SIZE]; 
  
  // Prepare the Json file for sending
  serializeJson(DATA, mqtt_buffer);


  
  // Consult MQTT.h for send_data()
  send_data(mqtt_buffer);

  Serial.print("Published: ");
  Serial.println(mqtt_buffer);
}

void loop() {
  // I did have some issues with the broker timing-out if I don't connect to it just before sending
  MQTT_connect();
  MQTT_loop();  
  // Send data each DATA_PUB_FREQUENCY (ms)
  if(millis() - last_time_published >= DATA_PUB_FREQUENCY){
  
    // Construct the Json file you want to send anyway you want
    String data_ = "Packet n° ";
    data_ += String(packet_id++);
    DATA["1"] = String(mcp.digitalRead(MCP_INPUTPIN));
    DATA["2"] = "1";
    DATA["3"] = "0";
    DATA["4"] = "1";
    DATA["5"] = "0";
    DATA["6"] = "1";
    DATA["7"] = "0";
    DATA["8"] = "0";
    Serial.println(data_);
    communicate_();
    
//    String imei = GPRS_imei();
//    Serial.print("Numero do IMEI: ");
//    Serial.println(imei);

    last_time_published = millis();
  }
}

void MQTT_callback(char*, byte*, unsigned int);

// Use this function in case the module is expected to receive data
// as well as send it. In such case, depending on the topic, 
// you can do whatever you want.
void MQTT_callback(char* topic, byte* message, unsigned int len){
  char *token;
  int i = 0;
  char *array[4]; 
  char *p = strtok (topic, "/");

  while (p != NULL)
  {
      array[i++] = p;
      p = strtok (NULL, "/");
  }
  
//  while ((token = strsep(&topic, "/")));
  String recieved_msg = "";
  for (int i = 0; i < len; i++) {
    recieved_msg += (char)message[i];
  }
  if (String(array[3]) == "do") {
    if (recieved_msg == "0") {
      mcp.digitalWrite(LED_PIN, LOW);        
    } 
    if (recieved_msg == "1") {
      mcp.digitalWrite(LED_PIN, HIGH);    
    }    
  } 
  if (String(array[3]) == "ao") {    
    dac_output_voltage(DAC_CHANNEL_1, String(recieved_msg).toInt());  
  }
  Serial.println(recieved_msg);
}
