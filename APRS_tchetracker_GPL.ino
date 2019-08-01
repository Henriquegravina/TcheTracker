
/*     Tchetracker/Tcheduino
 *     Criado em Dez/2017 por PU3IKE Henrique B. Gravina
 *     Com ajuda de PY3NZ Claudio Chicon e PU3MSR Marcelo Rocha
 *     Feito para funcionar com Tcheduino e Micromodem.
 *     
 *     Este código foi escrito e testado, no entanto modificações
 *     devem ser evitadas, caso necessário é imperativo o cuidado 
 *     com a utilização de variáveis e memória disponível.
 *     
 */


/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#include <LibAPRS.h>
#define ADC_REFERENCE REF_5V
#define OPEN_SQUELCH false

boolean gotPacket = false;
AX25Msg incomingPacket;
uint8_t *packetData;

#include <TinyGPS++.h>
TinyGPSPlus gps;
//TinyGPSCustom gps_fix(gps, "GPRMC", 2);
TinyGPSCustom gps_latitude(gps, "GPRMC", 3);
TinyGPSCustom gps_longitude(gps, "GPRMC", 5);


char latitude_mini_tche[9]  ="";
char longitude_mini_tche[10] ="";
//int sum_check=0;

bool isTime=false;
bool isCourse=false;
bool firstTX=true;
bool config_mode = true;

unsigned long int lastTime = 0;
unsigned long int smartDelay;
int last_course =0;

#define LEDBUZ 13
long int beeptime = 0; // APRS operations sinalization with buzzer/led
long int last_beeptime = 0; // 

unsigned int APRS_tx_beep = 1000;
unsigned int APRS_repeted_beep = 2000;
unsigned int APRS_rx_beep = 500; 

uint32_t last_fix_number=0;


#include <EEPROM.h>
#define eaCALL    10 // Endereço do CALL
#define eaSSID    20 // Endereço do ssid
#define eaSYMBOL  30 // Endereço do simbol
#define eaSYMBTAB 31 // Endereço do tipo de tabel...

#define eaPATH1     40
#define eaPATH1SSID 49
#define eaPATH2     50
#define eaPATH2SSID 59

#define eaPOWER       61
#define eaHEIGHT      62
#define eaGAIN        63
#define eaDIRECTIVITY 64

#define eaPREAMBLE 65 // 65 and 66
#define eaTAIL 67// 67 and 68


#define eaSPEED1  71 //
#define eaSPEED2  73 //
#define eaSPEED3  75 //

#define eaDELAY1 77 // ate 76
#define eaDELAY2 79 // ate 80
#define eaDELAY3 81 // ate 82
#define eaDELAY4 83 // ate 84

#define eaCOURSE 85  //     
#define eaCOURSETIME 87  // Tempo mínimo entre os gatilhos por mudanca de curso
#define eaCOURSESPEED 89 // Velocidade para aceitar o course


// Adudio / Blink codes
#define eaAPRS_tx_beep 91
#define eaAPRS_repeted_beep 93
#define eaAPRS_rx_beep 95

#define eaLATITUDE  100
#define eaLONGITUDE 120


#define eaMESSAGE 150 // 150 até 175 message[25]

// APRS BASIC
char    ecall[7] = "NO1ALL";  //Call maximum bytes in eeprom
uint8_t essid    = 10;   // SSID is a int in eeprom
char    esymbol  = 'j';       // simbols is one char in eeprom
bool    esymbtab = false;


// APRS PATH
char    epath1[7]  = "WIDE1";    //Call maximum bytes in eeprom
uint8_t epath1ssid = 1;
char    epath2[7]  = "WIDE2";    //Call maximum bytes in eeprom
uint8_t epath2ssid = 2;

//TX Settings
int ePreamble = 350;
int eTail     = 50;

int eCourse      = 45;
int eCourseSpeed = 10;
int eCourseTime  = 10;

int speed1 = 10;
int speed2 = 40;
int speed3 = 80;

unsigned int delay1 = 120;
unsigned int delay2 = 60;
unsigned int delay3 = 30;
unsigned int delay4 = 20;


char message[25] = "1234567890";
char comment[50] = "";

//APRS PHGD
uint8_t ePower       = 1;
uint8_t eHeight      = 1;
uint8_t eGain        = 1;
uint8_t eDirectivity = 1;

// Variaveis reutilizaveis 
int i = 0;
//int a;
   
void tche_printSettings(){
      Serial.print(F("Speed 1:      ")); Serial.println(speed1);
      Serial.print(F("Speed 2:      ")); Serial.println(speed2);
      Serial.print(F("Speed 3:      ")); Serial.println(speed3);
      Serial.print(F("Delay 1:      ")); Serial.println(delay1);
      Serial.print(F("Delay 2:      ")); Serial.println(delay2);
      Serial.print(F("Delay 3:      ")); Serial.println(delay3);
      Serial.print(F("Delay 4:      ")); Serial.println(delay4);
      Serial.print(F("CourseAngle:  ")); Serial.println(eCourse);
      Serial.print(F("CourseSpeed:  ")); Serial.println(eCourseSpeed);
      Serial.print(F("CourseTime:   ")); Serial.println(eCourseTime);
      Serial.print(F("Message:      ")); Serial.println(message);
      Serial.print(F("Beep TX:      ")); Serial.println(APRS_tx_beep);
      Serial.print(F("Beep Repeted: ")); Serial.println(APRS_repeted_beep);
      Serial.print(F("Beep RX:      ")); Serial.println(APRS_rx_beep);
}

