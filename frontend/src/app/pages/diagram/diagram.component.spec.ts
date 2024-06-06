import { Component, Inject, PLATFORM_ID, OnInit } from '@angular/core';
import { HeaderComponent } from '../../components/header/header.component';
import { FooterComponent } from '../../components/footer/footer.component';
import { Database, ref, set, onValue } from '@angular/fire/database';
import { CommonModule } from '@angular/common';
import { NgxEchartsDirective, provideEcharts } from 'ngx-echarts';
import { EChartsOption } from 'echarts';
import { isPlatformBrowser } from '@angular/common';
import { inject } from '@angular/core';
@Component({
  selector: 'app-diagram',
  standalone: true,
  imports: [HeaderComponent, FooterComponent, CommonModule, NgxEchartsDirective],
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
  private database: Database = inject(Database);
  isBrowser: boolean;
  Option: EChartsOption = {};

  constructor(@Inject(PLATFORM_ID) private platformId: Object) {
    this.loadData();
    this.isBrowser = isPlatformBrowser(this.platformId);
  }

  ngOnInit(): void {
    if (this.isBrowser) {
      this.Option = {
        title: {
          text: 'Temperatura e Umidade Durante a Semana'
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
          data: ['Dom', 'Seg', 'Ter', 'Qua', 'Qui', 'Sex', 'Sab']
        },
        yAxis: {
          type: 'value'
        },
        series: [
          {
            name: 'Temperatura',
            type: 'line',
            stack: 'Total',
            data: [{value:120}, 132, 101, 134, 90, 230, 210]
          },
          {
            name: 'Umidade',
            type: 'line',
            stack: 'Total',
            data: [220, 182, 191, 234, 290, 330, 310]
          },
        ]
    
      }
      
    }
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
    
  }

  descer(): void {
    
  }

  saveData(): void {
    const dbRef = ref(this.database, 'temperatura');
    set(dbRef, this.temp)
      .then(() => console.log('Dados salvos com sucesso:', this.temp))
      .catch(error => console.error('Erro ao salvar dados:', error));
  }

  loadData(): void {
    const dbRef = ref(this.database, '/base/temperature');
    onValue(dbRef, (snapshot) => {
      const data = snapshot.val();
      if (data !== null) {
        this.temp_ambient = data;
        console.log('Dados carregados com sucesso:', this.temp_ambient);
      }
    }, {
      onlyOnce: true // Para carregar dados apenas uma vez
    });
  }
}
