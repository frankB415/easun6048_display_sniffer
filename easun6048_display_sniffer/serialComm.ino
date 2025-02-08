/*
Serielle Communication with EASUN 6048 (DISPLAY)

based on:
https://github.com/MirecX/easun6048/blob/master/components/easun6048/easun6048.cpp
https://github.com/profesor79/easun6040reader/blob/main/readerWip.ino
*/

#define bufStackSize 100
#define bufSize 50

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

} serialComm;


/*! \brief add the serial buffer to the bufferstack  */
void serialComm_addBuffer(char src , unsigned long dt) {
  // ist das Starbyte richtig?
  if (serialComm.indata_raw[0]!=125) return;
  // die Länge des Datensatzes steht im b[1], allerings gibt es noch 1 startbyte, die Länge  , Daten, 2 stopbytes
  int len = serialComm.indata_raw[1] +4 ; 
  // checkSum
  unsigned int checkSum = 0;
  for ( int m = 0 ; m < len ; m++ ) {
    checkSum += serialComm.indata_raw[m];
  }
  // checkSum2
  uint8_t checkSum2 = 0;
  for ( int m = 1 ; m < len-2 ; m++ ) {
    checkSum2 += serialComm.indata_raw[m];
  }
  // die pruefsumme checken: hatte ich das schonmal?
  for (int k = 0 ; k<serialComm.bufStack_k ; k++) {
    if ( serialComm.bufStack[k].checkSum == checkSum ) {
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
  if (serialComm.bufStack_k > bufStackSize - 1) serialComm.bufStack_k = bufStackSize - 1 ; 
}


void serialComm_init() {
  Serial1.begin(9600, SERIAL_8N1, 25, 13);
  Serial2.begin(9600, SERIAL_8N1, 26, 15);

  const long maxWaitInterval = 50;
  Serial1.setTimeout( maxWaitInterval );
  Serial2.setTimeout( maxWaitInterval );

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
    serialComm_addBuffer(1 , millis()-lastSerial1);    
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
    serialComm_addBuffer(2 , millis()-lastSerial2 );    
  }
}