void readConf(){
  // Read config and setup 
  //int i;
  for(i = 0; i <= (sizeof(ecall)-1); i++) ecall[i] = EEPROM.read(eaCALL+i); // Carrega dados a eeprom para variavel ecall
  essid   = EEPROM.read(eaSSID);
  esymbol = EEPROM.read(eaSYMBOL);
  esymbtab = EEPROM.read(eaSYMBTAB);

  APRS_tx_beep = ((EEPROM.read(eaAPRS_tx_beep) << 0) & 0xFFFFFF) + ((EEPROM.read(eaAPRS_tx_beep+1     ) << 8) & 0xFFFFFFFF);
  APRS_repeted_beep = ((EEPROM.read(eaAPRS_repeted_beep) << 0) & 0xFFFFFF) + ((EEPROM.read(eaAPRS_repeted_beep+1     ) << 8) & 0xFFFFFFFF);
  APRS_rx_beep = ((EEPROM.read(eaAPRS_rx_beep) << 0) & 0xFFFFFF) + ((EEPROM.read(eaAPRS_rx_beep+1     ) << 8) & 0xFFFFFFFF);
  
  for(i = 0; i <= (sizeof(epath1)-1); i++) epath1[i] = EEPROM.read(eaPATH1+i);
  epath1ssid   = EEPROM.read(eaPATH1SSID);
  for(i = 0; i <= (sizeof(epath2)-1); i++) epath2[i] = EEPROM.read(eaPATH2+i);
  epath2ssid   = EEPROM.read(eaPATH2SSID);

  
  eCourse       = ((EEPROM.read(eaCOURSE     ) << 0) & 0xFFFFFF) + ((EEPROM.read(eaCOURSE+1     ) << 8) & 0xFFFFFFFF);
  eCourseTime   = ((EEPROM.read(eaCOURSETIME) << 0) & 0xFFFFFF) + ((EEPROM.read(eaCOURSETIME+1) << 8) & 0xFFFFFFFF);
  eCourseSpeed  = ((EEPROM.read(eaCOURSESPEED) << 0) & 0xFFFFFF) + ((EEPROM.read(eaCOURSESPEED+1) << 8) & 0xFFFFFFFF);

  speed1 = ((EEPROM.read(eaSPEED1) << 0) & 0xFFFFFF) + ((EEPROM.read(eaSPEED1+1) << 8) & 0xFFFFFFFF);
  speed2 = ((EEPROM.read(eaSPEED2) << 0) & 0xFFFFFF) + ((EEPROM.read(eaSPEED2+1) << 8) & 0xFFFFFFFF);
  speed3 = ((EEPROM.read(eaSPEED3) << 0) & 0xFFFFFF) + ((EEPROM.read(eaSPEED3+1) << 8) & 0xFFFFFFFF);

  delay1 = ((EEPROM.read(eaDELAY1) << 0) & 0xFFFFFF) + ((EEPROM.read(eaDELAY1+1) << 8) & 0xFFFFFFFF);
  delay2 = ((EEPROM.read(eaDELAY2) << 0) & 0xFFFFFF) + ((EEPROM.read(eaDELAY2+1) << 8) & 0xFFFFFFFF);
  delay3 = ((EEPROM.read(eaDELAY3) << 0) & 0xFFFFFF) + ((EEPROM.read(eaDELAY3+1) << 8) & 0xFFFFFFFF);
  delay4 = ((EEPROM.read(eaDELAY4) << 0) & 0xFFFFFF) + ((EEPROM.read(eaDELAY4+1) << 8) & 0xFFFFFFFF);




  ePreamble = ((EEPROM.read(eaPREAMBLE) << 0) & 0xFFFFFF) + ((EEPROM.read(eaPREAMBLE+1) << 8) & 0xFFFFFFFF);
  eTail = ((EEPROM.read(eaTAIL) << 0) & 0xFFFFFF) + ((EEPROM.read(eaTAIL+1) << 8) & 0xFFFFFFFF); 
  
  for(i = 0; i <= (sizeof(message)-1); i++) message[i] = EEPROM.read(eaMESSAGE+i);
  
   

  APRS_setCallsign(ecall, essid);
  APRS_setMessageDestination("APTCHE",0);
  APRS_setDestination("APTCHE",0);
  APRS_setPath1(epath1, epath1ssid);
  APRS_setPath2(epath2, epath2ssid);
  APRS_setPreamble(ePreamble);
  APRS_setTail(eTail);
  
  APRS_useAlternateSymbolTable(esymbtab);
  APRS_setSymbol(esymbol);

  // Estes dados são setados soemnte no Beacon Mode
   if( (speed1==0) and  (speed2==0) and  (speed3==0) or config_mode == true ){

    for(i = 0; i <= (7); i++) latitude_mini_tche[i]  = EEPROM.read(eaLATITUDE+i);  // Carrega dados a eeprom para variavel de posicao fixa
    for(i = 0; i <= (8); i++) longitude_mini_tche[i] = EEPROM.read(eaLONGITUDE+i); 

    APRS_setLat(latitude_mini_tche);
    APRS_setLon(longitude_mini_tche);

    
    ePower = EEPROM.read(eaPOWER);
    eHeight = EEPROM.read(eaHEIGHT);
    eGain = EEPROM.read(eaGAIN);
    eDirectivity = EEPROM.read(eaDIRECTIVITY);
    
    APRS_setPower(ePower); 
    APRS_setHeight(eHeight);
    APRS_setGain(eGain);
    APRS_setDirectivity(eDirectivity);
    
   }
 
 
}

