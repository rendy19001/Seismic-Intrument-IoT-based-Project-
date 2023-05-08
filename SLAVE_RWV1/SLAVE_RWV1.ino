/*
Rendy x Prof.Yudi - Geofisika Unpad 2019
SLAVE (SENSOR NODE)
*/

#include <WiFi.h>
#include <esp_now.h>

#define DEBUG
#define ADCN 34
#define ADCP 35
#define DAC 25
#define ILED 2
#define TIMEOUT 2000 //2 seconds approx

// give window before deepsleep (in ms)
//#define BOOTWINDOW 120000  
//#define BOOTWINDOW 30000

// an hour approximately
//#define EVERY 3600
//#define EVERY 30

uint8_t current_chunk=0;
volatile bool doacq=true;

// ---- DATA FORMAT ----
#define MSG_HEAD 0
#define MSG_INT 1
#define DLENGTH 8192
#define OVERSMP 50
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

head_message head;
struct_message mdat;

int16_t adcdata[DLENGTH];
volatile bool sendOK=false;
bool transferbusy=false;
//unsigned long twin=BOOTWINDOW;
unsigned long tsmp;
double timestamp;
int devid=0;
long every=3600;
int8_t ack_param=-1;
uint8_t bcastmac[]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

RTC_DATA_ATTR bool nodeack=false;
RTC_DATA_ATTR uint8_t masterAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

void blink(int n=1) {
  for(int i=0;i<n;i++) {
    digitalWrite(ILED,HIGH);
    delay(100);
    digitalWrite(ILED,LOW);
    delay(900);
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
      if(comparemac(masterAddress, bcastmac)) {
      // pair to master device
      esp_now_peer_info_t peerInfo;
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx=ESP_IF_WIFI_STA;
      memcpy(masterAddress, mac, 6);
      memcpy(peerInfo.peer_addr, mac, 6);
      if (esp_now_add_peer(&peerInfo) != ESP_OK) return;
      Serial.println("pairing to "+mactostring(mac));
    }

  String cmd="";
  String param="";
  
  String respstr((char*)incomingData);
  int sp=respstr.indexOf(' ');

  //Serial.println(respstr);

  if(sp<0) {
    cmd=respstr;
  }
  else {
    cmd=respstr.substring(0,sp);
    param=respstr.substring(sp+1);
  }

  if (cmd == "ping") {
    if(param == "") blink(10);
    else blink(param.toInt());
  }

  else if (cmd == "checking") {
    char msg[]="checking node-0.1";
    esp_now_send(masterAddress, (uint8_t *) msg, strlen(msg));
    blink(1);
  }

  else if (cmd == "time") {
    int dot=param.indexOf('.');
    time_t sec=param.substring(0,dot).toInt();
    time_t usec=param.substring(dot+1,dot+7).toInt();
    setTime(sec,usec);

    Serial.printf("timesync: %0.6f\n", getTimestamp());

  }

  else if(cmd=="stop") {
    doacq=false;
    nodeack=false;
    blink(2);
  }
  
  else if (cmd=="go") {
    doacq=true;
    nodeack = true;
    blink(2);
  }

  else if(cmd=="info") {
    char devinfo[]="info seismo-node-0.1 element:Z lat:0.0 lon:0.0";
    int devinfo_len = sizeof(devinfo) / sizeof(devinfo[0]);

    if(esp_now_send(mac, (uint8_t*)devinfo, devinfo_len) == ESP_OK) {;
      Serial.println("info sent!");
      }
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  sendOK=(status == ESP_NOW_SEND_SUCCESS);
}

bool esplink() {
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return false;
  }

    esp_now_register_recv_cb(OnDataRecv);
    esp_now_register_send_cb(OnDataSent);

    esp_now_peer_info_t peerInfo;  
    memcpy(peerInfo.peer_addr, masterAddress, 6);
  
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    peerInfo.ifidx=ESP_IF_WIFI_STA;
          
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return false;
    }

  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(ILED, OUTPUT);
  
  esplink();
  inscribe();
/*
  while(true) {
    while (!nodeack) {
      delay(100);
    } 

     while (nodeack) {
      getdata();
      senddata();
    }
  }
*/

}

void loop() {

    if(nodeack) {
      if(doacq) getdata();
      if(doacq) senddata();
      nodeack = false;
    }
}

