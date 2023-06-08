#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "settings.h"

const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;

#define LED_R     16    //D0
#define CTRLPZT    5    //D1
#define TBUTTON    4    //D2
#define LED_L      0    //D3
#define LED_B      2    //D4
#define LVLSENS   14    //D5
#define LED_S     12    //D6
#define LED_M     13    //D7
#define CTRLFAN   15    //D8


WiFiServer server(80);            // Create a webserver object that listens for HTTP request on port 80
String header;                    // For HTML

const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqtt_server = MQTT_BROKER;

String sum_str;

volatile char unit_state = 'M';
volatile char unit_level = 'D';
volatile char unit_error = 'E';

long lastMsg = 0;
char msg[50];

unsigned long volatile millis_from_button_interrupt;

WiFiClient Humidifyer_YECFU;
PubSubClient client(Humidifyer_YECFU);

void setState()
{
  if (digitalRead(LVLSENS) == HIGH)
    unit_error = 'E';
  else
    unit_error = 'N';

  if (unit_error == 'E')
  {
    digitalWrite(CTRLPZT, LOW);
    digitalWrite(CTRLFAN, LOW);
    digitalWrite(LED_B, LOW);
    if (unit_state == 'A')
    {
      analogWrite(LED_R, 511);
      digitalWrite(LED_S, LOW);
      digitalWrite(LED_M, LOW);
      digitalWrite(LED_L, LOW);
    }
    else if (unit_state == 'M')
    {  
      if (unit_level == 'D')
      {
        digitalWrite(LED_R, LOW);
        digitalWrite(LED_S, LOW);
        digitalWrite(LED_M, LOW);
        digitalWrite(LED_L, LOW);
      }
      else if (unit_level == 'S')
      {
        analogWrite(LED_R, 511);
        analogWrite(LED_S, 95);
        analogWrite(LED_M, 15);
        analogWrite(LED_L, 15);
      }
      else if (unit_level == 'M')
      {
        analogWrite(LED_R, 511);
        analogWrite(LED_S, 15);
        analogWrite(LED_M, 95);
        analogWrite(LED_L, 15);
      }
      else if (unit_level == 'L')
      {
        analogWrite(LED_R, 511);
        analogWrite(LED_S, 15);
        analogWrite(LED_M, 15);
        analogWrite(LED_L, 95);
      }
    }
  }
  else if ((unit_state == 'A') && (unit_error == 'N'))
  {
    if (unit_level == 'D')  digitalWrite(CTRLFAN, LOW);
    else                    digitalWrite(CTRLFAN, HIGH);
    
    analogWrite(LED_B, 255);
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_S, LOW);
    digitalWrite(LED_M, LOW);
    digitalWrite(LED_L, LOW); 
    
    if      (unit_level == 'D')  digitalWrite(CTRLPZT, LOW);
    else if (unit_level == 'L')  analogWrite(CTRLPZT, 1023);
    else if (unit_level == 'M')   analogWrite(CTRLPZT, 767);
    else if (unit_level == 'S')   analogWrite(CTRLPZT, 511);
  }
  else if ((unit_state == 'M') && (unit_error == 'N'))
  {
    if (unit_level == 'D')
    {
      digitalWrite(CTRLPZT, LOW);
      digitalWrite(CTRLFAN, LOW);
      digitalWrite(LED_B, LOW);
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_S, LOW);
      digitalWrite(LED_M, LOW);
      digitalWrite(LED_L, LOW);    
    }
    else if (unit_level == 'S')
    {
      analogWrite(CTRLPZT, 511);
      digitalWrite(CTRLFAN, HIGH);
      analogWrite(LED_B, 255);
      digitalWrite(LED_R, LOW);
      analogWrite(LED_S, 95);
      analogWrite(LED_M, 15);
      analogWrite(LED_L, 15);    
    }
    else if (unit_level == 'M')
    {
      analogWrite(CTRLPZT, 767);
      digitalWrite(CTRLFAN, HIGH);
      analogWrite(LED_B, 255);
      digitalWrite(LED_R, LOW);
      analogWrite(LED_S, 15);
      analogWrite(LED_M, 95);
      analogWrite(LED_L, 15);
    }
    else if (unit_level == 'L')
    {
      analogWrite(CTRLPZT, 1023);
      digitalWrite(CTRLFAN, HIGH);
      analogWrite(LED_B, 255);
      digitalWrite(LED_R, LOW);
      analogWrite(LED_S, 15);
      analogWrite(LED_M, 15);
      analogWrite(LED_L, 95);    
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  unit_state = (char)payload[0];
  unit_level = (char)payload[1];
  setState();
}

