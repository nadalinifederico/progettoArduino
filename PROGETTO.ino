#include <Servo.h>    // Libreria per controllare il servo motore
#define PIN_SERVO 8   // Definisce il pin a cui è collegato il servo
#define DATA_PIN 12   // Pin per il registro a scorrimento
#define LATCH_PIN 11  // Pin di latch per il registro a scorrimento
#define CLOCK_PIN 5   // Pin di clock per il registro a scorrimento

Servo Servo;    // Creazione di un oggetto servo per controllarlo
const int triggerPort = 10;   // Pin per inviare impulsi al sensore ad ultrasuoni
const int echoPort = 9;       // Pin per ricevere impulsi riflessi dal sensore ad ultrasuoni
const int ledRosso = 3;       // Pin per il LED rosso, usato per segnalare ostacoli
const int ledVerde = 6;       // Pin per il LED verde, indica che il sensore non rileva oggetti
const int led = 2;            // Pin per il LED bianco, controllato da un fotoresistore
const int FAN_PIN = 7;        // Pin collegato alla ventola per spazzare via

/*
primo bit = led E
secondo bit = D
terzo bit = F
quarto bit = led A
quinto bit = led B
sesto bit = led C
settimo bit = led H
ottavo bit = led G
*/

byte numbers[] = {  // Definizione di un array che rappresenta i numeri sul display a 7 segmenti
  B11111100,  // 0
  B00001100,  // 1
  B11011001,  // 2
  B01011101,  // 3
  B00101101,  // 4
  B01110101,  // 5
  B11110101,  // 6
  B00011100,  // 7
  B11111101,  // 8
  B01111101   // 9
};

int n = 0;    // Variabile per il conto alla rovescia
int pos = 90; // Posizione iniziale del servo

unsigned long previousMillis = 0;               // Memorizza il tempo precedente per il conto alla rovescia
unsigned long fanStartMillis = 0;               // Memorizza il tempo di avvio della ventola
unsigned long t1, dt;                           // Variabili per la gestione dei lampeggi e del tempo
const unsigned long countdownInterval = 1000;   // Intervallo di aggiornamento del countdown (1 secondo)
const unsigned long fanDuration = 5000;         // Durata della ventola (5 secondi)
const int luminositaSoglia = 400;               // Soglia di luminosità per attivare il sistema
int tblink = 100;                               // Tempo di lampeggio del LED rosso
int statoled = LOW;                             // Stato attuale del LED rosso (acceso/spento)
int freq = 220;                                 // Frequenza iniziale del segnale acustico
bool fanActive = false;                         // Stato della ventola (attiva o inattiva)

void setup() {
  pinMode(DATA_PIN, OUTPUT);      // Imposta il pin dei dati come uscita
  pinMode(LATCH_PIN, OUTPUT);     // Imposta il pin di latch come uscita
  pinMode(CLOCK_PIN, OUTPUT);     // Imposta il pin di clock come uscita
  pinMode(FAN_PIN, OUTPUT);       // Configura il pin della ventola come uscita
  Servo.attach(PIN_SERVO);        // Associa il servo al pin definito
  Servo.write(pos);               // Imposta la posizione iniziale del servo
  pinMode(led, OUTPUT);           // Configura il LED bianco come uscita
  pinMode(triggerPort, OUTPUT);   // Configura il trigger del sensore come uscita
  pinMode(echoPort, INPUT);       // Configura l'eco del sensore come ingresso
  pinMode(ledRosso, OUTPUT);      // Configura il LED rosso come uscita
  pinMode(ledVerde, OUTPUT);      // Configura il LED verde come uscita
  Serial.begin(9600);             // Avvia la comunicazione seriale a 9600 baud
}

void loop() {
  int luminosita = analogRead(A0);      // Legge il valore di luminosità dal sensore fotoresistore

  if (luminosita > luminositaSoglia) {  // Se la luminosità è superiore alla soglia:
    Servo.write(pos);                   // Mantiene il servo nella posizione iniziale
    digitalWrite(led, LOW);             // Spegne il LED bianco
    digitalWrite(ledRosso, LOW);        // Spegne il LED rosso
    digitalWrite(ledVerde, HIGH);       // Accende il LED verde
  } else {
    manovraAutomatica();                // Se la luminosità è bassa, avvia la manovra automatica
  }
}


void manovraAutomatica() {
  digitalWrite(ledVerde, LOW);    // Spegne il LED verde
  digitalWrite(led, HIGH);        // Accende il LED bianco
  digitalWrite(ledRosso, HIGH);   // Accende il LED rosso

  for (int i = 45; i <= 135; i++) {   // Movimento del servo da 45° a 135°
    Servo.write(i);                   // Modifica la posizione del servo
    delay(20);                        // Breve pausa per permettere il movimento del servo
    manovraDistanza();                // Controlla la distanza
  }

  for (int i = 135; i >= 45; i--) { // Movimento inverso del servo da 135° a 45°
    Servo.write(i);
    delay(20);
    manovraDistanza();
  }
}


