/*
  WiFiAccessPoint.ino creates a WiFi access point and provides a web server on it.

  Steps:
  1. Connect to the access point "esp"
  2. The AP will blink its built-in LED twice upon association
  3. The STA client requests data, and the AP responds with a random 2-digit integer (10-99)
     formatted as "VALUE:XX" via raw HTTP

  Created for arduino-esp32 on 04 July, 2018
  by Elochukwu Ifediora (fedy0)
  Updated to include background connection handling and custom text serving.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <NetworkClient.h>
#include <WiFiAP.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED
#endif

// Set these to your desired credentials.
const char *ssid = "esp";
const char *password = "123456789";

NetworkServer server(80);

// Helper function to blink the AP LED twice when a station connects
void blinkLEDTwice() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_BUILTIN, HIGH); delay(200);
    digitalWrite(LED_BUILTIN, LOW);  delay(200);
  }
}

// Background event handler that watches for new Wi-Fi station connections
void WiFiEvent(WiFiEvent_t event) {
  if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
    Serial.println("STA Connected! Blinking LED twice...");
    blinkLEDTwice();
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // Initialize the LED to be off
  
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  // Seed the random generator using internal ADC noise from an unconnected pin
  randomSeed(analogRead(0));

  // Register the background event listener before creating the AP
  WiFi.onEvent(WiFiEvent);

  // You can remove the password parameter if you want the AP to be open.
  // a valid password must have more than 7 characters
  if (!WiFi.softAP(ssid, password, 1, 0, 1)) {
    log_e("Soft AP creation failed.");
    while (1);
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
}

void loop() {
  NetworkClient client = server.accept();  // listen for incoming clients

  if (client) {                     // if you get a client,
    Serial.println("New Client.");  // print a message out the serial port
    String currentLine = "";        // make a String to hold incoming data from the client
    while (client.connected()) {    // loop while the client's connected
      if (client.available()) {     // if there's bytes to read from the client,
        char c = client.read();     // read a byte, then
        if (c == '\n') {            // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            
            // Generate random 2-digit number (10 to 99)
            int randomValue = random(10, 100); 
            Serial.print("Sending value to STA: ");
            Serial.println(randomValue);

            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/plain"); // Set as plain text for easy parsing by the STA
            client.println("Connection: close");
            client.println();
            
            // The content of the HTTP response follows the header:
            client.print("VALUE:");
            client.println(randomValue);
            
            // break out of the while loop:
            break;
          } else {  // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}