import { Component } from '@angular/core';
import { Router, RouterOutlet } from '@angular/router';
import { FooterComponent } from '../../components/footer/footer.component';
import { DiagramComponent } from '../diagram/diagram.component';
import { Auth as FirebaseAuth, signInWithEmailAndPassword, signOut, onAuthStateChanged } from '@angular/fire/auth';
import { FormsModule } from '@angular/forms';
import { CommonModule } from '@angular/common';
import { AuthService } from '../../services/auth.service';

@Component({
  selector: 'app-home',
  standalone: true,
  imports: [
    RouterOutlet,
    FooterComponent,
    DiagramComponent,
    FormsModule,
    CommonModule
  ],
  templateUrl: './home.component.html',
  styleUrls: ['./home.component.css']
})
export class HomeComponent {
  email: string = '';
  password: string = '';
  user: any;
  loginError: string = '';

  constructor(private router: Router, private auth: FirebaseAuth, private authService: AuthService) { // Injete o serviço corretamente
    onAuthStateChanged(this.auth, (user) => {
      if (user) {
        this.user = user;
        this.authService.setUid(user.uid);
      } else {
        this.user = null;
      }
    });
  }

  async login() {
    try {
      const userCredential = await signInWithEmailAndPassword(this.auth, this.email, this.password);
      this.user = userCredential.user;
      this.authService.setUid(this.user.uid);
      this.router.navigate(['/diagram']);
    } catch (error) {
      console.error('Login error:', error);
      this.loginError = 'Invalid email or password. Please try again.';
    }
  }

  navigateToDiagram() {
    this.router.navigate(['/diagram']);
  }

  async logout() {
    try {
      await signOut(this.auth);
      this.user = null;
      this.authService.clearUid();  // Limpa o UID no serviço
    } catch (error) {
      console.error('Logout error:', error);
    }
  }
}
