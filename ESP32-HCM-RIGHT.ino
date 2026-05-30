#include <ESP32Servo.h>
#include <FastLED.h>

#define ID_PROPRIU 0x01
#define ID_BROADCAST 0x11

#define RXD2 16
#define TXD2 17

#define FAZA_SCURTA_PIN   25 
#define FAZA_LUNGA_PIN    26 
#define CEATA_PIN         27 

#define SERVO_OX_PIN      18 
#define SERVO_OY_PIN      19 
#define SERVO_CENTRU_OY   90 
#define LIMITA_SUS        12 
#define LIMITA_JOS        15
#define SERVO_CENTRU_OX   90 
#define MAX_ROTATIE       25 
#define CORNERING_ANGLE   -25
#define CORNERING_STEP    5

#define LED_PIN           13
#define NUM_LEDS          12 
#define FULL_BRIGHTNESS   255 
#define MID_BRIGHTNESS    127
#define LOW_BRIGHTNESS    66
#define OFF_BRIGHTNESS    0
#define LED_TYPE          WS2812B
#define COLOR_ORDER       GRB 

Servo servoOx; 
Servo servoOy; 

CRGB leds[NUM_LEDS];

float pitchFiltrat = 0;
unsigned long lastFlashMillis = 0;
int stepSemnal = 0;
bool stareAvarii = false;
int corneringStart = 0;
int corneringStep = 5;

void setup() {
  Serial.begin(115200); 
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  servoOx.setPeriodHertz(50);    // Standard 50hz servo
  servoOx.attach(SERVO_OX_PIN, 500, 2400); 

  servoOy.setPeriodHertz(50); 
  servoOy.attach(SERVO_OY_PIN, 500, 2400); 

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  
  FastLED.setBrightness(FULL_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  
  Serial.println("Sistem Dual-Pin (Stanga/Dreapta) Pregatit.");

  ledcAttach(FAZA_SCURTA_PIN, 5000, 8);

  ledcAttach(FAZA_LUNGA_PIN, 5000, 8);

  ledcAttach(CEATA_PIN, 5000, 8);
}

void semnalizare() {
  FastLED.clear();
  
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(255, 120, 0);
    FastLED.show();
    delay(50);
  }

  delay(100);
  FastLED.show();
  delay(200);
}

void DRL(int lux) {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::White;
  }

  if (lux < 30) {
    FastLED.setBrightness(MID_BRIGHTNESS);
  } else {
    FastLED.setBrightness(FULL_BRIGHTNESS);
  }

  FastLED.show();
}

void fazaScurta(int lux) {
  ledcWrite(FAZA_SCURTA_PIN, lux < 30 ? MID_BRIGHTNESS : 0);
}

void fazaLunga(int dist) {
  ledcWrite(FAZA_LUNGA_PIN, dist > 500 ? FULL_BRIGHTNESS : 0);
}

void ceata(int hum) {
  ledcWrite(CEATA_PIN, hum > 75 ? FULL_BRIGHTNESS : 0);
}

