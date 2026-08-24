# IoT Environment Monitor

IoT sistem za praćenje uslova okruženja pomoću **STM32 NUCLEO-F401RE** razvojne ploče.

Sistem meri **temperaturu, relativnu vlažnost vazduha i intenzitet osvetljenja**, lokalno prikazuje podatke na OLED ekranu i signalizira trenutno stanje LED diodama. Izmereni podaci se preko serijske komunikacije prosleđuju Python aplikaciji, a zatim MQTT brokeru, Node-RED-u, backend aplikaciji, MongoDB bazi i web dashboardu.

## Arhitektura sistema

```text
DHT22 + LDR
     │
     ▼
STM32 NUCLEO-F401RE
     │
     ├── SSD1306 OLED
     ├── statusne LED diode
     │
     ▼
USART2 / USB Virtual COM Port
     │
     ▼
Python bridge
     │
     ▼
Mosquitto MQTT
     │
     ▼
Node-RED
     │
     ▼
ASP.NET Core Backend
     │
     ▼
MongoDB
     │
     ▼
Angular Dashboard
```

STM32 nije direktno povezan na mrežu. Ploča komunicira sa računarom preko USB-a, dok Python bridge predstavlja vezu između embedded dela sistema i MQTT infrastrukture.

## Funkcionalnosti

- merenje temperature i relativne vlažnosti pomoću **DHT22** senzora;
- merenje svetlosti pomoću **LDR fotootpornika i ADC-a**;
- periodično očitavanje senzora približno na svake 2 sekunde;
- određivanje stanja sistema: `GOOD`, `WARNING`, `BAD` ili `ERROR`;
- prikaz trenutnih vrednosti na **SSD1306 OLED** ekranu;
- LED indikacija trenutnog statusa;
- slanje podataka preko **USART2** u JSON formatu;
- Python bridge za serijsku komunikaciju i MQTT publish;
- dodavanje UTC timestamp-a pre slanja MQTT poruke;
- Node-RED flow za prijem MQTT poruka i prosleđivanje backendu;
- čuvanje istorijskih merenja u MongoDB bazi;
- web dashboard sa grafikonima temperature, vlažnosti i svetlosti;
- automatsko osvežavanje dashboarda;
- prikaz trenutnog statusa sistema;
- alarmni banner i popup za kritična stanja;
- pregled prethodnih upozorenja i alarma.

## Korišćene tehnologije

| Deo sistema | Tehnologije |
|---|---|
| Embedded | STM32 NUCLEO-F401RE, STM32CubeIDE, C, HAL |
| Senzori | DHT22, LDR |
| Lokalni prikaz | SSD1306 OLED, LED diode |
| Serijska komunikacija | USART2, 115200 baud |
| Bridge | Python, PySerial, Paho MQTT |
| MQTT | Eclipse Mosquitto |
| Obrada podataka | Node-RED |
| Backend | ASP.NET Core |
| Baza | MongoDB |
| Frontend | Angular, Chart.js, Lucide icons |

## Struktura projekta

```text
IoT-Environment-Monitor/
│
├── stm32/
│   ├── STM32F401-RE workflow/
│   ├── Python bridge/
│   │   ├── bridge.py
│   │   ├── serial-test.py
│   │   └── requirements.txt
│   └── README.md
│
├── backend/
│   └── IOTProjekat/
│
├── frontend/
│   └── dashboard/
│
├── flows.json
└── README.md
```

Detaljniji opis STM32 dela, pinova, periferija i implementacije nalazi se u [`stm32/README.md`](stm32/README.md).

---

# Pokretanje projekta

## Preduslovi

Pre pokretanja potrebno je instalirati:

- STM32CubeIDE
- Python 3.x
- Eclipse Mosquitto
- Node.js + npm
- Node-RED
- .NET SDK
- MongoDB
- Angular CLI

## 1. MongoDB

MongoDB treba da bude pokrenut lokalno na:

```text
mongodb://localhost:27017
```

Backend koristi bazu:

```text
SmartGarden
```

Ako je MongoDB instaliran kao Windows servis, dovoljno je proveriti da servis radi.

## 2. Mosquitto MQTT broker

Otvoriti terminal u direktorijumu u kome je Mosquitto instaliran.

Na primer:

```powershell
cd "C:\Program Files\mosquitto"
```

Pokrenuti broker:

```powershell
.\mosquitto.exe -v
```

Ako je Mosquitto već pokrenut kao Windows servis, nije potrebno ponovo pokretati broker.

Za proveru MQTT poruka opciono se može otvoriti dodatni terminal:

```powershell
.\mosquitto_sub.exe -h localhost -t "iot/environment" -v
```

