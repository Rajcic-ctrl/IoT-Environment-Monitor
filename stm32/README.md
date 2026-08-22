# IoT Environment Monitoring System

Studentski IoT projekat za merenje i praćenje uslova okruženja pomoću **STM32 NUCLEO-F401RE** razvojne ploče. Sistem meri temperaturu, relativnu vlažnost vazduha i intenzitet osvetljenja, lokalno prikazuje podatke na OLED ekranu, signalizira stanje pomoću LED dioda i šalje rezultate računaru preko serijske komunikacije. Python bridge zatim može da prosledi podatke MQTT brokeru radi daljeg rada sa Node-RED-om, bazom podataka i web dashboardom.

## Pregled sistema

Tok podataka u projektu je:

```text
DHT22 + LDR
     │
     ▼
STM32 NUCLEO-F401RE
     │
     ├── OLED SSD1306          lokalni prikaz
     ├── 3 statusne LED       GOOD / WARNING / BAD
     │
     ▼
USART2 / USB Virtual COM Port
     │
     ▼
Python bridge
     │
     ▼
Mosquitto MQTT broker
     │
     ▼
Node-RED / baza / web dashboard
```

STM32 i senzori ne moraju direktno da budu povezani na mrežu. Ploča je USB kablom povezana sa računarom, a Python aplikacija na računaru predstavlja vezu između embedded dela i MQTT infrastrukture.

---

## Funkcionalnosti

- merenje **temperature** i **relativne vlažnosti** pomoću DHT22 senzora;
- merenje **intenziteta svetlosti** pomoću LDR fotootpornika i ADC-a;
- pretvaranje sirove ADC vrednosti svetlosti u kalibrisanu vrednost `0–100%`;
- periodično novo merenje približno na svake **2 sekunde**;
- izračunavanje ukupnog stanja sistema: `GOOD`, `WARNING`, `BAD` ili `ERROR`;
- prikaz temperature, vlažnosti, svetlosti i statusa na **SSD1306 OLED** ekranu;
- fizička indikacija statusa pomoću tri LED diode;
- slanje podataka u JSON formatu preko **USART2**;
- Python test skripta za proveru serijske komunikacije;
- Python bridge za parsiranje podataka, dodavanje UTC timestamp-a i MQTT publish.

---

## Hardver

Glavne komponente:

| Komponenta | Uloga |
|---|---|
| STM32 NUCLEO-F401RE | centralni mikrokontroler |
| DHT22 | temperatura i relativna vlažnost |
| LDR fotootpornik | merenje intenziteta svetlosti |
| 10 kΩ otpornik | naponski delilnik za LDR |
| SSD1306 128×64 OLED | lokalni prikaz podataka |
| 3 LED diode | statusna indikacija |
| Otpornici za LED diode | ograničenje struje |
| USB kabl | napajanje, programiranje i serijska komunikacija sa računarom |

### Povezivanje

| Signal / komponenta | STM32 pin |
|---|---|
| LDR analogni izlaz | `PA0 / ADC1_IN0` |
| DHT22 DATA | `PA8 / TIM1_CH1` |
| USART2 TX | `PA2` |
| USART2 RX | `PA3` |
| OLED SCL | `PB6 / I2C1_SCL` |
| OLED SDA | `PB7 / I2C1_SDA` |
| LED izlazi | `PB3`, `PB4`, `PB5` |

LDR je povezan kao naponski delilnik:

```text
3.3 V ── LDR ──●── 10 kΩ ── GND
               │
              PA0
```

Kod ovog povezivanja veća količina svetlosti daje veću ADC vrednost.

---

## STM32 konfiguracija

Projekat je napravljen u **STM32CubeIDE**, a konfiguracija periferija se nalazi u fajlu:

```text
TemperatureHumidityLightSensors.ioc
```

Najvažnije periferije su:

| Periferija | Namena | Konfiguracija |
|---|---|---|
| ADC1 | LDR | Channel 0, 12-bit, interrupt |
| TIM1 | DHT22 timing i Input Capture | prescaler `83`, CH1 na PA8 |
| TIM2 | periodično pokretanje merenja | prescaler `8399`, period `19999` |
| I2C1 | OLED SSD1306 | 100 kHz |
| USART2 | slanje telemetrije računaru | 115200 baud |

