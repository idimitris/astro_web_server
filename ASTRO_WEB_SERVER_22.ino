
/*********
  Dimitris Kamas
  Astro Web Server
*********/

// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <TimeLord.h>
#include <Timezone.h>

//***************************AstroTime Srart

#define DEBUG 0  // <------  Set to 1 to enable debug messages through serial port monitor
static const char ver[3] = "22";  //  <------Version------
int StartOff = 180; //Set the start offtime in minute of day (90 -> 1:30, 120 -> 02:00, 150 -> 02:30, 180 -> 03:00, 210 -> 03:30)
int EndOff = 300; //Set the end offtime in minute of day (240 -> 04:00, 270 -> 04:30, 300 -> 05:00, 330 -> 05:30)
int del = 10; //Set the delay change time in minute of day

const float LATITUDE = 40.724083, LONGITUDE = 22.907523; // set your GPS position here
int mSunrise, mSunset, mSunriseD, mSunsetD, minOfDay, tizo; //sunrise and sunset expressed as minute of day (0-1439)
int hourNow, minNow, hourLast = -1, minLast = -1;

TimeLord myLord; // TimeLord Object, Global variable
//***************************AstroTime End

//***************************NTP Srart
char data[50];
char* dd;
String dRise;
String dSet;
String  dClock = "No NTP Server";
String  dSunRise = "No SunRise Time";
String  dSunset = "No SunSet Time";
String  ddel = "Delay Not Set";
String  ddia = "diagnostic Not Set";
String  soff = "StartOff Not Set";
String  eoff = "EndOff Not Set";
bool butt = true; 
bool b5on = true; 
bool b5off = true;
bool allon = true; 
bool astro; 
bool onoff;

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

// NTP Servers:
static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "europe.pool.ntp.org;
//static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

//const int TIMEZONE = 2;     // Eastern European Time
//const int TIMEZONE = 1;   // Central European Time
//const int TIMEZONE = -5;  // Eastern Standard Time (USA)
//const int TIMEZONE = -4;  // Eastern Daylight Time (USA)
//const int TIMEZONE = -8;  // Pacific Standard Time (USA)
//const int TIMEZONE = -7;  // Pacific Daylight Time (USA)

const int iterval = 86400;     //Set NTP Iterval sync time (sec)
const int iterval2 = 10;     //Set Iterval sync time when no NTP Connect (sec)


WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
time_t prevDisplay = 0; // when the digital clock was displayed

// NTP end *****************************************************

// Eastern European Time (Athens)
time_t eastern, utc;
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 180};     // Eastern European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 120};       // Eastern European Standard Time
Timezone eEastern(CEST, CET);


// Replace with your network credentials
  
// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output5State = "OFF";
String output4State = "off";
String output3State = "off";
String output2State = "off";
String alwon = "NO";

// Assign output variables to GPIO pins
const int output5 = 5;
//const int output4 = 4;
const int output2 = 2;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

   
const char* ssid     = "*********";
const char* password = "*********";
IPAddress local_IP(192, 168, 0, 22); // Set your Static IP address  Down  Router   
IPAddress gateway(192, 168, 0, 1);   // Set your Gateway IP address

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

void setup() {                            //                     *********** SETUP  **********
  Serial.begin(115200);
   while (!Serial) {
    delay(10); 
   }
  // Initialize the output variables as outputs
  pinMode(output5, OUTPUT);
  //pinMode(output4, OUTPUT);
  pinMode(output2, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output5, HIGH);
  //digitalWrite(output4, LOW);
  digitalWrite(output2, HIGH);
  
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  // Connect to Wi-Fi network with SSID and password
  
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("Dimitris Kamas ESP6288");
  Serial.println("----Version " && ver && "-----");
  Serial.println("-------------------");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  digitalWrite(output2, LOW);   //LOCAL LED ON
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");  
  setSyncProvider(getNtpTime);
 //setSyncInterval(iterval);   //Set Iterval sync time (sec)
  delay(1000);
  digitalWrite(output2, HIGH);  //LOCAL LED OFF

   /* TimeLord Object Initialization */
  
  myLord.Position(LATITUDE, LONGITUDE);
  myLord.DstRules(3,4,10,4,60); // DST Rules for EUROPE - myLord.DstRules(dstMonStart, dstWeekStart, dstMonEnd, dstWeekEnd, 60)

   if (DEBUG == 0) Serial.end();      
}

