import { Component } from '@angular/core';
import { Router, RouterOutlet } from '@angular/router';
import { FooterComponent } from '../../components/footer/footer.component';
import { DiagramComponent } from '../diagram/diagram.component';

@Component({
  selector: 'app-home',
  standalone: true,
  imports: [RouterOutlet, 
    FooterComponent, 
    DiagramComponent],
  templateUrl: './home.component.html',
  styleUrl: './home.component.css'
})
export class HomeComponent {
  constructor (private router: Router) { }

  open(): void {
    const codigoInput = document.getElementsByName('codigo')[0] as HTMLInputElement;
    const codigo = codigoInput.value;
    codigo === '123';
    if(codigo === '123') {
      this.router.navigate(['/diagram']);
    }
  }

}
