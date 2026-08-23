# IOT_Projekat

## Pokretanje

### Unapred potrebno:

- STM32Cube IDE
- Python 3.x
- Node.js + npm
- .NET SDK
- MongoDB
- Angular CLI

### STM32CUBE IDE

- Otvoriti projekat u STM32CUBE IDE i pokrenuti projekat nakon priključenja odgovarajućeg hardvera
- Proveriti da li je sve povezano kako treba

### Python bridge

- ``` cd stm32\Python bridge ``` odnosno na lokaciju gde je instaliran
- instalirati dependency-je sa ```pip install -r requirements.txt ```
- pokrenuti skriptu sa ```python bridge.py```
 
### MQTT - Mosquitto

- Preuzeti Eclipse Mosquitto sa linka: https://mosquitto.org/download/
- Pokrenuti preuzeti .exe fajl
- Završiti instalaciju
- Pokrenuti PowerShell
- ``` cd "C:\Program Files\mosquitto," ``` odnosno na lokaciju gde je instaliran
- ``` .\mosquitto_sub.exe -h localhost -t "iot/environment" ```

### Node Red

- ``` npm install -g --unsafe-perm node-red ```
- ``` node-red ```

### Backend

- ``` cd backend ```
- ``` dotnet restore ```
- ``` dotnet run ```

### MongoDb baza
- Konekcioni string
"MongoDB": {
  "ConnectionString": "mongodb://localhost:27017",
  "DatabaseName":  "SmartGarden"
}, 

### Frontend
- ``` cd frontend/dashboard ```
- ``` npm install ```
- ``` ng serve ```

### Portovi

- Mosquitto: http://localhost:1883/
- Node-RED: http://localhost:1880/
- Backend: http://localhost:5053 https://localhost:7097
- Frontend: http://localhost:4200/