void aprs_msg_callback(struct AX25Msg *msg) {
  // If we already have a packet waiting to be
  // processed, we must drop the new one.
  if (!gotPacket) {
    // Set flag to indicate we got a packet
    gotPacket = true;

    // The memory referenced as *msg is volatile
    // and we need to copy all the data to a
    // local variable for later processing.
    memcpy(&incomingPacket, msg, sizeof(AX25Msg));

    // We need to allocate a new buffer for the
    // data payload of the packet. First we check
    // if there is enough free RAM.
    if (freeMemory() > msg->len) {
      packetData = (uint8_t*)malloc(msg->len);
      memcpy(packetData, msg->info, msg->len);
      incomingPacket.info = packetData;
    } else {
      // We did not have enough free RAM to receive
      // this packet, so we drop it.
      gotPacket = false;
    }
  }
}

void setup() {
  pinMode(LEDBUZ,OUTPUT);
  pinMode(11,INPUT_PULLUP);
  // Set up serial port
  Serial.begin(9600);
//  mySerial.begin(9600);

  readConf();
  
  APRS_init(ADC_REFERENCE, OPEN_SQUELCH);

  //APRS_printSettings();
  //tche_printSettings();

}


void processPacket() {
  if (gotPacket) {
    gotPacket = false;
    
    Serial.println("");
    Serial.println(F("####### RECIVED #######"));

    Serial.print(F("Received APRS packet. SRC: "));
    Serial.print(incomingPacket.src.call);
    Serial.print(F("-"));
    Serial.print(incomingPacket.src.ssid);
    Serial.print(F(". DST: "));
    Serial.print(incomingPacket.dst.call);
    Serial.print(F("-"));
    Serial.print(incomingPacket.dst.ssid);
    Serial.print(F(". Data: "));


    for (int i = 0; i < incomingPacket.len; i++) {
      Serial.write(incomingPacket.info[i]);
    }

    
    if(incomingPacket.src.ssid == essid){
      if( strncmp(incomingPacket.src.call,ecall,6) == 0 ){
        Serial.println(F("FUI REPETIDO!"));
        
        if(APRS_repeted_beep > 0 ){
          last_beeptime = millis();
          digitalWrite(LEDBUZ,HIGH);
          beeptime =  APRS_repeted_beep;
        }
        
      } 
    }else{

      if(APRS_rx_beep > 0 ){ 
          last_beeptime = millis();
          digitalWrite(LEDBUZ,HIGH);
          beeptime =  APRS_rx_beep;
        } 
    }

    
    Serial.println("");
    Serial.println(F("####### ####### #######"));
    Serial.println("");

    // Remeber to free memory for our buffer!
    free(packetData);

  }
}

