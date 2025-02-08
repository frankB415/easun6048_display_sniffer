/*
Serielle Communication with EASUN 6048 (DISPLAY)

based on:
https://github.com/MirecX/easun6048/blob/master/components/easun6048/easun6048.cpp
https://github.com/profesor79/easun6040reader/blob/main/readerWip.ino
*/

#define bufStackSize 100
#define bufSize 25

struct serialComm_t {
  // die sequenz ist 24 bytes lang, also machen wir den Buffer doppelt so gross
  char indata_raw[bufSize];
  int bytesRead;

  struct {
    char data[bufSize];
    unsigned long time;
    unsigned long dt;
    unsigned int cnt;
    unsigned int len;
    unsigned int checkSum;
    uint8_t checkSum2;
    char src;
  } bufStack[bufStackSize];
  int bufStack_k = 0;

  struct {
    float boost;
  } actSetting;
  struct {
    float vBat;
    float vPV;
    float iBat;
    float temp;
  } actMeas;

  String parseError = "ok";

  float setBoost = -1;

  byte lastSendCmd1[bufSize] ;
  int lastSendCmd1_len = 0;
  byte lastRecvCmd3[bufSize] ;
  int lastRecvCmd3_len = 0;
  byte lastRecvCmd2[bufSize] ;
  int lastRecvCmd2_len = 0;

} serialComm;



/*! \brief parse the serial buffer */
void serialComm_parseBuffer() {
  char* b = serialComm.indata_raw;
  // is the packet valid?
  if (b[0] != 125) {
    serialComm.parseError = "wrong startByte";
    return;
  };
  int len = b[1] + 4;
  if (b[len - 1] != 13) {
    serialComm.parseError = "wrong stopByte";
    return;
  };
  uint8_t checkSum2 = makeCheckSum( b, len) ;
  if (b[len - 2] != checkSum2) {
    serialComm.parseError = "wrong checkSum";
    return;
  };
  if (len < 8) {
    serialComm.parseError = "len<5";
    return;
  };

  // parse
  int cmd = b[2];
  if (cmd == 3) {  // meas
    if (b[9] == 0) {
      serialComm.parseError = "vBat == 0: " + convertSerialToString();
      return;
    }
    serialComm.actMeas.vBat = (b[8] * 256 + b[9]) * 0.1;
    serialComm.actMeas.vPV = (b[10] * 256 + b[11]) * 0.1;
    serialComm.actMeas.iBat = (b[12] * 256 + b[13]) * 0.1;
    serialComm.actMeas.temp = b[14];
    // sichern
    memcpy(serialComm.lastRecvCmd3 , b , len);
    serialComm.lastRecvCmd3_len = len ;
  }
  if (cmd == 2) {  // setting
    serialComm.actSetting.boost = b[6] * 0.1;
    // sichern
    memcpy(serialComm.lastRecvCmd2 , b , len);
    serialComm.lastRecvCmd2_len = len ;
  }
}


/*! \brief add the serial buffer to the bufferstack  */
void serialComm_addBuffer(char src, unsigned long dt) {
  // ist das Starbyte richtig?
  //  if (serialComm.indata_raw[0]!=125) return;
  // die Länge des Datensatzes steht im b[1], allerings gibt es noch 1 startbyte, die Länge  , Daten, 2 stopbytes
  int len = serialComm.indata_raw[1] + 4;
  // checkSum
  unsigned int checkSum = 0;
  for (int m = 0; m < len; m++) {
    checkSum += serialComm.indata_raw[m];
  }
  // checkSum2
    uint8_t checkSum2 = makeCheckSum(serialComm.indata_raw, len) ;
  // die pruefsumme checken: hatte ich das schonmal?
  for (int k = 0; k < serialComm.bufStack_k; k++) {
    if (serialComm.bufStack[k].checkSum == checkSum) {
      serialComm.bufStack[k].cnt++;
      return;
    }
  }
  //
  int k = serialComm.bufStack_k;
  memcpy(serialComm.bufStack[k].data, serialComm.indata_raw, bufSize);
  serialComm.bufStack[k].time = millis();
  serialComm.bufStack[k].src = src;
  serialComm.bufStack[k].dt = dt;
  serialComm.bufStack[k].len = len;
  serialComm.bufStack[k].checkSum = checkSum;
  serialComm.bufStack[k].checkSum2 = checkSum2;
  //
  serialComm.bufStack_k++;
  if (serialComm.bufStack_k > bufStackSize - 1) serialComm.bufStack_k = bufStackSize - 1;
}


