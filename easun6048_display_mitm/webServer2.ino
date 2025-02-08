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


  for (int i = 0; i < server2.args(); i++) {    
    // achtung die Strings im Serveropjekt müssen kopiert werden. Ansonsten gibt es mem-probs
    String nam = server2.argName(i);
    String val = server2.arg(i);
    if ( nam == "setBoost") {
      serialComm.setBoost = server2.arg(i).toFloat();
    }
  }

  prepareHeaderJson();

  messageMeas =  "{\n" ;
  messageMeas += "     \"help1\":\"http://" + WiFi.localIP().toString() + "/?setBoost=13.2\",\n";

  messageMeas += "     \"millis\":" + String(millis()) + ",\n" ;

  messageMeas += "     \"actSetting\": {\n" ;
  messageMeas += "        \"boost\":" + String(serialComm.actSetting.boost) + "\n" ;
  messageMeas += "     },\n" ;

  messageMeas += "     \"actMeas\": {\n" ;
  messageMeas += "        \"vBat\":" + String(serialComm.actMeas.vBat) + ",\n" ;
  messageMeas += "        \"vPV\":" + String(serialComm.actMeas.vPV) + ",\n" ;
  messageMeas += "        \"iBat\":" + String(serialComm.actMeas.iBat) + ",\n" ;
  messageMeas += "        \"temp\":" + String(serialComm.actMeas.temp) + "\n" ;
  messageMeas += "     },\n" ;

  messageMeas += "    \"parseError\":\"" + serialComm.parseError + "\",\n" ;
  messageMeas += "    \"setBoost\":" + String(serialComm.setBoost ) + ",\n" ;

  messageMeas += "    \"lastSendCmd1\":\"" + convertRawToString( serialComm.lastSendCmd1 ,serialComm.lastSendCmd1_len ) + "\",\n" ;
  messageMeas += "    \"lastRecvCmd2\":\"" + convertRawToString( serialComm.lastRecvCmd2 ,serialComm.lastRecvCmd2_len ) + "\",\n" ;
  messageMeas += "    \"lastRecvCmd3\":\"" + convertRawToString( serialComm.lastRecvCmd3 ,serialComm.lastRecvCmd3_len ) + "\",\n" ;

  messageMeas += "    \"bufStack_k\":" + String( serialComm.bufStack_k ) + ",\n\n" ;

  messageMeas += "    \"bufStack\":[\n" ;

  for ( int k = 0 ; k<serialComm.bufStack_k ; k++) {

    if(k)     messageMeas += ",";
    messageMeas +=  "\n{" ;
    messageMeas += "\"src\":" + String(serialComm.bufStack[k].src*1) + "," ;

    messageMeas += "\"dat\":[" ;
    
//    for ( int m = 0 ; m<bufSize ; m++) {
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
