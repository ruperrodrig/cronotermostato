#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <DS3231.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>

#define PuArriba 7
#define PuAbajo 8
#define PuIzq 5
#define PuDrc 6
#define PuEnter 9
#define Rele 7

#define DHTPIN 2
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE); //objeto tipo DHT

DS3231 Clock; //objeto del rtc

LiquidCrystal_I2C lcd (0x27, 20, 4 ); //obeto lcd (display 20x4) la dirección ni idea porque es así pero funciona

float TempeRTC;
float TempeDHT;
float HumDHT;
float Histeresis;
float SetPoint;
byte Year;
byte Month;
byte Date;
byte DoW;
byte Hour;
byte Minute;
byte Second;
byte HoraSeguridad = 2 ; // Si me quedo dormido con la calefacción que no se quede encendida por la noche
byte MinutoSeguridad = 0 ;
byte HoraOn = 0;
byte MinutoOn = 0;
byte HoraOff = 0;
byte MinutoOff = 0;
byte NImagen=0;
byte NImagenAnterior;
byte ByteModo;
byte ByteCale;
byte CursorCol;
byte CursorFil;
byte ContadorCursor;
bool h12;
bool PM;
bool Ant_PuArriba = LOW ;
bool Ant_PuAbajo = LOW;
bool Ant_PuIzq = LOW;
bool Ant_PuDrc = LOW;
bool Ant_PuEnter = LOW;
bool FlancoIzq;
bool FlancoDrc;
bool FlancoArriba;
bool FlancoAbajo;
bool FlancoEnter ;
bool FlancoPu ;
bool Seleccionado ;
bool PrimerPulso;
bool FlancoCambioImagen;
String Modo ; //Modo de trabajo como texto 0=Off, 1=Man, 2=Auto
String Cale ; //String para el lcd 0=off, 1 =on

unsigned long MomentoLuz;  // guardamos cuando hay que encender la retoriluminación
unsigned long TLuz = 10000; // tiempo que está la retoiluminación



void setup() {

  // entradas
  pinMode(PuArriba, INPUT);
  pinMode(PuAbajo, INPUT);
  pinMode(PuIzq, INPUT);
  pinMode(PuDrc, INPUT);
  pinMode(PuEnter, INPUT);
 
  //Salidas
  pinMode(Rele, OUTPUT);
  
  // Inicializar puerto serie
  Serial.begin(9600);

  // Inicializar comunicación i2C
  Wire.begin();

  //inicializar sensor temperatura y humedad
  dht.begin();
  
  //inicializar pantalla LCD, 20 columna 4 filas + borrado
  lcd.begin (20,4); 
  lcd.clear();

  //iniciaizar variables
  ByteModo = 0;
  ByteCale = 0;
  SetPoint = 21.00;
  CursorCol =0;
  CursorFil =0; 
  ContadorCursor = 0;

  
   /*
    Clock.setClockMode(false);  // set to 24h
    //setClockMode(true); // set to 12h
    Clock.setYear(2020);
    Clock.setMonth(4);
    Clock.setDate(2);                             //Ajustar la hora y fecha
    Clock.setDoW(4);
    Clock.setHour(1);
    Clock.setMinute(7);
    Clock.setSecond(0);
*/





  lcd.backlight();
  delay (500);
  lcd.home();
  lcd.setCursor (0,1); // se empeza a contar desde 0
  lcd.print ("   CRONOTERMOSTATO   ");
  lcd.setCursor (0,2);
  lcd.print ("   JAVIER RUPEREZ   ");
  delay (2000);
  lcd.clear ();
  //lcd.noBacklight ();
  MomentoLuz = millis ();
  PrimerPulso= true;
}

void loop() {
  
  
  //analogWrite(11, 255);  // por si queremos fijar la retroiluminacion
  
  TomaDeDatos ();

  //EscrituraPSerie ();    //para debug

  DetectorFlancosPu ();

      //contador de imagen

  if (FlancoIzq == true && NImagen !=0 && Seleccionado == false ){
    NImagen--;
  }
  if (FlancoDrc == true && NImagen !=2 && Seleccionado == false ){
    NImagen++;
  }



    // flanco cambio de imagen
   if (NImagenAnterior != NImagen) {
      FlancoCambioImagen = true ;
   }
   else{
    FlancoCambioImagen = false;
   }
  NImagenAnterior = NImagen;
  

  EscribirLcd();

  Retroiluminacion ();
  
  
  


  //freimos los flancos que ya no nos hacen falta
  FlancoIzq = false;
  FlancoDrc = false;
  FlancoArriba = false;
  FlancoAbajo = false;
  FlancoEnter = false;
  FlancoPu = false ;


} //end del loop




