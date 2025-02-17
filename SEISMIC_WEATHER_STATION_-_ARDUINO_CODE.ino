#include "DHT.h"
#include <SdFat.h>

// Definicja czujnika DHT11
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Definicja pinów dla karty SD
#define SD_CS_PIN 4
SdFat sd;
SdFile logFile;

// Definicja pinów dla diody i czujnika wstrząsu
int ledCzerwona = 8;
int shockPin = 10;
int shockVal = HIGH;

// Odczyt temperatury i wilgotności co 2 minuty
unsigned long lastDHTRead = 0;
unsigned long dhtInterval = 120000;

// Zmienne do śledzenia wstrząsów
unsigned long shockStartTime = 0;
int shockCount = 0;
unsigned long lastShockCheckTime = 0;
unsigned long shockCheckInterval = 60000; // Sprawdzanie co minutę

// Czas zakończenia pracy programu (30 minut)
unsigned long startTime;
unsigned long programDuration = 1800000; // 30 minut

// Plik do zapisu danych
char filename[15];

void setup() {
    Serial.begin(9600);
    Serial.println("Witamy w stacji pogodowej. Pomiary temp. i wilg. co 2 minuty.");

    pinMode(shockPin, INPUT);
    pinMode(ledCzerwona, OUTPUT);
    dht.begin();

    // Inicjalizacja karty SD
    if (!sd.begin(SD_CS_PIN, SD_SCK_MHZ(10))) {
        Serial.println("❌ Błąd inicjalizacji karty SD!");
        return;
    }

    Serial.println("✅ Karta SD wykryta. Rozpoczynam zapis danych...");

    // Tworzenie nowego pliku z unikalną nazwą
    for (int i = 1; i < 100; i++) {
        sprintf(filename, "DANE%02d.TXT", i);
        if (!sd.exists(filename)) {
            if (logFile.open(filename, O_WRONLY | O_CREAT)) {
                Serial.print("📁 Utworzono plik: ");
                Serial.println(filename);
                logFile.println("Czas, Wilgotność (%), Temperatura (C), Wstrząs");
                logFile.close();
                break;
            }
        }
    }

    // Ustawienie startowego czasu działania
    startTime = millis();
}

void loop() {
    if (millis() - startTime >= programDuration) {
        Serial.println("⏳ Zakończono pracę stacji po 30 minutach.");
        Serial.println("📖 Odczytuję dane zapisane na karcie SD...");

        // Wywołanie funkcji odczytującej dane
        odczytajDaneZPliku();

        while (true);
    }

    shockVal = digitalRead(shockPin);

    // Wykrycie wstrząsu
    if (shockVal == LOW) {
        if (shockStartTime == 0) {
            shockStartTime = millis();  // Rozpoczęcie pomiaru czasu trwania wstrząsu
        }

        // Zliczanie liczby wstrząsów w ciągu minuty
        if (millis() - lastShockCheckTime < shockCheckInterval) {
            shockCount++;
        }

        // Miganie diodą przez 5 sekund
        digitalWrite(ledCzerwona, HIGH);
        delay(250);
        digitalWrite(ledCzerwona, LOW);
        delay(250);
    }

    // Sprawdzanie liczby wstrząsów w ciągu minuty
    if (millis() - lastShockCheckTime >= shockCheckInterval) {
        lastShockCheckTime = millis();

        // Jeśli wystąpiły więcej niż 3 wstrząsy w ciągu ostatniej minuty, zapisujemy to jako "duży wstrząs"
        if (shockCount > 3) {
            Serial.println("!!! DUŻY WSTRZĄS - ZAGROŻENIE !!!");

            float wilgotnosc = dht.readHumidity();
            float temperatura = dht.readTemperature();
            unsigned long shockDuration = millis() - shockStartTime;

            if (isnan(wilgotnosc) || isnan(temperatura)) {
                Serial.println("Błąd odczytu DHT11!");
            } else {
                Serial.print("Czas trwania wstrząsu: ");
                Serial.print(shockDuration);
                Serial.print(" ms, Temperatura: ");
                Serial.print(temperatura);
                Serial.print(" *C, Wilgotność: ");
                Serial.print(wilgotnosc);
                Serial.println(" %");

                if (logFile.open(filename, O_WRONLY | O_APPEND)) {
                    logFile.print(millis());
                    logFile.print(", ");
                    logFile.print(wilgotnosc);
                    logFile.print(", ");
                    logFile.print(temperatura);
                    logFile.print(", DUŻY WSTRZĄS (");
                    logFile.print(shockDuration);
                    logFile.println(" ms)");
                    logFile.close();
                } else {
                    Serial.println("❌ Błąd zapisu na SD!");
                }
            }
        }

        // Resetowanie liczby wstrząsów po każdej minucie
        shockCount = 0;
    }

    // Odczyt temperatury i wilgotności co 2 minuty
    if (millis() - lastDHTRead >= dhtInterval) {
        lastDHTRead = millis();

        float wilgotnosc = dht.readHumidity();
        float temperatura = dht.readTemperature();

        if (isnan(wilgotnosc) || isnan(temperatura)) {
            Serial.println("Błąd odczytu DHT11!");
        } else {
            Serial.print("Wilgotność: ");
            Serial.print(wilgotnosc);
            Serial.print(" %\t");
            Serial.print("Temperatura: ");
            Serial.print(temperatura);
            Serial.println(" *C");

            if (logFile.open(filename, O_WRONLY | O_APPEND)) {
                logFile.print(millis());
                logFile.print(", ");
                logFile.print(wilgotnosc);
                logFile.print(", ");
                logFile.print(temperatura);
                logFile.println(", BRAK WSTRZĄSU");
                logFile.close();
            } else {
                Serial.println("❌ Błąd zapisu na SD!");
            }
        }
    }
}

// Funkcja odczytująca zapisane dane z karty SD
void odczytajDaneZPliku() {
    if (!sd.exists(filename)) {
        Serial.println("❌ Plik nie istnieje! Brak zapisanych danych.");
        return;
    }

    if (!logFile.open(filename, O_RDONLY)) {
        Serial.println("❌ Błąd otwierania pliku!");
        return;
    }

    Serial.print("📖 Dane z pliku ");
    Serial.println(filename);
    Serial.println("----------------------------------");

    char znak;
    while (logFile.available()) {
        znak = logFile.read();
        Serial.write(znak); // Wyświetlanie danych w Serial Monitorze
    }

    logFile.close();
    Serial.println("\n----------------------------------");
    Serial.println("📁 Koniec odczytu pliku.");
}
