/*
Machine monitoring project:
Supported by Department of Industrial Promotion: Ministry of Industry, Thailand
This project captures machine data and stores in the desinated database using php script file located at the server
With the data in database, dashboard could be created using open source program such as Grafana. Then you could have
the real time machine data illustrated in line graph, guage or tables. With some calculation, OEE of the machine could be
calculated and reported real time.
Code: Developed by Dr. Pasidth Thanachotanankul
Date: December 2018
*/
#include <SPI.h>
#include <Ethernet.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <TimerOne.h>
#include <avr/wdt.h>
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//char server[] = (192,168,2,34);

//for changing IP address *********************************
IPAddress server(x,x,x,x);  // numeric IP for server
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
//for changing duration time to mySQL *********************
long timeInterval = 300000;//300000.miliseconds = 5 minutes

// Set the static IP address to use if the DHCP fails to assign
//IPAddress ip(192, 168, 2, 177);
//IPAddress myDns(192, 168, 2, 1);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;
String filename = "x.x.x.x.ino";
// Variables to measure the speed
bool printWebData = true;  // set to false for better speed measurement
char charBuf[512];
String dataString;
char dataBuf[256];
String dataArry[6];
bool connectFlag = false;
int pageIndex =0; //page index for LCD
const int BUFSIZE = 20;
char buf[BUFSIZE];
String myString; 
char myStringChar[BUFSIZE];

#define ipsw0 22 //input Start Time
#define ipsw1 23 //input Run Time
#define ipsw2 24 //input Stop Time
#define ipsw3 2 //input Counter .. interrupt
#define aip0 A0 //analog input #A0
#define op1 25 //output#1 pull down D10/D8
#define sw1 40 //dip switch #1 McNo
#define sw2 41 //dip switch #2 McNo
#define sw3 42 //dip switch #3 Relay on/off
#define sw4 43 //dip switch #4
#define ledonboard 12 //led built in

//#define wifienable 36 //send enable signal to wifi module
int addressee = 1;      //EEPROM address counter
int addressID = 5;

int valueEEPROM; //value of EEPROM
int valueEEPROM2; //value of EEPROM
int valueEEPROM3; //value of EEPROM
char inChar;
int sw[4]; //switch array
int ipsw[4]; //input array
int analogip; //analog input
int output; //output1
const byte interruptPin = 2; //GPIO 13 set to interrupt pin
int sumcounter = 0; //accumulate counter
boolean production_flag = 0;
unsigned long previousTime = 0;

unsigned int value =0;
unsigned int value2 =0;
unsigned int value3 =0;
unsigned int tvalue =0;
unsigned int tvalue2 =0;
unsigned int tvalue3 =0;
char sumcounterarry[6];
long numberText;
long longCounter=0;
long prevlongCounter =0;
long tempCounter,ttempCounter;

String textmsg = "";
String data;

String ID = "";
String LotNo = "";
String ItemNo = "";
long PlanQty = 0;
String ID2 = "";
String LotNo2 = "";
String ItemNo2 = "";
long PlanQty2 = 0;

String McNo = "MC-001"; //default value
String LotNoStr;
String ItemNoStr;
String McNoStr;
double TotalQty = 0;
float TotalDefect =0;
float Qty =0;
int Status =0;
int Status1 =0;

boolean StartTime =0;
boolean RunTime =0;
boolean StopTime =0;
String urlFinal = "";
String url = "";
boolean startFlag =0;
boolean runFlag =0;
boolean stopFlag =0;
boolean prestartFlag =0;
boolean prerunFlag =0;
boolean prestopFlag =0;
boolean eepromFlag =0;
boolean mcindexFlag =0;
boolean countFlag =0;
char statusMc;
int StatusPlan =0;
boolean serverFlag =0;
boolean errFlag =0;
bool startrunFlag =0;
bool overcountFlag =0;
bool relayFlag =0;
bool noLotFlag =1;
String IDstr ="";
bool eepromWriteFlag =0;
long timeDisplay = 5000;
long prevDisplay = 0;


//-----------------------for eeprom
const int EEPROM_MIN_ADDR = 0;
const int EEPROM_MAX_ADDR = 511;

boolean eeprom_is_addr_ok(int addr) {
  return ((addr >= EEPROM_MIN_ADDR) && (addr <= EEPROM_MAX_ADDR));
}

boolean eeprom_write_bytes(int startAddr, const byte* array, int numBytes) {
// counter
  int i;
  // both first byte and last byte addresses must fall within
  // the allowed range 
  if (!eeprom_is_addr_ok(startAddr) || !eeprom_is_addr_ok(startAddr + numBytes)) {
    return false;
  }
  for (i = 0; i < numBytes; i++) {
    EEPROM.write(startAddr + i, array[i]);
  }
    return true;
  }

