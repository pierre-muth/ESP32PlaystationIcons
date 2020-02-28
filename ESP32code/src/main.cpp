#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

const byte DNS_PORT = 53;
RgbwColor CROSScolor = RgbwColor(0,0,255, 32);
RgbwColor CIRCLEcolor = RgbwColor(255,0,0, 32);
RgbwColor SQUAREcolor = RgbwColor(255,0,255, 64);
RgbwColor TRIANGLEcolor = RgbwColor(0,255,0, 32);
RgbwColor STARTcolor = RgbwColor(0,0,0,0);

IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws");

int animateRainbow = 0;
float rainbowBrightness = 0.5;

// four element pixels, RGBW
NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt0Sk6812Method> stripCross(4, 16);
NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt1Sk6812Method> stripCircle(4, 17);
NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt2Sk6812Method> stripSquare(4, 18);
NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt3Sk6812Method> striptriangle(3, 19);

RgbwColor targetColor = RgbwColor(128, 128, 128, 128);

NeoPixelAnimator crossAnimations(4);
NeoPixelAnimator circleAnimations(4);
NeoPixelAnimator squareAnimations(4);
NeoPixelAnimator triangleAnimations(3); // NeoPixel animation management object

struct IconPixelAnimationState {
    RgbwColor StartingColor;
    RgbwColor EndingColor;
};

// one entry per pixel to match the animation timing manager
IconPixelAnimationState crossAnimationState[4];
IconPixelAnimationState circleAnimationState[4];
IconPixelAnimationState squareAnimationState[4];
IconPixelAnimationState triangleAnimationState[3];

void SetRandomSeed() {
    uint32_t seed;
    // random works best with a seed that can use 31 bits
    // analogRead on a unconnected pin tends toward less than four bits
    seed = analogRead(0);
    delay(1);

    for (int shifts = 3; shifts < 31; shifts += 3) {
        seed ^= analogRead(0) << shifts;
        delay(1);
    }

    randomSeed(seed);
}

void BlendAnimUpdateCross(const AnimationParam& param) {
    RgbwColor updatedColor = RgbwColor::LinearBlend(
        crossAnimationState[param.index].StartingColor,
        crossAnimationState[param.index].EndingColor,
        param.progress);
    stripCross.SetPixelColor(param.index, updatedColor); // apply the color to the strips
}

void BlendAnimUpdateCircle(const AnimationParam& param) {
    RgbwColor updatedColor = RgbwColor::LinearBlend(
        circleAnimationState[param.index].StartingColor,
        circleAnimationState[param.index].EndingColor,
        param.progress);
    stripCircle.SetPixelColor(param.index, updatedColor);
}

void BlendAnimUpdateSquare(const AnimationParam& param) {
    RgbwColor updatedColor = RgbwColor::LinearBlend(
        squareAnimationState[param.index].StartingColor,
        squareAnimationState[param.index].EndingColor,
        param.progress);
    stripSquare.SetPixelColor(param.index, updatedColor);
}

void BlendAnimUpdateTriangle(const AnimationParam& param) {
    RgbwColor updatedColor = RgbwColor::LinearBlend(
        triangleAnimationState[param.index].StartingColor,
        triangleAnimationState[param.index].EndingColor,
        param.progress);
    striptriangle.SetPixelColor(param.index, updatedColor);
}

String processor(const String& var) {
  if(var == "SR") return String(SQUAREcolor.R);
  if(var == "SG") return String(SQUAREcolor.G);
  if(var == "SB") return String(SQUAREcolor.B);
  if(var == "SW") return String(SQUAREcolor.W);

  if(var == "CR") return String(CIRCLEcolor.R);
  if(var == "CG") return String(CIRCLEcolor.G);
  if(var == "CB") return String(CIRCLEcolor.B);
  if(var == "CW") return String(CIRCLEcolor.W);

  if(var == "XR") return String(CROSScolor.R);
  if(var == "XG") return String(CROSScolor.G);
  if(var == "XB") return String(CROSScolor.B);
  if(var == "XW") return String(CROSScolor.W);

  if(var == "TR") return String(TRIANGLEcolor.R);
  if(var == "TG") return String(TRIANGLEcolor.G);
  if(var == "TB") return String(TRIANGLEcolor.B);
  if(var == "TW") return String(TRIANGLEcolor.W);

  if(var == "AR") {
    if (animateRainbow == 1) return "checked";
    else return " ";
  }

  if(var == "RB") return String(rainbowBrightness*100);
  
  return String();
}

void onWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("WiFi AP: connection");
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("WiFi AP: disconnection");
}

void onWebSocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.println("Websocket client connection");
  } else if(type == WS_EVT_DISCONNECT){
    Serial.println("Websocket Client disconnected");
  } else if(type == WS_EVT_DATA){
    Serial.print("Websocket Client Data received: ");
    for(int i=0; i < len; i++) {
          Serial.print((char) data[i]);
    }
    Serial.println();
    data[len]  ='\n';
    int value =  (int) strtol((const char *) &data[2], NULL, 10);

    if (data[0] == 's') {
      if (data[1] == 'r') SQUAREcolor.R = value;
      if (data[1] == 'g') SQUAREcolor.G = value;
      if (data[1] == 'b') SQUAREcolor.B = value;
      if (data[1] == 'w') SQUAREcolor.W = value;
      stripSquare.SetPixelColor(0, SQUAREcolor);
      stripSquare.SetPixelColor(1, SQUAREcolor);
      stripSquare.SetPixelColor(2, SQUAREcolor);
      stripSquare.SetPixelColor(3, SQUAREcolor);
      stripSquare.Show();    
    }
    if (data[0] == 'c') {
      if (data[1] == 'r') CIRCLEcolor.R = value;
      if (data[1] == 'g') CIRCLEcolor.G = value;
      if (data[1] == 'b') CIRCLEcolor.B = value;
      if (data[1] == 'w') CIRCLEcolor.W = value;
      stripCircle.SetPixelColor(0, CIRCLEcolor);
      stripCircle.SetPixelColor(1, CIRCLEcolor);
      stripCircle.SetPixelColor(2, CIRCLEcolor);
      stripCircle.SetPixelColor(3, CIRCLEcolor);
      stripCircle.Show();    
    }
    if (data[0] == 'x') {
      if (data[1] == 'r') CROSScolor.R = value;
      if (data[1] == 'g') CROSScolor.G = value;
      if (data[1] == 'b') CROSScolor.B = value;
      if (data[1] == 'w') CROSScolor.W = value;
      stripCross.SetPixelColor(0, CROSScolor);
      stripCross.SetPixelColor(1, CROSScolor);
      stripCross.SetPixelColor(2, CROSScolor);
      stripCross.SetPixelColor(3, CROSScolor);
      stripCross.Show();    
    }
    if (data[0] == 't') {
      if (data[1] == 'r') TRIANGLEcolor.R = value;
      if (data[1] == 'g') TRIANGLEcolor.G = value;
      if (data[1] == 'b') TRIANGLEcolor.B = value;
      if (data[1] == 'w') TRIANGLEcolor.W = value;
      striptriangle.SetPixelColor(0, TRIANGLEcolor);
      striptriangle.SetPixelColor(1, TRIANGLEcolor);
      striptriangle.SetPixelColor(2, TRIANGLEcolor);
      striptriangle.Show();    
    }
    if (data[0] == 'a') {
      if (data[1] == 'r') {
        if (data[2] == 't') animateRainbow = 1;
        if (data[2] == 'f') animateRainbow = 0;
      }
    }
    if (data[0] == 'r') {
      if (data[1] == 'b') {
        rainbowBrightness = value/100.0;
      }
    }
    if (data[0] == 'k') {
      if (data[1] == 'w') {
          WiFi.disconnect();   
          WiFi.mode(WIFI_OFF);
          dnsServer.stop();
          ledcWrite(0, 192);
      }
    } 
  }
}

