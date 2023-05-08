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