void TomaDeDatos (){
  //del RTC
  Hour= Clock.getHour(h12, PM);  
  Minute= Clock.getMinute(); 
  Second= Clock.getSecond();
  TempeRTC = Clock.getTemperature();
  // del DHT11
  TempeDHT = dht.readTemperature();
  HumDHT = dht.readHumidity();
  
}


void EscrituraPSerie (){
  Serial.print ("seleccionado:"  );
  Serial.println (Seleccionado);
}


void PosCursor (){

  if(FlancoCambioImagen == true){
      ContadorCursor = 0;
    }

  // contador del cursor
  if (Seleccionado == false){
          switch (NImagen){ // segun en numero de imagen limitamos el contador
    case 0:
      if (FlancoArriba == true && ContadorCursor !=2 ){
        ContadorCursor ++;
      }
      if (FlancoAbajo == true && ContadorCursor !=0 ){
        ContadorCursor --;
      }
    break; //case0


    case 1:
      if (FlancoArriba == true && ContadorCursor !=1 ){
        ContadorCursor ++;
      }
      if (FlancoAbajo == true && ContadorCursor !=0 ){
        ContadorCursor --;
      }
    break;


    case 2:
      /*if (FlancoArriba == true && ContadorCursor !=1 ){ //numero maximo de posiciones del cursor -1
        ContadorCursor ++;
      }
      if (FlancoAbajo == true && ContadorCursor !=0 ){
        ContadorCursor --;
      }
      */
      ContadorCursor = 0;
    break;


    default:
      
    break;
        
  } // final N imagen

  



          switch (NImagen){ // posiciones del cursor segun la imagen
        case 0 :  //imagen 0
          switch  (ContadorCursor){ //contador para posicionar en cada imagen
            case 0: //seleccionar SP
              CursorCol = 14;
              CursorFil = 2;
            break;
    
            case 1: // Seleccionar Modo
              CursorCol = 5;
              CursorFil = 3;
            break;
    
            case 2: // seleccionar on/off solo si está en modo manual, si no, visualización
             CursorCol = 16;
             CursorFil = 3;
            break;

            default:
            break;
           } //final contador de imagen 0       
        break; // final imagen 0
    
    
    
        case 1: //imagen 1
          switch  (ContadorCursor){
            case 0: //seleccionar AutoON
              CursorCol = 0;
              CursorFil = 3;
            break;
    
            case 1: // Seleccionar AutoOFF
              CursorCol = 13;
              CursorFil = 3;
            break;
            
            default:
            break;              
          }
        break; // final imagen 1
    
    
        case 2 : //imagen 2
          switch  (ContadorCursor){
            case 0: //seleccionar histeresis
              CursorCol = 13;
              CursorFil = 1;
            break;

            default:
            break;
          } // fin case imagen 2  
        break;


        default: // del N imagen
        break;
      }
    
  
  }//fin seleccionado
  




}


void DetectorFlancosPu (){
   
 
    if (Ant_PuIzq == LOW && digitalRead(PuIzq) == HIGH) {
      Serial.println("flanco izq ");
      FlancoIzq = true;      
    }
    if (Ant_PuDrc == LOW && digitalRead(PuDrc) == HIGH) {
      Serial.println("flanco drc");
      FlancoDrc = true; 
    }
    if (Ant_PuArriba == LOW && digitalRead(PuArriba) == HIGH) {
      Serial.println("flanco arriba");
      FlancoArriba = true; 
    }
    if (Ant_PuAbajo == LOW && digitalRead(PuAbajo) == HIGH) {
      Serial.println("flanco abajo");
      FlancoAbajo = true; 
      
    }
    if (Ant_PuEnter == LOW && digitalRead(PuEnter) == HIGH) {
      Serial.println("flanco enter");
      if (Seleccionado == true){
        Seleccionado = false;
      }
      else{
        Seleccionado = true;
      }
      FlancoEnter = true; 
      
    }

    
  if (FlancoIzq || FlancoDrc ||FlancoArriba || FlancoAbajo || FlancoEnter == true){
    FlancoPu = true;
  }
  else
  FlancoPu = false;


  
  Ant_PuIzq = digitalRead(PuIzq);
  Ant_PuDrc = digitalRead(PuDrc);
  Ant_PuArriba = digitalRead(PuArriba);
  Ant_PuAbajo = digitalRead(PuAbajo);
  Ant_PuEnter = digitalRead(PuEnter);
  delay (50);
}