void loop(){                          //                     *********** LOOP ************

  WiFiClient client = server.available();   // Listen for incoming clients
  
if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
      if (DEBUG == 1) {
      Serial.println(dClock);
      Serial.println(output5State+" "+butt+" "+b5on+" "+b5off);          
      }          
    }
    
    //***************************AstroTime Srart**********************
    
    minNow = minute(eastern);
    hourNow = hour(eastern);
    if (minNow != minLast) {
      minLast = minNow;        
      minOfDay = hourNow * 60 + minNow; //minute of day will be in the range 0-1439

      if (hourNow != hourLast) { // Noting that the Sunrise/Sunset is only calculated every hour => less power. 
        hourLast = hourNow;
        if (3 > month(eastern) < 11 ) {
           tizo = 3;
         }
            else {
             tizo = 2;
         }
         myLord.TimeZone(+tizo * 60);
        byte sunTime[6] = {
          0, 0, 0, byte(day(eastern)), byte(month(eastern)), byte(year(eastern)-2000) }; 
        myLord.SunRise(sunTime);
       // mSunrise = sunTime[2] * 60 + sunTime[1]; //SunRise minute of day in the range 0-1439;
       mSunrise = sunTime[2] * 60 + sunTime[1]; //SunRise minute of day in the range 0-1439;
        mSunriseD = mSunrise - (del/2);
        dRise = stst(mSunriseD);        
      if (DEBUG == 1) {
        Serial.print("SunRise= ");
        print2digits(sunTime[2]);
        Serial.write(':');       
        print2digits(sunTime[1]);
        Serial.write(':');
        print2digits(sunTime[0]);
        Serial.print("  ");             
        print2digits(sunTime[3]);
        Serial.write('/');        
        print2digits(sunTime[4]);
        Serial.write('/');          
        print2digits(sunTime[5]);
        Serial.print("  (");
        Serial.print(mSunrise);
        Serial.print(" - ");
        Serial.print(del);
        Serial.print(" = ");
        Serial.print(mSunriseD);
        Serial.println(")");
        }
        
        sprintf(data, "SunRise = %02d:%02d  (",sunTime[2],sunTime[1]);
        dSunRise = data;
        
        myLord.SunSet(sunTime); 
        mSunset = sunTime[2] * 60 + sunTime[1]; //SunSet minute of day in the range 0-1439;
        mSunsetD = mSunset + del;
        dSet = stst(mSunsetD);           
  if (DEBUG == 1) {
        Serial.print("SunSet= ");       
        print2digits(sunTime[2]);
        Serial.write(':');        
        print2digits(sunTime[1]);
        Serial.write(':');
        print2digits(sunTime[0]);
        Serial.print("  ");        
        print2digits(sunTime[3]);
        Serial.write('/');        
        print2digits(sunTime[4]);
        Serial.write('/');       
        print2digits(sunTime[5]);
        Serial.print("  (");
        Serial.print(mSunset);
        Serial.print(" + ");
        Serial.print(del);
        Serial.print(" = ");
        Serial.print(mSunsetD);
        Serial.println(")");
                            
        Serial.print("StartOff= ");
        Serial.print(StartOff);
        Serial.print("  EndOff= ");
        Serial.print(EndOff);
        Serial.print("  MinOfDel: ");
        Serial.print(del);
        Serial.println();
        }
           
        sprintf(data, "SunSet = %02d:%02d  (",sunTime[2],sunTime[1]);
        dSunset = data;
        
    }  // End: if (hourNow != hourLast)
    
   if (minOfDay == StartOff or minOfDay == mSunriseD) butt = true;
    
      if (butt) {
       if ((minOfDay < mSunriseD || minOfDay >= mSunsetD) && (minOfDay < StartOff || minOfDay >= EndOff)) {
        digitalWrite(output5, LOW);         
        output5State = "ON"; 
        client.println("<p><a href=\"/5/off\"><button class=\"button button2\">OFF</button></a></p>");
        }
       else {
        if (allon) {
        digitalWrite(output5, HIGH);         
        output5State = "OFF";
        client.println("<p><a href=\"/5/on\"><button class=\"button\">ON</button></a></p>");   
          }                  
        }
      }  
          
  if (DEBUG == 1) {
      Serial.print("Ok, Time= ");
      print2digits(hour(eastern));
      Serial.write(':');
      print2digits(minute(eastern));
      Serial.write(':');
      print2digits(second(eastern));
      Serial.print(", Date (D/M/Y)= ");
      Serial.print(day(eastern));
      Serial.write('/');
      Serial.print(month(eastern));
      Serial.write('/');
      Serial.print(year(eastern));
      Serial.println();
      Serial.print("MinOfDay = ");
      Serial.print(minOfDay);
      Serial.print("  LED:");
      Serial.print((minOfDay < mSunriseD || minOfDay >= mSunsetD) && (minOfDay < StartOff || minOfDay >= EndOff));
      Serial.println();
    }
   } // End: if (minNow != minLast)
  } // End:  if (timeStatus()!= timeNotSet)
  delay(1000);
  
    //***************************AstroTime End******************************
  
  if (client) {                             // If a new client connects,
    if (DEBUG == 1) Serial.println("New Client.");      // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
       // Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
           
            // turns the GPIOs on and off
            if (header.indexOf("GET /5/on") >= 0 && b5off) {
             if (DEBUG == 1) Serial.println("GPIO 5 on");
              output5State = "ON";
              digitalWrite(output5, LOW);
              if (butt==false) butt = true;
              else butt = false;
            
            } else if (header.indexOf("GET /5/off") >= 0 && b5on) {
              if (DEBUG == 1) Serial.println("GPIO 5 off");
              output5State = "OFF";
              digitalWrite(output5, HIGH);
              if (butt==false) butt = true;
              else butt = false;
              
            } else if (header.indexOf("GET /4/on") >= 0) {
              if (DEBUG == 1) Serial.println("GPIO 4 on");
              output4State = "on";
              del=del+5;             
              dSet = stst(mSunset + del);
              dRise = stst(mSunrise + (del/2));                              
                       
            } else if (header.indexOf("GET /4/off") >= 0) {
              if (DEBUG == 1) Serial.println("GPIO 4 off");
              output4State = "off";
              del=del-5;
              dSet = stst(mSunset + del);              
              dRise = stst(mSunrise + (del/2));  
                          
            } else if (header.indexOf("GET /3/on") >= 0) {
              if (DEBUG == 1) Serial.println("GPIO 3 on");
              output3State = "on";
              StartOff=StartOff+30;
                       
            } else if (header.indexOf("GET /3/off") >= 0) {
              if (DEBUG == 1) Serial.println("GPIO 3 off");
              output3State = "off";
              StartOff=StartOff-30;  

            } else if (header.indexOf("GET /2/on") >= 0) {
              if (DEBUG == 1) Serial.println("GPIO 2 on");
              output2State = "on";
              EndOff=EndOff+30;
                       
            } else if (header.indexOf("GET /2/off") >= 0) {
              if (DEBUG == 1) Serial.println("GPIO 2 off");
              output2State = "off";
              EndOff=EndOff-30;    

            } else if (header.indexOf("GET /6/on") >= 0) {
              if (DEBUG == 1) Serial.println("AllON is on");
              alwon = "YES";
              
                       
            } else if (header.indexOf("GET /6/off") >= 0) {
              if (DEBUG == 1) Serial.println("AllON is off");
              alwon = "NO";
                          
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");            
            //client.println("<meta http-equiv='refresh' content='5; URL=http://dimitris.dyndns.info'>");
            
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #42a1d0;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Astro Time Server</h1>");
            client.println("<h3>" + dClock + "</h3>");
            client.println("<h3>" + dSunRise + dRise + ")" + "</h3>");
            client.println("<h3>" + dSunset + dSet + ")" + "</h3>");
                                 
            // Display current state, and ON/OFF buttons for GPIO 5  
            client.println("<p><b>Lights - State : " + output5State +"</b></p>");
            
            // If the output5State is off, it displays the ON button       
            if (output5State=="OFF") {
              client.println("<p><a href=\"/5/on\"><button class=\"button\">ON</button></a></p>");
              b5on = false;
              b5off = true;
            } else {
              client.println("<p><a href=\"/5/off\"><button class=\"button button2\">OFF</button></a></p>");
              b5on = true;
              b5off = false;
            } 
               
            // Display current state, and ON/OFF buttons for GPIO 4  
            sprintf(data, "= %d min", del);
            ddel = data;            
            client.println("<p><b>Delay Time " + ddel + "</b></p>");           
         
             client.println("<p><a href=\"/4/on\"><button class=\"button\">+</button></a>");    
             client.println("<a href=\"/4/off\"><button class=\"button\">-</button></a></p>");

             soff = "StartOFF  " + stst(StartOff);  
             client.println("<p><b>" + soff + "</b></p>");
             client.println("<p><a href=\"/3/on\"><button class=\"button\">+</button></a>");
             client.println("<a href=\"/3/off\"><button class=\"button\">-</button></a></p>");

              eoff = "  EndOFF  " + stst(EndOff);
             client.println("<p><b>" + eoff +"</b></p>");
             client.println("<p><a href=\"/2/on\"><button class=\"button\">+</button></a>");
             client.println("<a href=\"/2/off\"><button class=\"button\">-</button></a></p>");

              // displays always on status    
             client.println("<p><b>Always on - State : " + alwon +"</b></p>");   
            if (alwon=="NO") {
              client.println("<p><a href=\"/6/on\"><button class=\"button\">YES</button></a></p>");
              allon = true;
            } else {
              client.println("<p><a href=\"/6/off\"><button class=\"button button2\">NO</button></a></p>");                
              allon = false;
            } 
             
             astro=(minOfDay < mSunriseD || minOfDay >= mSunsetD) && (minOfDay < StartOff || minOfDay >= EndOff);
             if (output5State=="OFF") {
              onoff = false;
            } else {            
              onoff = true;             
            } 
             sprintf(data, "( diagnostic: %d, %d, %d, %d, %d, (%s)-%d )", onoff, astro, butt, b5on, b5off, ver, DEBUG);
             ddia=data;
             client.println("<p><b>" + ddia + "</b></p>");                  
               
             client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    if (DEBUG == 1) {
    Serial.println("Client disconnected.");
    Serial.println("");
    }
  }       
}