void reconnect()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    //Serial.println("WiFi is OffLine");
    WiFi.begin(ssid, password);
    //delay(500);
    /*if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("");
      Serial.println("WiFi is OnLine.");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }*/
  }
  else
  {
    if ((WiFi.status() == WL_CONNECTED) && (!client.connected()))
    {
      Serial.println();
      Serial.print("WiFi is online. ");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      
      Serial.print("Attempting MQTT connection... ");
      String clientId = "Humidifyer_YECFU";
      if (client.connect(clientId.c_str()))                             // Attempt to connect
      {
        Serial.println("Connected");
        client.publish("humidifyer/YECFU/tele", "Online");                  // Once connected, publish an announcement...
        client.subscribe("humidifyer/YECFU/ctrl");                          // ... and resubscribe
      }
      else
      {
        Serial.println("Connection failed");     //, rc=");
        //Serial.println(client.state());                                
        //delay(500);                                                  // Wait 0.5 seconds before retrying
      }
    }
  }
}

void handleButtonInterrupt()
{
  if (digitalRead(TBUTTON) == HIGH)
  {
    millis_from_button_interrupt = millis();
  }
  else if (digitalRead(TBUTTON) == LOW)
  {
    millis_from_button_interrupt = (millis() - millis_from_button_interrupt);
    
    if (millis_from_button_interrupt >= 1000)
    {
      if ((unit_state == 'M') && (unit_level == 'D'))
      {
        unit_state = 'A';
        unit_level = 'D';
      }
      else
      {
        unit_state = 'M';
        unit_level = 'D';
      }
    }
    else if (unit_state != 'D')
    {
      nextState();
    }
    setState();
  }
}

void handleLvlSnsInterrupt()
{
  if (digitalRead(LVLSENS) == HIGH)
  {
    unit_error = 'E';
  }
  else if (digitalRead(LVLSENS) == LOW)
  {
    unit_error = 'N';
  }
  setState();
}

void nextState()
{
  if      (unit_state == 'A')
  {
    unit_state = 'M';
    unit_level = 'L'; 
  }
  else if (unit_state == 'M')
  {
    if      (unit_level == 'L') unit_level = 'M';
    else if (unit_level == 'M') unit_level = 'S';
    else if (unit_level == 'S')
    {
      unit_state = 'A';
      unit_level = 'D';
    }
  }
}

void setup()
{
  pinMode(CTRLPZT, OUTPUT);
  pinMode(LVLSENS, INPUT_PULLUP);  
  pinMode(TBUTTON, INPUT);
  pinMode(CTRLFAN, OUTPUT);  
  pinMode(LED_B, OUTPUT);  
  pinMode(LED_R, OUTPUT);
  pinMode(LED_S, OUTPUT);
  pinMode(LED_M, OUTPUT);
  pinMode(LED_L, OUTPUT);
  
  attachInterrupt(digitalPinToInterrupt(LVLSENS), handleLvlSnsInterrupt, CHANGE);  
  attachInterrupt(digitalPinToInterrupt(TBUTTON), handleButtonInterrupt, CHANGE);

  setState();
  
  Serial.begin(115200);
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  server.begin();                               // Actually start the server
  Serial.println("HTTP server started");
}