void Retroiluminacion (){

  // encender la retroiluminación durante x tiempo al máximo resto del tiempo a lo establecido en define

    if (FlancoPu == true) {
      MomentoLuz = millis ();
      analogWrite(11, 255);
      //Serial.println ("RETRO ON");
    }

    if (millis() > (MomentoLuz + TLuz)) {
      analogWrite (11, 10);
      //Serial.println ("RETRO OFF");
    }

  
}



void EscribirLcd (){
    
    switch (NImagen){
      
      case 0:
        //Serial.println ("Imagen_0");
        //lcd.clear();
        lcd.home ();
        
        ImprimirHoraLcd();
                                              
        lcd.setCursor (0,1);              //segunda linea el PV
        lcd.print(F("Temp.Actual  : "));
        lcd.print(TempeDHT);
        
        if (RefrescarImagen()>=1){ 
        lcd.clear ();                                    
        
        //Serial.print ("Seleccionado");
        //Serial.println (Seleccionado);
        if(Seleccionado == true){
          switch (ContadorCursor ){ //modificando valores de ajuste
                        case 0:
                          if (FlancoArriba == true){
                            SetPoint = SetPoint + 0.2;
                          }
                          if (FlancoAbajo == true){
                            SetPoint = SetPoint - 0.2;
                          }
                        break;
              
                        case 1:
                          if (FlancoArriba && ByteModo !=2){
                            ByteModo ++;
                          }
                          if (FlancoAbajo && ByteModo !=0){
                            ByteModo --;
                          }
                        break;
              
              
                        case 2:
                          if (ByteModo == 1 && (FlancoArriba && ByteCale !=1)){
                            ByteCale ++;
                          }
                          if (ByteModo == 1 && (FlancoAbajo && ByteCale !=0)){
                            ByteCale --;
                          }
                        break;
                        
                        default:
                        break;
                } // modificando valores de ajuste
        } // seleccionado valor
                
        
        lcd.setCursor(0,2);
        //lcd.print(F("Hum. actual: "));
        //lcd.print(HumDHT);
        lcd.print(F("Temp.Consigna: "));
        lcd.print(SetPoint);
        lcd.setCursor(0,3);
        lcd.print(F("MODO: "));
          switch (ByteModo){
            case 0:
            Modo = "OFF";
            break;
            case 1:
            Modo = "MAN";
            break;
            case 2 :
            Modo = "AUTO";
            break;
            default:
            Modo = "Error";
            break;
          }
        lcd.print (Modo);
        lcd.setCursor (11,3);
        lcd.print ("CALE: ");
          switch (ByteCale){
            case 0:
            Cale = "OFF";
            break;
            case 1:
            Cale ="ON";
            break;
            default:
            Cale ="ERR";
            break;
          }
      
     
        lcd.print (Cale);
        PosCursor ();  // vamos a buscar la posición del cursor para saber donde poner la flecha
        ImprimirCursor (); 
       }// final if refrescar Imagen
       
      
      break; // final case Imagen 0
    


  
    case 1: //imagen 1
      
      lcd.home ();
      //Serial.print("imagen_1");
      ImprimirHoraLcd();

            
      if (RefrescarImagen()>=1){
        lcd.clear ();
        lcd.setCursor (0,1);              //segunda linea Hora auto 
        lcd.print(F(" HORARIO AUTOMATICO"));
        lcd.setCursor (0,2);
        lcd.print ("  ON:          OFF:");
        lcd.setCursor (1,3);

          if(Seleccionado == true){
              switch (ContadorCursor){
                case 0:                           //cursor en hora on
                  if(FlancoArriba == true){
                    MinutoOn = MinutoOn + 15;
                    if (MinutoOn >= 60){
                      MinutoOn = 0;
                      if (HoraOn == 23){
                        HoraOn = 0;
                      }
                      else{
                        HoraOn++;
                      }
                      
                    }
                  }
                  if (FlancoAbajo == true ){
                    if (MinutoOn == 0 && HoraOn == 0 ){
                        MinutoOn = 45 ;
                        HoraOn = 23;
                      }
                      else{
                        if (MinutoOn == 0){
                          MinutoOn = 45;
                          HoraOn --; 
                        }
                        else{
                          MinutoOn = MinutoOn - 15 ;
                        }
                    }
                                                         
                  }
                break;



                case 1:                         //cursor en hora off
                  if(FlancoArriba == true){
                    MinutoOff = MinutoOff + 15;
                    if (MinutoOff >= 60){
                      MinutoOff = 0;
                      if (HoraOff == 23){
                        HoraOff = 0;
                      }
                      else{
                        HoraOff ++ ;
                      }
                      
                    }
                  }
                  if (FlancoAbajo == true ){
                    if (MinutoOff == 0 && HoraOff == 0 ){
                        MinutoOff = 45 ;
                        HoraOff = 23;
                      }
                      else{
                        if (MinutoOff == 0){
                          MinutoOff = 45;
                          HoraOff --; 
                        }
                        else{
                          MinutoOff = MinutoOff - 15 ;
                        }
                    }
                                                         
                  }
                break;


                default:
                break;
               
              }
          }
        
        if (HoraOn<10){       //siempre formato de dos digitos
        lcd.print (F("0")); 
        }
        lcd.print (HoraOn);        
        lcd.print (":");
        if (MinutoOn<10){       //siempre formato de dos digitos
        lcd.print (F("0")); 
        }
        lcd.print (MinutoOn); 
        lcd.print ("        ");
        if (HoraOff<10){       //siempre formato de dos digitos
        lcd.print (F("0")); 
        }
        lcd.print (HoraOff);
        lcd.print (":");
        if (MinutoOff<10){       //siempre formato de dos digitos
        lcd.print (F("0")); 
        }
        lcd.print (MinutoOff); 
      }// fin if refrescar imagen
    
      PosCursor ();
      ImprimirCursor ();
      break;


      case 2:
        //Serial.println ("Imagen_2");
  
        lcd.home ();
        if (RefrescarImagen()>=1){
          lcd.clear ();
          lcd.print ("   CONFIGURACION");
          lcd.setCursor(0,1);
          lcd.print ("Histeresis:   ");
            if (Seleccionado == true){
              if (FlancoArriba == true){
                Histeresis = Histeresis + 0.1;
              }
              if (FlancoAbajo == true && Histeresis > 0.2){
                Histeresis = Histeresis - 0.1;
              }
            } 
          lcd.print (Histeresis);
          lcd.print (" C");
          lcd.setCursor (0,2);
          lcd.print ("Apagado olvido:");
          lcd.print ("0");
          lcd.print (HoraSeguridad);
          lcd.print (":");
          lcd.print ("0");
          lcd.print (MinutoSeguridad);
          lcd.setCursor (0,3);
          lcd.print("Humedad:      ");
          
        }
        lcd.setCursor (13,3);       
        lcd.print (HumDHT);
        lcd.print ("%");
        PosCursor ();
        ImprimirCursor (); 
      break;


      default:
      PosCursor ();
      break;
     
    
     }// fin del switch de imagen

}//fin void escribir LCD


