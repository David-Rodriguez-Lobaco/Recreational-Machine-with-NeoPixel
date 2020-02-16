#include <Adafruit_NeoPixel.h>
#include <TimerOne.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define LED_PIN     6   //Pin de los datos
#define NUM_LEDS    256   //numero de leds
int     BRIGHTNESS = 20;  //luminosidad
#define COLOR_ORDER GRB
#define LED_TYPE    WS2812B
#define DUTY        512
#define PIN_PWM    11 //sonido

//NOTAS MUSICALES
#define A_BEMOL_PERIODO    2408
#define B_BEMOL_PERIODO    2145
#define C_PERIODO          1911

#define NEGRA         550
#define SILENCIO      50
#define SEMICORCHEA   150


//MÁQUINA DE ESTADOS

#define CONFIGURACION    0 //Configura las variables a su estado inicial y asigna las interrupciones
#define ESTADO_INICIAL   1 //pantalla de inicio. Se puede elegir la dificultad, color y brillo.
#define STACKER          2 //Juego 
#define WIN              3 //Animacion de victoria
#define GAME_OVER        4 //Animacion de derrota
#define LOADING          9 //pantalla de carga con el logo.

int estado = 9; //Estado por el que va a empezar

//CRGB leds[NUM_LEDS];
Adafruit_NeoPixel matriz = Adafruit_NeoPixel(256, LED_PIN, NEO_GRB + NEO_KHZ800); //Configura la matriz

int fila = 0;
const int timeThreshold = 300; //Tiempo entre interrupciones
const int intPin1 = 2; //boton para parar stacker
const int intPin2 = 3; //boton del nivelint
const int intPin3 = 18; //Boton del color
const int intPin4 = 19; //Boton del brillo
long antiRebotes = 0; 
long startTime = 0;
int luces = 3; //Numero de leds que se mueven
int alternar = 0;//variable para la animacion
int direccion = 0; // '0' es que va para la derecha y '1' hacia la izquierda
int i = 2; //La posicion por la que empieza en la matriz
int matrizPos[32][8]; //posicion de los leds para comparar
int VelocidadStack = 200;
int lvl = 1;  //Nivel de dificultad
int color = 0;
uint32_t c = matriz.Color(0, 0, 255);

long tiempo = 0;
int tempo = 0;
int puntuacionStackerMaxima = 0;
int puntuacionStacker = 0;
int tiempoCarga = 0;



int matrizLeds[32][8] = { //Posicion de cada led en la matriz
  {0, 1, 2, 3, 4, 5, 6, 7},
  {15, 14, 13, 12, 11, 10, 9, 8},
  {16, 17, 18, 19, 20, 21, 22, 23},
  {31, 30, 29, 28, 27, 26, 25, 24},
  {32, 33, 34, 35, 36, 37, 38, 39},
  {47, 46, 45, 44, 43, 42, 41, 40},
  {48, 49, 50, 51, 52, 53, 54, 55},
  {63, 62, 61, 60, 59, 58, 57, 56},
  {64, 65, 66, 67, 68, 69, 70, 71},
  {79, 78, 77, 76, 75, 74, 73, 72},
  {80, 81, 82, 83, 84, 85, 86, 87},
  {95, 94, 93, 92, 91, 90, 89, 88},
  {96, 97, 98, 99, 100, 101, 102, 103},
  {111, 110, 109, 108, 107, 106, 105, 104},
  {112, 113, 114, 115, 116, 117, 118, 119},
  {127, 126, 125, 124, 123, 122, 121, 120},
  {128, 129, 130, 131, 132, 133, 134, 135},
  {143, 142, 141, 140, 139, 138, 137, 136},
  {144, 145, 146, 147, 148, 149, 150, 151},
  {159, 158, 157, 156, 155, 154, 153, 152},
  {160, 161, 162, 163, 164, 165, 166, 167},
  {175, 174, 173, 172, 171, 170, 169, 168},
  {176, 177, 178, 179, 180, 181, 182, 183},
  {191, 190, 189, 188, 187, 186, 185, 184},
  {192, 193, 194, 195, 196, 197, 198, 199},
  {207, 206, 205, 204, 203, 202, 201, 200},
  {208, 209, 210, 211, 212, 213, 214, 215},
  {223, 222, 221, 220, 219, 218, 217, 216},
  {224, 225, 226, 227, 228, 229, 230, 231},
  {239, 238, 237, 236, 235, 234, 233, 232},
  {240, 241, 242, 243, 244, 245, 246, 247},
  {255, 254, 253, 252, 251, 250, 249, 248}
};


