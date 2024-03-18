import { Component } from '@angular/core';
import { HeaderComponent } from '../../components/header/header.component';
import { FooterComponent } from '../../components/footer/footer.component';

@Component({
  selector: 'app-diagram',
  standalone: true,
  imports: [HeaderComponent, FooterComponent],
  templateUrl: './diagram.component.html',
  styleUrl: './diagram.component.css'
})
export class DiagramComponent {
  temp: number = 0;

  constructor() { }

  increment(): void {
    this.temp++;
  }

  decrement(): void {
    this.temp--;
  }

  salvar(): void {
    // Aqui você pode adicionar lógica para salvar o número
    console.log('Número salvo:', this.temp);
  }

}