void ImprimirHoraLcd (){
      lcd.print(F("      "));
      if (Hour<10){       //siempre formato de dos digitos
        lcd.print (F("0")); 
      }
      lcd.print(Hour);
      lcd.print(F(":"));
      if (Minute<10){     //siempre formato de dos digitos
       lcd.print (F("0"));
      }
      lcd.print(Minute);
      lcd.print(F(":"));
      if (Second<10){     //siempre formato de dos digitos
        lcd.print (F("0"));  
      }
      lcd.print(Second);

}

void ImprimirCursor (){
  lcd.setCursor (CursorCol,CursorFil); //primero lo posicionamos

  if (Seleccionado == true){
    lcd.write(35);
  }
  else{
    lcd.write(126); 
  }
   
}


int RefrescarImagen(){
  int respuesta;
  if (FlancoPu == true || PrimerPulso == true ){
    respuesta = 1;
  }
  else{
    respuesta = 0;
  }
  PrimerPulso = false;
  return respuesta;
}








/*
 *lcd.noBacklight(); 
 *lcd.backlight(); 
  lcd.setCursor(3,0);
  lcd.print("Hello, world!");
  lcd.setCursor(2,1);
  lcd.print("Ywrobot Arduino!");
   lcd.setCursor(0,2);
  lcd.print("Arduino LCM IIC 2004");
  lcd.setCursor(2,3);
  lcd.print("Power By Ec-yuan!");
  lcd.noDisplay(); //apaga el texto pero sin borrar
  lcd.display();
  lcd.blink_on(); //parpadea el cursos en cuadrado
  lcd.blink_off();
  lcd.createChar


  //Serial.println ((char)126);  escribir decimal ascii
*/