// Writes a string starting at the specified address.
// Returns true if the whole string is successfully written.
// Returns false if the address of one or more bytes fall outside the allowed range.
// If false is returned, nothing gets written to the eeprom.
boolean eeprom_write_string(int addr, const char* string) {

    int numBytes; // actual number of bytes to be written
    //write the string contents plus the string terminator byte (0x00)
      numBytes = strlen(string) + 1;
        return eeprom_write_bytes(addr, (const byte*)string, numBytes);
}

// Reads a string starting from the specified address.
// Returns true if at least one byte (even only the string terminator one) is read.
// Returns false if the start address falls outside the allowed range or declare buffer size is zero.
// 
// The reading might stop for several reasons:
// - no more space in the provided buffer
// - last eeprom address reached
// - string terminator byte (0x00) encountered.
boolean eeprom_read_string(int addr, char* buffer, int bufSize) {
    byte ch; // byte read from eeprom
    int bytesRead; // number of bytes read so far
      if (!eeprom_is_addr_ok(addr)) { // check start address
        return false;
      }
    
      if (bufSize == 0) { // how can we store bytes in an empty buffer ?
        return false;
      }
    // is there is room for the string terminator only, no reason to go further
      if (bufSize == 1) {
        buffer[0] = 0;
        return true;
      }
    bytesRead = 0; // initialize byte counter
    ch = EEPROM.read(addr + bytesRead); // read next byte from eeprom
    buffer[bytesRead] = ch; // store it into the user buffer
    bytesRead++; // increment byte counter
    // stop conditions:
    // - the character just read is the string terminator one (0x00)
    // - we have filled the user buffer
    // - we have reached the last eeprom address
    while ( (ch != 0x00) && (bytesRead < bufSize) && ((addr + bytesRead) <= EEPROM_MAX_ADDR) ) {
        // if no stop condition is met, read the next byte from eeprom
        ch = EEPROM.read(addr + bytesRead);
        buffer[bytesRead] = ch; // store it into the user buffer
        bytesRead++; // increment byte counter
     }
      // make sure the user buffer has a string terminator, (0x00) as its last byte
      if ((ch != 0x00) && (bytesRead >= 1)) {
        buffer[bytesRead - 1] = 0;
      }
      return true;
  }

//-----------------------end eeprom
void timerIsr()
{
    // Toggle relay
    if (digitalRead(sw3) ==1){
      if (digitalRead(ipsw0) ==1 || digitalRead(ipsw1) ==1){
        digitalWrite( op1, digitalRead( op1 ) ^ 1 );
      }
      else digitalWrite(op1,LOW);
  }
}
void count(){
  if (ipsw[0] ==1 && ipsw[1] == 1 && ipsw[2] ==0){
      //ipsw[3] = digitalRead(ipsw3);
      value ++;
      longCounter ++;
      //writeCounter2();//write to eeprom
      if(value>=256){
        value = 0;
        value2 ++;
        if (value2 >= 256){
          value2 = 0;
          value3++;
        }
      }
      //Serial.print("ipsw[3](counter)= ");
      //Serial.println(ipsw[3]);  
      Serial.print("long counter +++ = ");
      Serial.println(longCounter);
      Serial.print("value3= ");Serial.println(value3);
      Serial.print("value2= ");Serial.println(value2);
      Serial.print("value= ");Serial.println(value);         
      long ttemp = value + value2*256 + value3*65536;
      Serial.print("Cal counter= ");
      Serial.println(ttemp);
    }
    else Serial.println("No Counting");
}

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  Serial.print(filename);
  Serial.println(" ...Run Setup");
  ipsw[3] =0; //counter
  lcd.init();                      // initialize the lcd 
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Machine Monitor ");
  lcd.setCursor(0,1);
  lcd.print("     System     ");
  delay(1000);
  lcdBlink(3,500);
  pinMode(sw1,INPUT_PULLUP);//dip switch mcno
  pinMode(sw2,INPUT_PULLUP);//mcno
  pinMode(sw3,INPUT_PULLUP);//relay on/off
  pinMode(sw4,INPUT_PULLUP);//output 1 enable
  
  pinMode(ipsw0,INPUT_PULLUP);//start
  pinMode(ipsw1,INPUT_PULLUP);//run
  pinMode(ipsw2,INPUT_PULLUP);//stop
  pinMode(ipsw3,INPUT_PULLUP); //interrupt pin
  
  pinMode(op1,OUTPUT);
  pinMode(ledonboard,OUTPUT);
  digitalWrite(op1,LOW);
  delay(100);