TIM1 radi sa taktom od 84 MHz i prescalerom 83, pa jedan timer tick predstavlja približno **1 µs**. To omogućava merenje kratkih impulsa koje šalje DHT22.

TIM2 se koristi kao periodični scheduler. Njegov interrupt ne izvršava merenja direktno, već samo postavlja flag kojim se u glavnoj petlji pokreće novi ciklus merenja.

---

## Non-blocking pristup

Jedan od ciljeva implementacije bio je da se izbegne blokiranje glavne petlje tokom komunikacije sa periferijama.

Zbog toga se u korisničkom kodu ne koristi klasičan pristup sa dugim `HAL_Delay()` pozivima ili aktivnim čekanjem da se periferija završi. Umesto toga koriste se:

- ADC interrupt za LDR;
- TIM1 Input Capture interrupt za DHT22;
- I2C interrupt za OLED;
- UART transmit interrupt za telemetriju;
- state machine za vremenski osetljivu DHT22 komunikaciju;
- state machine za inicijalizaciju i slanje OLED framebuffer-a.

Glavna petlja zato ostaje slobodna da obrađuje više delova sistema:

```c
while (1)
{
    DHT22_Process();
    OLED_Process();

    /* pokretanje merenja i obrada pristiglih rezultata */
}
```

---

## Organizacija STM32 koda

Najvažniji moduli nalaze se u `Core/Inc` i `Core/Src` direktorijumima.

### `light_sensor.c/.h`

Modul za LDR senzor.

- pokreće ADC konverziju pomoću `HAL_ADC_Start_IT()`;
- rezultat preuzima u ADC callback-u;
- čuva sirovu 12-bitnu ADC vrednost;
- pretvara rezultat u procenat osvetljenja.

Kalibracija koja se trenutno koristi:

```c
#define LIGHT_RAW_MIN 40
#define LIGHT_RAW_MAX 3200
```

Vrednosti ispod minimuma se tretiraju kao `0%`, a vrednosti iznad maksimuma kao `100%`.

### `dht22.c/.h`

DHT22 komunikacija je implementirana kao state machine:

```text
IDLE
  ↓
START_LOW
  ↓
START_HIGH
  ↓
RECEIVING
  ↓
COMPLETE / ERROR
```

STM32 najpre preuzima PA8 kao GPIO output i generiše početni signal senzoru. Nakon toga se PA8 prebacuje na `TIM1_CH1` Input Capture. Trajanje HIGH impulsa određuje da li je primljeni bit `0` ili `1`.

DHT22 šalje ukupno 40 bitova:

```text
16 bita humidity
16 bita temperature
8 bita checksum
```

Checksum se proverava pre nego što rezultat bude označen kao validan.

### `sensor_data.h`

Zajednička struktura za trenutna merenja:

```c
typedef struct
{
    float temperature;
    float humidity;
    uint16_t lightRaw;
    uint8_t lightPercent;
    bool dhtValid;
    bool lightValid;
} SensorData;
```

Definisani su i sistemski statusi:

```c
SYSTEM_STATUS_GOOD
SYSTEM_STATUS_WARNING
SYSTEM_STATUS_BAD
SYSTEM_STATUS_ERROR
```

### `status.c/.h`

Status se određuje na osnovu temperature, vlažnosti i svetlosti. Ukupni rezultat odgovara **najkritičnijem** pojedinačnom stanju.

Trenutni pragovi su:

| Parametar | GOOD | WARNING | BAD |
|---|---|---|---|
| Temperatura | 18–30 °C | 15–<18 °C ili >30–33 °C | <15 °C ili >33 °C |
| Vlažnost | 40–70% | 30–<40% ili >70–80% | <30% ili >80% |
| Svetlost | 21–90% | 6–20% ili 91–100% | 0–5% |

Ako podaci senzora nisu validni, status je `ERROR`.

Ovi pragovi predstavljaju pravila korišćena za demonstraciju sistema i mogu jednostavno da se promene u `status.c`.

### `status_led.c/.h`

