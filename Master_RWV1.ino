/*
Rendy x Prof.Yudi - Geofisika Unpad 2019
MASTER
*/

//to do
//deep sleep
//disconneted confirmasion
//data storing slave
//data fetching from slave
//python (user interface)

#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Ticker.h>

#define MAXCLIENT 10
#define ILED 2

// ---- DATA FORMAT ----
#define MSG_HEAD 0
#define MSG_INT 1
#define MSG_INT2D 2
#define MSG_INT3D 3

#define MSG_INFO 20
#define MSG_RAW 21

// start with readable ascii
#define MSG_TEXT 32 
#define CHUNKSIZE 100

typedef struct struct_message {
  uint8_t msgtype;  // this must be the first field
  uint8_t length; // length of this chunk
  uint8_t chunk;
  int16_t data[CHUNKSIZE];
};

typedef struct head_message {
  uint8_t msgtype;
  size_t length;
  uint8_t dtype;
  unsigned long tsample;
  double timestamp;
};

// ----

esp_now_peer_info_t bCastpeer;
esp_now_peer_info_t uniCastpeer;

// allocate for max #client=20
size_t dlen[MAXCLIENT];
size_t recvd[MAXCLIENT];
int16_t* adcdata[MAXCLIENT];
volatile bool completed[MAXCLIENT];
int current_chunk[MAXCLIENT];
unsigned long tsample[MAXCLIENT];
double timestamp[MAXCLIENT];
uint8_t dtype[MAXCLIENT];
uint8_t clientmac[MAXCLIENT][6];
bool clientpeer[MAXCLIENT];
uint8_t incomingmyData[100];
int incomingDataLen = 0;

uint8_t zeromac[]={0,0,0,0,0,0};
uint8_t bcastmac[]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

Ticker timer;

bool setTime(time_t sec, time_t usec) {
  struct timeval tm;
  struct timezone tz={-420,0}; // GMT+7
  tm.tv_sec=sec;
  tm.tv_usec=usec;
  int retv=settimeofday(&tm,&tz);
  return (retv==-1)?false:true;
}

double getTimestamp() {
  struct timeval tm;
  struct timezone tz;
  gettimeofday(&tm, &tz);
  double st=(double)tm.tv_sec;
  double ut=(double)tm.tv_usec;
  double tt;
  ut=ut*1e-6;
  tt=st+ut;
  return tt;
}