void serialComm_init() {
  Serial1.begin(9600, SERIAL_8N1, 25, 13);
  Serial2.begin(9600, SERIAL_8N1, 26, 15);

  const long maxWaitInterval = 50;
  Serial1.setTimeout(maxWaitInterval);
  Serial2.setTimeout(maxWaitInterval);

  Serial1.setRxBufferSize(512);
  Serial2.setRxBufferSize(512);
}


void serialComm_run() {

  static unsigned long lastSerial1 = 0;
  static unsigned long lastSerial2 = 0;

  if (Serial1.available() > 0) {
    // Zeit sichern
    lastSerial1 = millis();
    // nullen
    memset(serialComm.indata_raw, 0, sizeof(serialComm.indata_raw));
    serialComm.bytesRead = 0;
    // lesen mt timeout
    serialComm.bytesRead = Serial1.readBytes(serialComm.indata_raw, sizeof(serialComm.indata_raw));
    // kopieren
    serialComm_addBuffer(1, millis() - lastSerial1);
    return;
  }

  if (Serial2.available() > 0) {
    // Zeit sichern
    lastSerial2 = millis();
    // nullen
    memset(serialComm.indata_raw, 0, sizeof(serialComm.indata_raw));
    serialComm.bytesRead = 0;
    // lesen mt timeout
    serialComm.bytesRead = Serial2.readBytes(serialComm.indata_raw, sizeof(serialComm.indata_raw));
    // kopieren
    serialComm_addBuffer(2, millis() - lastSerial2);
    serialComm_parseBuffer();
    return;
  }

//  static unsigned long t1 = 0;
//  if ( millis()-t1 > 2000 ) {
//    t1 = millis();
  if (serialComm.setBoost > 0) {
    // das paket fertigmachen
    int len = serialComm.lastRecvCmd2_len;
    serialComm.lastSendCmd1_len = len;
    byte* b = serialComm.lastSendCmd1;
    memcpy( b, serialComm.lastRecvCmd2 , len);
    // cmd
    b[2] = 1;
    // boost Spannung
    b[6] = serialComm.setBoost*10;
    // checksum
    insertCheckSum(b);
    // senden
    Serial1.end();
    Serial1.begin(9600, SERIAL_8N1, 13, 25);
    Serial1.write(b, len);
    Serial1.flush();
    delay(10);
    Serial1.end();
    Serial1.begin(9600, SERIAL_8N1, 25, 13);

    serialComm.setBoost = -1;
    return;
  }
}


String convertSerialToString() {
  return convertRawToString(serialComm.indata_raw, serialComm.bytesRead);
}
/*! \brief konvertiert einen Bytetream zu einer (int8 Zahlenreihe)
*/
String convertRawToString(void* raw, int len) {
  String ss = " (" + String(len) + ") ";
  for (int k = 0; k < len; k++) {
    if (k) ss += " ";
    ss += String(((uint8_t*)raw)[k]);
  }
  return ss;
}

int8_t makeCheckSum(void* b, int len) {
  uint8_t checkSum2 = 0;
  for (int m = 1; m < len - 2; m++) {
    checkSum2 += ((byte*)b)[m];
  }
  return checkSum2;
}

void insertCheckSum( void* b ) {
  int len = ((byte*)b)[1] + 4;
  uint8_t checkSum2 = makeCheckSum( b, len) ;
  ((byte*)b)[len-2] = checkSum2;    
}