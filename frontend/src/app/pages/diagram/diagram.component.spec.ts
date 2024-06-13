import { ComponentFixture, TestBed } from '@angular/core/testing';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgxEchartsDirective, NgxEchartsModule } from 'ngx-echarts';
import { AuthService } from '../../services/auth.service';
import { DiagramComponent } from './diagram.component';

describe('DiagramComponent', () => {
  let component: DiagramComponent;
  let fixture: ComponentFixture<DiagramComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ DiagramComponent ],
      imports: [
        CommonModule,
        FormsModule,
        NgxEchartsModule.forRoot({
          echarts: () => import('echarts')
        })
      ],
      providers: [
        AuthService  // Fornecer o serviço mockado se necessário
      ]
    })
    .compileComponents();
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(DiagramComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });

  it('should increment temp by 1', () => {
    const initialTemp = component.temp;
    component.increment();
    expect(component.temp).toBe(initialTemp + 1);
  });

  it('should decrement temp by 1', () => {
    const initialTemp = component.temp;
    component.decrement();
    expect(component.temp).toBe(initialTemp - 1);
  });

  it('should save data correctly', () => {
    spyOn(console, 'log'); // Mock para evitar logs no console durante os testes
    component.temp = 25; // Definindo um valor de temperatura para o teste
    component.saveData();
    expect(console.log).toHaveBeenCalledWith('Dados salvos com sucesso:', 25);
  });

  // Testes para outras funções e métodos conforme necessário

});
