import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable } from 'rxjs';

export interface SensorData {
  id: string,
  light: number,
  temperature: number,
  airHumidity: number,
  status: string,
  time: string
};

@Injectable({
  providedIn: 'root',
})
export class ApiService {
  private apiUrl = `http://localhost:5053/api/sensor/all`;

  constructor(private http: HttpClient) {}

  getData(): Observable<SensorData[]> {
    return this.http.get<SensorData[]>(this.apiUrl);
  }
}