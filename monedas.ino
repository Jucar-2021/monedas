#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ADDR 0x27
LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);

/* ===== Pines ===== */
const int PIN_PULSOS = 15;        // Entrada (monedero)
const bool USA_PULLUP = true;     // true => INPUT_PULLUP para PIN_PULSOS
const int PIN_BOBINA  = 11;       // Salida a relé/SSR (110 VAC via relé/SSR)
const int PIN_MICRO   = 9;        // Microswitch N.O. para cancelar bobina
const int PIN_BTN     = 8;        // Botón para cambiar tiempo de bobina

/* ===== Lógica de bobina ===== */
const bool ACTIVO_LOW = false;    // tu placa: ON con HIGH

// tiempos disponibles (ms) y selección actual
const unsigned long TIEMPOS_MS[4] = { 5000UL, 10000UL, 15000UL, 20000UL };
uint8_t idxTiempo = 3;            // 0:5s, 1:10s, 2:15s, 3:20s (por defecto 20 s)
unsigned long tiempo_on_ms = TIEMPOS_MS[idxTiempo];

/* ===== Detección por cambios ===== */
const unsigned long MIN_CHANGE_US = 15000UL; // antirrebote: 15 ms
const unsigned long VENTANA_MS    = 900UL;   // cierra moneda sin nuevos cambios

/* ===== Estado ===== */
unsigned long lastChangeUs = 0;
unsigned long lastEdgeMs   = 0;
int lastLevel;

volatile unsigned long cambios_acum   = 0; // SOLO diagnóstico por Serial
volatile unsigned int  cambios_moneda = 0; // cambios de la moneda actual

bool bobinaOn = false;
unsigned long tBobinaIni = 0;

unsigned long total_cents = 0;  // suma total del ciclo (reinicia con cada venta)

/* ===== Micro N.O. con pullup: reposo=HIGH, pulsado=LOW ===== */
const bool MICRO_ACTIVO_ES_LOW = false; // pon true si tu micro está a GND con pullup
inline bool microActivo() {
  int v = digitalRead(PIN_MICRO);
  return MICRO_ACTIVO_ES_LOW ? (v == LOW) : (v == HIGH);
}

/* ===== Botón de configuración (D8) ===== */
const bool BTN_ACTIVO_ES_LOW = true;    // botón a GND con INPUT_PULLUP
const unsigned long DEBOUNCE_MS = 50;
int  lastBtn = HIGH;
unsigned long tBtnChange = 0;
bool mostrarCfg = false;
unsigned long tMostrarCfgHasta = 0;

inline bool btnPresionadoEdge() {
  int b = digitalRead(PIN_BTN);
  unsigned long now = millis();
  if (b != lastBtn && (now - tBtnChange) > DEBOUNCE_MS) {
    tBtnChange = now;
    lastBtn = b;
    bool activo = BTN_ACTIVO_ES_LOW ? (b == LOW) : (b == HIGH);
    if (activo) return true;   // flanco de PRESIONADO
  }
  return false;
}