void setup() {
  Serial.begin(9600);
  matriz.begin();
  matriz.setBrightness(BRIGHTNESS);
  matriz.clear();
  matriz.show();

  //Interrupciones Hardware
  //attachInterrupt(digitalPinToInterrupt(intPin), InterruptEmpezar, FALLING);
  pinMode(intPin1, INPUT_PULLUP);
  pinMode(intPin2, INPUT_PULLUP);
  pinMode(intPin3, INPUT_PULLUP);
  pinMode(intPin4, INPUT_PULLUP);
  //  pinMode(pinOut, OUTPUT);

  Timer1.initialize(2000);
  Timer1.pwm(PIN_PWM, DUTY);
  Timer1.stop();
  //tone(pinOut, 600, 500);

  // Inicializar el LCD
  lcd.init();

  //Encender la luz de fondo.
  lcd.backlight();
  tiempoCarga = millis();

}

void loop()
{
  // Escribimos el Mensaje en el LCD.
  Stacker();
  lcd.setCursor(0, 0);
  lcd.print("record: ");
  lcd.print(puntuacionStackerMaxima);
  lcd.setCursor(0, 1);
  lcd.print("puntuacion: ");
  lcd.setCursor(12, 1);
  lcd.print(puntuacionStacker);
  lcd.setCursor(0, 2);
  lcd.print("Nivel: ");
  lcd.setCursor(7, 2);
  lcd.print(lvl);

}

void pararFila() //para la fila en el lugar y aumenta la fila
{
  noInterrupts();
  if (millis() - antiRebotes > timeThreshold) //pongo los valores en la matriz
  {
    detachInterrupt(digitalPinToInterrupt(intPin1)); //deshabilito la interrupcion, por si acaso
    comprobarStack();//comprueba si se apila y si no elimina leds
    direccion = 0;
    i = luces - 1;
    antiRebotes = millis();
    attachInterrupt(digitalPinToInterrupt(intPin1), pararFila, FALLING); //vuelvo a habilitar la interrupcion.
  }
  interrupts();
}


void comprobarStack() //Compruebo si se estan apilando correctamente
{
  int luces_act = luces;
  if (fila > 0) //si es la fila 2 o mayor compruebo con la fila de abajo
  {
    for (int j = 0; j < 8; j++)
    {
      if (matrizPos[fila][j] == 1 && matrizPos[fila - 1][j] != 1)
      {
        matrizPos[fila][j] = 2; //pongo a 2 el que falla
        matriz.setPixelColor(matrizLeds[fila][j], c);
        luces--;
      }
    }

    if (luces_act != luces) //si ha habido algun fallo
    {
      Fallo_Al_Pulsar();
    }

  }
  fila++; //sube de fila
  if (luces == 0) //si fallo todas 
  {
    estado = 4; //GAME OVER
    //break;
  }
  else if (fila >= 32) //Si he llegado hasta la ultima fila 
  {
    estado = 3; //WIN
  }
  else  
  {
    actualizar_Nivel(); //Aumento la velocidad
  }

}