//  while (!Serial) {
//    ; // wait for serial port to connect. Needed for native USB port only
//  }

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  Ethernet.begin(mac);
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
      lcd.setCursor(0,0);
      lcd.print("   Pls check   ");
      lcd.setCursor(0,1);
      lcd.print("ethernet Cable ");
      lcdBlink(3,500);
      connectFlag =0;
  }
  else connectFlag =1;
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      lcd.setCursor(0,0);
      lcd.print("   Pls check   ");
      lcd.setCursor(0,1);
      lcd.print("ethernet shield");
      lcdBlink(3,400);
      connectFlag =0;
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  else connectFlag =1;
  
  // give the Ethernet shield a second to initialize:
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   My I.P. is   ");
  lcd.setCursor(0,1);
  lcd.print(Ethernet.localIP());
  delay(1000);
  Serial.print("connecting to ");
  Serial.print(server);
  Serial.println("...");
  lcd.clear();
  textmsg = "Connecting server";
  lcdprint(0);
  delay(2000);
  lcd.clear();
      // read input/switch status
  readstatus();
    sw[0] = !digitalRead(sw1); //dip switch McNo
    sw[1] = !digitalRead(sw2); //for McNo
    sw[2] = !digitalRead(sw3); //start-run alarm on
    sw[3] = !digitalRead(sw4); //over counting alarm on
    //check for mc no
    if(sw[0] ==0 && sw[1] ==0){ // sw[0]:sw[1] = 0:0 = MC-001
      McNo = "MC-001";
    }
    else if (sw[0] ==0 && sw[1] ==1) //sw[0]:sw[1] = 0:1 = MC-002
      McNo = "MC-002";
    else if (sw[0] ==1 && sw[1] ==0) //sw[0]:sw[1] = 1:0 = MC-003
      McNo = "MC-003";
    else if (sw[0] ==1 && sw[1] ==1) //sw[0]:sw[1] = 1:1 = MC-004
      McNo = "MC-004"; 
    else   McNo = "MC-001"; //default
    
    //check for start-run alarm enable
    startrunFlag = sw[2];
    Serial.print("Start-Run Flag= ");
    Serial.println(startrunFlag);
    //check for over counting alarm enable
    overcountFlag = sw[3];
    Serial.print("overcount Flag= ");
    Serial.println(overcountFlag);    
    textmsg = "MCNo = "+McNo;
    lcdprint(1); 

  if (connectFlag ==1){
      readcounterEEPROM();
      delay(2000);
      attachInterrupt(digitalPinToInterrupt(interruptPin),count, RISING);
      //lcd.clear();
      sendRequest();
      delay(100);
      incomingData();
      delay(100); 
      if (Status1 >0){
        ID = ID2;
        PlanQty = PlanQty2;
        LotNo = LotNo2;
        ItemNo = ItemNo2;   
      }
  }
  
  Timer1.initialize(1500000); // set a timer of length 1 seconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
  Timer1.attachInterrupt( timerIsr ); // attach the service routine here

  wdt_enable(WDTO_8S); //watchdog enable 8 sec.
  wdt_reset(); //use this reset command before 8sec. if longer than 8 sec, system restart 
  
}

//LoooooooooooooooooooooooooooooP//
void loop() {
  Serial.print("Connection Flag= ");
  Serial.println(connectFlag);
  Serial.print("Lot No err Flag= ");
  Serial.println(errFlag);
    if (connectFlag == false){ //connection lossed
          lcd.setCursor(0,0);
          lcd.print("Connection      ");
          lcd.setCursor(0,1);
          lcd.print("Error ?         ");
        /*
        readPlanning();
        if (LotNo == ""){
             errFlag = 1;
        }
        else {
            errFlag =0;noLotFlag =0;
        }*/
    }
    else { //connectFlag = 1 or Has connection
        if (errFlag ==1){ //if no Lot No
          wdt_reset();
          lcd.setCursor(0,0);
          lcd.print("No LotNo. Check ");
          lcd.setCursor(0,1);
          lcd.print("Production Plan?");
          noLotFlag =1;   
          lcdBlink(2,250);
          wdt_reset();
          delay(5000);
          wdt_reset();
          readPlanning();
          lcd.clear();
          if (LotNo == ""){
             errFlag = 1;
          }
          else {
            errFlag =0;noLotFlag =0;
          }
        }
        else { //if has LotNo
          unsigned long currentTime = millis();
          //checkEEPROM();
          urlData();
          wdt_reset();
          if (ipsw[0] == 0 ){//
              Serial.println("Start Butt =0");
              //readPlanning();
              if (serverFlag == 0){
                 readPlanning();
                 if (LotNo == ""){
                  errFlag =1;
                 }
                 else errFlag =0;
                 wdt_reset();
                }
              wdt_reset();
              delay(200);
              ipsw[0] = !digitalRead(ipsw0);//start switch
              lcd.setCursor(0,0);
              lcd.print("LotNo:");
              lcd.print(LotNo);
              lcd.setCursor(0,1);
              lcd.print("Press Start Butt");
              lcdBlink(2,500);
              wdt_reset(); 
              lcd.clear();     
              }
          else {
      
              Serial.println("Start Butt =1");
              if (currentTime - previousTime >= timeInterval){
                  previousTime = currentTime;
                  if (mcindexFlag ==0){
                    Serial.println("Not post data");
                    return;
                  }
                  else {
                   Serial.println("Posting data");
                   Qty = longCounter - prevlongCounter;
                   callmcIndex();
                   prevlongCounter = longCounter;       
                  }
              }
              wdt_reset();
              readstatus();
              writeCounter2();//eeprom writer
              //relay output
              lcdDisp();
              //lcd.setCursor(15,1);
              //lcd.print(" "); 
              wdt_reset();
              //delay(100);
              //serverFlag =0;
          }
      }   
  }
  if (!digitalRead(sw3) ==0){
    startrunFlag =0;
  }
  else startrunFlag =1;
  delay(100);
}
void sendRequest(){
    // if you get a connection, report back via serial:
  Serial.println("Sending request");
  if (client.connect(server, 80)) {
    Serial.print("connected to ");
    Serial.println(client.remoteIP());
    connectFlag = true;
      //send data to web server
    String data = "McNo=";
    data.concat('"');data.concat(McNo);data.concat('"');  
    Serial.print("This is ");Serial.println(McNo);
    // Make a HTTP request:
    client.println("POST /getNextPlan.php HTTP/1.1");
    client.println("Host: localhost");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(data.length());
    client.println();
    client.print(data);
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
    connectFlag = false;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connection");
    lcd.setCursor(0,1);
    lcd.print("Failed !!!");
    delay(500);
    lcd.clear();
    //relayFlag =1;    
  }
}
void readPlanning(){
      sendRequest();
      delay(100);
      if (connectFlag == true){
        incomingData();
      }
}

