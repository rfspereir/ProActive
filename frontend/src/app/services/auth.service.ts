import { Injectable } from '@angular/core';

@Injectable({
  providedIn: 'root'
})
export class AuthService {
  private uid: string = '';

  private isBrowser: boolean = (typeof window !== 'undefined');

  setUid(uid: string): void {
    this.uid = uid;
    if (this.isBrowser) {
      localStorage.setItem('uid', uid); // Armazena o UID no localStorage
    }
  }

  getUid(): string {
    if (!this.uid && this.isBrowser) {
      this.uid = localStorage.getItem('uid') || ''; // Recupera o UID do localStorage
    }
    return this.uid;
  }

  clearUid(): void {
    this.uid = '';
    if (this.isBrowser) {
      localStorage.removeItem('uid'); // Remove o UID do localStorage
    }
  }
}