void Fallo_Al_Pulsar() //animacion al fallar
{
  alternar = 0;
  int tono = 1;
  long tiempo1 = millis();
  //tone(pinOut, 600, 500);
  Timer1.setPeriod(2000);
  Timer1.start();
  int r = 0;
  while (r < 6)
  {
    if (millis() - tiempo1 > 500 && tono == 1) //pongo los valores en la matriz
    {
      //tone(pinOut, 500, 500);
      Timer1.setPeriod(2500);
      tono = 2;
    }
    if (millis() - tiempo1 > 1000 && tono == 2) //pongo los valores en la matriz1
    {
      Timer1.setPeriod(3000);
      tono = 3;
    }
    if (millis() - tiempo1 > 1500 && tono == 3) //pongo los valores en la matriz1
    {
      Timer1.stop();
      tono = 0;
    }
    if (millis() - antiRebotes > timeThreshold) //pongo los valores en la matriz
    {
      for (int k = 0; k < 8; k++)
      {
        if (matrizPos[fila][k] == 2 && alternar == 0)
        {
          matriz.setPixelColor(matrizLeds[fila][k], c);
        }
        else if (matrizPos[fila][k] == 2 && alternar == 1)
        {
          matriz.setPixelColor(matrizLeds[fila][k], 0, 0, 0);
        }
      }
      matriz.show();
      if (alternar == 0)
      {
        alternar = 1;
      }
      else {
        alternar = 0;
      }
      r++;
      antiRebotes = millis();
    }
  }
  tono = 1;
}

void actualizar_Nivel() //Dependiendo del nivel aumenta la velocidad a la que se mueven los leds y añade puntuacion
{
  if (lvl == 1)
  {
    VelocidadStack = VelocidadStack - 2;
    puntuacionStacker = puntuacionStacker + 100 * luces;
    if (puntuacionStacker > puntuacionStackerMaxima)
    {
      puntuacionStackerMaxima = puntuacionStacker;
    }
  }
  else if (lvl == 2)
  {
    VelocidadStack = VelocidadStack - 3;
    puntuacionStacker = puntuacionStacker + 225 * luces;
    if (puntuacionStacker > puntuacionStackerMaxima)
    {
      puntuacionStackerMaxima = puntuacionStacker;
    }
    if (fila == 17 && luces == 3) // 17
    {
      luces--;
    }
    else if (fila == 25 && luces == 2) //25
    {
      luces--;
    }
  }
  else if (lvl == 3)
  {
    VelocidadStack = VelocidadStack - 4;
    puntuacionStacker = puntuacionStacker + 475 * luces;
    if (puntuacionStacker > puntuacionStackerMaxima)
    {
      puntuacionStackerMaxima = puntuacionStacker;
    }
    if (fila == 12 && luces == 3)
    {
      luces--;
    }
    else if (fila == 20 && luces == 2)
    {
      luces--;
    }
  }
}

void ISR_Mover() //Animacion del juego principal. Hacen el movimiento de los leds
{
  noInterrupts();
  if (direccion == 0)
  {
    if (i > luces - 1)
    {
      matriz.setPixelColor(matrizLeds[fila][i - luces], 0, 0, 0);
      matrizPos[fila][i - luces] = 0;
    }
    if (luces == 3)
    {
      matriz.setPixelColor(matrizLeds[fila][i - 2], c);
      matriz.setPixelColor(matrizLeds[fila][i - 1], c);
      matriz.setPixelColor(matrizLeds[fila][i], c);

      
      matrizPos[fila][i - 2] = 1;
      matrizPos[fila][i - 1] = 1;
      matrizPos[fila][i] = 1;
    }
    else if (luces == 2)
    {
      matriz.setPixelColor(matrizLeds[fila][i - 1], c);
      matriz.setPixelColor(matrizLeds[fila][i], c);

      //matrizPos[fila][i - 2] = 0;
      matrizPos[fila][i - 1] = 1;
      matrizPos[fila][i] = 1;
    }
    else if (luces == 1)
    {
      matriz.setPixelColor(matrizLeds[fila][i], c);

      //matrizPos[fila][i - 1] = 0;
      matrizPos[fila][i] = 1;
    }
  }
  else
  {
    if (luces == 3)
    {
      matriz.setPixelColor(matrizLeds[fila][i + 3], 0, 0, 0);
      matriz.setPixelColor(matrizLeds[fila][i + 2], c);
      matriz.setPixelColor(matrizLeds[fila][i + 1], c);
      matriz.setPixelColor(matrizLeds[fila][i], c);

      matrizPos[fila][i + 3] = 0;
      matrizPos[fila][i + 2] = 1;
      matrizPos[fila][i + 1] = 1;
      matrizPos[fila][i] = 1;
    }
    else if (luces == 2)
    {
      matriz.setPixelColor(matrizLeds[fila][i + 2], 0, 0, 0);
      matriz.setPixelColor(matrizLeds[fila][i + 1], c);
      matriz.setPixelColor(matrizLeds[fila][i], c);

      matrizPos[fila][i + 2] = 0;
      matrizPos[fila][i + 1] = 1;
      matrizPos[fila][i] = 1;
    }
    else if (luces == 1)
    {
      matriz.setPixelColor(matrizLeds[fila][i + 1], 0, 0, 0);
      matriz.setPixelColor(matrizLeds[fila][i], c);

      matrizPos[fila][i + 1] = 0;
      matrizPos[fila][i] = 1;
    }
  }
  matriz.show();
  interrupts();
  if (direccion == 0)
  {
    i++;
    if (i == 8)
    {
      direccion = 1;
      i = 7 - luces;
    }
  }
  else if (direccion == 1)
  {
    i--;
    if (i == -1)
    {
      direccion = 0;
      i = luces;
    }
  }
}