String mactostring(const uint8_t* mac) {
  char ms[20];
  sprintf(ms,"%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  return ms;
}


bool comparemac(const uint8_t* maca, const uint8_t* macb) {
  for (int i=0;i<6;i++) {
    if (maca[i] != macb[i]) return false;
  }
  return true;
}

int findmac(const uint8_t* mac) {
  for (int i=0;i<MAXCLIENT;i++) {
    if(comparemac(mac,clientmac[i])) return (i+1);
  }
  return 0;
}

int insertmac(const uint8_t* mac) {
  uint8_t zeromac[]={0,0,0,0,0,0};
  int fm=findmac(mac);
  if(fm) return fm;
   
  for (int i=0;i<MAXCLIENT;i++) {
    if(comparemac(clientmac[i],zeromac)) {
      for(int m=0;m<6;m++) {
        clientmac[i][m]=mac[m];
      }
      
      esp_now_peer_info_t peerInfo;
      memcpy(peerInfo.peer_addr, clientmac[i], 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx=ESP_IF_WIFI_STA;
      
      if(esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("peer failed: "+mactostring(clientmac[i]));
      }
      return (i+1);
    }
  }
  return 0;
}


void blink(int n=1) {
  for(int i=0;i<n;i++) {
    digitalWrite(ILED,HIGH);
    delay(100);
    digitalWrite(ILED,LOW);
    delay(900);
  }
}

void sendcmd(const char* cmd, const uint8_t *mac=bcastmac) {
  if(findmac(mac)) {
    if(not comparemac(zeromac,mac))
      esp_now_send(mac, (uint8_t *) cmd, strlen(cmd));
  } 
  else { // broadcast
  esp_now_send(bCastpeer.peer_addr, (uint8_t *) cmd, strlen(cmd));
  }
}


void parsemsg(const uint8_t* mac, String msg) {

  char stmp[64];
  char devinfo[msg.length() + 1];
  strcpy(devinfo, msg.c_str());

  bool callbackFound = false;
  if (msg == strstr(devinfo, "checking")) {
    for (int i=0; i < MAXCLIENT; i++) {
      if (comparemac(mac, clientmac[i])) {
        char* substr = strstr(devinfo, "checking") + strlen("checking") + 1;
        //Serial.println(substr);
        Serial.println(" >> " + mactostring(mac)+ " standby");
        callbackFound = true;
      }
    }
    if (callbackFound != true) {
      const char* substr = strstr(devinfo, "checking") + strlen("checking") + 1;
      Serial.println("callback received from >> " + mactostring(mac));
      Serial.println("peer not register yet");
    }

  }

  else if (msg=="peer") {
    Serial.println("peer_req: "+mactostring(mac));
    sprintf(stmp,"sync %0.6f",getTimestamp());
    esp_now_send(mac, (uint8_t*) stmp, strlen(stmp));

    if(not findmac(mac)) {
    int id=insertmac(mac);
    Serial.println("regist_node "+mactostring(mac));
    }
  }
  
  else if (msg == strstr(devinfo, "time")) {
    Serial.println();
    Serial.print(" <> ");
    Serial.print(devinfo);
    Serial.println();

    memset(devinfo, '\0', sizeof(devinfo));
    }
  
  else if (msg == strstr(devinfo, "start")) {
    char* substr = strstr(devinfo, "start") + strlen("start") + 1;
    substr[23] = '\0';
    Serial.println();
    Serial.print(" <> ");
    Serial.print(substr);
    Serial.println();

    memset(devinfo, '\0', sizeof(devinfo));
    }

  else if (msg == strstr(devinfo, "stop")) {
    char* substr = strstr(devinfo, "stop") + strlen("stop") + 1;
    substr[20] = '\0';
    Serial.println();
    Serial.print(" <> ");
    Serial.print(substr);
    Serial.println();

    memset(devinfo, '\0', sizeof(devinfo));
    }
  
  else if (msg == strstr(devinfo, "info")) {
    char* substr = strstr(devinfo, "info") + strlen("info") + 1;
    Serial.println(substr);
    }
}

void recvhead(const uint8_t * mac, const uint8_t *incomingData) {
  
  head_message* head=(head_message*) incomingData;
  int devid=findmac(mac);
  int idx;

  if(completed[idx]) return; // previous data is still available
  
  if(devid) idx=devid-1;
  else {
    Serial.println("registering node "+mactostring(mac));
    devid=insertmac(mac);
    parsemsg(mac,"sync");
    idx=devid-1;
    if(devid==0) return;
  }

  if(idx>MAXCLIENT) {
    Serial.printf("maximum client exceeded\n");
    return;
  }
  
  if(adcdata[idx]) free(adcdata[idx]);
  recvd[idx]=0;
  
  dlen[idx]=head->length;
  tsample[idx]=head->tsample;
  timestamp[idx]=head->timestamp;
  dtype[idx]=head->dtype;
  
  adcdata[idx]=(int16_t*) malloc(dlen[idx]*sizeof(int16_t));
}

void recvdata(const uint8_t * mac, const uint8_t *incomingData, int len) {
  
  struct_message* mdat=(struct_message*)incomingData;
  
  int devid=findmac(mac);
  int idx;
  
  if(devid) idx=devid-1;
  else return;
  
  if ((int)mdat->chunk == current_chunk[idx]) return;
  current_chunk[idx]=(int)mdat->chunk;

  if(adcdata[idx] == NULL) { // avoid racing problem
    delay(100);
    if(adcdata[idx] == NULL) return;
  }
  
  int16_t* pc=adcdata[idx]+((int)mdat->chunk*CHUNKSIZE);
  
  memcpy(pc,mdat->data,(int)mdat->length*sizeof(int16_t));
  recvd[idx]+=(unsigned int)mdat->length;
  
  //Serial.printf("received=%d\n",recvd[devid]);   
  if(recvd[idx] >= dlen[idx]) {
    completed[idx]=true;
    current_chunk[idx]=-1;
    digitalWrite(ILED,LOW);
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {

  memcpy(incomingmyData, incomingData, len);
  incomingDataLen = len;

  //slave request
  if(incomingData[0]>=MSG_TEXT) {
    parsemsg(mac, String((char*) incomingData)); 
  }

  if(incomingData[0]==MSG_RAW) {
    Serial.printf("bytes %d\n",(len-1));
    Serial.write(incomingData+1,len-1);
  }

  // data head
  else if (incomingData[0]==0) {
    recvhead(mac, incomingData);
    }

  // data body
  else {
    recvdata(mac, incomingData,len);
  }

}

void checkReceive() {
  static unsigned long lastRecvTime = 0;
  unsigned long currentMillis = millis();

  if (incomingDataLen > 0) {
    incomingDataLen = 0;
    lastRecvTime = currentMillis;
    return;
  }

  if (currentMillis - lastRecvTime >= 5000) {
    // Tidak ada callback yang diterima dalam 5 detik terakhir
    Serial.println("No receive callback within the last 5 seconds");
  }

  lastRecvTime = currentMillis;
}

void OnDataSent(const uint8_t *mac, esp_now_send_status_t status) {
 Serial.println((status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ") + mactostring(mac));
}

void fileWriter(void *pvParameters) {
  while (true) {
    for (int id=0; id<MAXCLIENT; id++) {
      //newline is "end of message"
      if (completed[id]) {
        Serial.printf("json\n");
        Serial.printf("{\"length\":%d,",dlen[id]);
        Serial.printf("\"dtype\":%d,",dtype[id]);
        Serial.print("\"devid\":\""+mactostring(clientmac[id])+"\",");
        Serial.printf("\"tsample\":%d,",tsample[id]);
        Serial.printf("\"timestamp\":%0.6f,",timestamp[id]);
        Serial.printf("\"data\":[");

        for(int i=0;i<dlen[id];i++) {
          if(i==0) Serial.printf("\n%d",adcdata[id][i]);
          else Serial.printf(",\n%d",adcdata[id][i]);
        }
    

        Serial.printf("]}\n");
        
        completed[id]=false; // we may accept new data
        free(adcdata[id]);
        adcdata[id]=NULL;

      }
    }
  }
}

void setup() {

  for (int i=0;i<MAXCLIENT;i++) {
    dlen[i]=0;
    adcdata[i]=NULL;
    current_chunk[i]=-1;
    completed[i]=false;
    for(int m=0;m<6;m++) clientmac[i][m]=0;
  }

  bCastpeer.channel=0;
  bCastpeer.encrypt=false;
  bCastpeer.ifidx=ESP_IF_WIFI_STA;
  for(int i=0;i<6;i++) bCastpeer.peer_addr[i]=0xFF;

  Serial.begin(115200);
  pinMode(ILED,OUTPUT);

  
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_add_peer(&bCastpeer);
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

/*
  //addpeer Bcast
  for(int i=0;i<6;i++) {bCastpeer.peer_addr[i]=0xFF;
    bCastpeer.channel = 0;
    bCastpeer.encrypt =false;
    if (esp_now_add_peer(&bCastpeer) != ESP_OK){
    Serial.println("Failed to add bcast peer");
    return;
     }
  }*/

  xTaskCreatePinnedToCore(
    fileWriter
    ,  "fileWriter"
    ,  10240  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL 
    ,  1);

    blink(2);

}

void loop() {

  String cmd;
  String param;

  if (Serial.available()) {
    String line=Serial.readStringUntil('\n');
    int spc=line.indexOf(' ');

    if(spc>0) {
      cmd=line.substring(0,spc);
      param=line.substring(spc+1);
    }
    else {
      cmd=line;
      param="";
    }

    if (cmd=="sync") // time synchronization: sync sec.usec
    {
      int dot=param.indexOf('.');
      String ss=param.substring(0, dot);
      String us=param.substring(dot+1,dot+7);
      
      time_t sec=ss.toInt();
      time_t usec=us.toInt();

      setTime(sec,usec);      
      sendcmd(("time " + String(sec) + "." + String(usec)).c_str());
      Serial.printf("timesync: %0.6f\n", getTimestamp());

    }

    else if (cmd=="list") {
      bool anyClients = false;
      for (int i =0;i<MAXCLIENT;i++){
        if(not comparemac(zeromac,clientmac[i])) {
          Serial.println(String(i)+" "+mactostring(clientmac[i]));
          anyClients = true;
        }
      }
      if (not anyClients) {
        Serial.println("list empty.");
      }
    }

    else if (cmd == "del") {  // del idx
      int node = param.toInt();

      if (node < MAXCLIENT) {
        if(esp_now_del_peer(clientmac[node]) == ESP_OK){
          Serial.println("node:" + String(node) + " " + "has been removed !" );
        }
        memcpy(clientmac[node], zeromac, 6);
      }
    }

    else if (cmd=="sendto") { //send cmd to certain sensor node
      int sp=param.indexOf(' ');
      uint8_t node=param.substring(0,sp).toInt();

      if (node<MAXCLIENT){
        sendcmd(param.substring(sp+1).c_str(),clientmac[node]);

        if (param.substring(sp+1) == "ping"){
          Serial.println("node :" + String(node));
          String pingMessage = "ping";
          esp_err_t result = esp_now_send(clientmac[node], (uint8_t*) pingMessage.c_str(), pingMessage.length() + 1);
          if (result == ESP_OK) {
            Serial.println("mac :" + mactostring(clientmac[node]) + " " +"is ready !"); 
          } else {
            Serial.println("mac :" + mactostring(clientmac[node]) + " " +"fail to send message !"); 
          } 
        }

        if (param.substring(sp+1) == "info") {
          Serial.println("here's node " + mactostring(clientmac[node]) + " " + "info" );
          }     
      }
      else{
        Serial.println("invalid node");
      }
    }

    else if (cmd == "bcast") { //send cmd to all connected node
      sendcmd(param.substring(sp+1).c_str(), bcastmac);
      
      timer.once(5, checkReceive);
    }
  }
}
