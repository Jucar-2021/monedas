// === PRUEBA AUTOMÁTICA DE ACTIVACIÓN DE RELEVADOR / BOBINA ===
// Recorre los pines del Arduino Mega para identificar cuál activa el relevador a 110 V

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ADDR 0x27
LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);

const bool ACTIVO_LOW = false;   // Cambia a true si tu relevador se activa con LOW
const unsigned long TIEMPO_ACTIVO = 5000; // Tiempo de activación por pin (ms)
const unsigned long TIEMPO_PAUSA  = 2000; // Pausa entre cada pin (ms)

// --- Rango de pines a probar ---
const int PIN_INICIO = 22;   // Puedes cambiar a 2 si quieres incluir los pines digitales bajos
const int PIN_FIN     = 53;  // Hasta 53 en la Mega

void setup() {
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0); lcd.print("     TEST DE PINES    ");
  lcd.setCursor(0, 1); lcd.print("Test de Pines Salida");
  lcd.setCursor(0, 2); lcd.print("Relevador 110V AC");

  Serial.begin(115200);
  Serial.println(F("=== PRUEBA DE SALIDAS ==="));
  Serial.println(F("Buscando pin que activa el relevador..."));

  // Configura todos los pines como salida inicialmente apagados
  for (int p = PIN_INICIO; p <= PIN_FIN; p++) {
    pinMode(p, OUTPUT);
    if (ACTIVO_LOW) digitalWrite(p, HIGH);
    else            digitalWrite(p, LOW);
  }

  delay(3000);
}

void loop() {
  for (int p = PIN_INICIO; p <= PIN_FIN; p++) {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("SERVICIO FRANCHUZ");
    lcd.setCursor(0, 1); lcd.print("Probando pin: D");
    lcd.print(p);
    lcd.setCursor(0, 2); lcd.print("Tiempo: ");
    lcd.print(TIEMPO_ACTIVO / 1000);
    lcd.print(" s");

    Serial.print(F("Probando pin D"));
    Serial.println(p);

    // Activa el pin
    if (ACTIVO_LOW) digitalWrite(p, LOW);
    else            digitalWrite(p, HIGH);

    lcd.setCursor(0, 3); lcd.print("Estado: ACTIVADO   ");
    delay(TIEMPO_ACTIVO); // Espera con el pin activo

    // Desactiva el pin
    if (ACTIVO_LOW) digitalWrite(p, HIGH);
    else            digitalWrite(p, LOW);

    lcd.setCursor(0, 3); lcd.print("Estado: APAGADO    ");
    delay(TIEMPO_PAUSA); // Espera antes de cambiar al siguiente
  }

  // Cuando termina todo el recorrido, vuelve a empezar
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Ciclo completado");
  lcd.setCursor(0, 1); lcd.print("Repitiendo prueba");
  delay(3000);
}