Na osnovu izračunatog `SystemStatus` uključuje se odgovarajuća LED dioda.

U trenutnom hardverskom povezivanju projekta:

```text
PB4 → GOOD
PB5 → WARNING
PB3 → BAD / ERROR
```

Boja zavisi od fizički povezane LED diode na odgovarajućem pinu.

### `oled.c/.h`

SSD1306 OLED koristi I2C1 i adresu `0x3C`.

Ekran prikazuje:

```text
TEMP:   xx.x C
HUM:    xx.x %
LIGHT:  xxx %
STATUS: GOOD/WARNING/BAD/ERROR
```

Modul koristi framebuffer veličine 1024 bajta za ekran rezolucije 128×64. Podaci se šalju preko `HAL_I2C_Master_Transmit_IT()` u manjim blokovima, pa slanje ekrana ne blokira ostatak aplikacije.

### `telemetry.c/.h`

Nakon završenog ciklusa merenja rezultati se formiraju kao JSON i šalju preko USART2 pomoću `HAL_UART_Transmit_IT()`.

Primer izlaza:

```json
{"temperature":25.4,"humidity":52.1,"light":27,"status":"GOOD"}
```

Svaka poruka završava sa `\r\n`, odnosno jedan JSON objekat predstavlja jednu liniju serijskog toka.

---

## Ciklus merenja

TIM2 približno na svake dve sekunde postavlja `measurementDue` flag. Glavna petlja tada pokreće oba senzora.

```text
TIM2 interrupt
      │
      ▼
measurementDue = true
      │
      ▼
LDR measurement + DHT22 measurement
      │                 │
      ▼                 ▼
ADC interrupt      TIM1 capture interrupt
      │                 │
      └────────┬────────┘
               ▼
           SensorData
               │
               ▼
        Status_Evaluate()
               │
        ┌──────┼──────┐
        ▼      ▼      ▼
       LED    OLED   USART2
```

Status, OLED i UART se ažuriraju tek kada su oba merenja za trenutni ciklus završena.

---

# Python bridge

Python deo se nalazi u direktorijumu:

```text
Python bridge/
```

Sadrži:

```text
bridge.py
serial-test.py
requirements.txt
```

## Instalacija zavisnosti

Potrebni paketi su navedeni u `requirements.txt`:

```text
pyserial
paho-mqtt
```

Instalacija:

```bash
pip install -r requirements.txt
```

---

## `serial-test.py`

Ova skripta služi za proveru komunikacije između STM32-a i računara **bez MQTT brokera**.

Podrazumevana konfiguracija je:

```python
SERIAL_PORT = "COM3"
BAUD_RATE = 115200
```

COM port može biti drugačiji na drugom računaru i tada je potrebno promeniti `SERIAL_PORT`.

Pokretanje:

```bash
python serial-test.py
```

Primer ispisa:

```text
Connected to COM3
RAW: {"temperature":25.4,"humidity":52.1,"light":27,"status":"GOOD"}
Temperature: 25.4 C | Humidity: 52.1% | Light: 27% | Status: GOOD
```

PuTTY ili drugi serial terminal mora biti zatvoren dok Python koristi isti COM port.

---

## `bridge.py`

`bridge.py` povezuje serijski STM32 izlaz sa MQTT brokerom.

Podrazumevane vrednosti u trenutnoj verziji su:

```python
SERIAL_PORT = "COM3"
BAUD_RATE = 115200

MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_TOPIC = "iot/environment"
```

Skripta:

1. čita jednu serijsku liniju;
2. pokušava da je parsira kao JSON;
3. proverava da li postoje sva obavezna polja;
4. dodaje UTC timestamp;
5. objavljuje rezultat na MQTT topic.

Obavezna polja su:

```text
temperature
humidity
light
status
```

Ako nema podataka na COM portu, skripta čeka sledeću poruku. Ako je JSON neispravan ili nedostaje neko obavezno polje, taj paket se preskače i ne prosleđuje MQTT-u.

Python dodaje timestamp zato što STM32 projekat nema RTC i nema pouzdan izvor stvarnog datuma i vremena.

Primer MQTT payload-a:

