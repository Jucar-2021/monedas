#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

/* ========= CONFIGURA AQUÍ ========= */

// Lista de pines a monitorear (evitamos 0,1,20,21 y 31,33)
int PINS[] = {
  // fila izquierda común en shields: 2..19
  2,3,4,5,6,7,8,9,10,12,13,14,15,16,17,18,19,
  // fila derecha: 22..53 (omite 31 y 33 si están dañados/puenteados)
  22,23,24,25,26,27,28,29,30,32,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53
};
// si quieres reducir, deja por ejemplo {15,18,22,24,26,28} etc.

const bool PULSO_ACTIVO_BAJO = true;   // true: entradas tiran a GND por opto
const unsigned long MIN_PULSE_US = 15000;   // 15 ms mínimo para considerar "pulso"
const unsigned long MAX_PULSE_US = 200000;  // 200 ms máximo (evita falsos largos)

/* ========= FIN DE CONFIG ========= */

struct Chan {
  int pin;
  int lastLevel;
  bool enPulso;
  unsigned long tEdge;
  unsigned long lastWidth;
} ch[sizeof(PINS)/sizeof(PINS[0])];

void showHeader() {
  lcd.setCursor(0,0); lcd.print("SERVICIO HENRIQUEZ  ");
}

void showIdle() {
  lcd.clear(); showHeader();
  lcd.setCursor(0,1); lcd.print("BUSCADOR DE PINES   ");
  lcd.setCursor(0,2); lcd.print("Inserte moneda...   ");
  lcd.setCursor(0,3); lcd.print("Esperando cambios   ");
}

void showEvent(const Chan& c, const char* tipo, bool withWidth){
  lcd.clear(); showHeader();
  char b[21];

  snprintf(b, sizeof(b), "Pin: D%-3d %s", c.pin, tipo);
  lcd.setCursor(0,1); lcd.print(b);

  if (withWidth){
    float ms = c.lastWidth / 1000.0f;
    char w[16]; dtostrf(ms, 5, 1, w);
    lcd.setCursor(0,2); lcd.print("Ancho: "); lcd.print(w); lcd.print(" ms   ");
  } else {
    lcd.setCursor(0,2); lcd.print("Cambio detectado    ");
  }

  lcd.setCursor(0,3); lcd.print("Inserte otra moneda ");
}

void setup() {
  Wire.begin(); lcd.init(); lcd.backlight();
  showIdle();

  size_t N = sizeof(PINS)/sizeof(PINS[0]);
  for (size_t i=0;i<N;i++){
    ch[i].pin = PINS[i];
    if (PULSO_ACTIVO_BAJO) pinMode(ch[i].pin, INPUT_PULLUP);
    else                   pinMode(ch[i].pin, INPUT);
    ch[i].lastLevel = digitalRead(ch[i].pin);
    ch[i].enPulso = false;
    ch[i].tEdge = micros();
    ch[i].lastWidth = 0;
  }

  // Asegura que D11 (bobina) no se active accidentalmente durante el escaneo
  pinMode(11, OUTPUT);
  digitalWrite(11, LOW);   // tu placa es activo alto
}

void loop() {
  size_t N = sizeof(PINS)/sizeof(PINS[0]);
  unsigned long now = micros();

  for (size_t i=0;i<N;i++){
    int pin = ch[i].pin;
    int level = digitalRead(pin);

    if (PULSO_ACTIVO_BAJO){
      // Pulso válido es tramo LOW
      if (!ch[i].enPulso){
        if (ch[i].lastLevel == HIGH && level == LOW){
          ch[i].enPulso = true;
          ch[i].tEdge = now;                 // inicio de LOW
          showEvent(ch[i], "FALL", false);   // flanco de bajada
        }
      } else {
        if (ch[i].lastLevel == LOW && level == HIGH){
          unsigned long width = now - ch[i].tEdge; // duración LOW
          ch[i].enPulso = false;
          if (width >= MIN_PULSE_US && width <= MAX_PULSE_US){
            ch[i].lastWidth = width;
            showEvent(ch[i], "PULSO", true); // pulso completo
          } else {
            showEvent(ch[i], "RUIDO", false);
          }
        }
      }
    } else {
      // Pulso válido es tramo HIGH
      if (!ch[i].enPulso){
        if (ch[i].lastLevel == LOW && level == HIGH){
          ch[i].enPulso = true;
          ch[i].tEdge = now;                 // inicio de HIGH
          showEvent(ch[i], "RISE", false);
        }
      } else {
        if (ch[i].lastLevel == HIGH && level == LOW){
          unsigned long width = now - ch[i].tEdge; // duración HIGH
          ch[i].enPulso = false;
          if (width >= MIN_PULSE_US && width <= MAX_PULSE_US){
            ch[i].lastWidth = width;
            showEvent(ch[i], "PULSO", true);
          } else {
            showEvent(ch[i], "RUIDO", false);
          }
        }
      }
    }

    ch[i].lastLevel = level;
  }

  // Refresca header cada ~150 ms
  static unsigned long tUi=0;
  if (millis()-tUi>150){ tUi=millis(); showHeader(); }
}