void setup() { 

  Serial.begin(115200);

  WiFi.disconnect();   
  WiFi.mode(WIFI_OFF); //added to start with the wifi off, avoid crashing
  dnsServer.stop();

  delay(100);
  
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Playstation Icons");
  WiFi.onEvent(onWiFiStationConnected, SYSTEM_EVENT_AP_STACONNECTED);
  WiFi.onEvent(onWiFiStationConnected, SYSTEM_EVENT_AP_STADISCONNECTED);

  dnsServer.start(DNS_PORT, "*", apIP); // if DNSServer is started with "*" for domain name, it will reply with provided IP to all DNS request

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  stripCross.Begin();
  stripCross.Show();
  stripCircle.Begin();
  stripCircle.Show();
  stripSquare.Begin();
  stripSquare.Show();
  striptriangle.Begin();
  striptriangle.Show();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
    Serial.println("requested / served index.html");
  });

  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
    Serial.println("requested index.html served index.html");
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/favicon.ico", "image/png");
    Serial.println("requested / served favicon.ico");
  });

  server.on("/square.jpg", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/square.jpg", "image/jpeg");
    Serial.println("requested / served favicon.ico");
  });

  server.on("/circle.jpg", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/circle.jpg", "image/jpeg");
    Serial.println("requested / served favicon.ico");
  });

  server.on("/cross.jpg", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/cross.jpg", "image/jpeg");
    Serial.println("requested / served favicon.ico");
  });

  server.on("/triangle.jpg", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/triangle.jpg", "image/jpeg");
    Serial.println("requested / served favicon.ico");
  });

  webSocket.onEvent(onWebSocketEvent);
  server.addHandler(&webSocket);
  server.begin();
  int i = 0;
  
  for (i=0; i<4; i++) {
    crossAnimationState[i].StartingColor = STARTcolor;
    crossAnimationState[i].EndingColor = CROSScolor;
    crossAnimations.StartAnimation(i, 1000, BlendAnimUpdateCross);
  }
  while (crossAnimations.IsAnimating()) {
    crossAnimations.UpdateAnimations();
    stripCross.Show();
  }

  for (i=0; i<4; i++) {
    circleAnimationState[i].StartingColor = STARTcolor;
    circleAnimationState[i].EndingColor = CIRCLEcolor;
    circleAnimations.StartAnimation(i, 1000, BlendAnimUpdateCircle);
  }
  while (circleAnimations.IsAnimating()) {
    circleAnimations.UpdateAnimations();
    stripCircle.Show();
  }

  for (i=0; i<4; i++) {
    squareAnimationState[i].StartingColor = STARTcolor;
    squareAnimationState[i].EndingColor = SQUAREcolor;
    squareAnimations.StartAnimation(i, 1000, BlendAnimUpdateSquare);
  }
  while (squareAnimations.IsAnimating()) {
    squareAnimations.UpdateAnimations();
    stripSquare.Show();
  }

  for (i=0; i<3; i++) {
    triangleAnimationState[i].StartingColor = STARTcolor;
    triangleAnimationState[i].EndingColor = TRIANGLEcolor;
    triangleAnimations.StartAnimation(i, 1000, BlendAnimUpdateTriangle);
  }
  while (triangleAnimations.IsAnimating()) {
    triangleAnimations.UpdateAnimations();
    striptriangle.Show();
  }
  
  SetRandomSeed();

  ledcSetup(0, 1000, 8);
  ledcAttachPin(22, 0);
  ledcWrite(0, 64);

  Serial.println("End of setup().");
}

void loop() {

  if(WiFi.isConnected()){
    dnsServer.processNextRequest();
  }

  if (animateRainbow == 1) {

  if (crossAnimations.IsAnimating()) {
        crossAnimations.UpdateAnimations();  // the normal loop just needs these two to run the active animations
        stripCross.Show();
    } else {
      uint16_t count = random(4);
      while (count > 0) {
          uint16_t pixel = random(4);
          uint16_t time = random(1000, 2000);
          crossAnimationState[pixel].StartingColor = stripCross.GetPixelColor(pixel);
          crossAnimationState[pixel].EndingColor = HslColor(random(360) / 360.0f, 1.0f, rainbowBrightness);
          crossAnimations.StartAnimation(pixel, time, BlendAnimUpdateCross);
          count--;
      }
    }

    if (circleAnimations.IsAnimating()) {
        circleAnimations.UpdateAnimations();  // the normal loop just needs these two to run the active animations
        stripCircle.Show();
    } else {
      uint16_t count = random(4);
      while (count > 0) {
          uint16_t pixel = random(4);
          uint16_t time = random(1000, 2000);
          circleAnimationState[pixel].StartingColor = stripCircle.GetPixelColor(pixel);
          circleAnimationState[pixel].EndingColor = HslColor(random(360) / 360.0f, 1.0f, rainbowBrightness);
          circleAnimations.StartAnimation(pixel, time, BlendAnimUpdateCircle);
          count--;
      }
    }

    if (squareAnimations.IsAnimating()) {
        squareAnimations.UpdateAnimations();  // the normal loop just needs these two to run the active animations
        stripSquare.Show();
    } else {
      uint16_t count = random(4);
      while (count > 0) {
          uint16_t pixel = random(4);
          uint16_t time = random(1000, 2000);
          squareAnimationState[pixel].StartingColor = stripSquare.GetPixelColor(pixel);
          squareAnimationState[pixel].EndingColor = HslColor(random(360) / 360.0f, 1.0f, rainbowBrightness);
          squareAnimations.StartAnimation(pixel, time, BlendAnimUpdateSquare);
          count--;
      }
    }

    if (triangleAnimations.IsAnimating()) {
        triangleAnimations.UpdateAnimations();  // the normal loop just needs these two to run the active animations
        striptriangle.Show();
    } else {
      uint16_t count = random(3);
      while (count > 0) {
          uint16_t pixel = random(3);
          uint16_t time = random(1000, 2000);
          triangleAnimationState[pixel].StartingColor = striptriangle.GetPixelColor(pixel);
          triangleAnimationState[pixel].EndingColor = HslColor(random(360) / 360.0f, 1.0f, rainbowBrightness);
          triangleAnimations.StartAnimation(pixel, time, BlendAnimUpdateTriangle);
          count--;
      }
    }

  }
}