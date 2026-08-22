# IOT_Projekat

## Pokretanje

### Unapred potrebno:

- Python 3.x
- Node.js + npm
- .NET SDK
- MongoDB
- Angular CLI

### MQTT - Mosquitto

- Preuzeti Eclipse Mosquitto sa linka: https://mosquitto.org/download/
- Pokrenuti preuzeti .exe fajl
- Završiti instalaciju
- Pokrenuti PowerShell
- ``` cd "C:\Program Files\mosquitto" ```
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