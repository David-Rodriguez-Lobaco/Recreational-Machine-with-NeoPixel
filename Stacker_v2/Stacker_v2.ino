#include <Adafruit_NeoPixel.h>
#include <TimerOne.h>


#define LED_PIN     6   //Pin de los datos
#define NUM_LEDS    256   //numero de leds
#define BRIGHTNESS  20  //luminosidad
#define COLOR_ORDER GRB
#define LED_TYPE    WS2812B
#define DUTY        512
#define PIN_PWM    11


//MÁQUINA DE ESTADOS

#define CONFIGURACION    0
#define ESTADO_INICIAL   1
#define STACKER          2
#define WIN              3
#define GAME_OVER        4

int estado = 0;

//CRGB leds[NUM_LEDS];
Adafruit_NeoPixel matriz = Adafruit_NeoPixel(256, LED_PIN, NEO_GRB + NEO_KHZ800);

int fila = 0;
const int timeThreshold = 300;
const int intPin1 = 2; //boton para parar stacker
const int intPin2 = 3; //boton del nivelint
long antiRebotes = 0;
long startTime = 0;
int luces = 3;
int alternar = 0;//variable para la animacion
int direccion = 0; // '0' es que va para la derecha y '1' hacia la izquierda
int i = 2; //La posicion por la que empieza en la matriz
int matrizPos[32][8]; //posicion de los leds para comparar
int VelocidadStack = 200;
int lvl = 2;
int pinOut = 8;

int matrizLeds[32][8] = {
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
  pinMode(pinOut, OUTPUT);

  Timer1.initialize(2000);
  Timer1.start();
  Timer1.pwm(PIN_PWM, DUTY);
  delay(500);
  Timer1.stop();
  //tone(pinOut, 600, 500);
}

void loop()
{

  Stacker();
  //matriz.setPixelColor(matrizLeds[5][5], 0, 0, 255);
  //matriz.show();
}

void pararFila() //para la fila en el lugar y aumenta la fila
{
  noInterrupts();
  if (millis() - antiRebotes > timeThreshold) //pongo los valores en la matriz
  {
    detachInterrupt(digitalPinToInterrupt(intPin1));
    comprobarStack();//comprueba si se apila y si no elimina leds
    direccion = 0;
    i = luces - 1;
    antiRebotes = millis();
    attachInterrupt(digitalPinToInterrupt(intPin1), pararFila, FALLING);
  }
  interrupts();
}