void readstatus(){
    ipsw[0] = !digitalRead(ipsw0);//start switch
    ipsw[1] = !digitalRead(ipsw1);//run switch
    ipsw[2] = !digitalRead(ipsw2);//stop switch
    ipsw[3] = !digitalRead(ipsw3);//digital input

   //check start button **************************
    if (ipsw[0] == 1){ //machine time start
        Serial.println("Machine Started");
        startFlag = 1;
        //readcounterEEPROM();
        Serial.println("startFlag =1");
        //if status changed post to DB
        if (prestartFlag != startFlag){
          Serial.println("Start Time --> call mcIndex ");
          callmcIndex(); //post to mySQL
          prestartFlag = startFlag;
          EEPROM.write(10, 1); //write status byte= 1
          setplanStartTime();
        }
        
        //check run button ********************************
        ipsw[1] = !digitalRead(ipsw1); //production start
        if (ipsw[1] == 1){ //machine start and run time start
            Serial.println("Production Started");//reset counter if first time production started
            runFlag = 1;relayFlag =0;
            Serial.println("runFlag =1");
            //if status changed post to DB
            if (prerunFlag != runFlag){
              Serial.println("Run Time --> call mcIndex ");
              callmcIndex(); //post to mySQL
              prerunFlag = runFlag;
              setplanRunTime();
              EEPROM.write(10, 2); //write status byte= 2
            }
         }//end if run time start
         
        //else if run time not pressed = not ok --> freezed
        else { //mc start, run time not start
           Serial.println("Waiting for Production Start 2, runFlag =0");
           runFlag =0; //reset runFlag
           //if status changed post to DB
           if (prerunFlag != runFlag){
              Serial.println("Run time --> call mcIndex ");
              callmcIndex(); //post to mySQL
              prerunFlag = runFlag;
           }
          Serial.println("Counter Paused");
          relayFlag =1;//on-off toggle 1 second
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Total Count=    ");
          lcd.setCursor(0,1);
          //lcd.print(longCounter);
            char buffer [18]; // a few bytes larger than your LCD line
            uint16_t ccounter = longCounter; // data read from instrument
            sprintf (buffer, "%06u", ccounter); // send data to the buffer
            lcd.print (buffer); // display line on buffer
          delay(1000);
//          lcd.setCursor(0,0);
//          lcd.print("Counter Paused  ");
          lcd.setCursor(0,1);
          lcd.print("release Run Time Butt");
          
        }
    }
    else{ //start button not pressed
      Serial.println("Waiting for Production Start 1");
      value =0;
      longCounter =0;
      production_flag =0;
      //clearCounter();
      startFlag =0;
      eepromFlag =0; //disable eeprom 
      Serial.println("eeprom Flag disabled ......");
      if (prestartFlag != startFlag){
          Serial.println("Start Butt. = 0 call mcIndex ");
          callmcIndex(); //post to mySQL
          prestartFlag = startFlag;
          //setplanStopTime();
      }
      
    }
    relayFlag =1;
    stopButton();
}
void stopButton(){
        Serial.println("Check Stop Button");
        if(stopFlag == 0){ //stop flag =0, check stop switch 
          //check stop button **********************************        
          if (ipsw[2] == 0){ //stop time not pressed = ok
                //run below when never press stop button
                Serial.println("Start counting");
                Serial.println("eeprom Flag enabled ....eeeeeee....");
                stopFlag =0; //stop flag reset
                eepromFlag =1; //enable eeprom
                mcindexFlag =1;
                countFlag =1;
                
                Serial.println("stopFlag =0, mcindexFlag =1,countFlag =1");
                //if status changed post to DB
                if (prestopFlag != stopFlag){
                    Serial.println("Stop time --> call mcIndex ");
                    callmcIndex(); //post to mySQL
                    prestopFlag = stopFlag;
                }
          } //stop time not pressed    
          else { //stop button pressed = not ok,--> freeze
                stopFlag =1;relayFlag =1;
                countFlag = 0; //disable counter flag
                eepromFlag =0; //eeprom disabled
                mcindexFlag =0;
                serverFlag =0;
                errFlag =0;
                cleardataArry();
                connectFlag = 0;
                prevlongCounter =0;
                Qty =0;
                
                setplanStopTime();//change status to 3 = Done
                Serial.println("stopFlag =1, eepromFlag = countFlag = mcindexFlag =0");
                Serial.println("Waiting for release stop button ");
                Serial.println("clear EEPROM and Reset Counter");
                //if status changed post to DB
                if (prestopFlag != stopFlag){
                    Serial.println("Stop time --> call mcIndex ");
                    callmcIndex(); //post to mySQL
                    prestopFlag = stopFlag;
                    //setplanStopTime();//change status to 3 = Done
                    clearCounter();//clear all eeprom 100 bytes
                }
                longCounter =0;
          } //stop button pressed 
      }
      else { //if stop flag =1
        ipsw[2] = !digitalRead(ipsw2); //check if stop button released
        if (ipsw[2] == 0){ //stop button released
          Serial.println("Reset counter and EEPROM");
          longCounter =0;
          clearCounter();//clear counter eeprom
          stopFlag =0;
          clearCounter();
          serverFlag =0;
          errFlag =0;
          cleardataArry();
          Qty =0;
          prevlongCounter =0;
          Serial.println("stopFlag =0");
          if (prestopFlag != stopFlag){
              Serial.println("Stop time --> call mcIndex ");
              callmcIndex(); //post to mySQL
              prestopFlag = stopFlag;
              LotNo = "";
          }
          return; //out
        }
      }
}

