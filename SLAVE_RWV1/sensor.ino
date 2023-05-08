void getdata() {
  Serial.println("data acquisition");

  
  dacWrite(DAC,45); // 0.58V 
  analogSetAttenuation(ADC_0db);
  long dd;
  int ap, an;

  timestamp=getTimestamp();
  unsigned long tt=millis();  
  for (int i=0;i<DLENGTH;i++) {
    
    if (not doacq) break;
    digitalWrite(ILED, HIGH);
    dd=0;
    for(int s=0;s<OVERSMP;s++) {
      ap=analogRead(ADCP);
      an=analogRead(ADCN);
      dd+=(ap-an);
    }
    adcdata[i]=dd/OVERSMP;
  }
  tsmp=millis()-tt;
  
  digitalWrite(ILED, LOW);
}
