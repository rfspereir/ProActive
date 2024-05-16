import { Component } from '@angular/core';
import { HeaderComponent } from '../../components/header/header.component';
import { FooterComponent } from '../../components/footer/footer.component';
import { Database, ref, set, onValue } from '@angular/fire/database';
import { inject } from '@angular/core';

@Component({
  selector: 'app-diagram',
  standalone: true,
  imports: [HeaderComponent, FooterComponent],
  templateUrl: './diagram.component.html',
  styleUrls: ['./diagram.component.css']
})
export class DiagramComponent {
  temp: number = 0;
  private database: Database = inject(Database);
temp_ambient: any;

  constructor() {
    this.loadData();
  }

  increment(): void {
    this.temp++;
    this.saveData();
  }

  decrement(): void {
    this.temp--;
    this.saveData();
  }

  saveData(): void {
    const dbRef = ref(this.database, 'temperatura');
    set(dbRef, this.temp)
      .then(() => console.log('Dados salvos com sucesso:', this.temp))
      .catch(error => console.error('Erro ao salvar dados:', error));
  }

  loadData(): void {
    const dbRef = ref(this.database, 'temperatura');
    onValue(dbRef, (snapshot) => {
      const data = snapshot.val();
      if (data !== null) {
        this.temp = data;
        console.log('Dados carregados com sucesso:', this.temp);
      }
    }, {
      onlyOnce: true // Para carregar dados apenas uma vez
    });
  }

  salvar(): void {
    this.saveData();
  }
}