Koristi se MQTT topic:

```text
iot/environment
```

## 3. Node-RED

Ako Node-RED nije instaliran:

```powershell
npm install -g node-red
```

Pokrenuti ga komandom:

```powershell
node-red
```

Editor je dostupan na:

```text
http://localhost:1880
```

Importovati `flows.json` iz root direktorijuma projekta i kliknuti **Deploy**.

Node-RED prima podatke sa MQTT topic-a `iot/environment` i prosleđuje ih backendu.

## 4. Backend

Iz root direktorijuma projekta:

```powershell
cd backend\IOTProjekat
```

Prvi put:

```powershell
dotnet restore
```

Pokretanje:

```powershell
dotnet run
```

Backend je dostupan na:

```text
http://localhost:5053
```

Glavni endpoint-i su:

```text
POST /api/Sensor
GET  /api/Sensor/all
GET  /api/Sensor/latest
```

## 5. STM32

Otvoriti STM32 projekat iz:

```text
stm32/STM32F401-RE workflow/
```

u STM32CubeIDE.

Povezati NUCLEO-F401RE ploču preko USB-a, buildovati projekat i programirati mikrokontroler.

STM32 preko USART2 šalje poruke u formatu:

```json
{
  "temperature": 31.8,
  "humidity": 47.9,
  "light": 28,
  "status": "WARNING"
}
```

Serijska komunikacija koristi:

```text
115200 baud
8 data bits
no parity
1 stop bit
```

## 6. Python bridge

Pre pokretanja bridge-a treba zatvoriti PuTTY ili bilo koji drugi program koji koristi isti COM port.

Preći u:

```powershell
cd "stm32\Python bridge"
```

Instalirati dependencies:

```powershell
pip install -r requirements.txt
```

U `bridge.py` proveriti da `SERIAL_PORT` odgovara COM portu STM32 ploče:

```python
SERIAL_PORT = "COM3"
```

Pokrenuti bridge:

```powershell
python bridge.py
```

Bridge:

1. čita JSON poruke sa STM32;
2. proverava da li postoje potrebna polja;
3. dodaje UTC timestamp;
4. objavljuje podatke na MQTT topic `iot/environment`.

MQTT poruka tada izgleda približno ovako:

```json
{
  "temperature": 31.8,
  "humidity": 47.9,
  "light": 28,
  "status": "WARNING",
  "timestamp": "2026-08-22T18:29:28.834811+00:00"
}
```

Za proveru samo STM32 → Python komunikacije može se koristiti:

```powershell
python serial-test.py
```

## 7. Frontend

Preći u:

```powershell
cd frontend\dashboard
```

Prvi put instalirati dependencies:

```powershell
npm install
```

Pokrenuti aplikaciju:

```powershell
ng serve
```

Dashboard je dostupan na:

```text
http://localhost:4200
```

Dashboard prikazuje:

- trenutnu temperaturu;
- trenutnu vlažnost;
- trenutni nivo svetlosti;
- istorijske grafikone za sva tri parametra;
- trenutni sistemski status;
- alarmni banner;
- popup za `BAD` i `ERROR` stanje;
- log prethodnih `WARNING`, `BAD` i `ERROR` merenja.

Podaci na dashboardu se automatski osvežavaju približno na svake **4 sekunde**.

---

## Portovi

| Servis | Adresa |
|---|---|
| Mosquitto MQTT | `localhost:1883` |
| Node-RED | `http://localhost:1880` |
| Backend | `http://localhost:5053` |
| Frontend | `http://localhost:4200` |
| MongoDB | `localhost:27017` |

## Alarmni sistem

STM32 na osnovu izmerenih vrednosti određuje trenutno stanje:

```text
GOOD
WARNING
BAD
ERROR
```

Status se zajedno sa ostalim podacima prosleđuje kroz ceo sistem i čuva u bazi.

Frontend koristi status za:

- prikaz trenutnog stanja sistema;
- promenu izgleda statusnog bannera;
- prikaz popup upozorenja kada sistem pređe u `BAD` ili `ERROR`;
- formiranje istorije prethodnih upozorenja i alarma.

Time je moguće uživo demonstrirati promenu stanja sistema promenom temperature, vlažnosti ili količine svetlosti.

## Tok jednog merenja

```text
Senzori
   ↓
STM32
   ↓
lokalni OLED + LED status
   ↓
JSON preko USART2
   ↓
Python bridge
   ↓
MQTT / Mosquitto
   ↓
Node-RED
   ↓
ASP.NET Core API
   ↓
MongoDB
   ↓
Angular dashboard
```
