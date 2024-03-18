import { Routes } from '@angular/router';
import { DiagramComponent } from './pages/diagram/diagram.component';
import { HomeComponent } from './pages/home/home.component';

export const routes: Routes = [
    { path: '', redirectTo: '/home', pathMatch: 'full' },
    { path: 'diagram', component: DiagramComponent },
    { path: 'home', component: HomeComponent },
];
