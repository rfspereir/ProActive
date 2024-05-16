import { ApplicationConfig, importProvidersFrom } from '@angular/core';

import { provideRouter } from '@angular/router';

import { routes } from './app.routes';
import { provideClientHydration } from '@angular/platform-browser';
import { initializeApp, provideFirebaseApp } from '@angular/fire/app';
import { getAuth, provideAuth } from '@angular/fire/auth';
import { getDatabase, provideDatabase } from '@angular/fire/database';

export const appConfig: ApplicationConfig = {
  providers: [provideRouter(routes), provideClientHydration(), importProvidersFrom(provideFirebaseApp(() => initializeApp({"projectId":"proactive-ae334","appId":"1:98316423271:web:594349f7713fdbd0f8c4c2","databaseURL":"https://proactive-ae334-default-rtdb.firebaseio.com","storageBucket":"proactive-ae334.appspot.com","apiKey":"AIzaSyDuCIrTT_CQjwBTzwdqT8exzWlqmqrs2ao","authDomain":"proactive-ae334.firebaseapp.com","messagingSenderId":"98316423271"}))), importProvidersFrom(provideAuth(() => getAuth())), importProvidersFrom(provideDatabase(() => getDatabase()))]
};