void cleardataArry(){
  for(int i =0;i<6;i++){
    dataArry[i] = "";
  }
}
void clearCounter3(){
    EEPROM.write(addressee, 0);      //write value to current address counter address
    EEPROM.write(addressee+1, 0);      //write value to current address counter address
    EEPROM.write(addressee+2, 0);      //write value to current address counter address
    lcd.setCursor(0,0);
    lcd.print("Clear Counter   ");
    lcd.setCursor(0,1);
    lcd.print("----------------");         
    //lcd.setCursor(15,1);
    //lcd.print("*");
    delay(1000);
    lcd.clear();
}
void clearCounter(){
    for (int i =0;i<=105;i++){
       EEPROM.write(i, 0);
    }
    lcd.setCursor(0,0);
    lcd.print("Clear AllCounter");
    lcd.setCursor(0,1);
    lcd.print("----------------");         
    //lcd.setCursor(15,1);
    //lcd.print("*");
    delay(1000);
    lcd.clear();
}

void writeCounter2()
{
  if (eepromFlag == 0){
    Serial.println("EEPROM Disabled");
  }
  else {
    Serial.println("EEPROM Enabled");
    tvalue = EEPROM.read(addressee);//read EEPROM data at address i
    tvalue2 = EEPROM.read(addressee+1);//read EEPROM data at address i
    tvalue3 = EEPROM.read(addressee+2);//read EEPROM data at address i
    ttempCounter = tvalue3*65536+tvalue2*(256)+tvalue;
    tempCounter = value3*65536+value2*(256)+value; 
    if(ttempCounter != longCounter){
        Serial.println(" * * * Write to EEPROM * * * *");
        EEPROM.write(addressee, value);      //write value to current address counter address
        EEPROM.write(addressee+1, value2);      //write value to current address counter address
        EEPROM.write(addressee+2, value3);      //write value to current address counter address
        if (eepromWriteFlag ==0){
          lcd.setCursor(15,1);
          lcd.print("*");
          eepromWriteFlag =1;
        }
        else {
          lcd.setCursor(15,1);
          lcd.print("-");
          eepromWriteFlag =0;          
        }
      }
    }
}