void reiniciarStacker() //Al finalizar el juego reinicia los valores por defecto
{
  fila = 0;
  luces = 3;
  i = 3;
  matriz.clear();
  matriz.show();
  for (int x = 0 ; x < 32 ; ++x)
  {
    for (int y = 0; y < 8 ; ++y)
    {
      matrizPos[x][y] = 0;
    }
  }
  VelocidadStack = 200;
  estado = CONFIGURACION;
  /**********************************************************************/
  if (puntuacionStacker > puntuacionStackerMaxima) {
    puntuacionStackerMaxima = puntuacionStacker;
  }
  lcd.clear();
  puntuacionStacker = 0;
}

void ISR_Animacion() //Animacion de la pantalla de inicio donde puedes elegir nivel, color,...
{
  if (alternar == 0)
  {
    matriz.clear();
    for (int f = 0; f < 87; f = f + 2)
    {
      matriz.setPixelColor(f, c);
    }
    for (int f = 254; f > 167; f = f - 2)
    {
      matriz.setPixelColor(f, c);
    }
    alternar = 1;
  }
  else
  {
    matriz.clear();
    for (int f = 1; f < 88; f = f + 2)
    {
      matriz.setPixelColor(f, c);
    }
    for (int f = 255; f > 168; f = f - 2)
    {
      matriz.setPixelColor(f, c);
    }
    alternar = 0;
  }
  if (lvl == 1)
  {
    matriz.setPixelColor(matrizLeds[18][3], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[19][4], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[18][4], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[17][4], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[16][4], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[15][4], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[14][4], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[13][4], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[13][5], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[13][3], 255, 0, 0);
  }
  else if (lvl == 2)
  {
    matriz.setPixelColor(matrizLeds[18][2], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[19][3], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[19][4], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[18][5], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[17][5], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[16][4], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[15][3], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[14][2], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[13][2], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[13][3], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[13][4], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[13][5], 255, 0, 0);
  }
  else if (lvl == 3)
  {
    matriz.setPixelColor(matrizLeds[18][2], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[19][3], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[19][4], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[18][5], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[17][5], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[16][4], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[15][5], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[14][5], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[13][4], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[13][3], 255, 0, 0);
    matriz.setPixelColor(matrizLeds[14][2], 255, 0, 0);
  }
  matriz.show();
}

