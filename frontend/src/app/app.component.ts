import { Component, inject } from '@angular/core';
import { Database } from '@angular/fire/database';
import { CommonModule } from '@angular/common';
import { RouterOutlet } from '@angular/router';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [CommonModule, RouterOutlet],
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent {
  title = 'GeladeiraInteligente';
  private database = inject(Database);
}
