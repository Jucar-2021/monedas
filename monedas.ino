#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ADDR 0x27
LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);

/* ===== Pines ===== */
const int PIN_PULSOS = 15;        // Entrada (monedero)
const bool USA_PULLUP = true;     // true => INPUT_PULLUP
const int PIN_BOBINA  = 11;       // Salida a relé/SSR

/* ===== Bobina ===== */
const bool ACTIVO_LOW = false;          // tu placa: ON con HIGH
const unsigned long TIEMPO_ON = 5000UL; // 5 s

/* ===== Detección por cambios ===== */
const unsigned long MIN_CHANGE_US = 15000UL; // antirrebote: 15 ms
const unsigned long VENTANA_MS    = 900UL;   // cierra moneda sin nuevos cambios

/* ===== Estado ===== */
unsigned long lastChangeUs = 0;
unsigned long lastEdgeMs   = 0;
int lastLevel;

volatile unsigned long cambios_acum   = 0; // acum hacia 10 cambios (para $5)
volatile unsigned int  cambios_moneda = 0; // cambios de la moneda actual

bool bobinaOn = false;
unsigned long tBobinaIni = 0;

unsigned long total_cents = 0;  // suma total del ciclo (reinicia con cada venta)

/* ===== UI (solo lo que pides) ===== */
void header(){
  lcd.setCursor(0,0); lcd.print("SERVICIO FRANCHUZ   ");
}
void splash(){
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(" Desarrollador      ");
  lcd.setCursor(0,1); lcd.print(" Juan Carlos Garcia ");
  lcd.setCursor(0,2); lcd.print(" Tel:      ");
  lcd.setCursor(0,3); lcd.print(" Iniciando...       ");
  delay(10000); // 10 s
}
void uiInicio(){
  lcd.clear(); header();
  lcd.setCursor(0,1); lcd.print("Inserte monedas     ");
  lcd.setCursor(0,2); lcd.print("Moneda: --          ");
  lcd.setCursor(0,3); lcd.print("Depositado: $0.00   ");
}
void uiMonedaDetectada(unsigned long centsMoneda){
  header();
  // Línea 2: Moneda detectada
  lcd.setCursor(0,2);
  char l2[21];
  snprintf(l2,sizeof(l2),"Moneda: $%lu.%02lu     ",
           (unsigned long)(centsMoneda/100),
           (unsigned long)(centsMoneda%100));
  lcd.print(l2);
  // Línea 3: Depositado (total del ciclo)
  lcd.setCursor(0,3);
  char l3[21];
  snprintf(l3,sizeof(l3),"Depositado: $%lu.%02lu ",
           (unsigned long)(total_cents/100),
           (unsigned long)(total_cents%100));
  lcd.print(l3);
}
void uiPaso(unsigned long restMs){
  lcd.clear(); header();
  lcd.setCursor(0,2); lcd.print("Tiempo restante:    ");
  char b[21];
  snprintf(b, sizeof(b), "%2lus                ",
           (unsigned long)((restMs+999)/1000));
  lcd.setCursor(0,3); lcd.print(b);
}

/* ===== Utilidades ===== */
inline void setBobina(bool on){
  pinMode(PIN_BOBINA, OUTPUT);
  if (ACTIVO_LOW) digitalWrite(PIN_BOBINA, on ? LOW : HIGH);
  else            digitalWrite(PIN_BOBINA, on ? HIGH : LOW);
}
void activarVend(){
  bobinaOn   = true;
  tBobinaIni = millis();
  setBobina(true);

  // reset de contadores del ciclo
  cambios_acum   = 0;
  cambios_moneda = 0;
  total_cents    = 0; // reinicia el total mostrado en LCD

  uiPaso(TIEMPO_ON); // sólo countdown
}

/* ===== Setup ===== */
void setup() {
  Wire.begin(); lcd.init(); lcd.backlight();
  Serial.begin(115200);

  if (USA_PULLUP) pinMode(PIN_PULSOS, INPUT_PULLUP);
  else            pinMode(PIN_PULSOS, INPUT);

  lastLevel = digitalRead(PIN_PULSOS);
  lastChangeUs = micros();
  lastEdgeMs   = millis();

  setBobina(false);

  splash();
  uiInicio();

  // Avisos por Serial (diagnóstico completo)
  Serial.println(F("Modo: cambios"));
  Serial.println(F("Reglas: moneda >=6 cambios => venta inmediata ($5)."));
  Serial.println(F("        monedas de 2/4 cambios se acumulan; a 10 cambios => venta."));
}

/* ===== Loop ===== */
void loop() {
  unsigned long nowUs = micros();
  unsigned long nowMs = millis();

  // Si está vendiendo, solo countdown
  if (bobinaOn){
    unsigned long trans = nowMs - tBobinaIni;
    if (trans >= TIEMPO_ON){
      setBobina(false);
      bobinaOn = false;
      uiInicio();
    } else {
      uiPaso(TIEMPO_ON - trans);
    }
    return;
  }

  // Detección de cualquier flanco con antirrebote
  int level = digitalRead(PIN_PULSOS);
  if (level != lastLevel){
    unsigned long dtus = nowUs - lastChangeUs;
    if (dtus >= MIN_CHANGE_US){
      lastChangeUs = nowUs;
      lastEdgeMs   = nowMs;
      cambios_moneda++;         // contamos cambios de esta moneda

      // Debug por Serial (pero NO en LCD)
      Serial.print(F("Moneda: cambio #")); Serial.print(cambios_moneda);
      Serial.print(F("  nivel=")); Serial.println(level==HIGH ? "HIGH" : "LOW");

      lastLevel = level;
    }
  }

  // Cierre de moneda por ventana
  if (cambios_moneda > 0 && (nowMs - lastEdgeMs) > VENTANA_MS){
    unsigned long centsMoneda = 0;

    if (cambios_moneda >= 6){
      // Moneda “grande” (~$5)
      centsMoneda = 500;
      total_cents = 500;     // para mostrar $5 en pantalla antes de reiniciar en venta
      Serial.println(F("Moneda detectada: >=6 cambios -> $5 (venta inmediata)"));
      activarVend();
      return;
    } else {
      // Moneda chica: 2 cambios = $1; 4 cambios = $2
      centsMoneda = (cambios_moneda/2) * 100UL;
      total_cents += centsMoneda;
      cambios_acum += cambios_moneda;

      Serial.print(F("Moneda cerrada: cambios=")); Serial.print(cambios_moneda);
      Serial.print(F("  valor=$")); Serial.println(centsMoneda/100);
      Serial.print(F("Acumulado cambios: ")); Serial.println(cambios_acum % 10);

      // Mostrar SOLO “Moneda” y “Depositado” en LCD
      uiMonedaDetectada(centsMoneda);

      if ((cambios_acum % 10) == 0){
        Serial.println(F("== 10 cambios alcanzados -> VENTA =="));
        activarVend();
        return;
      }

      cambios_moneda = 0; // listo para la siguiente moneda
    }
  }

  // refresco ligero del encabezado
  static unsigned long tUi=0;
  if (millis()-tUi > 300){ tUi=millis(); header(); }
}
