#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ADDR 0x27       // Cambia a 0x3F si tu LCD usa esa dirección
LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);

const int pinBobina = 11;   // Pin que activa la bobina
const bool ACTIVO_LOW = false; // true si se activa con LOW, false si se activa con HIGH

// Tiempos (milisegundos)
const unsigned long TIEMPO_ON  = 10000;  // 15 segundos encendido
const unsigned long TIEMPO_OFF = 10000;  // 15 segundos apagado

void setup() {
  Wire.begin();
  lcd.init();
  lcd.backlight();

  pinMode(pinBobina, OUTPUT);
  if (ACTIVO_LOW)
    digitalWrite(pinBobina, HIGH); // Inicia apagado
  else
    digitalWrite(pinBobina, LOW);

  lcd.setCursor(0,0); lcd.print("SERVICIO HENRIQUEZ");
  lcd.setCursor(0,1); lcd.print("Prueba de bobina");
  lcd.setCursor(0,2); lcd.print("Pin D11");
  delay(2000);
}

void loop() {
  // --- Activar bobina ---
  if (ACTIVO_LOW)
    digitalWrite(pinBobina, LOW);
  else
    digitalWrite(pinBobina, HIGH);

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("SERVICIO HENRIQUEZ");
  lcd.setCursor(0,1); lcd.print("aqui prende");
  lcd.setCursor(0,2); lcd.print("Pin D11 ON (10s)");
  delay(TIEMPO_ON);

  // --- Desactivar bobina ---
  if (ACTIVO_LOW)
    digitalWrite(pinBobina, HIGH);
  else
    digitalWrite(pinBobina, LOW);

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("SERVICIO HENRIQUEZ");
  lcd.setCursor(0,1); lcd.print("BOBINA DESACTIVADA");
  lcd.setCursor(0,2); lcd.print("Pin D11 OFF (10s)");
  delay(TIEMPO_OFF);
}