char cgps;
void tracker_mode() {
  
  //Check if is time to turnf off beep
  if( (last_beeptime + (long(beeptime)))  <= millis()) { digitalWrite(LEDBUZ,LOW); }

  // Recepcao dados do GPS
  while (Serial.available() > 0){
    cgps = Serial.read();
    gps.encode(cgps);
    //Serial.print(cgps);
  }

  //Check if is time to turnf off beep  
  if( (last_beeptime + (long(beeptime)))  <= millis()) { digitalWrite(LEDBUZ,LOW); }

  
  // Every time anything is updated, print everything.
  // Esta situacao abaixo é chata, a biblioteca nao sabe realemnte dizer se os dados sao validos ou nao,
  // então temos que controlar pelo numero de vezes que conseguiu o fix.
  if (  ( abs( gps.sentencesWithFix() - last_fix_number  ) >= 2  ) and gps_latitude.isValid() and gps_longitude.isValid() and gps_latitude.isUpdated() and gps_longitude.isUpdated() and /* gps_fix.value()[0]== 'A' and*/ gps.altitude.isValid() ) //and gps.location.isValid() 
  {

    last_fix_number = gps.sentencesWithFix();

    Serial.print(F("Sentences that failed checksum="));
    Serial.println(gps.failedChecksum());
  
    
    latitude_mini_tche[0] = gps_latitude.value()[0];
    latitude_mini_tche[1] = gps_latitude.value()[1];
    latitude_mini_tche[2] = gps_latitude.value()[2];
    latitude_mini_tche[3] = gps_latitude.value()[3];
    latitude_mini_tche[4] = gps_latitude.value()[4];
    latitude_mini_tche[5] = gps_latitude.value()[5];
    latitude_mini_tche[6] = gps_latitude.value()[6];
    latitude_mini_tche[7] = (gps.location.rawLat().negative ? 'S' : 'N');
    latitude_mini_tche[8] = 0;
  
    
    Serial.println("");

    Serial.print(F("Latitude: "));
    Serial.print(latitude_mini_tche);
    Serial.print(F(":"));
    Serial.print(gps_latitude.value());
    
    
    APRS_setLat(latitude_mini_tche);
    

    longitude_mini_tche[0] = gps_longitude.value()[0];
    longitude_mini_tche[1] = gps_longitude.value()[1];
    longitude_mini_tche[2] = gps_longitude.value()[2];
    longitude_mini_tche[3] = gps_longitude.value()[3];
    longitude_mini_tche[4] = gps_longitude.value()[4];
    longitude_mini_tche[5] = gps_longitude.value()[5];
    longitude_mini_tche[6] = gps_longitude.value()[6];
    longitude_mini_tche[7] = gps_longitude.value()[7];
    longitude_mini_tche[8] = (gps.location.rawLng().negative ? 'W' : 'E');
    longitude_mini_tche[9] = 0;

    Serial.print(F(" Longitude: "));
    Serial.println(longitude_mini_tche);
    
    APRS_setLon(longitude_mini_tche);
    
    // SmartCourse
    if(eCourse > 0){ // Somente aciona modo SmartCourse se for deinido valor maior que zero para a diferença.
      if(gps.speed.kmph() > (eCourseSpeed) and ( (lastTime + (eCourseTime * 1000 ))  <= millis()) ) { 
        if( abs(int(gps.course.deg()) - last_course) >= eCourse ) { isCourse = true;}
        else isCourse=false;
      } 
    }else { isCourse=false; }
    
    //Serial.println(abs(int(gps.course.deg()) - last_course)); // Print course diference

    if(gps.speed.kmph() <= (speed1)) smartDelay=delay1 ;
    else if(gps.speed.kmph() <= (speed2)) smartDelay=delay2;
    else if(gps.speed.kmph() <= (speed3)) smartDelay=delay3;
    else smartDelay=delay4;
 
    if( (lastTime + (smartDelay * 1000 ))  <= millis()) isTime=true;
    else isTime = false; 

    Serial.print(F("TIME FROM LAST TX: "));
    Serial.println( (millis() - lastTime ) / 1000   );

    // Antes de transmitir verificamos os 3 motivos e o sum_check
    if(( isTime or isCourse or firstTX ) /*and ( gps_fix.value()[0]== 'A')*/ ) {
        sprintf(comment, "%03d/%03d/A=%06d T%dC%dF%d %s ",int(gps.course.deg()),int(gps.speed.knots()),abs(int(gps.altitude.feet())),int(isTime),int(isCourse),int(firstTX),message);
        APRS_sendLoc(comment, strlen(comment));
        lastTime = millis();
        firstTX=false;
        isCourse=false;
        isTime=false;
        last_course = gps.course.deg();
        Serial.print(F("Free RAM:     ")); Serial.println(freeMemory());
        //sum_check = 0;
        if(APRS_tx_beep > 0 ){ 
          last_beeptime = millis();
          digitalWrite(LEDBUZ,HIGH);
          beeptime =  APRS_tx_beep;
        } 
    }
    
  }
    
  //Check if is time to turnf off beep
  if( (last_beeptime + (long(beeptime)))  <= millis()) { digitalWrite(LEDBUZ,LOW); }

  processPacket();
  
  //Check if is time to turnf off beep
  if( (last_beeptime + (long(beeptime)))  <= millis()) { digitalWrite(LEDBUZ,LOW); }

}

void beacon_mode(){
  smartDelay = delay1;
  if( (lastTime + (smartDelay * 1000 ))  <= millis()) isTime=true;
  else isTime = false; 
  
  if(( isTime ) or ( firstTX )) {
        Serial.println(millis() - ( lastTime + smartDelay * 1000 ) );
        
        //sprintf(comment, "%03d/%03d/A=%06d T%dC%dF%d %s ",int(gps.course.deg()),int(gps.speed.kmph()),abs(int(gps.altitude.feet())),int(isTime),int(isCourse),int(firstTX),message);
        sprintf(comment, " %s",message);
        APRS_sendLoc(comment, strlen(comment));
        lastTime = millis();
        firstTX=false;
        isTime=false;
        Serial.print(F("Free RAM:     ")); Serial.println(freeMemory());
        //sum_check = 0;
        if(APRS_tx_beep > 0 ){ 
          last_beeptime = millis();
          digitalWrite(LEDBUZ,HIGH);
          beeptime =  APRS_tx_beep;
        } 
    }

  if( (last_beeptime + (long(beeptime)))  <= millis()) { digitalWrite(LEDBUZ,LOW); }

  processPacket();
  
  //Check if is time to turnf off beep
  if( (last_beeptime + (long(beeptime)))  <= millis()) { digitalWrite(LEDBUZ,LOW); }

}