void InterruptEmpezar() //Elimina todas las interrupciones y asigna al boton principal la funcion de parar la fila
{
  noInterrupts();
  if (millis() - antiRebotes > timeThreshold) //pongo los valores en la matriz
  {
    matriz.clear();
    matriz.show();
    detachInterrupt(digitalPinToInterrupt(intPin1));
    detachInterrupt(digitalPinToInterrupt(intPin2));
    detachInterrupt(digitalPinToInterrupt(intPin3));
    detachInterrupt(digitalPinToInterrupt(intPin4));
    attachInterrupt(digitalPinToInterrupt(intPin1), pararFila, FALLING); //FALLING

    estado = 2;
    antiRebotes = millis();
    Timer1.stop();
    tempo = 0;
  }
  interrupts();
}

void animacionGanadoraStacker() //Animacion ganadora
{
  int d = 0;
  long periodo;
  int tono = 1;
  long tiempo1;
  noInterrupts();

  Timer1.setPeriod(C_PERIODO);
  Timer1.start();

  delay(150);
  Timer1.stop();
  delay(50);

////////////////////////////////    SONIDO ////////////////////////////
  alternar = 0;
  periodo = millis() - 251;
  tiempo1 = millis();
  while (d < 18)
  {
    if (millis() - tiempo1 > SEMICORCHEA && tono == 1) //1
    {
      Timer1.stop();
      tono = 2;
      tiempo1 = millis();
    } else if (millis() - tiempo1 > SILENCIO && tono == 2)
    {
      //Timer1.setPeriod(C_PERIODO);
      tono = 3;
      Timer1.start();
      tiempo1 = millis();
    } if (millis() - tiempo1 > SEMICORCHEA && tono == 3) //1
    {
      Timer1.stop();
      tono = 4;
      tiempo1 = millis();
    } else if (millis() - tiempo1 > SILENCIO && tono == 4)
    {
      //Timer1.setPeriod(C_PERIODO);
      tono = 5;
      Timer1.start();
      tiempo1 = millis();
    }
    else if (millis() - tiempo1 > SEMICORCHEA && tono == 5) //2
    {
      tono = 6;
      Timer1.stop();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > SILENCIO && tono == 6)
    {
      // Timer1.setPeriod(C_PERIODO);
      tono = 7;
      Timer1.start();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > SEMICORCHEA && tono == 7) //3
    {
      tono = 8;
      Timer1.stop();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > SILENCIO && tono == 8)
    {
      tono = 9;
      Timer1.setPeriod(C_PERIODO);
      Timer1.start();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > NEGRA && tono == 9) //4
    {
      tono = 10;
      Timer1.stop();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > SILENCIO && tono == 10)
    {
      tono = 11;
      Timer1.setPeriod(A_BEMOL_PERIODO);
      Timer1.start();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > NEGRA && tono == 11) //5
    {
      tono = 12;
      Timer1.stop();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > SILENCIO && tono == 12)
    {
      tono = 13;
      Timer1.setPeriod(B_BEMOL_PERIODO);
      Timer1.start();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > NEGRA && tono == 13) //6
    {
      tono = 14;
      Timer1.stop();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > SILENCIO && tono == 14)
    {
      tono = 15;
      Timer1.setPeriod(C_PERIODO);
      Timer1.start();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > SEMICORCHEA + 200 && tono == 15)
    {
      tono = 16;
      Timer1.stop();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > SILENCIO && tono == 16)
    {
      tono = 17;
      Timer1.setPeriod(B_BEMOL_PERIODO);
      Timer1.start();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > SEMICORCHEA && tono == 17)
    {
      tono = 18;
      Timer1.stop();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > SILENCIO && tono == 18)
    {
      tono = 19;
      Timer1.setPeriod(C_PERIODO);
      Timer1.start();
      tiempo1 = millis();
    } else if (millis() - tiempo1 > NEGRA && tono == 19)
    {
      tono = 0;
      Timer1.stop();
      tiempo1 = millis();
    }
    
///////////////////////////// animacion //////////////////////////////////

    if (millis() - periodo > 250) //pongo los valores en la matriz
    {
      for (int a = 0; a < 32; a++)
      {
        for (int b = 0; b < 8; b++)
        {
          if (alternar == 0 && matrizPos[a][b] == 1)
          {
            matriz.setPixelColor(matrizLeds[a][b], 0, 0, 0);
          }
          else if (alternar == 1 && matrizPos[a][b] == 1)
          {
            matriz.setPixelColor(matrizLeds[a][b], c);
          }
        }
      }
      matriz.show();
      if (alternar == 0)
      {
        alternar = 1;
      }
      else {
        alternar = 0;
      }
      d++;
      periodo = millis();
    }
  }
  interrupts();
  reiniciarStacker();
}

