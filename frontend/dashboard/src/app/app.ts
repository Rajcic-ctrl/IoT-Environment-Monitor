import { Component, AfterViewInit, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import Chart from 'chart.js/auto';
import { ApiService, SensorData } from './services/api-service';

import {
  LucideActivity,
  LucideClock,
  LucideDroplets,
  LucideRefreshCw,
  LucideSun,
  LucideThermometer,
  LucideTriangleAlert,
  LucideX
} from '@lucide/angular';

@Component({
  selector: 'app-root',
  imports: [
    CommonModule,
    LucideActivity,
    LucideClock,
    LucideDroplets,
    LucideRefreshCw,
    LucideSun,
    LucideThermometer,
    LucideTriangleAlert,
    LucideX
  ],
  templateUrl: './app.html',
  styleUrl: './app.css'
})
export class App implements AfterViewInit {
  currentData = signal<SensorData | null>(null);
  alarmLogs = signal<SensorData[]>([]);
  showAlarmPopup = signal(false);
  lastUpdated = signal<Date | null>(null);

  private previousStatus = '';

  private chart1?: Chart;
  private chart2?: Chart;
  private chart3?: Chart;

  constructor(private apiService: ApiService) {}

  ngAfterViewInit(): void {
    this.loadData();

    setInterval(() => {
      this.loadData();
    }, 4000);
  }

  loadData(): void {
    this.apiService.getData().subscribe(data => {
      if (data.length === 0) return;

      data = [...data].sort(
        (a, b) => new Date(a.time).getTime() - new Date(b.time).getTime()
      );

      const latest = data[data.length - 1];

      this.currentData.set(latest);
      this.lastUpdated.set(new Date(latest.time));

      this.alarmLogs.set(
        [...data]
          .reverse()
          .filter(x => x.status !== 'GOOD')
          .slice(0, 8)
      );

      if (
        (latest.status === 'BAD' || latest.status === 'ERROR') &&
        latest.status !== this.previousStatus
      ) {
        this.showAlarmPopup.set(true);
      }

      if (latest.status === 'GOOD' || latest.status === 'WARNING') {
        this.showAlarmPopup.set(false);
      }

      this.previousStatus = latest.status;

      const chartData = data.slice(-12);

      const labels = chartData.map(x =>
        new Date(x.time).toLocaleTimeString()
      );

      const temperatures = chartData.map(x => x.temperature);
      const humidity = chartData.map(x => x.airHumidity);
      const light = chartData.map(x => x.light);

      if (!this.chart1) {
        this.chart1 = new Chart('chart1', {
          type: 'line',
          data: {
            labels: labels,
            datasets: [{
              label: 'Temperature',
              data: temperatures,
              tension: 0.4,
              borderWidth: 2,
              pointRadius: 0,
              borderColor: '#252b27',
              backgroundColor: 'rgba(37, 43, 39, 0.06)',
              fill: true
            }]
          },
          options: this.getChartOptions()
        });
      } else {
        this.chart1.data.labels = labels;
        this.chart1.data.datasets[0].data = temperatures;
        this.chart1.update();
      }

      if (!this.chart2) {
        this.chart2 = new Chart('chart2', {
          type: 'line',
          data: {
            labels: labels,
            datasets: [{
              label: 'Humidity',
              data: humidity,
              tension: 0.4,
              borderWidth: 2,
              pointRadius: 0,
              borderColor: '#56796e',
              backgroundColor: 'rgba(86, 121, 110, 0.08)',
              fill: true
            }]
          },
          options: this.getChartOptions()
        });
      } else {
        this.chart2.data.labels = labels;
        this.chart2.data.datasets[0].data = humidity;
        this.chart2.update();
      }

      if (!this.chart3) {
        this.chart3 = new Chart('chart3', {
          type: 'line',
          data: {
            labels: labels,
            datasets: [{
              label: 'Light',
              data: light,
              tension: 0.4,
              borderWidth: 2,
              pointRadius: 0,
              borderColor: '#b18534',
              backgroundColor: 'rgba(177, 133, 52, 0.08)',
              fill: true
            }]
          },
          options: this.getChartOptions()
        });
      } else {
        this.chart3.data.labels = labels;
        this.chart3.data.datasets[0].data = light;
        this.chart3.update();
      }
    });
  }

  getChartOptions(): any {
    return {
      responsive: true,
      maintainAspectRatio: false,
      plugins: {
        legend: {
          display: false
        }
      },
      scales: {
        x: {
          grid: {
            display: false
          },
          ticks: {
            maxTicksLimit: 5
          }
        },
        y: {
          grid: {
            color: '#edf0ee'
          }
        }
      }
    };
  }

  getStatusClass(): string {
    return this.currentData()?.status?.toLowerCase() || 'offline';
  }

  getStatusTitle(): string {
    switch (this.currentData()?.status) {
      case 'GOOD':
        return 'System normal';

      case 'WARNING':
        return 'Warning';

      case 'BAD':
        return 'Critical condition';

      case 'ERROR':
        return 'Sensor error';

      default:
        return 'No data';
    }
  }

  getStatusMessage(): string {
    switch (this.currentData()?.status) {
      case 'GOOD':
        return 'All environmental parameters are within the expected range.';

      case 'WARNING':
        return 'One or more environmental parameters are outside the optimal range.';

      case 'BAD':
        return 'One or more environmental parameters are outside the safe range.';

      case 'ERROR':
        return 'One or more sensor readings are currently unavailable.';

      default:
        return 'Waiting for sensor data.';
    }
  }

  closeAlarmPopup(): void {
    this.showAlarmPopup.set(false);
  }
}