void readcounterEEPROM()
{
    value = EEPROM.read(addressee);//read EEPROM data at address i
    value2 = EEPROM.read(addressee+1);//read EEPROM data at address i
    value3 = EEPROM.read(addressee+2);//read EEPROM data at address i
    Serial.print("read Sensor value first time at address ");
    Serial.print(addressee);
    Serial.print("= ");
    Serial.println(value);
    Serial.print("read Sensor value first time at address ");
    Serial.print(addressee+1);
    Serial.print("= ");
    Serial.println(value2);
    Serial.print("read Sensor value first time at address ");
    Serial.print(addressee+2);
    Serial.print("= ");
    Serial.println(value3);
    longCounter = value3*65536+value2*(256)+value;
    Serial.print("longCounter= ");
    Serial.println(longCounter);

  Serial.print("Reading string from address 10: "); //status
  Status1 = EEPROM.read(10);
  Serial.print("Status1= ");Serial.println(Status1);

//read size of data
  unsigned int sizeID = EEPROM.read(101);
  unsigned int sizePlanQty = EEPROM.read(104);
  unsigned int sizeItemNo = EEPROM.read(103);
  unsigned int sizeLotNo = EEPROM.read(102);
  if (sizeID == 255){
    sizeID =2;
    sizePlanQty =5;
    sizeItemNo =5;
    sizeLotNo =5;
  }

  String sizeData = String(sizeID)+","+String(sizePlanQty)
  +","+String(sizeItemNo)+","+String(sizeLotNo);
  Serial.print("Size of data --- *** --- ");Serial.println(sizeData);  
  String tStr = "";
  Serial.print("Reading string from address 11(buf): ");//ID
  eeprom_read_string(11, buf, sizeID);
  Serial.print("ID= ");Serial.println(buf);
  for (int i =0;i<sizeID;i++) tStr += buf[i];
  ID2 = tStr.toInt();
  Serial.print("Reading string from address 11(int): ");
  Serial.print("ID2= ");Serial.println(ID2);
  
  tStr = "";
  Serial.print("Reading string from address 21(buf): ");//PlanQty
  eeprom_read_string(21, buf, sizePlanQty);
  Serial.print("PlanQty= ");Serial.println(buf);
  for (int i =0;i<sizePlanQty;i++) tStr += buf[i];
  PlanQty2 = tStr.toInt();
  Serial.print("Reading string from address 21(int): ");
  Serial.print("PlanQty2= ");Serial.println(PlanQty2);
  
  LotNo2 = "";  
  Serial.print("Reading string from address 31(buf): ");//LotNo
  eeprom_read_string(31, buf, sizeLotNo);
  Serial.print("LotNo= ");Serial.println(buf);
  for (int i =0;i<sizeLotNo;i++) LotNo2 += buf[i];
  Serial.print("Reading string from address 31(str): ");
  Serial.print("LotNo2= ");Serial.println(LotNo2);
  
  ItemNo2 = "";
  Serial.print("Reading string from address 61(buf): ");//ItemNo
  eeprom_read_string(61, buf, sizeItemNo);
  Serial.print("ItemNo= ");Serial.println(buf);
  for (int i =0;i<sizeItemNo;i++) ItemNo2 += buf[i];
  Serial.print("Reading string from address 61{str}: ");
  Serial.print("ItemNo2= ");Serial.println(ItemNo2);
}
void incomingData(){
    // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.connected()){
    //int len = client.available();
    //Serial.print("len= ");Serial.println(len);
    if (client.available()){
        char c = client.read();
        dataString += c;
    }
  }
  Serial.println();
  Serial.print ("DataString= ");
  Serial.println(dataString); //print to serial monitor for debuging
  Serial.println("End DataString!!!");
  Serial.println("*** Start encoding Data ***");
  separateString();
  //dataString.toCharArray(charBuf,100); 
  dataString = "";
  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
}

