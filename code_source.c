#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h>

// --- Matériel ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo monServo;
const int PIN_SERVO = 13;

// --- Clavier ---
const byte LIGNES = 4; 
const byte COLONNES = 4; 
char keys[LIGNES][COLONNES] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte pinLignes[LIGNES] = {19, 18, 5, 17}; 
byte pinColonnes[COLONNES] = {16, 4, 0, 2}; 
Keypad clavier = Keypad(makeKeymap(keys), pinLignes, pinColonnes, LIGNES, COLONNES);

// --- Sécurité ---
String codeSaisi = "";
const uint32_t HASH_SECRET = 4257489661; 

// Variables Anti-Brute Force
int tentatives = 0;
bool estBloque = false;
unsigned long debutBlocage = 0;
const unsigned long DUREE_BLOCAGE_MS = 10000UL;

// Algo Hachage FNV-1a
uint32_t calculerHash(String input) {
  uint32_t hash = 2166136261;
  for (int i = 0; i < input.length(); i++) {
    hash ^= input[i];
    hash *= 16777619;
  }
  return hash;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  monServo.attach(PIN_SERVO);
  monServo.write(0);
  resetEcran();
}

void loop() {
  // 1. Vérification du blocage (Anti-Brute Force)
  if (estBloque) {
    if (millis() - debutBlocage >= DUREE_BLOCAGE_MS) {
      estBloque = false;
      tentatives = 0;
      resetEcran();
    }
    return; // On ignore les touches tant que c'est bloqué
  }

  // 2. Gestion Clavier
  char touche = clavier.getKey();
  if (touche) {
    codeSaisi += touche;
    lcd.setCursor(0, 1);
    lcd.print(codeSaisi); // Affiche ce qu'on tape

    if (codeSaisi.length() == 4) {
      verifierSecurite();
    }
  }
  delay(10);
}

void verifierSecurite() {
  uint32_t hashCalcule = calculerHash(codeSaisi);

  // Log Terminal (Audit sans code PIN en clair)
  Serial.print("Hash: "); Serial.print(hashCalcule);
  Serial.println(hashCalcule == HASH_SECRET ? " | Status: VALID" : " | Status: INVALID");

  lcd.clear();
  lcd.setCursor(0, 0);

  if (hashCalcule == HASH_SECRET) {
      // SUCCÈS
      lcd.print("HASH VALIDE");
      lcd.setCursor(0, 1);
      lcd.print("ACCES AUTORISE");
      monServo.write(90);
      tentatives = 0; // Reset compteur
      delay(3000);
  } else {
      // ÉCHEC
      tentatives++;
      lcd.print("HASH INVALIDE");
      lcd.setCursor(0, 1);
      lcd.print("Essai " + String(tentatives) + "/3");
      monServo.write(0);
      delay(2000);

      // Activation Blocage si 3 erreurs
      if (tentatives >= 3) {
        estBloque = true;
        debutBlocage = millis(); // Bloque 10s
        lcd.clear();
        lcd.print("SYSTEME BLOQUE");
        lcd.setCursor(0, 1);
        lcd.print("Attendre 10s...");
      }
  }

  if (!estBloque) {
    monServo.write(0);
    codeSaisi = "";
    resetEcran();
  } else {
    codeSaisi = ""; // On vide juste le code
  }
}

void resetEcran() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Systeme Secure");
  lcd.setCursor(0, 1);
  lcd.print("Entrez Code:");
}