void loop()
{
  long now = millis();
  if (client.connected())
  {
    if (now - lastMsg > 5000)
    {
      lastMsg = now;
      char state_to_publish[2];
      sum_str = (String(unit_state) + String(unit_level) + String(unit_error));
      sum_str.toCharArray(state_to_publish, sum_str.length() + 1);
      client.publish("humidifyer/YECFU/stat", state_to_publish);
    }
  }
  else
  {
    if ((!client.connected()) && (now - lastMsg > 60000))
    {
      lastMsg = now;
      reconnect();
    }
  }
  
  WiFiClient htmlclient = server.available();   // Listen for incoming clients
  
    if (htmlclient)
    {                                           // If a new client connects,
      Serial.println("A New Client.");         // print a message out in the serial port
      String currentLine = "";                  // make a String to hold incoming data from the client
      while (htmlclient.connected())            // loop while the client's connected
      {
        if (htmlclient.available())             // if there's bytes to read from the client,
        {
          char c = htmlclient.read();           // read a byte, then
          //Serial.write(c);                    // print it out the serial monitor
          header += c;
          if (c == '\n')
          {                                                       // if the byte is a newline character
            if (currentLine.length() == 0)                        // if the current line is blank, you got two newline characters in a row.
            {                                                     // that's the end of the client HTTP request, so send a response:
              htmlclient.println("HTTP/1.1 200 OK");              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              htmlclient.println("Content-type:text/html");       // and a content-type so the client knows what's coming, then a blank line:
              htmlclient.println("Connection: close");
              htmlclient.println();

              if (header.indexOf("GET /C/A") >= 0)
              {
                unit_state = 'A';
                unit_level = 'D';
              }
              else if (header.indexOf("GET /C/S") >= 0)
              {
                unit_state = 'M';
                unit_level = 'S';
              }
              else if (header.indexOf("GET /C/M") >= 0)
              {
                unit_state = 'M';
                unit_level = 'M';
              }
              else if (header.indexOf("GET /C/L") >= 0)
              {
                unit_state = 'M';
                unit_level = 'L';
              }
              else if (header.indexOf("GET /C/D") >= 0)
              {
                unit_state = 'M';
                unit_level = 'D';
              }

              setState();
              
              htmlclient.println("<!DOCTYPE html><html>");
              htmlclient.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              htmlclient.println("<link rel=\"icon\" href=\"data:,\">");
              htmlclient.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
              htmlclient.println(".buttonNON { background-color: #195B6A; border: none; color: white; height:60px; width:75px;");
              htmlclient.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              htmlclient.println(".buttonNOFF { background-color: #77878A; border: none; color: white; height:60px; width:75px;");
              htmlclient.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              htmlclient.println(".buttonSON { background-colo r: #195B6A; border: none; color: white; height:60px; width:115px;");
              htmlclient.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              htmlclient.println(".buttonSOFF { background-color: #77878A; border: none; color: white; height:60px; width:115px;");
              htmlclient.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              htmlclient.println(".buttonON { background-color: #195B6A; border: none; color: white; height:60px; width:240px;");
              htmlclient.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
              htmlclient.println(".buttonOFF { background-color: #77878A; border: none; color: white; height:60px; width:240px;");
              htmlclient.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}</style></head>");        
              htmlclient.println("<body><h1>Humidifyer YECFU</h1>");            // Web Page Heading
             
              if  ((unit_state == 'M') && (unit_level == 'D'))  htmlclient.println("<p><a href=\"/C/A\"><button class=\"buttonSON\">" + String(unit_state) + String(unit_level) + "</button></a>");
              else                                              htmlclient.println("<p><a href=\"/C/D\"><button class=\"buttonSOFF\">" + String(unit_state) + String(unit_level) + "</button></a>");
              if  (unit_error == 'E')                           htmlclient.println("<a href=""><button class=\"buttonSON\">" + String(unit_error) + "</button></a></p>");
              else                                              htmlclient.println("<a href=""><button class=\"buttonSOFF\">" + String(unit_error) + "</button></a></p>");
              
              htmlclient.println("<br/><br/>");
              
              if  (unit_state == 'A')                           htmlclient.println("<p><a href=\"/C/A\"><button class=\"buttonON\">AUTO</button></a></p>");
              else                                              htmlclient.println("<p><a href=\"/C/A\"><button class=\"buttonOFF\">AUTO</button></a></p>");
              if  ((unit_state == 'M') && (unit_level == 'S'))  htmlclient.println("<p><a href=\"/C/S\"><button class=\"buttonNON\">S</button></a>");
              else                                              htmlclient.println("<p><a href=\"/C/S\"><button class=\"buttonNOFF\">S</button></a>");
              if  ((unit_state == 'M') && (unit_level == 'M'))  htmlclient.println("<a href=\"/C/M\"><button class=\"buttonNON\">M</button></a>");
              else                                              htmlclient.println("<a href=\"/C/M\"><button class=\"buttonNOFF\">M</button></a>");
              if  ((unit_state == 'M') && (unit_level == 'L'))  htmlclient.println("<a href=\"/C/L\"><button class=\"buttonNON\">L</button></a></p><br/><br/>");
              else                                              htmlclient.println("<a href=\"/C/L\"><button class=\"buttonNOFF\">L</button></a></p><br/><br/>");
              if  ((unit_state == 'M') && (unit_level == 'D'))  htmlclient.println("<p><a href=\"/C/D\"><button class=\"buttonON\">OFF</button></a></p>");
              else                                              htmlclient.println("<p><a href=\"/C/D\"><button class=\"buttonOFF\">OFF</button></a></p>");
              
              htmlclient.println("<br/>");
              htmlclient.println("</body></html>");
            
              htmlclient.println();             // The HTTP response ends with another blank line
              //Serial.print(current_state0); Serial.print(" "); Serial.println(current_state1);
              break;
            }
            else
            { // if you got a newline, then clear currentLine
              currentLine = "";
            }
          }
          else if (c != '\r')
          {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }
        }
      }
      header = "";                 // Clear the header variable
      htmlclient.stop();           // Close the connection
      //htmlclient.flush();        // test this
      Serial.println("Client disconnected.");
      Serial.println("");
    }
  client.loop();
}