void digitalClockDisplay()
{
  
  // digital clock display of the time
  switch (weekday()) {
  case 2:
      dd = "Monday";
    break;
  case 3:
      dd = "Tuesday";
    break;
  case 4:
      dd = "Wednesday";
    break;
  case 5:
      dd = "Thursday";
    break;
  case 6:
      dd = "Friday";
    break;
  case 7:
      dd = "Saturday";
    break; 
  case 1:
      dd = "Sunday";
    break;     
  default:
      dd = "Error";
    break;
}
  utc = now();  //current time from the Time Library
  eastern = eEastern.toLocal(utc); //convert to Local Time
  sprintf(data, "%s %02d/%02d/%d %02d:%02d:%02d", dd, day(eastern), month(eastern),year(eastern),hour(eastern),minute(eastern),second(eastern));
  dClock = data;
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}

/*-------- NTP code ----------*/
time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      setSyncInterval(iterval);
      return secsSince1900 - 2208988800UL + 0 * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  setSyncInterval(iterval2);
  return 0; // return 0 if unable to get the time
}


// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{  
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

String stst(long ttt) {
    long days = 0;
    long hours = 0;
    long mins = 0;
    String mins_o = ":";
    String hours_o = " ";
    hours = ttt / 60; //convert minutes to hours
    days = hours / 24; //convert hours to days
    mins = ttt - (hours * 60); //subtract the coverted minutes to hours in order to display 59 
    hours = hours - (days * 24); //subtract the coverted hours to days in order to display 23 
     if (mins < 10) {
      mins_o = ":0";
      }
     if (hours < 10) {
      hours_o = "0";
      }
      sprintf(data, "%02d:%02d", hours, mins );
      return data;
    }