void separateString(){
  long startIndex = 0;
  long runIndex =0;
  cleardataArry();
  dataString.toCharArray(charBuf,512);
  for (long i=0;i<511;i++){
    if (charBuf[i] == '*'){
      startIndex = i;
      runIndex =i;
      for(int j = 0;j<255;j++){
        if (charBuf[runIndex] != ';'){
          dataBuf[j] = charBuf[runIndex];
          runIndex ++;
        }
      }
    }
  }
  long amtIndex = runIndex - startIndex;
  int startB =1;
  
  for (int a =0;a<6;a++){
    for (int b = startB;b<amtIndex;b++){
      if (dataBuf[b] != ','){
         dataArry[a] += dataBuf[b];
      }
      else {
        startB = b+1;break;
      }
    }//loop item
  } //loop planStr
  dataArry[0] = dataArry[0].substring(3);//ID
  dataArry[1] = dataArry[1].substring(5);//McNo
  dataArry[2] = dataArry[2].substring(6);//LotNo
  dataArry[3] = dataArry[3].substring(7);//ItemNo
  dataArry[4] = dataArry[4].substring(11);//PlannedQty
  dataArry[5] = dataArry[5].substring(7);//Status
  for (int c = 0;c<6;c++){
    Serial.print("dataArry[");Serial.print(c);;Serial.print("]= ");
    Serial.println(dataArry[c]);
  }
  ID = dataArry[0];//ID
  //McNo = dataArry[1].substring(5);//McNo
  LotNo = dataArry[2];//LotNo
  ItemNo = dataArry[3];//ItemNo
  PlanQty = dataArry[4].toInt();//plannedQty  
  Status = dataArry[5].toInt();//Status

  if (LotNo != ""){ //write to eeprom if LotNo availabled
      Serial.println("Write to EEPROM");
      EEPROM.write(10, 0);//Status address 10 = 0
      
      ID.toCharArray(buf,ID.length()+1);
      eeprom_write_string(11,buf); //write ID to address 11 (max 10 bytes)
    
      //PlanQty.toCharArray(myStringChar,10);
      sprintf(buf,"%d",PlanQty);
      eeprom_write_string(21,buf); //write PlanQty to add 21 (10 BYTES)
    
      LotNo.toCharArray(buf,LotNo.length()+1);
      eeprom_write_string(31,buf); //WRITE LotNo TO ADD 31 (30 BYTES)
    
      ItemNo.toCharArray(buf,ItemNo.length()+1);
      eeprom_write_string(61,buf); //WRITE ItemNo TO ADD 61 (30 BYTES)

      //write data length to eeprom start at address 101
      EEPROM.write(101, ID.length()+1);//Status address 10 = 0
      EEPROM.write(102, LotNo.length()+1);//Status address 10 = 0
      EEPROM.write(103, ItemNo.length()+1);//Status address 10 = 0
      EEPROM.write(104, sizeof(PlanQty)+1);//Status address 10 = 0

      Serial.print("SizeID= ");Serial.println(EEPROM.read(101));
      Serial.print("SizeLotNo= ");Serial.println(EEPROM.read(102));
      Serial.print("SizeItemNo= ");Serial.println(EEPROM.read(103));
      Serial.print("SizePlanQty= ");Serial.println(EEPROM.read(104));   

      Serial.print("Reading string from address 10: "); 
      int statusT = EEPROM.read(10);
      Serial.print("Status= ");Serial.println(statusT);
    
      Serial.print("Reading string from address 11: ");
      eeprom_read_string(11, buf, ID.length()+1);
      Serial.print("ID= ");Serial.println(buf);
    
      Serial.print("Reading string from address 21: ");
      eeprom_read_string(21, buf, sizeof(PlanQty)+1);
      Serial.print("PlanQty= ");Serial.println(buf);
    
      Serial.print("Reading string from address 31: ");
      eeprom_read_string(31, buf, LotNo.length()+1);
      Serial.print("LotNo= ");Serial.println(buf);
    
      Serial.print("Reading string from address 61: ");
      eeprom_read_string(61, buf, ItemNo.length()+1);
      Serial.print("ItemNo= ");Serial.println(buf); 

  }
  else {
    Serial.println("Not found LotNo from Server --> Use from EEPROM");
    LotNo = LotNo2;
    ItemNo = ItemNo2;
    PlanQty = PlanQty2;
    ID = ID2;
    urlData();//print data 
  }
}

void urlData(){
  Serial.println(" *** Data Printout ***");
  Serial.print("ID= ");Serial.println(ID);
  Serial.print("McNo= ");Serial.println(McNo);
  Serial.print("LotNo= ");Serial.println(LotNo);
  Serial.print("ItemNo= ");Serial.println(ItemNo);
  Serial.print("startFlag= ");Serial.println(startFlag); 
  Serial.print("runFlag= ");Serial.println(runFlag);
  Serial.print("stopFlag= ");Serial.println(stopFlag);
  Serial.print("longCounter= ");Serial.println(longCounter); 
  Serial.println();      
  }
void lcdprint(int i){
  lcd.setCursor(0,i);
  lcd.print(textmsg);  
}
void lcdprint1(int i){
  lcd.setCursor(1,i);
  lcd.print(textmsg);  
}
//long timeDisplay = 5000;
//long prevDisplay = 0;
void lcdDisp2(){
 long currentDisplay = millis();
 if (currentDisplay - prevDisplay > timeDisplay){
   prevDisplay = millis();
   switch (pageIndex){
      case 0:
          lcdDisp();
          pageIndex ++;
          break;
      case 1:
          dispPage2();
          pageIndex =0;
          break;
  }
 }

}
void dispPage2(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("LotNo= ");
  lcd.print(LotNo);
  lcd.setCursor(0,1);
  lcd.print("ItemNo= ");
  lcd.print(ItemNo);
}