/* ===== UI ===== */
void header(){ lcd.setCursor(0,0); lcd.print("SERVICIO FRANCHUZ   "); }
void splash(){
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(" Desarrollador      ");
  lcd.setCursor(0,1); lcd.print(" Juan Carlos Garcia ");
  lcd.setCursor(0,2); lcd.print(" Tel: 773126961     ");
  lcd.setCursor(0,3); lcd.print(" Iniciando...       ");
  delay(10000);
}
void uiInicio(){
  lcd.clear(); header();
  lcd.setCursor(0,1); lcd.print("Insertar monedas     ");
  lcd.setCursor(0,2); lcd.print("Moneda: --           ");
  lcd.setCursor(0,3); lcd.print("Depositado: $0.00    ");
}
void uiMonedaDetectada(unsigned long centsMoneda){
  header();
  lcd.setCursor(0,2);
  char l2[21];
  snprintf(l2,sizeof(l2),"Moneda: $%lu.%02lu     ",
           (unsigned long)(centsMoneda/100),
           (unsigned long)(centsMoneda%100));
  lcd.print(l2);

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

/* ===== Relé ===== */
inline void setBobina(bool on){
  pinMode(PIN_BOBINA, OUTPUT);
  if (ACTIVO_LOW) digitalWrite(PIN_BOBINA, on ? LOW : HIGH);
  else            digitalWrite(PIN_BOBINA, on ? HIGH : LOW);
}
void desactivarBobinaYReset(){
  setBobina(false);
  bobinaOn = false;
  total_cents    = 0;
  cambios_acum   = 0;
  cambios_moneda = 0;
  uiInicio();
}
void activarVend(){
  bobinaOn   = true;
  tBobinaIni = millis();
  setBobina(true);
  uiPaso(tiempo_on_ms);
}

/* ===== Setup ===== */
void setup() {
  Wire.begin(); lcd.init(); lcd.backlight();
  Serial.begin(115200);

  if (USA_PULLUP) pinMode(PIN_PULSOS, INPUT_PULLUP);
  else            pinMode(PIN_PULSOS, INPUT);

  pinMode(PIN_MICRO, INPUT_PULLUP);   // micro N.O. a GND
  pinMode(PIN_BTN,   INPUT_PULLUP);   // botón a GND

  lastBtn = digitalRead(PIN_BTN);

  lastLevel = digitalRead(PIN_PULSOS);
  lastChangeUs = micros();
  lastEdgeMs   = millis();

  setBobina(false);

  splash();
  uiInicio();

  Serial.println(F("== Sistema listo =="));
  Serial.print(F("Tiempo bobina inicial: "));
  Serial.print(tiempo_on_ms/1000);
  Serial.println(F(" s"));
}

/* ===== Loop ===== */
void loop() {
  unsigned long nowUs = micros();
  unsigned long nowMs = millis();

  /* ---- Manejo del botón D8 (solo si no está vendiendo) ---- */
  if (!bobinaOn && btnPresionadoEdge()){
    idxTiempo = (idxTiempo + 1) % 4;
    tiempo_on_ms = TIEMPOS_MS[idxTiempo];

    // Mensajes
    Serial.print(F("Tiempo de bobina configurado a "));
    Serial.print(tiempo_on_ms/1000);
    Serial.println(F(" s"));

    header();
    lcd.setCursor(0,3);
    lcd.print("Tiempo cfg: ");
    lcd.print((unsigned long)(tiempo_on_ms/1000));
    lcd.print("s         ");
    mostrarCfg = true;
    tMostrarCfgHasta = nowMs + 1200; // mostrar ~1.2 s
  }

  // limpiar mensaje temporal de configuración
  if (mostrarCfg && nowMs > tMostrarCfgHasta && !bobinaOn){
    mostrarCfg = false;
    // refrescar línea inferior con depositado actual
    lcd.setCursor(0,3);
    char l3[21];
    snprintf(l3,sizeof(l3),"Depositado: $%lu.%02lu ",
             (unsigned long)(total_cents/100),
             (unsigned long)(total_cents%100));
    lcd.print(l3);
  }

  /* ---- Venta activa ---- */
  if (bobinaOn){
    if (microActivo()){
      Serial.println(F("** MICRO ACTIVO -> Cancelando bobina **"));
      desactivarBobinaYReset();
      return;
    }
    unsigned long trans = nowMs - tBobinaIni;
    if (trans >= tiempo_on_ms){
      desactivarBobinaYReset();
    } else {
      uiPaso(tiempo_on_ms - trans);
    }
    return;
  }

  /* ---- Conteo de pasos / monedas ---- */
  int level = digitalRead(PIN_PULSOS);
  if (level != lastLevel){
    unsigned long dtus = nowUs - lastChangeUs;
    if (dtus >= MIN_CHANGE_US){
      lastChangeUs = nowUs;
      lastEdgeMs   = nowMs;
      cambios_moneda++;
      lastLevel = level;

      Serial.print(F("Cambio detectado. cambios_moneda="));
      Serial.println(cambios_moneda);
    }
  }

  if (cambios_moneda > 0 && (nowMs - lastEdgeMs) > VENTANA_MS){
    unsigned long centsMoneda = 0;
    if (cambios_moneda >= 6)  centsMoneda = 500;                  // $5
    else                      centsMoneda = (cambios_moneda/2) * 100UL; // $1 o $2

    total_cents += centsMoneda;
    cambios_acum += cambios_moneda;

    Serial.print(F("Moneda cerrada: cambios=")); Serial.print(cambios_moneda);
    Serial.print(F(" valor=$")); Serial.println(centsMoneda/100);
    Serial.print(F("Depositado: $")); Serial.println(total_cents/100.0, 2);

    uiMonedaDetectada(centsMoneda);

    if (total_cents >= 500){
      Serial.print(F("== Depositado >= $5.00 -> VENTA ("));
      Serial.print(tiempo_on_ms/1000);
      Serial.println(F(" s) =="));
      activarVend();
      return;
    }

    cambios_moneda = 0;
  }

  // refresco ligero del header
  static unsigned long tUi=0;
  if (millis()-tUi > 300){ tUi=millis(); header(); }
}
