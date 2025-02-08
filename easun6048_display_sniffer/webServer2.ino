#include <WebServer.h>

WebServer server2(80);

/* *********************************************
*/
void webServer2_init() {
  // webserver starten
  server2.on("/", wwwHandleSerialBuffer);
  server2.begin();
  Serial.println("Web Server listening");
}

/* *********************************************
*/
void webServer2_run() {
  server2.handleClient();
}
/* *********************************************
*/
String messageMeas = "";

void prepareHeaderJson() {
  //https://www.esp8266.com/viewtopic.php?f=8&t=16830&start=4
  server2.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server2.sendHeader("Pragma", "no-cache");
  server2.sendHeader("Expires", "-1");
  server2.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server2.send(200, "application/json; charset=UTF-8", "");
}

void wwwHandleSerialBuffer() {
  //Serial.println( "+++ WebServer : wwwHandleSerialBuffer()" );

  prepareHeaderJson();

  messageMeas =  "{\n" ;
  messageMeas += "    ,\"millis\":" + String(millis()) + "\n" ;
  messageMeas += "    ,\"bufStack_k\":" + String( serialComm.bufStack_k ) + "\n" ;

  messageMeas += "    ,\"bufStack\":[\n" ;

  for ( int k = 0 ; k<serialComm.bufStack_k ; k++) {

    if(k)     messageMeas += ",";
    messageMeas +=  "\n{" ;
    messageMeas += "\"src\":" + String(serialComm.bufStack[k].src*1) + "," ;

    messageMeas += "\"dat\":[" ;
    for ( int m = 0 ; m<serialComm.bufStack[k].len ; m++) {
      if(m)     messageMeas += ",";
      messageMeas += String(serialComm.bufStack[k].data[m]*1);
    }
    messageMeas += "]," ;

    messageMeas += "\"dt\":" + String(serialComm.bufStack[k].dt) ;
    messageMeas += ",\"cnt\":" + String(serialComm.bufStack[k].cnt) ;
    messageMeas += ",\"time\":" + String(serialComm.bufStack[k].time) ;
    messageMeas += ",\"checkSum\":" + String(serialComm.bufStack[k].checkSum) ;
    messageMeas += ",\"checkSum2\":" + String(serialComm.bufStack[k].checkSum2) ;
    messageMeas += ",\"len\":" + String(serialComm.bufStack[k].len) ;

    messageMeas += "}";

    if (messageMeas.length() > 1500 ) { 
      server2.sendContent( messageMeas );
      messageMeas = ""; 
    }

  }
  messageMeas +=  "\n\n]}" ;
  /* */
  server2.sendContent( messageMeas ); 
  messageMeas = "";
  server2.sendContent(F("")); // this tells web client that transfer is done
  server2.client().stop();
}