```json
{
  "temperature": 25.4,
  "humidity": 52.1,
  "light": 27,
  "status": "GOOD",
  "timestamp": "2026-08-21T20:32:15.482731+00:00"
}
```

Timestamp je u UTC vremenskoj zoni i pogodan je za čuvanje podataka i kasniji prikaz istorije na grafikonima.

Pokretanje bridge-a:

```bash
python bridge.py
```

Za podrazumevanu vrednost `MQTT_BROKER = "localhost"` MQTT broker mora biti pokrenut na istom računaru na portu `1883`.

---

## Serijski interfejs za integraciju

Ako se STM32 deo koristi nezavisno od priloženog Python bridge-a, drugi program može da čita podatke preko sledećeg interfejsa:

```text
Baud rate:    115200
Data bits:    8
Parity:       None
Stop bits:    1
Flow control: None
Format:       jedan JSON objekat po liniji
Period:       približno 2 s
```

Primer:

```json
{"temperature":33.3,"humidity":49.8,"light":23,"status":"BAD"}
```

### Tipovi podataka

| Polje | Tip | Opis |
|---|---|---|
| `temperature` | number | temperatura u °C |
| `humidity` | number | relativna vlažnost u % |
| `light` | integer | relativno osvetljenje 0–100% |
| `status` | string | `GOOD`, `WARNING`, `BAD` ili `ERROR` |

> `light` nije vrednost u luksima. To je relativna vrednost dobijena kalibracijom konkretnog LDR senzora u ovom projektu.

---

## Testiranje

Za osnovnu proveru kompletnog embedded dela preporučen je sledeći redosled:

1. Povezati NUCLEO-F401RE preko USB-a.
2. Buildovati i flashovati STM32 projekat iz STM32CubeIDE-a.
3. Proveriti da OLED prikazuje trenutne vrednosti.
4. Promeniti osvetljenje LDR-a i proveriti promenu `LIGHT` vrednosti i statusne LED diode.
5. Otvoriti odgovarajući Virtual COM Port sa 115200 8N1 i proveriti JSON izlaz.
6. Zatvoriti serial terminal i pokrenuti `serial-test.py`.
7. Kada se koristi MQTT, pokrenuti broker i zatim `bridge.py`.

Za demonstraciju promene svetlosti LDR se može pokriti ili osvetliti lampom. Sistem će novi status izračunati u sledećem ciklusu merenja, približno u roku od dve sekunde.

---

## Struktura repozitorijuma

```text
IoT Environment Monitor/
│
├── Python bridge/
│   ├── bridge.py
│   ├── serial-test.py
│   └── requirements.txt
│
└── STM32F401-RE workflow/
    └── TemperatureHumidityLightSensors/
        ├── Core/
        │   ├── Inc/
        │   └── Src/
        ├── Drivers/
        ├── TemperatureHumidityLightSensors.ioc
        └── ...
```

STM32CubeIDE automatski generisani fajlovi i HAL/CMSIS biblioteke nalaze se zajedno sa korisničkim modulima unutar STM32 projekta.

---

## Dalja integracija

Repozitorijum trenutno pokriva embedded deo i Python MQTT bridge. Nakon MQTT publish-a podaci mogu dalje da se obrađuju nezavisno od STM32 implementacije.

Planirani kompletan IoT tok je:

```text
STM32
  → UART/USB
Python bridge
  → MQTT
Mosquitto
  → Node-RED
baza podataka
  → web dashboard / grafikoni
```

Na taj način je embedded deo odvojen od prezentacionog i serverskog dela sistema: STM32 je odgovoran za senzore, lokalni status i pouzdano slanje merenja, dok se mrežna obrada, čuvanje istorije i vizualizacija obavljaju na računaru/serveru.

---

## Napomena

Projekat je razvijen kao studentski IoT sistem sa naglaskom na povezivanje više nivoa jednog IoT rešenja: **senzori → mikrokontroler → komunikacija → MQTT → obrada i vizualizacija**. Pragovi statusa i LDR kalibracija prilagođeni su trenutnom hardverskom prototipu i mogu se menjati bez promene osnovne arhitekture sistema.