void cambiarDificultad() //Permite cambiar la dificultad del juego en el menu principal.
{
  noInterrupts();
  if (millis() - antiRebotes > timeThreshold) //pongo los valores en la matriz
  {
    if (lvl == 1)
    {
      lvl = 2;
      //****************************************************************************************************
      //lcd.setCursor(7,2);
      // lcd.print("1");
    }
    else if (lvl == 2)
    {
      lvl = 3;
      //lcd.clear();
      // lcd.setCursor(7,2);
      //lcd.print("2");
    }
    else if (lvl == 3)
    {
      lvl = 1;
      //lcd.setCursor(7,2);
      //lcd.print("3");
    }
    antiRebotes = millis();
  }
  interrupts();
}

void sonido_inicio()
{
  if (millis() - tiempo > 200 && tempo == 0)
  {

    Timer1.setPeriod(3000);
    Timer1.start();
    tempo = 1;
    //tiempo = millis();
  } else if (millis() - tiempo > 400 && tempo == 1)
  {

    Timer1.setPeriod(2500);
    tempo = 2;
    //tiempo = millis();
  } else if (millis() - tiempo > 600 && tempo == 2)
  {
    Timer1.setPeriod(2000);
    tempo = 3;
    //tiempo = millis();
  } else if (millis() - tiempo > 800 && tempo == 3)
  {
    Timer1.setPeriod(1500);
    tempo = 0;
    tiempo = millis();
  }/*else if(millis() - tiempo > 2500 && tempo == 4)
    {
      Timer1.setPeriod(4500);
      tempo = 5;
       tiempo = millis();
    }else if(millis() - tiempo > 3000 && tempo == 5)
    {
      Timer1.setPeriod(4500);
      tempo = 0;
      tiempo = millis();
    }*/
}

void sonido_victoria()
{
  if (millis() - tiempo > 200 && tempo == 0)
  {

    Timer1.setPeriod(3000);
    Timer1.start();
    tempo = 1;
    //tiempo = millis();
  } else if (millis() - tiempo > 400 && tempo == 1)
  {

    Timer1.setPeriod(2500);
    tempo = 2;
    //tiempo = millis();
  } else if (millis() - tiempo > 600 && tempo == 2)
  {
    Timer1.setPeriod(2000);
    tempo = 3;
    //tiempo = millis();
  } else if (millis() - tiempo > 800 && tempo == 3)
  {
    Timer1.setPeriod(1500);
    tempo = 0;
    tiempo = millis();
  }/*else if(millis() - tiempo > 2500 && tempo == 4)
    {
      Timer1.setPeriod(4500);
      tempo = 5;
       tiempo = millis();
    }else if(millis() - tiempo > 3000 && tempo == 5)
    {
      Timer1.setPeriod(4500);
      tempo = 0;
      tiempo = millis();
    }*/
}

void CambiarColor() //Permite cambiar el color en la pantalla principal
{
  noInterrupts();
  if (millis() - antiRebotes > timeThreshold) //pongo los valores en la matriz
  {
    if (color == 0)
    {
      c = matriz.Color(0, 0, 255); //azul
      color = 1;
    }
    else if (color == 1)
    {
      c = matriz.Color(0, 255, 0); //Verde
      color = 2;
    }
    else if (color == 2)
    {
      c = matriz.Color(255, 0, 0); //Rojo
      color = 3;
    }
    else if (color == 3)
    {
      c = matriz.Color(0, 255, 255); //Cian
      color = 4;
    }
    else if (color == 4)
    {
      c = matriz.Color(255, 0, 255); //Morado
      color = 5;
    }
    else if (color == 5)
    {
      c = matriz.Color(255, 255, 0); //Naranja
      color = 0;
    }
    antiRebotes = millis();
  }
  interrupts();
}