void lcdDisp(){
  //lcd.clear();
  if(ipsw[0] == 1 && ipsw[1] ==1 && ipsw[2] ==0){
    lcd.setCursor(0,0);    
    lcd.print(PlanQty);
    lcd.setCursor(8,0);
    lcd.print(Qty);
    lcd.setCursor(0,1);
    char buffer [18]; // a few bytes larger than your LCD line
    uint16_t ccounter = longCounter; // data read from instrument
    sprintf (buffer, "Acc.Q= %06u", ccounter); // send data to the buffer
    lcd.print (buffer); // display line on buffer
    return;
  }
  lcd.setCursor(0,0);
  if(ipsw[0] ==1)lcd.print("MC-> ");
  else {
    lcd.print("MC-- ");
    lcd.setCursor(0,1);
    lcd.print("Press M/C Button");
    lcdBlink(1,300);
    lcd.clear();
  }
  lcd.setCursor(5,0);
  if(ipsw[1] ==1)lcd.print("Prod-> ");
  else {
    lcd.print("Prod-- ");
    lcd.setCursor(0,1);
    lcd.print("Pause Counting  ");
    lcdBlink(2,250);
    lcd.clear();
  }
  lcd.setCursor(12,0);
  if(ipsw[2] ==1){
    lcd.print("Stop");
    lcd.setCursor(0,1);
    lcd.print("Release Stop But");
    lcdBlink(3,150);
    lcd.clear();
  }
  else {
    lcd.print("Norm");
   }
  lcd.setCursor(0,1);
  char buffer [18]; // a few bytes larger than your LCD line
  uint16_t ccounter = longCounter; // data read from instrument
  sprintf (buffer, "Acc.Q= %06u", ccounter); // send data to the buffer
  lcd.print (buffer); // display line on buffer
  delay(1000);
  lcd.clear();
}
void lcdBlink(int a,int b){
    for (int f = 0; f <a;f++){
    lcd.noBacklight();
    delay(b);
    lcd.backlight();
    delay(b);
  }
}
void callmcIndex(){
  data = "LotNo=";
  data.concat('"');data.concat(LotNo);data.concat('"');  
  data.concat("&ItemNo="); 
  data.concat('"');data.concat(ItemNo);data.concat('"');
  data.concat("&McNo="); 
  data.concat('"');data.concat(McNo);data.concat('"'); 
  data.concat("&StartTime="); 
  data.concat(startFlag); 
  data.concat("&RunTime="); 
  data.concat(runFlag); 
  data.concat("&StopTime="); 
  data.concat(stopFlag); 
  data.concat("&TotalQty="); 
  data.concat(longCounter); 
  data.concat("&TotalDefect="); 
  data.concat(TotalDefect);
  data.concat("&Qty=");
  data.concat(Qty);   
  Serial.print("Data= ");
  Serial.println(data);
    wdt_reset();
  if (client.connect(server, 80)) {
    //Serial.print("connected. My IP is ");
    //Serial.println(Ethernet.localIP());
    client.println("POST /mcIndex.php HTTP/1.1");
    client.println("Host: localhost");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(data.length());
    client.println();
    client.print(data);
    Serial.println("Data Sent --->");
    client.stop();
  }
  else {
    Serial.println("connection failed");
    client.stop();
  }
}
void setplanRunTime(){
    wdt_reset();
    data = "ID=";
    data.concat(ID);  
    Serial.println("******* Update Run Status ************");
    Serial.println(data);
    if (client.connect(server, 80)) {
    //Serial.print("connected. My IP is ");
    //Serial.println(Ethernet.localIP());
    client.println("POST /updateplanStatus.php HTTP/1.1");
    client.println("Host: localhost");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(data.length());
    client.println();
    client.print(data);
    Serial.println("<<< ---- Run Status Changed");
    client.stop();
  }
  else {
    Serial.println("connection failed");
    client.stop();
  }
}
void setplanStopTime(){
    wdt_reset();
    data = "ID=";
    data.concat(ID);
    data.concat("&McNo=");
    data.concat(McNo);  
    data.concat("&ProducedQty=");
    data.concat(longCounter);
    data.concat("&LotNo=");
    data.concat(LotNo);      

    Serial.println("******* Update Stop Status ************");
    Serial.println(data);
    if (client.connect(server, 80)) {
    //Serial.print("connected. My IP is ");
    //Serial.println(Ethernet.localIP());
    client.println("POST /updateplanStatusStopAvg.php HTTP/1.1");
    client.println("Host: localhost");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(data.length());
    client.println();
    client.print(data);
    Serial.println("<< ----- Stop Status Changed");
    client.stop();
  }
  else {
    Serial.println("connection failed");
    client.stop();
  }
}
void setplanStartTime(){
    wdt_reset();
    data = "ID=";
    data.concat(ID);  
    Serial.println("******* Update Start Status ************");
    Serial.println(data);
    if (client.connect(server, 80)) {
    //Serial.print("connected. My IP is ");
    //Serial.println(Ethernet.localIP());
    client.println("POST /updateplanStatusStart.php HTTP/1.1");
    client.println("Host: localhost");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(data.length());
    client.println();
    client.print(data);
    Serial.println("<<< ----- Start status changed ");
        client.stop();
  }
  else {
    Serial.println("connection failed");
    client.stop();
  }
}