// Funcție pentru animația de semnalizare (Non-blocking)
void gestioneazaLEDuri(int mod, int lux) {
  unsigned long currentMillis = millis();

  // Resetăm banda dacă nu este semnal/avarie
  if (mod == 0) {
    for(int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::White;
    // Reglăm luminozitatea DRL în funcție de lux
    FastLED.setBrightness(lux < 30 ? MID_BRIGHTNESS : FULL_BRIGHTNESS);
    FastLED.show();
    stepSemnal = 0; // Reset pas semnalizare
    return;
  }

  // SEMNALIZARE (1=Stanga, 2=Dreapta)
  // Verificăm dacă ID-ul farului corespunde cu partea cerută
  bool esteParteaMea = (mod == 1 && ID_PROPRIU == 0x10) || (mod == 2 && ID_PROPRIU == 0x01);

  if (esteParteaMea) {
    if (currentMillis - lastFlashMillis > 60) { // Viteza animației
      if (stepSemnal == 0) FastLED.clear();

      FastLED.setBrightness(FULL_BRIGHTNESS);
      leds[stepSemnal] = CRGB(255, 120, 0); // Portocaliu
      FastLED.show();
      
      stepSemnal++;
      if (stepSemnal >= NUM_LEDS) {
        stepSemnal = 0;
        // O scurtă pauză după ce s-a umplut banda
        lastFlashMillis = currentMillis + 200; 
      } else {
        lastFlashMillis = currentMillis;
      }
    }
  } 
  // AVARII
  else if (mod == 3) {
    if (currentMillis - lastFlashMillis > 500) {
      stareAvarii = !stareAvarii;
      FastLED.setBrightness(FULL_BRIGHTNESS);
      for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = stareAvarii ? CRGB(255, 120, 0) : CRGB::Black;
      }
      FastLED.show();
      lastFlashMillis = currentMillis;
    }
  }
  // Dacă e semnal pentru cealaltă parte, rămânem pe DRL sau stingem
  else {
    for(int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::White;
    FastLED.setBrightness(lux < 30 ? MID_BRIGHTNESS : FULL_BRIGHTNESS);
    FastLED.show();
  }
}

void actualizeazaAutonivelareLina(int pitchNou) {
  // Filtru simplu: 90% valoarea veche, 10% valoarea noua
  pitchFiltrat = (pitchFiltrat * 0.9) + (pitchNou * 0.1);
  
  int unghiServo = map((int)pitchFiltrat, -20, 20, SERVO_CENTRU_OY - LIMITA_JOS, SERVO_CENTRU_OY + LIMITA_SUS); 
  servoOy.write(unghiServo);
}

void actualizeazaDirectie(int steer) {
  int steerConstrained = constrain(steer, -90, 90);
  int unghiServoOx = map(steerConstrained, -90, 90, (SERVO_CENTRU_OX - MAX_ROTATIE), (SERVO_CENTRU_OX + MAX_ROTATIE));

  servoOx.write(unghiServoOx);
}

void cornering(int ceataState, int steer) {
  if (ceataState) {
    ledcWrite(CEATA_PIN, FULL_BRIGHTNESS);
  }
  else {
    if (steer < CORNERING_ANGLE) {
      ledcWrite(CEATA_PIN, corneringStart);
      if (corneringStart >= 250) {
        
      }
      else {
        corneringStart += CORNERING_STEP;
      }
    }
    else {
      ledcWrite(CEATA_PIN, corneringStart);
      if (corneringStart <= 0) {
        
      }
      else {
        corneringStart -= CORNERING_STEP;
      }
    }
  }
}

void loop() {
  if (Serial2.available() > 0) {
    String mesaj = Serial2.readStringUntil('\n');
    mesaj.trim();

    if (mesaj.startsWith("#") && mesaj.endsWith("!")) {
      int separatorIndex = mesaj.indexOf('|');
      int idPrimit = mesaj.substring(1, separatorIndex).toInt();

      if (idPrimit == ID_BROADCAST || idPrimit == ID_PROPRIU) {
        String datele = mesaj.substring(separatorIndex + 1, mesaj.length() - 1);
        
        int p[8];
        p[0] = datele.indexOf('|');
        for(int i = 1; i < 8; i++) {
          p[i] = datele.indexOf('|', p[i-1] + 1);
        }

        int s = datele.substring(0, p[0]).toInt();
        int pitch = datele.substring(p[0] + 1, p[1]).toInt();
        int dist = datele.substring(p[1] + 1, p[2]).toInt();
        int lux = datele.substring(p[2] + 1, p[3]).toInt();
        int hum = datele.substring(p[3] + 1, p[4]).toInt();
        int modSemnal = datele.substring(p[4] + 1, p[5]).toInt();
        
        int scurtaState = datele.substring(p[5] + 1, p[6]).toInt();
        int lungaState = datele.substring(p[6] + 1, p[7]).toInt();
        int ceataState = datele.substring(p[7] + 1).toInt();

        ledcWrite(FAZA_SCURTA_PIN, scurtaState ? MID_BRIGHTNESS : 0);
        ledcWrite(FAZA_LUNGA_PIN, lungaState ? FULL_BRIGHTNESS : 0);
        cornering(ceataState, s);

        gestioneazaLEDuri(modSemnal, lux);
        actualizeazaDirectie(s);
        actualizeazaAutonivelareLina(pitch);

        Serial.println(mesaj);
      }
    }
  }
}