void CambiarBrillo() //Permite cambiar el brillo en la pagina principal.
{
  noInterrupts();
  if (millis() - antiRebotes > timeThreshold) //pongo los valores en la matriz
  {
    BRIGHTNESS += 10;
    if (BRIGHTNESS >= 120)
    {
      BRIGHTNESS = 10;
    }
    matriz.setBrightness(BRIGHTNESS);
    antiRebotes = millis();
  }
  interrupts();
}

void pantallaCarga() //Pantalla de carga con la palagra 'LOADING'
{

  matriz.clear();
  //Fondo
  /*for (int f = 0; f < 255; f++)
  {
    matriz.setPixelColor(f, 0, 30, 0);
  }*/

  //L
  matriz.setPixelColor(matrizLeds[31][2], 255, 255, 0);
  matriz.setPixelColor(matrizLeds[30][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[29][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[28][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[28][3],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[28][4],  255, 255, 0);

  //O
  matriz.setPixelColor(matrizLeds[26][3],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[26][4],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[25][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[25][5],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[24][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[24][5],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[23][3],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[23][4],  255, 255, 0);

  //A
  matriz.setPixelColor(matrizLeds[21][4],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[20][3],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[20][4],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[19][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[19][3],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[19][4],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[19][5],  255, 255, 0);


  //D
  matriz.setPixelColor(matrizLeds[17][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[17][3],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[17][4],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[16][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[16][5],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[15][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[15][5],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[14][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[14][3],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[14][4],  255, 255, 0);

  //I
  matriz.setPixelColor(matrizLeds[12][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[12][3],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[12][4],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[12][5],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[11][3],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[11][4],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[10][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[10][3],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[10][4],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[10][5],  255, 255, 0);

  //N
  matriz.setPixelColor(matrizLeds[8][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[8][5],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[7][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[7][3],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[7][5],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[6][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[6][4],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[6][5],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[5][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[5][5],  255, 255, 0);

  //G
  matriz.setPixelColor(matrizLeds[3][3],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[3][4],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[2][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[1][2],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[1][4],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[1][5],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[0][3],  255, 255, 0);
  matriz.setPixelColor(matrizLeds[0][4],  255, 255, 0);


  matriz.show();
  if (millis() >= tiempoCarga + 3000)
  {
    estado = 0;
  }
}

/////////////////////////////////////// MAQUINA DE ESTADOS ///////////////////////////////////////

void Stacker()
{
  switch (estado)
  {
    case LOADING:
      pantallaCarga();
      break;

    case CONFIGURACION:
      detachInterrupt(digitalPinToInterrupt(intPin1));
      attachInterrupt(digitalPinToInterrupt(intPin1), InterruptEmpezar, FALLING);
      attachInterrupt(digitalPinToInterrupt(intPin2), cambiarDificultad, FALLING);
      attachInterrupt(digitalPinToInterrupt(intPin3), CambiarColor, FALLING);
      attachInterrupt(digitalPinToInterrupt(intPin4), CambiarBrillo, FALLING);
      estado = ESTADO_INICIAL;
      tiempo = millis();
      break;

    case ESTADO_INICIAL:
      sonido_inicio();
      if (millis() - startTime > VelocidadStack) //pongo los valores en la matriz
      {
        ISR_Animacion();
        startTime = millis();
      }
      break;

    case STACKER:

      if (millis() - startTime > VelocidadStack) //pongo los valores en la matriz
      {
        ISR_Mover();
        startTime = millis();
      }
      break;

    case WIN:
      animacionGanadoraStacker();
      break;

    case GAME_OVER:
      reiniciarStacker();
      break;
  }
}