void manovraDistanza() {
  long distanza = misura();   // Chiama la funzione misura() per ottenere la distanza rilevata

  if (distanza < 5) {               // Se la distanza è inferiore a 5 cm:
    digitalWrite(ledRosso, HIGH);   // Accende il LED rosso
    tone(4, 100, 100);              // Emette un suono a 100 Hz per 100 ms
    startCountdown();               // Avvia il conto alla rovescia
  } else if (distanza > 30) {     // Se la distanza è superiore a 30 cm:
    n = 9;                        // Reset del conto alla rovescia
    writeRegister(numbers[n]);    // Scrive il valore 9 nel display a 7 segmenti
    digitalWrite(ledRosso, LOW);  // Spegne il LED rosso
    digitalWrite(ledVerde, HIGH); // Accende il LED verde
    noTone(4);                    // Disattiva il suono
  } else {                        // Se la distanza è compresa tra 5 cm e 30 cm:
    digitalWrite(ledVerde, LOW);              // Spegne il LED verde
    tblink = map(distanza, 5, 40, 30, 300);   // Mappa la distanza per determinare il tempo di lampeggio del LED rosso
    freq = map(distanza, 5, 40, 500, 220);    // Mappa la distanza per determinare la frequenza del suono
    dt = millis() - t1;                       // Calcola il tempo trascorso dall'ultima iterazione

    if (dt >= tblink) {         // Se il tempo trascorso supera il tempo di lampeggio calcolato:
      statoled = !statoled;     // Alterna lo stato del LED rosso
      t1 = millis();            // Aggiorna il tempo di riferimento

      if (statoled) {                   // Se il LED deve essere acceso:
        tone(4, freq, 100);             // Emette un suono con frequenza variabile
        digitalWrite(ledRosso, HIGH);   // Accende il LED rosso
        startCountdown();               // Avvia il conto alla rovescia
      } else {                        // Se il LED deve essere spento:
        noTone(4);                    // Spegne il suono
        digitalWrite(ledRosso, LOW);  // Spegne il LED rosso
      }
    }
  }
}


void startCountdown() {
  unsigned long currentMillis = millis();  // Ottiene il tempo corrente

  if (fanActive) {                                        // Se la ventola è attiva:
    if (currentMillis - fanStartMillis >= fanDuration) {  // Se la ventola è accesa da più di 5 secondi:
      digitalWrite(FAN_PIN, LOW);                         // Spegne la ventola
      fanActive = false;                                  // Aggiorna lo stato della ventola a "spenta"
    }
  } else {                                                      // Se la ventola è inattiva:
    if (currentMillis - previousMillis >= countdownInterval) {  // Se è trascorso 1 secondo dal precedente aggiornamento:
      previousMillis = currentMillis;                           // Aggiorna il tempo di riferimento
      n = (n - 1 + 10) % 10;                                    // Decrementa il conto alla rovescia

      writeRegister(numbers[n]);                                // Aggiorna il display con il nuovo valore del countdown

      if (n == 0) {                                             // Se il conto alla rovescia è arrivato a 0:
        digitalWrite(FAN_PIN, HIGH);                            // Accende la ventola
        fanActive = true;                                       // Aggiorna lo stato della ventola a "attiva"
        delay(5000);                                            // Mantiene la ventola accesa per 5 secondi
        digitalWrite(FAN_PIN, LOW);                             // Spegne la ventola
        fanActive = false;                                      // Aggiorna lo stato della ventola a "spenta"
        fanStartMillis = currentMillis;                         // Memorizza il momento di attivazione della ventola
      }
    }
  }
}


void writeRegister(byte value) {
  digitalWrite(LATCH_PIN, LOW);                     // Disabilita temporaneamente il latch
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, value);   // Invia il byte corrispondente al numero al registro a scorrimento
  digitalWrite(LATCH_PIN, HIGH);                    // Riabilita il latch per mostrare il nuovo numero
}


long misura() {
  digitalWrite(triggerPort, LOW);     // Imposta il trigger a basso per garantire un segnale pulito
  delayMicroseconds(2);               // Breve pausa per stabilizzazione
  digitalWrite(triggerPort, HIGH);    // Impulso alto per avviare la misurazione
  delayMicroseconds(10);              // Durata dell'impulso inviato al sensore
  digitalWrite(triggerPort, LOW);     // Imposta il trigger nuovamente a basso

  long durata = pulseIn(echoPort, HIGH);  // Misura il tempo impiegato dall'eco a tornare al sensore
  return 0.034 * durata / 2;              // Calcola la distanza in cm utilizzando la formula per il tempo di volo del suono
}
