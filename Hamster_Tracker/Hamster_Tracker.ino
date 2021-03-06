
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#include <Bounce.h>
//setup for time tracking and timestamps
#define NTP_OFFSET   0      // In seconds
#define NTP_INTERVAL 60 * 10000    // In miliseconds
#define NTP_ADDRESS  "pool.ntp.org"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

// Update these with values suitable for your network.

const char* mqtt_server = "ha.lan";
const char* topic = "Tracker";     // this is the [root topic]
const char* starttopic = "Online"; 
short timeBetweenMessages = 5000;

int rotationstart = 0;
int rotationendtime;
int rotationstarttime;
int rotationtime;
int rotations =0;


int wheelspeed = 0;
//diameter of inside of wheel
int wheeldiamter = 293;
int distance = 0;
int period = 10000;
unsigned long time_now=0;
byte startup = 0;
int maxspeed = 0;
int minspeed = 0;
int avgspeed = 0;
byte speedstart;
int speedbegin;
int speedend;
int speedduration;
//calculate how long sprint goes for 
byte sprintstart = 0;
short speedincrement;
long sprintstartTime = 0;
long sprintendTime  = 0;
long sprintduration = 0;
//define reed switch
int ReedState = 0;
int LastReedState = 0;
const int  ReedPin = D6;  

WiFiClient espClient; 
PubSubClient client(espClient);
long lastMsg = 0;
int value = 0;

int status = WL_IDLE_STATUS;     // the starting Wifi radio's status
void reconnect() 
{
  // Loop until we're reconnected
  Serial.println("Reconnect Mode");
  while (!client.connected()) 
   {
    Serial.print("Attempting MQTT connection...");
    String clientId = "Tracker" ;
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" wifi=");
      Serial.print(WiFi.status());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() 
{
   //setup reed switch need to pull down 
   pinMode(ReedPin, INPUT_PULLUP);  
   Serial.begin(115200);
   timeClient.begin();
   // We start by connecting to a WiFi network
   WiFiManager MyWifiManager;
   MyWifiManager.autoConnect("Trackers");
   client.setServer(mqtt_server, 1883);
}

void loop() 
{
  // put your main code here, to run repeatedly:
  // confirm still connected to mqtt server
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
  timeClient.update();
  String formattedTime = timeClient.getFormattedTime();
  //setup pin 
  ReedState = digitalRead(ReedPin);
  long now = millis();

  //check if wheel is running if it is increment rotations

   if (ReedState != LastReedState )
   {
      if (ReedState == LOW)
        { Serial.println("Wheel Cycle");
         Serial.println("rotations plus plus");
         
        rotations ++;
        //calculate the time it takes to do one rotation
        if(startup == 1)
        {
          speedbegin = millis();
          
        }
        if(startup == 0)
        {
          speedend = millis();
          startup = 1;
        }
        //calculate how long he runs for
        if(sprintstart == 0)
        {
          sprintstart = 1;
          sprintstartTime = millis();
          Serial.println("Sprint Start");
        }
        //reset counter
       lastMsg = now;
       
    // delay(50);
       sprintendTime = millis();
       }
      if(ReedState == HIGH)
        {

        speedduration = speedbegin - speedend;
        Serial.println(speedduration);
        startup = 0;
        }
    }
    LastReedState = ReedState;
    //if there has been movement and it is after 10 seconds of no activity then publish results 
    //this calc is not working for some reason causing mqtt not to publish but calculation is right 
     sprintduration=  sprintendTime - sprintstartTime;
    if (rotations > 0)
    {
      //compensate for double trigger of rotations
        int  calcrotations = rotations/2;
        if (now - lastMsg > timeBetweenMessages ) 
        {
         
          distance = calcrotations * (wheeldiamter*3.14);
          lastMsg = now;
          ++value;
          String payload = "{\"r\":";
          payload += calcrotations;
          payload += ",\"as\":";
          payload += avgspeed;
          payload += ",\"mas\":";
          payload += maxspeed;
          payload += ",\"mis\":";
          payload += minspeed;    
          payload += ",\"d\":";
          payload += distance;   
          payload += ",\"ts\":";
          payload += '"';          
          payload += formattedTime;   
          payload += '"';
          payload += ",\"st\":";
          payload += sprintduration;                                      
          payload += "}";
          String pubTopic;
           pubTopic += topic ;
          Serial.print("Publish topic: ");
          Serial.println(pubTopic);
          Serial.print("Publish message: ");
          Serial.println(payload);
          client.publish( (char*) pubTopic.c_str() , (char*) payload.c_str(), true );
          distance = 0;
          rotations = 0;
          wheelspeed = 0;   
         sprintstart = 0;
        }
     }
}
