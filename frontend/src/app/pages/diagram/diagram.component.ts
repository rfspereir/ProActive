import { Component, Inject, PLATFORM_ID, OnInit } from '@angular/core';
import { HeaderComponent } from '../../components/header/header.component';
import { FooterComponent } from '../../components/footer/footer.component';
import { Database, ref, set, get, onValue, query, orderByKey, limitToLast, startAt, endAt } from '@angular/fire/database';
import { CommonModule } from '@angular/common';
import { NgxEchartsDirective, provideEcharts } from 'ngx-echarts';
import { EChartsOption } from 'echarts';
import { isPlatformBrowser } from '@angular/common';
import { inject } from '@angular/core';
import { FormsModule } from '@angular/forms';

@Component({
  selector: 'app-diagram',
  standalone: true,
  imports: [HeaderComponent, FooterComponent, CommonModule, NgxEchartsDirective, FormsModule],
  templateUrl: './diagram.component.html',
  styleUrls: ['./diagram.component.css'],
  providers: [
    provideEcharts(),
  ]
})
export class DiagramComponent {
  temp: number = 0;
  temp_ambient: number = 0;
  umidade: number = 0;
  door_status: string = '';
  startDateTime: string = '';
  endDateTime: string = '';
  private database: Database = inject(Database);
  isBrowser: boolean;
  Option: EChartsOption = {};

  constructor(@Inject(PLATFORM_ID) private platformId: Object) {
    this.loadData();
    this.loadDataPorta();
    this.loadTemp();
    this.isBrowser = isPlatformBrowser(this.platformId);
  }

  ngOnInit(): void {
   
      const dbRef = ref(this.database, '/sensorData');
      const queryRef = query(dbRef, orderByKey());
      onValue(queryRef, (snapshot) => {
        const chartData: { temperature: number[], humidity: number[], timestamps: string[] } = {
          temperature: [],
          humidity: [],
          timestamps: []
        };

        snapshot.forEach((childSnapshot) => {
          const data = childSnapshot.val();
          chartData.temperature.push(data.temperature);
          chartData.humidity.push(data.humidity);
          chartData.timestamps.push(data.timestamp);
        });

        this.updateChart(chartData);
      });
  }

  increment(): void {
    this.temp++;
    this.saveData();
  }

  decrement(): void {
    this.temp--;
    this.saveData();
  }

  subir(): void {
    this.saveServoData('u');
  }

  descer(): void {
    this.saveServoData('d');
  }

  saveData(): void {
    const dbRef = ref(this.database, 'temperatura');
    set(dbRef, this.temp)
      .then(() => console.log('Dados salvos com sucesso:', this.temp))
      .catch(error => console.error('Erro ao salvar dados:', error));
  }

  saveServoData(value: string): void {
    const dbRef = ref(this.database, '/servo/');
    set(dbRef, value)
      .then(() => console.log(`Dados '${value}' salvos com sucesso`))
      .catch(error => console.error('Erro ao salvar dados:', error));
  }

  loadTemp(): void {
    const dbRef = ref(this.database, 'temperatura');
    get(dbRef)
      .then((snapshot) => {
        if (snapshot.exists()) {
          const data = snapshot.val();
          console.log('Dados consultados com sucesso:', data);
          this.temp = data;  // Atualiza a variável temp com os dados consultados, se necessário
        } else {
          console.log('Nenhum dado disponível');
        }
      })
      .catch(error => {
        console.error('Erro ao consultar dados:', error);
      });
  }


  loadData(): void {
    const dbRef = ref(this.database, '/sensorData');
    const queryRef = query(dbRef, orderByKey(), limitToLast(1));
    onValue(queryRef, (snapshot) => {
      snapshot.forEach((childSnapshot) => {
        const data = childSnapshot.val();
        if (data !== null) {
          this.umidade = data.humidity;
          this.temp_ambient = data.temperature;
          console.log('Últimos dados carregados com sucesso:', data);
        }
      });
    });
    
  }

  loadDataPorta(): void {
    const dbRef = ref(this.database, '/doorStatus');
    const queryRef = query(dbRef, orderByKey(), limitToLast(1));
    onValue(queryRef, (snapshot) => {
      snapshot.forEach((childSnapshot) => {
        const porta = childSnapshot.val();
        if (porta !== null) {
          this.door_status = porta.door_status;
        }
      });
    });
    
  }
  filterData(): void {

    if (this.startDateTime && this.endDateTime) {
      const dbRef = ref(this.database, '/sensorData');
      const queryRef = query(dbRef, orderByKey(), startAt(this.startDateTime), endAt(this.endDateTime));
      onValue(queryRef, (snapshot) => {
        const chartData: { temperature: number[], humidity: number[], timestamps: string[] } = {
          temperature: [],
          humidity: [],
          timestamps: []
        };

        snapshot.forEach((childSnapshot) => {
          const data = childSnapshot.val();
          chartData.temperature.push(data.temperature);
          chartData.humidity.push(data.humidity);
          chartData.timestamps.push(data.timestamp);
        });

        console.log('Dados filtrados com sucesso:', chartData.temperature);
        console.log('Dados filtrador com sucesso:', chartData.humidity);
        console.log('Dados filtrador com sucesso:', chartData.timestamps);

        this.updateChart(chartData);
      });
    } else {
      console.error('Por favor, insira ambas as datas.');
      console.log('startDate', this.startDateTime);
      console.log('endDate', this.endDateTime);
    }
  }

  updateChart(data: { temperature: number[], humidity: number[], timestamps: string[] }): void {
    this.Option = {
      title: {
        text: 'Temperatura e Umidade'
      },
      tooltip: {
        trigger: 'axis'
      },
      legend: {
        data: ['Temperatura', 'Umidade']
      },
      grid: {
        left: '3%',
        right: '4%',
        bottom: '3%',
        containLabel: true
      },
      xAxis: {
        type: 'category',
        boundaryGap: false,
        data: data.timestamps
      },
      yAxis: {
        type: 'value',
        min: 0.0,  // Valor mínimo do eixo y
        max: 100.0,  // Valor máximo do eixo y
        interval: 10.0  // Intervalo entre os valores no eixo y
      },
      series: [
        {
          name: 'Temperatura',
          type: 'line',
          data: data.temperature
          
        },
        {
          name: 'Umidade',
          type: 'line',
          data: data.humidity
          
        }
      ]
    };
    console.log('Dados carregados com sucesso:', data.temperature);
    console.log('Dados carregados com sucesso:', data.humidity);
  }
}