void comprobarStack()
{
  int luces_act = luces;
  if (fila > 0) //si es la fila 2 o mayor compruebo con la fila de abajo
  {
    for (int j = 0; j < 8; j++)
    {
      if (matrizPos[fila][j] == 1 && matrizPos[fila - 1][j] != 1)
      {
        matrizPos[fila][j] = 2; //pongo a 2 el que falla
        matriz.setPixelColor(matrizLeds[fila][j], 0, 0, 255);
        luces--;
      }
    }

    if (luces_act != luces)
    {
      Fallo_Al_Pulsar();
    }

  }
  fila++;
  if (luces == 0)
  {
    estado = 4;
    //break;
  }
  else if (fila >= 32)
  {
    estado = 3;
  }
  else
  {
    actualizar_Nivel();
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
          matriz.setPixelColor(matrizLeds[fila][k], 0, 0, 255);
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

void actualizar_Nivel() //Dependiendo del nivel aumenta la velocidad a la que se mueven los leds
{
  if (lvl == 1)
  {
    VelocidadStack = velocidadStack - 2 * fila;
  }
  else if (lvl == 2)
  {
    VelocidadStack = velocidadStack - 3 * fila;
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
    VelocidadStack = velocidadStack - 4 * fila;
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

void ISR_Mover()
{
  noInterrupts();
  if (direccion == 0)
  {
    if (i > luces - 1)
    {
      matriz.setPixelColor(matrizLeds[fila][i - luces], 0, 0, 0);
    }
    if (luces == 3)
    {
      matriz.setPixelColor(matrizLeds[fila][i - 2], 0, 0, 255);
      matriz.setPixelColor(matrizLeds[fila][i - 1], 0, 0, 255);
      matriz.setPixelColor(matrizLeds[fila][i], 0, 0, 255);

      matrizPos[fila][i - 3] = 0;
      matrizPos[fila][i - 2] = 1;
      matrizPos[fila][i - 1] = 1;
      matrizPos[fila][i] = 1;
    }
    else if (luces == 2)
    {
      matriz.setPixelColor(matrizLeds[fila][i - 1], 0, 0, 255);
      matriz.setPixelColor(matrizLeds[fila][i], 0, 0, 255);

      matrizPos[fila][i - 2] = 0;
      matrizPos[fila][i - 1] = 1;
      matrizPos[fila][i] = 1;
    }
    else if (luces == 1)
    {
      matriz.setPixelColor(matrizLeds[fila][i], 0, 0, 255);

      matrizPos[fila][i - 1] = 0;
      matrizPos[fila][i] = 1;
    }
  }
  else
  {
    if (luces == 3)
    {
      matriz.setPixelColor(matrizLeds[fila][i + 3], 0, 0, 0);
      matriz.setPixelColor(matrizLeds[fila][i + 2], 0, 0, 255);
      matriz.setPixelColor(matrizLeds[fila][i + 1], 0, 0, 255);
      matriz.setPixelColor(matrizLeds[fila][i], 0, 0, 255);

      matrizPos[fila][i + 3] = 0;
      matrizPos[fila][i + 2] = 1;
      matrizPos[fila][i + 1] = 1;
      matrizPos[fila][i] = 1;
    }
    else if (luces == 2)
    {
      matriz.setPixelColor(matrizLeds[fila][i + 2], 0, 0, 0);
      matriz.setPixelColor(matrizLeds[fila][i + 1], 0, 0, 255);
      matriz.setPixelColor(matrizLeds[fila][i], 0, 0, 255);

      matrizPos[fila][i + 2] = 0;
      matrizPos[fila][i + 1] = 1;
      matrizPos[fila][i] = 1;
    }
    else if (luces == 1)
    {
      matriz.setPixelColor(matrizLeds[fila][i + 1], 0, 0, 0);
      matriz.setPixelColor(matrizLeds[fila][i], 0, 0, 255);

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

void reiniciarStacker()
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
}

void ISR_Animacion()
{
  if (alternar == 0)
  {
    matriz.clear();
    for (int f = 0; f < 87; f = f + 2)
    {
      matriz.setPixelColor(f, 0, 0, 255);
    }
    for (int f = 254; f > 167; f = f - 2)
    {
      matriz.setPixelColor(f, 0, 0, 255);
    }
    alternar = 1;
  }
  else
  {
    matriz.clear();
    for (int f = 1; f < 88; f = f + 2)
    {
      matriz.setPixelColor(f, 0, 0, 255);
    }
    for (int f = 255; f > 168; f = f - 2)
    {
      matriz.setPixelColor(f, 0, 0, 255);
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

void InterruptEmpezar()
{
  noInterrupts();
  if (millis() - antiRebotes > timeThreshold) //pongo los valores en la matriz
  {
    matriz.clear();
    matriz.show();
    detachInterrupt(digitalPinToInterrupt(intPin1));
    attachInterrupt(digitalPinToInterrupt(intPin1), pararFila, FALLING); //FALLING
    if (lvl == 1 || lvl == 2)
    {
      VelocidadStack = 200;
    }
    else
    {
      VelocidadStack = 180;
    }
    estado = 2;
    antiRebotes = millis();
  }
  interrupts();
}

void animacionGanadoraStacker()
{
  noInterrupts();
  alternar = 0;
  for (int d = 0; d < 8; d++)
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
          matriz.setPixelColor(matrizLeds[a][b], 0, 0, 255);
        }
      }
    }
    matriz.show();
    delay(250);
    if (alternar == 0)
    {
      alternar = 1;
    }
    else {
      alternar = 0;
    }
  }
  interrupts();
  reiniciarStacker();
}

void cambiarDificultad()
{
  noInterrupts();
  if (millis() - antiRebotes > timeThreshold) //pongo los valores en la matriz
  {
    if (lvl == 1)
    {
      lvl = 2;
    }
    else if (lvl == 2)
    {
      lvl = 3;
    }
    else if (lvl == 3)
    {
      lvl = 1;
    }
    antiRebotes = millis();
  }
  interrupts();
}

void Stacker()
{
  switch (estado)
  {
    case CONFIGURACION:
      detachInterrupt(digitalPinToInterrupt(intPin1));
      attachInterrupt(digitalPinToInterrupt(intPin1), InterruptEmpezar, FALLING);
      attachInterrupt(digitalPinToInterrupt(intPin2), cambiarDificultad, FALLING);
      velocidadStack = 200;
      estado = ESTADO_INICIAL;
      break;

    case ESTADO_INICIAL:
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