String command;

void loop(){
  if(!digitalRead(11)) { 
    if( (speed1==0) and  (speed2==0) and  (speed3==0) ){
      Serial.println(F("Beacon Mode!"));
      while(true) beacon_mode();  
    }else {
      Serial.println(F("Tracker Mode!"));
      while(true)  tracker_mode();
    }
  }else{ 
    config_mode = true;
    readConf();
    Serial.println(F("CFG")); {
      APRS_setLat(latitude_mini_tche);
      APRS_setLon(longitude_mini_tche);
      while(true)
      {
        if(Serial.available() > 0)
        {
          char c = Serial.read();
          if( c == 13 )
          {
            parseCommand(command);
            command = "";
          }
          else if( c == 10) { /*Do nothing*/ } // To use Line feed
          else { command += c; }
        }    
      }
    }
  }
}

void parseCommand(String com)
{

     String part1;
     String part2;

     part1 = com.substring(0, com.indexOf(" "));
     part2 = com.substring(com.indexOf(" ")+1);

     //CALL setup 
     if(part1.equalsIgnoreCase(F("CALL"))){      
      Serial.print(F("CALL: "));
      part2.toUpperCase();
      for(i = 0; i <= (sizeof(ecall)-1); i++) EEPROM.update(eaCALL+i,part2[i]);
      for(i = 0; i <= (sizeof(ecall)-1); i++) Serial.print(part2[i]);
      Serial.println("");
            
      // SSID Setup 
     }else if(part1.equalsIgnoreCase(F("SSID"))){
      Serial.print(F("SSID: "));
      EEPROM.update(eaSSID,part2.toInt());
      Serial.println(part2.toInt());
    
      // PATH1 Setup
     }else if(part1.equalsIgnoreCase(F("PATH1"))){
      Serial.print(F("PATH1: "));
      part2.toUpperCase();
      for(i = 0; i <= (sizeof(epath1)-1); i++) EEPROM.update(eaPATH1+i,part2[i]);
      Serial.println(part2);
     
     // PATH1 SSID Setup   
     }else if(part1.equalsIgnoreCase(F("PATH1SSID"))){
      Serial.print(F("PATH1 SSID: "));
      EEPROM.update(eaPATH1SSID,part2.toInt());
      Serial.println(part2.toInt());

     // PATH2 Setup
     }else if(part1.equalsIgnoreCase(F("PATH2"))){ 
      Serial.print(F("PATH2: "));
      part2.toUpperCase();
      for(i = 0; i <= sizeof((epath2)-1); i++) EEPROM.update(eaPATH2+i,part2[i]);
      Serial.println(part2);

     // PATH2 SSID Setup   
     }else if(part1.equalsIgnoreCase(F("PATH2SSID"))){
      Serial.print(F("PATH2 SSID: "));
      EEPROM.update(eaPATH2SSID,part2.toInt());
      Serial.println(part2.toInt());

     // RX Preamble setup 
     }else if(part1.equalsIgnoreCase(F("PREAMBLE"))){
      Serial.print(F("TX PREAMBLE: "));
      EEPROM.update(eaPREAMBLE,(part2.toInt() & 0xFF));
      EEPROM.update(eaPREAMBLE+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2.toInt());
      
     // TX Tail Setup 
     }else if(part1.equalsIgnoreCase(F("TAIL"))){
      Serial.print(F("TX TAIL: "));
      EEPROM.update(eaTAIL,(part2.toInt() & 0xFF));
      EEPROM.update(eaTAIL+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2.toInt());

     // 
     }else if(part1.equalsIgnoreCase(F("SYMBOL"))){
      Serial.print(F("SYMBOL: "));
      EEPROM.update(eaSYMBOL,part2[0]);
      Serial.println(part2[0]);
      
     }else if(part1.equalsIgnoreCase(F("SYMBOLTABLE"))){
      Serial.print(F("SYMBOL TABLE: "));
      EEPROM.update(eaSYMBTAB,part2.toInt());
      Serial.println(part2.toInt());
      
     }else if(part1.equalsIgnoreCase(F("COURSEANGLE"))){
      Serial.print(F("COURSE TRIGER: "));
      EEPROM.update(eaCOURSE,(part2.toInt() & 0xFF));
      EEPROM.update(eaCOURSE+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2.toInt());
    
     }else if(part1.equalsIgnoreCase(F("COURSETIME"))){
      Serial.print(F("COURSE TIME: "));
      EEPROM.update(eaCOURSETIME,(part2.toInt() & 0xFF));
      EEPROM.update(eaCOURSETIME+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2.toInt());
    
     }else if(part1.equalsIgnoreCase(F("COURSESPEED"))){
      Serial.print(F("COURSE SPEED: "));
      EEPROM.update(eaCOURSESPEED,(part2.toInt() & 0xFF));
      EEPROM.update(eaCOURSESPEED+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2.toInt());
    
      
     }else if(part1.equalsIgnoreCase(F("DELAY1"))){
      Serial.print(F("DELAY to SPEED1: "));
      EEPROM.update(eaDELAY1,(part2.toInt() & 0xFF));
      EEPROM.update(eaDELAY1+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2.toInt());
    
     }else if(part1.equalsIgnoreCase(F("DELAY2"))){
      Serial.print(F("DELAY to SPEED2: "));
      EEPROM.update(eaDELAY2,(part2.toInt() & 0xFF));
      EEPROM.update(eaDELAY2+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2.toInt());
    
     }else if(part1.equalsIgnoreCase(F("DELAY3"))){
      Serial.print(F("DELAY to SPEED3: "));
      EEPROM.update(eaDELAY3,(part2.toInt() & 0xFF));
      EEPROM.update(eaDELAY3+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2.toInt());
    
     }else if(part1.equalsIgnoreCase(F("DELAY4"))){
      Serial.print(F("DELAY to speed over SPEED3: "));
      EEPROM.update(eaDELAY4,(part2.toInt() & 0xFF));
      EEPROM.update(eaDELAY4+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2.toInt());
    
     }else if(part1.equalsIgnoreCase(F("SPEED1"))){
      Serial.print(F("SPEED 1 to SmartBeacon: "));
      EEPROM.update(eaSPEED1,(part2.toInt() & 0xFF));
      EEPROM.update(eaSPEED1+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2.toInt());
    
     }else if(part1.equalsIgnoreCase(F("SPEED2"))){
      Serial.print(F("SPEED 2 to SmartBeacon: "));
      EEPROM.update(eaSPEED2,(part2.toInt() & 0xFF));
      EEPROM.update(eaSPEED2+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2.toInt());
    
     }else if(part1.equalsIgnoreCase(F("SPEED3"))){
      Serial.print(F("SPEED 3 to SmartBeacon: "));
      EEPROM.update(eaSPEED3,(part2.toInt() & 0xFF));
      EEPROM.update(eaSPEED3+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2.toInt());
    
     }else if(part1.equalsIgnoreCase(F("TXBEEP"))){
      Serial.print(F("TXBEEP: "));
      EEPROM.update(eaAPRS_tx_beep,(part2.toInt() & 0xFF));
      EEPROM.update(eaAPRS_tx_beep+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2);
      
     }else if(part1.equalsIgnoreCase(F("REPBEEP"))){
      Serial.print(F("REPBEEP: "));
      EEPROM.update(eaAPRS_repeted_beep,(part2.toInt() & 0xFF));
      EEPROM.update(eaAPRS_repeted_beep+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2);
            
     }else if(part1.equalsIgnoreCase(F("RXBEEP"))){
      Serial.print(F("RXBEEP: "));
      EEPROM.update(eaAPRS_rx_beep,(part2.toInt() & 0xFF));
      EEPROM.update(eaAPRS_rx_beep+1,((part2.toInt() >> 8) & 0xFF) );
      Serial.println(part2);
      
     }else if(part1.equalsIgnoreCase(F("PHGDP"))){
      Serial.print(F("PHGD Power: "));
      EEPROM.update(eaPOWER,part2.toInt());
      Serial.println(part2);
      
     }else if(part1.equalsIgnoreCase(F("PHGDH"))){
      Serial.print(F("PHGD Height: "));
      EEPROM.update(eaHEIGHT,part2.toInt());
      Serial.println(part2);
      
     }else if(part1.equalsIgnoreCase(F("PHGDG"))){
      Serial.print(F("PHGD Gain: "));
      EEPROM.update(eaGAIN,part2.toInt());
      Serial.println(part2);
      
     }else if(part1.equalsIgnoreCase(F("PHGDD"))){
      Serial.print(F("PHGD Directivity: "));
      EEPROM.update(eaDIRECTIVITY,part2.toInt());
      Serial.println(part2);
      
     }else if(part1.equalsIgnoreCase(F("FIXLAT"))){
      Serial.print(F("FIX LAT: "));
      part2.toUpperCase();
      for(i = 0; i <= (sizeof(latitude_mini_tche)-1); i++) EEPROM.update(eaLATITUDE+i,part2[i]);
      Serial.println(part2);
     
       
     }else if(part1.equalsIgnoreCase(F("FIXLON"))){
      Serial.print(F("FIX LON: "));
      part2.toUpperCase();
      for(i = 0; i <= (sizeof(longitude_mini_tche)-1); i++) EEPROM.update(eaLONGITUDE+i,part2[i]);
      Serial.println(part2);
     
       
     }else if(part1.equalsIgnoreCase(F("MESSAGE"))){
      Serial.print(F("MESSAGE: "));
      for(i = 0; i <= (sizeof(message)-1); i++) EEPROM.update(eaMESSAGE+i,part2[i]);
      for(i = 0; i <= (sizeof(message)-1); i++) Serial.print(part2[i]);
      Serial.println("");
      
     }else if(part1.equalsIgnoreCase(F("CONFIG"))){
      Serial.println(F("###CFG###"));
      readConf();
      APRS_printSettings();
      tche_printSettings(); 
      Serial.println(F("###END###"));
                    
     }else if(part1.equalsIgnoreCase(F("FORMAT"))){
      for (int i = 0 ; i < EEPROM.length() ; i++) EEPROM.write(i, 0);
      Serial.println(F("FORMATED"));
           
      EEPROM.put(eaCALL,"NO1CAL");
      EEPROM.write(eaSSID,9);
      EEPROM.put(eaPATH1,"WIDE1");
      EEPROM.write(eaPATH1SSID,1);
      EEPROM.put(eaPATH2,"WIDE2");
      EEPROM.write(eaPATH2SSID,1);
      EEPROM.write(eaPREAMBLE,( 450 & 0xFF));
      EEPROM.write(eaPREAMBLE+1,((450 >> 8) & 0xFF) );
      EEPROM.write(eaTAIL,( 50 & 0xFF));
      EEPROM.write(eaTAIL+1,(( 50 >> 8) & 0xFF) );
      EEPROM.write(eaSYMBOL,'j');
      EEPROM.write(eaSYMBTAB,0);
      EEPROM.put(eaMESSAGE,"Tcheduino/TcheTracker V8");

      EEPROM.write(eaCOURSE,( 35 & 0xFF));
      EEPROM.write(eaCOURSE+1,((35 >> 8) & 0xFF) );

      EEPROM.write(eaCOURSETIME,( 10 & 0xFF));
      EEPROM.write(eaCOURSETIME+1,((10 >> 8) & 0xFF) );

      EEPROM.write(eaCOURSESPEED,( 7 & 0xFF));
      EEPROM.write(eaCOURSESPEED+1,((7 >> 8) & 0xFF) );

      EEPROM.write(eaSPEED1,( 10 & 0xFF));
      EEPROM.write(eaSPEED1+1,((10 >> 8) & 0xFF) );

      EEPROM.write(eaSPEED2,( 40 & 0xFF));
      EEPROM.write(eaSPEED2+1,((40 >> 8) & 0xFF) );

      EEPROM.write(eaSPEED3,( 80 & 0xFF));
      EEPROM.write(eaSPEED3+1,((80 >> 8) & 0xFF) );

      EEPROM.write(eaDELAY1,( 120 & 0xFF));
      EEPROM.write(eaDELAY1+1,((120 >> 8) & 0xFF) );
      
      EEPROM.write(eaDELAY2,( 60 & 0xFF));
      EEPROM.write(eaDELAY2+1,((60 >> 8) & 0xFF) );

      EEPROM.write(eaDELAY3,( 30 & 0xFF));
      EEPROM.write(eaDELAY3+1,((30 >> 8) & 0xFF) );
      
      EEPROM.write(eaDELAY4,( 20 & 0xFF));
      EEPROM.write(eaDELAY4+1,((20 >> 8) & 0xFF) );

      EEPROM.put(eaLATITUDE,  "2969.00S");
      EEPROM.put(eaLONGITUDE,"05339.00W");

      EEPROM.write(eaPOWER,0);
      EEPROM.write(eaHEIGHT,0);
      EEPROM.write(eaGAIN,0);
      EEPROM.write(eaDIRECTIVITY,0);

     
      EEPROM.update(eaAPRS_tx_beep,(1000 & 0xFF));
      EEPROM.update(eaAPRS_tx_beep+1,((1000 >> 8) & 0xFF) );

      EEPROM.update(eaAPRS_repeted_beep,(2000 & 0xFF));
      EEPROM.update(eaAPRS_repeted_beep+1,((2000 >> 8) & 0xFF) );

      EEPROM.update(eaAPRS_rx_beep,(500 & 0xFF));
      EEPROM.update(eaAPRS_rx_beep+1,((500 >> 8) & 0xFF) );
            
      Serial.println(F("DONE"));
            
      readConf();
      //APRS_printSettings();
      //tche_printSettings();


     }else if(part1.equalsIgnoreCase(F("HELP"))){
      Serial.println(F("Comands examples: | Obs:. "));
      Serial.println(F("FORMAT            | Erease all eeprom and configs"));
      Serial.println(F("CONFIG            | Show configuration"));
      Serial.println(F("CALL NO1CAL       | Max 6 Chars "));
      Serial.println(F("SSID 9            | 1 to 255    "));
      Serial.println(F("PATH1 WIDE1       | PATH1 Value"));
      Serial.println(F("PATH1SSID 1       | SSID of PATH1"));
      Serial.println(F("PATH2 WIDE2       | PATH2 Value"));
      Serial.println(F("PATH2SSID 2       | SSID of PATH2"));
      Serial.println(F("PREAMBLE 450      | Time Before TX"));
      Serial.println(F("TAIL 50           | Time After TX"));
      Serial.println(F("SYMBOL j          | See APRS Doc"));
      Serial.println(F("SYMBOLTABLE 1     | 0=normal 1=alternate"));
      Serial.println(F("COURSEANGLE 35    | Angle diference to trigger tx"));
      Serial.println(F("COURSETIME 10     | Minimum time(seconds) betwen course tx"));
      Serial.println(F("COURSESPEED 7     | Minimum speed(km/h) to enable course tx"));
      Serial.println(F("SPEED1 10         | Speed (km/h) to smart beacon delay 1"));
      Serial.println(F("SPEED2 40         | Speed (km/h) to smart beacon delay 2"));
      Serial.println(F("SPEED1 80         | Speed (km/h) to smart beacon delay 3"));
      Serial.println(F("DELAY1 120        | Delay (sec) to speeds under speed 1"));
      Serial.println(F("DELAY2 60         | Delay (sec) to speeds under speed 2"));
      Serial.println(F("DELAY3 30         | Delay (sec) to speeds under speed 3"));
      Serial.println(F("DELAY4 20         | Delay (sec) to speeds above speed 1"));
      Serial.println(F("TXBEEP 35         | Beep duration on tx"));
      Serial.println(F("REPBEEP 20        | Beep durantion when repeted"));
      Serial.println(F("RXBEEP 5          | Beep duration on RX"));
      Serial.println(F("PHGDP 0           | Power value of PHGD ( 0 to 9 ) "));
      Serial.println(F("PHGDH 0           | Height value of PHGD ( 0 to 9 )"));
      Serial.println(F("PHGDG 0           | Gain value of PHGD ( 0 to 9 )"));
      Serial.println(F("PHGDD 0           | Directivity value of PHGD ( 0 to 9 )"));
      Serial.println(F("FIXLAT 2969.00S   | Fixed Latitude to Beacon mode  (in APRS Format)"));
      Serial.println(F("FIXLON 05339.00W  | Fixed Longitude to Beacon mode (in APRS Format)"));
      Serial.println(F("MESSAGE           | Configure comment message (max 25)"));
     
     
     
     }else if(part1.equalsIgnoreCase(F("CFG"))){
      readConf();
      Serial.println("###START###");
      Serial.print(F("CALL:")); Serial.println(ecall);
      Serial.print(F("SSID:")); Serial.println(essid);
      Serial.print(F("PATH1:")); Serial.println(epath1);
      Serial.print(F("PATH1SSID:")); Serial.println(epath1ssid);
      Serial.print(F("PATH2:")); Serial.println(epath2);
      Serial.print(F("PATH2SSID:")); Serial.println(epath2ssid);
      Serial.print(F("PREAMBLE:")); Serial.println(ePreamble);
      Serial.print(F("TAIL:")); Serial.println(eTail);
      Serial.print(F("SYMBOL:")); Serial.println(esymbol);
      Serial.print(F("SYMBOLTABLE:")); Serial.println(esymbtab);
      Serial.print(F("COURSEANGLE:")); Serial.println(eCourse);
      Serial.print(F("COURSETIME:")); Serial.println(eCourseTime);
      Serial.print(F("COURSESPEED:")); Serial.println(eCourseSpeed);
      Serial.print(F("SPEED1:")); Serial.println(speed1);
      Serial.print(F("SPEED2:")); Serial.println(speed2);
      Serial.print(F("SPEED3:")); Serial.println(speed3);
      Serial.print(F("DELAY1:")); Serial.println(delay1);
      Serial.print(F("DELAY2:")); Serial.println(delay2);
      Serial.print(F("DELAY3:")); Serial.println(delay3);
      Serial.print(F("DELAY4:")); Serial.println(delay4);
      Serial.print(F("TXBEEP:")); Serial.println(APRS_tx_beep);
      Serial.print(F("REPBEEP:")); Serial.println(APRS_repeted_beep);
      Serial.print(F("RXBEEP:")); Serial.println(APRS_rx_beep);
      Serial.print(F("PHGDP:")); Serial.println(ePower);
      Serial.print(F("PHGDH:")); Serial.println(eHeight);
      Serial.print(F("PHGDG:")); Serial.println(eGain);
      Serial.print(F("PHGDD:")); Serial.println(eDirectivity);
      Serial.print(F("FIXLAT:")); Serial.println(latitude_mini_tche);
      Serial.print(F("FIXLON:")); Serial.println(longitude_mini_tche);
      Serial.print(F("MESSAGE:")); Serial.println(message);
      Serial.println("####END####");
     }
      
  }
