bool inscribe() {
  sendOK = false;
  char msg[]="peer"; // request to peer
  esp_now_send(masterAddress, (uint8_t *) msg, strlen(msg));

  return true;
}

void senddata() {
  transferbusy=true;
  int tout=0;  
  int n=DLENGTH/CHUNKSIZE;
  int r=DLENGTH%CHUNKSIZE;
  long dt; 
  
  esp_err_t result;

  head.msgtype=MSG_HEAD;
  head.length=DLENGTH;
  head.dtype=MSG_INT;
  head.tsample=tsmp;
  head.timestamp=timestamp;  
  mdat.msgtype=MSG_INT;   
  mdat.length=CHUNKSIZE;

 // send head
  sendOK=false;
  tout=0;

  while(true) {
    result=esp_now_send(masterAddress, (uint8_t*) &head, sizeof(head));

    dt=millis()+200;    
    while (not sendOK) {
      if(millis()>dt) break;
    }
 
    if(sendOK) break;

    else if (++tout>=TIMEOUT) {
      Serial.println("data transfer timeout");    
      return;  
    }
  }

    // send body
  for (int i=0;i<n;i++) {

    sendOK=false;

    mdat.chunk=i;
  
    int16_t* pc=adcdata+i*CHUNKSIZE;
    memcpy(mdat.data, pc, CHUNKSIZE*sizeof(int16_t));
    
    tout=0;
    
    while(true) {
      result=esp_now_send(masterAddress, (uint8_t *) &mdat, sizeof(mdat));
      
      dt=millis()+200;    
      while (not sendOK) {
        if(millis()>dt) break;
      }
      
      if (sendOK) {        
        pc+=CHUNKSIZE;
        break;
      } 
        
      if (++tout>=TIMEOUT) {
        Serial.println("data transfer timeout");    
        return;
      }
    }
        
  }

    if (r) {
    Serial.println("sending last data");
    sendOK=false;
    mdat.length=r;
    mdat.chunk=n;
    
    int16_t* pc=adcdata+n*CHUNKSIZE;
    memcpy(mdat.data, pc, r*sizeof(int16_t));
    
    tout=0;
    
    while(true) {
      result=esp_now_send(masterAddress, (uint8_t *) &mdat, sizeof(mdat));
      
      dt=millis()+200;    
      while (not sendOK) {
        if(millis()>dt) break;
      }
 
      if (sendOK) break;
      if (++tout>=TIMEOUT) {         
        Serial.printf("data transfer timeout (tail:%d)\n",r);
        return;
      }
        
    }

    transferbusy=false;
  }
  Serial.println("data successfully sent");
}