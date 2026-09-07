

 **********************CODIGO FONTE RECEPTOR ESP32 COM SOM REALISTA PARA CONTROLE PS4 SEM FIO**********************



// Importando Bibliotecas
#include <Bluepad32.h>   // (você deve incluir) Biblioteca com as funções do controle (Varios Modelos de GamePads)
#include <ESP32Servo.h>  // (você deve incluir) Biblioteca com as funções de controle do Servo Motor no ESP32, fundamental para o funcionamento do código.


// Bibliotecas Já disponíveis no IDE do Arduino
#include "driver/mcpwm.h"  // for servo PWM output
#include "rom/rtc.h"       // for displaying reset reason
#include "soc/rtc_wdt.h"   // for watchdog timer
#include <Esp.h>           // for displaying memory information
#include <ESP32Servo.h>    // Biblioteca com as funções de controle do Servo Motor no ESP32, fundamental para o funcionamento do código.


// Adicionando quias com configurações específicas
#include "1_Vehicle.h"       // <<------- Select the vehicle you want to simulate
#include "3_ESC.h"           // <<------- ESC related adjustments
#include "4_Transmission.h"  // <<------- Transmission related adjustments

// Incluindo sket com configurções adicionais
#include "src/curves.h"  // Nonlinear throttle curve arrays

// This stuff is required for Visual Studio Code IDE, if .ino is renamed into .cpp!
void Task1code(void *parameters);
void processRawChannels();


//**********************************************************************************************************************
//***ATENÇÃO************************************************************************************************************
//**********************************************************************************************************************

// AJUSTE A LEITURA DA TENSÃO DA BATERIA E O NÚMERO DE BATERIAS QUE VAI USAR
#define Numero_de_Baterias 1  // Use o número 1 se usar apenas uma bateria de 4,2v e 2 se usar duas baterias em série 8.4v
#define AtivaLeituraTensao 0  // Ativa e Desativa Leitura de Tensão (1 para Ativado e 0 para Desativado)


// AJUSTE A SUAVIDADE DE RESPOSTAS DOS SERVOS AO EXECUTAR UM COMANDO NO CONTROLE
int VelServoDirecao = 1;  // VALORES DE 0 a 10 AJUSTA VELOCIDADE DO SERVO AUX01 (0 MAIS RÁPIDO E 10 MAIS LENTO) PADRÃO 5
int VelSerAux01 = 5;      // VALORES DE 0 a 20 AJUSTA VELOCIDADE DO SERVO AUX01 (0 MAIS RÁPIDO E 20 MAIS LENTO) PADRÃO 5
int VelSerAux02 = 5;      // VALORES DE 0 a 20 AJUSTA VELOCIDADE DO SERVO AUX02 (0 MAIS RÁPIDO E 20 MAIS LENTO) PADRÃO 5

// AJUSTE ATIVA E DESATIVA A SIMULAÇÃO DE FREIO AO VOLTAR O JOYSTICK DE ACELERAÇÃO PARA POSIÇÃO CENTRAL
#define AtivaSimulaFreio 1  //( 1 ATIVA, 0 DESATIVA) SE DEIXAR ATIVO IRÁ FREAR, ACENDER A LUZ DE FREIO POR 1 SEGUNDO E ATIVAR O SOM DO ESCAPE DO AR DO FREIO SEMPRE QUE O BASTÃO DO JOYSTICK VOLTAR PARA POSIÇÃO CENTRAL

///////////////////////////////////////

// AJUSTE - Configurações para o Motor de Vibração da Cabine (simulando vibrações do motor)
const uint8_t VibracaoPartida = 120;          // Potência do agitador durante a partida do motor (máx. 255, cerca de 100)
const uint8_t VibracaoMarchaLenta = 55;       // Potência do agitador durante a marcha lenta (máx. 255, cerca de 49)
const uint8_t VibracaoAceleracaoMaxima = 60;  // Potência do agitador durante aceleração total (máx. 255, cerca de 40)
const uint8_t VibracaoDesligar = 70;          // Potência do agitador durante a parada do motor (máx. 255, cerca de 60)

// LEITURA DA BATERIA
// AJUSTE A CALIBRAÇÃO DA LEITURA DE TENSÃO DA BATERIA ** CONFIRA NA AULA 10.4
float calibration = 0.00;  // Confira tensão da bateria com o Multímetro e adiciona o valor da diferença entre a tensão lida pelo ESP32 E PELO MULTIMETRO

float voltage;
int bat_percentage;
int sensorValue;
uint32_t Millis_Alerta_Bateria;

// SISTEMA ANTI FALHAS
// ATIVE, DESATIVE E AJUSTE O TEMPO EM CICLOS QUE A MINIATURA VAI CONTINUAR COM OS MOTORES ATIVOS CASO TENHA UMA PERCA DO SINAL DO CONTROLE REMOTO.
// ESSE TEMPO NÃO DEVE SER MENOR QUE 10000 CICLOS
bool AtivaSistemaAnteFalhas = 1;  // 1 PARA ATIVAR E 0 PARA DESATIVAR
int TempoSemSinal = 50000;        // PADRÃO 50000 - USE VALORES DE 10000 A 100000 CICLOS


//********************************************************************************
// Declarando variáveis globais que irão receber os valores lidos dos botões e joysticks do controle de PS3

boolean ps = 0;
boolean options = 0;
boolean share = 0;

boolean x = 0;  // (Buzina)
boolean quadrado = 0;
boolean triangulo = 0;
boolean circulo = 0;

boolean setaCima = 0;
boolean setaBaixo = 0;
boolean setaDireita = 0;
boolean setaEsquerda = 0;

boolean l1 = 0;
boolean l2 = 0;

boolean r1 = 0;
boolean r2 = 0;

int32_t GD = 128;  // r2
int32_t GE = 128;  // l2

boolean r3 = 0;  // r3 Botão do Joystick Direito
boolean l3 = 1;  // l3 Botão do Joystick Esquerdo

int joyDireitaX = 128;
int joyDireitaY = 128;
int joyEsquerdaX = 128;
int joyEsquerdaY = 128;

// Declarando variáveis globais que irão auxiliar no tratamenteo dos dados lidos dos botões e joysticks do controle de PS3

boolean Auxps = 0;
boolean Auxoptions = 0;
boolean Auxshare = 0;

int AUXx = 0;
boolean Auxquadrado = 0;
boolean Auxtriangulo = 0;
boolean Auxcirculo = 0;

boolean AuxsetaCima = 0;
boolean AuxsetaBaixo = 0;
boolean AuxsetaDireita = 0;
boolean AuxsetaEsquerda = 0;

boolean Auxl1 = 0;
boolean Auxr1 = 0;
boolean Auxr2 = 0;
boolean Auxl2 = 0;

boolean Auxr3 = 0;  // r3
boolean Auxl3 = 0;  // l3


GamepadPtr myGamepads[1];

bool Conectado = false;
// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedGamepad(GamepadPtr gp) {
  if (myGamepads[0] == nullptr) {
    Serial.println("");
    Serial.printf(" => Controle Conectado com Sucesso!");
    Serial.println("");
    GamepadProperties properties = gp->getProperties();
    Serial.printf(" => Modelo do Controle: %s, VID=0x%04x, PID=0x%04x\n", gp->getModelName().c_str(), properties.vendor_id, properties.product_id);
    Serial.println("");
    myGamepads[0] = gp;
  }
  Conectado = true;
}

void onDisconnectedGamepad(GamepadPtr gp) {
  bool foundGamepad = false;
  Conectado = false;

  if (myGamepads[0] == gp) {
    Serial.printf(" => XXX Controle Desconectado! XXX");
    Serial.println("");
    myGamepads[0] = nullptr;
  }
}

// Variáveis para os Sons
bool SomPartida = false;
bool SomBuzina = false;
bool SomSirene = false;
bool SomSeta = false;


// Volume
// Crawler mode
const uint8_t masterVolumeCrawlerThreshold = 100;  // Se o volume mestre for <= este limite, o modo crawler (sem inércia virtual) estará ativo

// Channels signal range calibration -----
const uint16_t pulseNeutral = 30;
const uint16_t pulseSpan = 480;


// Instânciando objetos do tipo Servo para controle do Servo Motor de direção e auxiliares
Servo Bug;
Servo servoDirecao;  // Servo de direção
Servo servoAux1;     // Servo auxiliar 01 (Use em qualquer função do seu modelo)
Servo servoAux2;     // Servo auxiliar 02 (Use em qualquer função do seu modelo)



// DEFININDO NOMES PARA OS GPIOs USADOS NESTE CÓDIGO
#define BAT_Pin 34
#define FAROL 15
#define FAROL_MILHA 02
#define LUZ_FREIO 04
#define SETA_DIREITA 16
#define SETA_ESQUERDA 17
#define GIROFLEX_VERMEHO 05
#define GIROFLEX_AZUL 18
#define LUZ_AUX01 19
#define LUZ_AUX02 21
#define LUZ_AUX03 03
#define LUZ_AUX04 01
#define LUZ_RE 22
#define MOTOR_VIBRACAO 23

#define SERVO_DIRECAO 13
#define SERVO_AUX01 12
#define SERVO_AUX02 14
#define PONTE_H_IN01 27
#define PONTE_H_IN02 32
#define ESC_PIN 33


#define DAC1 25  // AUDIO
#define DAC2 26  // AUDIO


int ConteGiroflex = 0;
int Farol = 0;

// Variáveis globais para auxiliar na contagem de tempo em millis
uint32_t Millis_PiscaAlerta, Millis_SetaDireita, Millis_SetaEsquerda, Millis_Giroflex, Millis_LuzRe, Millis_ServoAux02, Millis_ServoAux01, Millis_ServoDirecao, Millis_PerdeSinal, Sinal_LuzFreio, Millis_Desconectado2, Millis_Desconectado;

boolean ReESC = 1;
int Volume = 3;
bool Incio = false;
int Cont = 0;


//======================================================================================
// Declarando outras variáveis globais que irão auxiliar na implementação das diversas funções do código
//boolean x = 0;  // (Buzina)
boolean Re = 0;
boolean PiscaAlertaOFF = LOW;
boolean SinalSeta = 0;
boolean SinalPiscaPercaSinal = LOW;
boolean Freio = false;
boolean FreioReal = false;

//int Farol = 0;
//int Giroflex = 0;
int Buzina = 1500;
int ServoAux01 = 0;
int ServoAux02 = 0;
int PWM = 0;
int AuxSomSeta = 1500;
int AuxjoyDireitaX = 1500;
int Liga_Desliga = 1500;
int Cont_Liga_Desliga = 0;
int VolumeMaster = 100;
int Velocidade = 3;


int GrauServo = 90;  // variável para receber a conversão dos valores dos joysticks compatíveis com os valores dos servos
int AuxGrauServo = 90;


bool sinalDesconectado = false;

///////////////////////////////////////////////////////////////


// Interrupt latches
//volatile boolean couplerSwitchInteruptLatch;  // this is enabled, if the coupler switch pin change interrupt is detected

// Control input signals
#define PULSE_ARRAY_SIZE 14                 // 13 channels (+ the unused CH0)
uint16_t pulseWidthRaw[PULSE_ARRAY_SIZE];   // Current RC signal RAW pulse width [X] = channel number
uint16_t pulseWidthRaw2[PULSE_ARRAY_SIZE];  // Current RC signal RAW pulse width with linearity compensation [X] = channel number
uint16_t pulseWidthRaw3[PULSE_ARRAY_SIZE];  // Current RC signal RAW pulse width before averaging [X] = channel number
uint16_t pulseWidth[PULSE_ARRAY_SIZE];      // Current RC signal pulse width [X] = channel number
int16_t pulseOffset[PULSE_ARRAY_SIZE];      // Offset for auto zero adjustment

uint16_t pulseMaxNeutral[PULSE_ARRAY_SIZE];  // PWM input signal configuration storage variables
uint16_t pulseMinNeutral[PULSE_ARRAY_SIZE];
uint16_t pulseMax[PULSE_ARRAY_SIZE];
uint16_t pulseMin[PULSE_ARRAY_SIZE];
uint16_t pulseMaxLimit[PULSE_ARRAY_SIZE];
uint16_t pulseMinLimit[PULSE_ARRAY_SIZE];

uint16_t pulseZero[PULSE_ARRAY_SIZE];  // Usually 1500 (The mid point of 1000 - 2000 Microseconds)
uint16_t pulseLimit = 1100;            // pulseZero +/- this value (1100)
uint16_t pulseMinValid = 700;          // The minimum valid pulsewidth (was 950)
uint16_t pulseMaxValid = 2300;         // The maximum valid pulsewidth (was 2050)
bool autoZeroDone;                     // Auto zero offset calibration done
#define NONE 16                        // The non existing "Dummy" channel number (usually 16) TODO

volatile boolean failSafe = false;  // Triggered in emergency situations like: throttle signal lost etc.

boolean mode1;  // Signal state variables
boolean mode2;
boolean momentary1;
boolean hazard;
boolean left;
boolean right;
boolean unlock5thWheel;

boolean winchEnabled;

// Sound
volatile boolean engineOn = false;                 // Signal for engine on / off
volatile boolean engineStart = false;              // Active, if engine is starting up
volatile boolean engineRunning = false;            // Active, if engine is running
volatile boolean engineStop = false;               // Active, if engine is shutting down
volatile boolean jakeBrakeRequest = false;         // Active, if engine jake braking is requested
volatile boolean engineJakeBraking = false;        // Active, if engine is jake braking
volatile boolean wastegateTrigger = false;         // Trigger wastegate (blowoff) after rapid throttle drop
volatile boolean blowoffTrigger = false;           // Trigger jake brake sound (blowoff) after rapid throttle drop
volatile boolean dieselKnockTrigger = false;       // Trigger Diesel ignition "knock"
volatile boolean dieselKnockTriggerFirst = false;  // The first Diesel ignition "knock" per sequence
volatile boolean airBrakeTrigger = false;          // Trigger for air brake noise
volatile boolean parkingBrakeTrigger = false;      // Trigger for air parking brake noise
volatile boolean shiftingTrigger = false;          // Trigger for shifting noise
volatile boolean hornTrigger = false;              // Trigger for horn on / off
volatile boolean sirenTrigger = false;             // Trigger for siren  on / off
volatile boolean sound1trigger = false;            // Trigger for sound1  on / off
volatile boolean couplingTrigger = false;          // Trigger for trailer coupling  sound
volatile boolean uncouplingTrigger = false;        // Trigger for trailer uncoupling  sound
volatile boolean bucketRattleTrigger = false;      // Trigger for bucket rattling  sound
volatile boolean indicatorSoundOn = false;         // active, if indicator bulb is on
// Mensagem de falta de combustível
volatile boolean outOfFuelMessageTrigger = false;  // Trigger for out of fuel message
volatile boolean Bateria_10_Trigger = false;       // Acionar mensagem de falta de combustível

// Sound latches
volatile boolean hornLatch = false;   // Horn latch bit
volatile boolean sirenLatch = false;  // Siren latch bit

// Sound volumes
volatile uint16_t throttleDependentVolume = 0;         // engine volume according to throttle position
volatile uint16_t throttleDependentRevVolume = 0;      // engine rev volume according to throttle position
volatile uint16_t rpmDependentJakeBrakeVolume = 0;     // Engine rpm dependent jake brake volume
volatile uint16_t throttleDependentKnockVolume = 0;    // engine Diesel knock volume according to throttle position
volatile uint16_t rpmDependentKnockVolume = 0;         // engine Diesel knock volume according to engine RPM
volatile uint16_t throttleDependentTurboVolume = 0;    // turbo volume according to rpm
volatile uint16_t throttleDependentFanVolume = 0;      // cooling fan volume according to rpm
volatile uint16_t throttleDependentChargerVolume = 0;  // cooling fan volume according to rpm
volatile uint16_t rpmDependentWastegateVolume = 0;     // wastegate volume according to rpm
volatile uint16_t tireSquealVolume = 0;                // Tire squeal volume according to speed and cornering radius
// for excavator mode:
volatile uint16_t hydraulicPumpVolume = 0;              // hydraulic pump volume
volatile uint16_t hydraulicFlowVolume = 0;              // hydraulic flow volume
volatile uint16_t trackRattleVolume = 0;                // track rattling volume
volatile uint16_t hydraulicDependentKnockVolume = 100;  // engine Diesel knock volume according to hydraulic load
volatile uint16_t hydraulicLoad = 0;                    // Hydraulic load dependent RPM drop

volatile uint64_t dacDebug = 0;  // DAC debug variable TODO

volatile int16_t masterVolume = 100;  // Master volume percentage
volatile uint8_t dacOffset = 0;       // 128, but needs to be ramped up slowly to prevent popping noise, if switched on

// Throttle
int16_t currentThrottle = 0;       // 0 - 500 (Throttle trigger input)
int16_t currentThrottleFaded = 0;  // faded throttle for volume calculations etc.

// Engine
const int16_t maxRpm = 500;        // always 500
const int16_t minRpm = 0;          // always 0
int32_t currentRpm = 0;            // 0 - 500 (signed required!)
volatile uint8_t engineState = 0;  // Engine state
enum EngineState                   // Engine state enum
{
  OFF,       // Engine is off
  STARTING,  // Engine is starting
  RUNNING,   // Engine is running
  STOPPING,  // Engine is stopping
  PARKING_BRAKE
};
int16_t engineLoad = 0;                  // 0 - 500
volatile uint16_t engineSampleRate = 0;  // Engine sample rate
int32_t speedLimit = maxRpm;             // The speed limit, depending on selected virtual gear

// Clutch
boolean clutchDisengaged = true;  // Active while clutch is disengaged

// Transmission
uint8_t selectedGear = 1;              // The currently used gear of our shifting gearbox
uint8_t selectedAutomaticGear = 1;     // The currently used gear of our automatic gearbox
boolean gearUpShiftingInProgress;      // Active while shifting upwards
boolean doubleClutchInProgress;        // Double-clutch (Zwischengas)
boolean gearDownShiftingInProgress;    // Active while shifting downwards
boolean gearUpShiftingPulse;           // Active, if shifting upwards begins
boolean gearDownShiftingPulse;         // Active, if shifting downwards begins
volatile boolean neutralGear = false;  // Transmission in neutral
boolean lowRange = false;              // Transmission range (off road reducer)

// ESC
volatile boolean escIsBraking = false;  // ESC is in a braking state
volatile boolean escIsDriving = false;  // ESC is in a driving state
volatile boolean escInReverse = false;  // ESC is driving or braking backwards
volatile boolean brakeDetect = false;   // Additional brake detect signal, enabled immediately, if brake applied
int8_t driveState = 0;                  // for ESC state machine
uint16_t escPulseMax = 2000;            // ESC calibration variables (values will be changed later)
uint16_t escPulseMin = 1000;
uint16_t escPulseMaxNeutral = 1500;
uint16_t escPulseMinNeutral = 1500;
uint16_t currentSpeed = 0;          // 0 - 500 (current ESC power)
volatile bool crawlerMode = false;  // Crawler mode intended for crawling competitons (withouth sound and virtual inertia)

// Lights
int8_t lightsState = 0;                         // for lights state machine
volatile boolean lightsOn = false;              // Lights on
volatile boolean headLightsFlasherOn = false;   // Headlights flasher impulse (Lichthupe)
volatile boolean headLightsHighBeamOn = false;  // Headlights high beam (Fernlicht)
volatile boolean blueLightTrigger = false;      // Bluelight on (Blaulicht)
boolean indicatorLon = false;                   // Left indicator (Blinker links)
boolean indicatorRon = false;                   // Right indicator (Blinker rechts)
boolean fogLightOn = false;                     // Fog light is on
boolean cannonFlash = false;                    // Flashing cannon fire


// DEBUG stuff
volatile uint8_t coreId = 99;

// Our main tasks
TaskHandle_t Task1;

// Loop time (for debug)
uint16_t loopTime;

// Sampling intervals for interrupt timer (adjusted according to your sound file sampling rate)
uint32_t maxSampleInterval = 4000000 / sampleRate;
uint32_t minSampleInterval = 4000000 / sampleRate * 100 / MAX_RPM_PERCENTAGE;

// Interrupt timer for variable sample rate playback (engine sound)
hw_timer_t *variableTimer = NULL;
portMUX_TYPE variableTimerMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t variableTimerTicks = maxSampleInterval;

// Interrupt timer for fixed sample rate playback (horn etc., playing in parallel with engine sound)
hw_timer_t *fixedTimer = NULL;
portMUX_TYPE fixedTimerMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t fixedTimerTicks = maxSampleInterval;

// Declare a mutex Semaphore Handles.
// It will be used to ensure only only one Task is accessing this resource at any time.
SemaphoreHandle_t xPwmSemaphore;
SemaphoreHandle_t xRpmSemaphore;

// These are used to print the reset reason on startup
const char *RESET_REASONS[] = { "POWERON_RESET", "NO_REASON", "SW_RESET", "OWDT_RESET", "DEEPSLEEP_RESET", "SDIO_RESET", "TG0WDT_SYS_RESET", "TG1WDT_SYS_RESET", "RTCWDT_SYS_RESET", "INTRUSION_RESET", "TGWDT_CPU_RESET", "SW_CPU_RESET", "RTCWDT_CPU_RESET", "EXT_CPU_RESET", "RTCWDT_BROWN_OUT_RESET", "RTCWDT_RTC_RESET" };

// Convert µs to degrees (°)
float us2degree(uint16_t value) {
  return (value - 500) / 11.111 - 90.0;
}

//
// =======================================================================================================
// INTERRUPT FOR VARIABLE SPEED PLAYBACK (Engine sound, turbo sound)
// =======================================================================================================
//

void IRAM_ATTR variablePlaybackTimer() {

  // coreId = xPortGetCoreID(); // Running on core 1

  static uint32_t attenuatorMillis = 0;
  static uint32_t curEngineSample = 0;         // Index of currently loaded engine sample
  static uint32_t curRevSample = 0;            // Index of currently loaded engine rev sample
  static uint32_t curTurboSample = 0;          // Index of currently loaded turbo sample
  static uint32_t curFanSample = 0;            // Index of currently loaded fan sample
  static uint32_t curChargerSample = 0;        // Index of currently loaded charger sample
  static uint32_t curStartSample = 0;          // Index of currently loaded start sample
  static uint32_t curJakeBrakeSample = 0;      // Index of currently loaded jake brake sample
  static uint32_t curHydraulicPumpSample = 0;  // Index of currently loaded hydraulic pump sample
  static uint32_t curTrackRattleSample = 0;    // Index of currently loaded train track rattle sample
  static uint32_t lastDieselKnockSample = 0;   // Index of last Diesel knock sample
  static uint16_t attenuator = 0;              // Used for volume adjustment during engine switch off
  static uint16_t speedPercentage = 0;         // slows the engine down during shutdown

  // portENTER_CRITICAL_ISR(&variableTimerMux); // disables C callable interrupts (on the current core) and locks the mutex by the current core.
  int32_t soundVal = 0;

  switch (engineState) {

    case OFF:                                                    // Engine off -----------------------------------------------------------------------
      variableTimerTicks = 4000000 / startSampleRate;            // our fixed sampling rate
      timerAlarmWrite(variableTimer, variableTimerTicks, true);  // // change timer ticks, autoreload true

      if (engineOn) {
        engineState = STARTING;
        engineStart = true;
      }
      break;

    case STARTING:                                               // Engine start --------------------------------------------------------------------
      variableTimerTicks = 4000000 / startSampleRate;            // our fixed sampling rate
      timerAlarmWrite(variableTimer, variableTimerTicks, true);  // // change timer ticks, autoreload true

      if (curStartSample < startSampleCount - 1) {
#if defined STEAM_LOCOMOTIVE_MODE
        soundVal += (startSamples[curStartSample] * startVolumePercentage / 100);
#else
        soundVal += (startSamples[curStartSample] * throttleDependentVolume / 100 * startVolumePercentage / 100);
#endif
        curStartSample++;
      } else {
        curStartSample = 0;
        engineState = RUNNING;
        engineStart = false;
        engineRunning = true;
        airBrakeTrigger = true;
      }
      break;

    case RUNNING:  // Engine running ------------------------------------------------------------------
      {

        // different sounds that are mixed together
        int32_t idleVal = 0;
        int32_t revVal = 0;
        int32_t brakeVal = 0;

        // Engine idle & revving sounds (mixed together according to engine rpm, new in v5.0)
        variableTimerTicks = engineSampleRate;                     // our variable idle sampling rate!
        timerAlarmWrite(variableTimer, variableTimerTicks, true);  // // change timer ticks, autoreload true

        if (!engineJakeBraking && !blowoffTrigger) {
          if (curEngineSample < sampleCount - 1) {
            idleVal = (samples[curEngineSample] * throttleDependentVolume / 100 * idleVolumePercentage / 100);  // Idle sound
            curEngineSample++;

            // Optional rev sound, recorded at medium rpm. Note, that it needs to represent the same number of ignition cycles as the
            // idle sound. For example 4 or 8 for a V8 engine. It also needs to have about the same length. In order to adjust the length
            // or "revSampleCount", change the "Rate" setting in Audacity until it is about the same.
#ifdef REV_SOUND
            revVal = (revSamples[curRevSample] * throttleDependentRevVolume / 100 * revVolumePercentage / 100);  // Rev sound
            if (curRevSample < revSampleCount)
              curRevSample++;
#endif

            // Trigger throttle dependent Diesel ignition "knock" sound (played in the fixed sample rate interrupt)
            if (curEngineSample - lastDieselKnockSample > (sampleCount / dieselKnockInterval)) {
              dieselKnockTrigger = true;
              dieselKnockTriggerFirst = false;
              lastDieselKnockSample = curEngineSample;
            }
          } else {
            curEngineSample = 0;
            if (jakeBrakeRequest)
              engineJakeBraking = true;
#ifdef REV_SOUND
            curRevSample = 0;
#endif
            lastDieselKnockSample = 0;
            dieselKnockTrigger = true;
            dieselKnockTriggerFirst = true;
          }
          curJakeBrakeSample = 0;
        } else {  // Jake brake sound ----
#ifdef JAKE_BRAKE_SOUND
          brakeVal = (jakeBrakeSamples[curJakeBrakeSample] * rpmDependentJakeBrakeVolume / 100 * jakeBrakeVolumePercentage / 100);  // Jake brake sound
          if (curJakeBrakeSample < jakeBrakeSampleCount - 1)
            curJakeBrakeSample++;
          else {
            curJakeBrakeSample = 0;
            if (!jakeBrakeRequest)
              engineJakeBraking = false;
          }

          curEngineSample = 0;
          curRevSample = 0;
#endif
        }

        // Engine sound mixer ----
#ifdef REV_SOUND
        // Mixing the idle and rev sounds together, according to engine rpm
        // Below the "revSwitchPoint" target, the idle volume precentage is 90%, then falling to 0% @ max. rpm.
        // The total of idle and rev volume percentage is always 100%

        int32_t mixer;
        if (currentRpm > revSwitchPoint)
          mixer = map(currentRpm, idleEndPoint, revSwitchPoint, 0, idleVolumeProportionPercentage);
        else
          mixer = idleVolumeProportionPercentage;  // 90 - 100% proportion
        if (currentRpm > idleEndPoint)
          mixer = 0;

        idleVal = (idleVal * mixer) / 100;        // Idle volume
        revVal = (revVal * (100 - mixer)) / 100;  // Rev volume
#endif

        soundVal += idleVal + revVal + brakeVal;

        // Turbo sound ----------------------------------
        if (curTurboSample >= turboSampleCount) {
          curTurboSample = 0;
        }
        soundVal += (turboSamples[curTurboSample] * throttleDependentTurboVolume / 100 * turboVolumePercentage / 100);
        curTurboSample++;

        // Fan sound / gearbox whining --------------------
#if defined GEARBOX_WHINING
        // used for gearbox whining simulation, so not active in gearbox neutral
        if (!neutralGear) {
#endif
          if (curFanSample >= fanSampleCount) {
            curFanSample = 0;
          }
          soundVal += (fanSamples[curFanSample] * throttleDependentFanVolume / 100 * fanVolumePercentage / 100);
          curFanSample++;
#if defined GEARBOX_WHINING
        }
#endif

        // Supercharger sound --------------------------
        if (curChargerSample >= chargerSampleCount) {
          curChargerSample = 0;
        }
        soundVal += (chargerSamples[curChargerSample] * throttleDependentChargerVolume / 100 * chargerVolumePercentage / 100);
        curChargerSample++;

        // Hydraulic pump sound -----------------------
#if defined EXCAVATOR_MODE
        if (curHydraulicPumpSample >= hydraulicPumpSampleCount) {
          curHydraulicPumpSample = 0;
        }
        soundVal += (hydraulicPumpSamples[curHydraulicPumpSample] * hydraulicPumpVolumePercentage / 100 * hydraulicPumpVolume / 100);
        curHydraulicPumpSample++;
#endif

#if defined STEAM_LOCOMOTIVE_MODE
        // Track rattle sound -----------------------
        if (curTrackRattleSample < trackRattleSampleCount - 1) {
          curTrackRattleSample = 0;
        }
        soundVal += (trackRattleSamples[curTrackRattleSample] * trackRattleVolumePercentage / 100 * trackRattleVolume / 100);
        curTrackRattleSample++;
#endif

        if (!engineOn) {
          speedPercentage = 100;
          attenuator = 1;
          engineState = STOPPING;
          engineStop = true;
          engineRunning = false;
        }
        break;
      }
    case STOPPING:                                                        // Engine stop --------------------------------------------------------------------
      variableTimerTicks = 4000000 / sampleRate * speedPercentage / 100;  // our fixed sampling rate
      timerAlarmWrite(variableTimer, variableTimerTicks, true);           // // change timer ticks, autoreload true

      if (curEngineSample >= sampleCount - 1) {
        curEngineSample = 0;
      }
      soundVal += (samples[curEngineSample] * throttleDependentVolume / 100 * idleVolumePercentage / 100 / attenuator);
      curEngineSample++;

      // fade engine sound out
      if (millis() - attenuatorMillis > 100) {  // Every 50ms
        attenuatorMillis = millis();
        attenuator++;           // attenuate volume
        speedPercentage += 20;  // make it slower (10)
      }

      if (attenuator >= 50 || speedPercentage >= 500) {  // 50 & 500
        speedPercentage = 100;
        parkingBrakeTrigger = true;
        engineState = PARKING_BRAKE;
        engineStop = false;
      }
      break;

    case PARKING_BRAKE:  // parking brake bleeding air sound after engine is off ----------------------------

      if (!parkingBrakeTrigger) {
        engineState = OFF;
      }
      break;

  }  // end of switch case


  uint8_t value = constrain(soundVal * masterVolume / 100 + dacOffset, 0, 255);

  SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, value, RTC_IO_PDAC1_DAC_S);

  // portEXIT_CRITICAL_ISR(&variableTimerMux);
}

//
// =======================================================================================================
// INTERRUPT FOR FIXED SPEED PLAYBACK (Horn etc., played in parallel with engine sound)
// =======================================================================================================
//

void IRAM_ATTR fixedPlaybackTimer() {
  Cont++;
  // coreId = xPortGetCoreID(); // Running on core 1

  static uint32_t curHornSample = 0;           // Index of currently loaded horn sample
  static uint32_t curSirenSample = 0;          // Index of currently loaded siren sample
  static uint32_t curSound1Sample = 0;         // Index of currently loaded sound1 sample
  static uint32_t curReversingSample = 0;      // Index of currently loaded reversing beep sample
  static uint32_t curIndicatorSample = 0;      // Index of currently loaded indicator tick sample
  static uint32_t curWastegateSample = 0;      // Index of currently loaded wastegate sample
  static uint32_t curBrakeSample = 0;          // Index of currently loaded brake sound sample
  static uint32_t curParkingBrakeSample = 0;   // Index of currently loaded brake sound sample
  static uint32_t curShiftingSample = 0;       // Index of currently loaded shifting sample
  static uint32_t curDieselKnockSample = 0;    // Index of currently loaded Diesel knock sample
  static uint32_t curCouplingSample = 0;       // Index of currently loaded trailer coupling sample
  static uint32_t curUncouplingSample = 0;     // Index of currently loaded trailer uncoupling sample
  static uint32_t curHydraulicFlowSample = 0;  // Index of currently loaded hydraulic flow sample
  static uint32_t curTrackRattleSample = 0;    // Index of currently loaded track rattle sample
  static uint32_t curBucketRattleSample = 0;   // Index of currently loaded bucket rattle sample
  static uint32_t curTireSquealSample = 0;     // Index of currently loaded tire squeal sample
  static uint32_t curOutOfFuelSample = 0;      // Index of currently loaded out of fuel sample
  static boolean knockSilent = 0;              // This knock will be more silent
  static boolean knockMedium = 0;              // This knock will be medium
  static uint8_t curKnockCylinder = 0;         // Index of currently ignited zylinder

  // portENTER_CRITICAL_ISR(&fixedTimerMux);

  int32_t soundVal = 0;

  // horn *************************************************
  if (curHornSample >= hornSampleCount) {  // End of sample
    curHornSample = 0;
    hornLatch = false;
  }
  if (hornTrigger || hornLatch) {
    soundVal += (hornSamples[curHornSample] * hornVolumePercentage / 100);
    curHornSample++;
#ifdef HORN_LOOP  // Optional "endless loop" (points to be defined manually in horn file)
    if (hornTrigger && curHornSample == hornLoopEnd)
      curHornSample = hornLoopBegin;  // Loop, if trigger still present
#endif
  }

  // siren *************************************************
  if (curSirenSample >= sirenSampleCount) {  // End of sample
    curSirenSample = 0;
    sirenLatch = false;
  }
  if (sirenTrigger || sirenLatch) {
#if defined SIREN_STOP
    if (!sirenTrigger) {
      curSirenSample = 0;
      sirenLatch = false;
    }
#endif

    soundVal += (sirenSamples[curSirenSample] * sirenVolumePercentage / 100);
    curSirenSample++;
#ifdef SIREN_LOOP  // Optional "endless loop" (points to be defined manually in siren file)
    if (sirenTrigger && curSirenSample == sirenLoopEnd)
      curSirenSample = sirenLoopBegin;  // Loop, if trigger still present
#endif
  }


  // other sounds *********************************************

  if (curSound1Sample < sound1SampleCount) {
    curSound1Sample = 0;  // ensure, next sound will start @ first sample
  }
  if (sound1trigger) {
    soundVal += (sound1Samples[curSound1Sample] * sound1VolumePercentage / 100);
    curSound1Sample++;
    if (curSound1Sample == sound1SampleCount) {
      sound1trigger = false;
      curSound1Sample = 0;  // ensure, next sound will start @ first sample
    }
  }

  // Reversing beep sound "b1" ----
  if (curReversingSample >= reversingSampleCount) {
    curReversingSample = 0;
  }
  if (engineRunning && escInReverse) {
    soundVal += (reversingSamples[curReversingSample] * reversingVolumePercentage / 100);
    curReversingSample++;
  } else {
    curReversingSample = 0;  // ensure, next sound will start @ first sample
  }

  // Indicator tick sound ------------------------------------------
#if not defined NO_INDICATOR_SOUND

  if (indicatorSoundOn) {

    if (curIndicatorSample >= indicatorSampleCount) {
      curIndicatorSample = 0;
    }
    soundVal += (indicatorSamples[curIndicatorSample] * indicatorVolumePercentage / 100);
    curIndicatorSample++;
    if (curIndicatorSample == indicatorSampleCount - 400) {
      indicatorSoundOn = false;
      curIndicatorSample = 0;  // ensure, next sound will start @ first sample
    }
  }
#endif

  // Wastegate (blowoff) sound, triggered after rapid throttle drop -----------------------------------
  if (curWastegateSample >= wastegateSampleCount) {
    curWastegateSample = 0;
  }
  if (wastegateTrigger) {
    soundVal += (wastegateSamples[curWastegateSample] * rpmDependentWastegateVolume / 100 * wastegateVolumePercentage / 100);
    curWastegateSample++;
    if (curWastegateSample == wastegateSampleCount) {
      wastegateTrigger = false;
      curWastegateSample = 0;  // ensure, next sound will start @ first sample
    }
  }

  // Air brake release sound, triggered after stop -----------------------------------------------
  if (curBrakeSample >= brakeSampleCount) {
    curBrakeSample = 0;
  }
  if (airBrakeTrigger) {
    soundVal += (brakeSamples[curBrakeSample] * brakeVolumePercentage / 100);
    curBrakeSample++;
    if (curBrakeSample == brakeSampleCount) {
      airBrakeTrigger = false;
      curBrakeSample = 0;  // ensure, next sound will start @ first sample
    }
  }

  // Air parking brake attaching sound, triggered after engine off --------------------------------
  if (curParkingBrakeSample >= parkingBrakeSampleCount) {
    curParkingBrakeSample = 0;
  }
  if (parkingBrakeTrigger) {
    soundVal += (parkingBrakeSamples[curParkingBrakeSample] * parkingBrakeVolumePercentage / 100);
    curParkingBrakeSample++;
    if (curParkingBrakeSample == parkingBrakeSampleCount) {
      parkingBrakeTrigger = false;
      curParkingBrakeSample = 0;  // ensure, next sound will start @ first sample
    }
  }

  // Pneumatic gear shifting sound, triggered while shifting the TAMIYA 3 speed transmission ------
  if (curShiftingSample >= shiftingSampleCount) {
    curShiftingSample = 0;
  }
  //if (shiftingTrigger && engineRunning && !automatic && !doubleClutch) {
  if (shiftingTrigger) {
    soundVal = (shiftingSamples[curShiftingSample] * shiftingVolumePercentage / 100);
    curShiftingSample++;
    if (curShiftingSample == shiftingSampleCount) {
      shiftingTrigger = false;
      curShiftingSample = 0;  // ensure, next sound will start @ first sample
    }
  }

  // Diesel ignition "knock" is played in fixed sample rate section, because we don't want changing pitch! ------
  if (dieselKnockTriggerFirst) {
    dieselKnockTriggerFirst = false;
    curKnockCylinder = 0;
  }

  if (dieselKnockTrigger) {
    dieselKnockTrigger = false;
    curKnockCylinder++;  // Count ignition sequence
    curDieselKnockSample = 0;
  }

#ifdef V8  // (former ADAPTIVE_KNOCK_VOLUME, rename it in your config file!)
  // Ford or Scania V8 ignition sequence: 1 - 5 - 4 - 2* - 6 - 3 - 7 - 8* (* = louder knock pulses, because 2nd exhaust in same manifold after 90°)
  if (curKnockCylinder == 4 || curKnockCylinder == 8)
    knockSilent = false;
  else
    knockSilent = true;
#endif

#ifdef V8_MEDIUM  // (former ADAPTIVE_KNOCK_VOLUME, rename it in your config file!)
  // This is EXPERIMENTAL!! TODO
  if (curKnockCylinder == 5 || curKnockCylinder == 1)
    knockMedium = false;
  else
    knockMedium = true;
#endif

#ifdef V8_468  // (Chevy 468, containing 16 ignition pulses)
  // 1th, 5th, 9th and 13th are the loudest
  // Ignition sequence: 1 - 8 - 4* - 3 - 6 - 5 - 7* - 2
  if (curKnockCylinder == 1 || curKnockCylinder == 5 || curKnockCylinder == 9 || curKnockCylinder == 13)
    knockSilent = false;
  else
    knockSilent = true;
#endif

#ifdef V2
  // V2 engine: 1st and 2nd knock pulses (of 4) will be louder
  if (curKnockCylinder == 1 || curKnockCylinder == 2)
    knockSilent = false;
  else
    knockSilent = true;
#endif

#ifdef R6
  // R6 inline 6 engine: 6th knock pulse (of 6) will be louder
  if (curKnockCylinder == 6)
    knockSilent = false;
  else
    knockSilent = true;
#endif

#ifdef R6_2
  // R6 inline 6 engine: 6th and 3rd knock pulse (of 6) will be louder
  if (curKnockCylinder == 6 || curKnockCylinder == 3)
    knockSilent = false;
  else
    knockSilent = true;
#endif
  // Modificado dieselVolume *= por dieselVolume =
  if (curDieselKnockSample < knockSampleCount) {
    // multiplier for volume (we divide by 100 at the end)
    int32_t dieselVolume = dieselKnockVolumePercentage;
    dieselVolume = throttleDependentKnockVolume;

#if defined RPM_DEPENDENT_KNOCK  // knock volume also depending on engine rpm
    dieselVolume = rpmDependentKnockVolume;
#elif defined EXCAVATOR_MODE  // knock volume also depending on hydraulic load
    dieselVolume = hydraulicDependentKnockVolume;
#else
    dieselVolume = 100;
#endif

    // changing knock volume according to engine type and cylinder!
    if (knockSilent && !knockMedium)
      dieselVolume = dieselKnockAdaptiveVolumePercentage / 100;
    if (knockMedium)
      dieselVolume = dieselKnockAdaptiveVolumePercentage / 75;

    soundVal += knockSamples[curDieselKnockSample] * dieselVolume / (100 * 100);
    curDieselKnockSample++;
  }

#if not defined EXCAVATOR_MODE
  // Trailer coupling sound, triggered by switch -----------------------------------------------
#ifdef COUPLING_SOUND
  if (curCouplingSample >= couplingSampleCount) {
    curCouplingSample = 0;
  }
  if (couplingTrigger) {
    soundVal += (couplingSamples[curCouplingSample] * couplingVolumePercentage / 100);
    curCouplingSample++;
    if (curCouplingSample == couplingSampleCount) {
      couplingTrigger = false;
      curCouplingSample = 0;  // ensure, next sound will start @ first sample
    }
  }

  // Trailer uncoupling sound, triggered by switch -----------------------------------------------
  if (curUncouplingSample >= uncouplingSampleCount) {
    curUncouplingSample = 0;
  }
  if (uncouplingTrigger) {
    soundVal += (uncouplingSamples[curUncouplingSample] * couplingVolumePercentage / 100);
    curUncouplingSample++;
    if (curUncouplingSample == uncouplingSampleCount) {
      uncouplingTrigger = false;
      curUncouplingSample = 0;
    }
  }
#endif
#endif

  // excavator sounds **************************************************

#if defined EXCAVATOR_MODE

  // Hydraulic fluid flow sound -----------------------
  if (curHydraulicFlowSample >= hydraulicFlowSampleCount) {
    curHydraulicFlowSample = 0;
  }
  soundVal += (hydraulicFlowSamples[curHydraulicFlowSample] * hydraulicFlowVolumePercentage / 100 * hydraulicFlowVolume / 100);
  curHydraulicFlowSample++;

  // Track rattle sound -----------------------
  if (curTrackRattleSample >= trackRattleSampleCount) {
    curTrackRattleSample = 0;
  }
  soundVal += (trackRattleSamples[curTrackRattleSample] * trackRattleVolumePercentage / 100 * trackRattleVolume / 100);
  curTrackRattleSample++;

  // Bucket rattle sound -----------------------
  if (curBucketRattleSample >= bucketRattleSampleCount) {
    curBucketRattleSample = 0;
  }
  if (bucketRattleTrigger) {
    soundVal += (bucketRattleSamples[curBucketRattleSample] * bucketRattleVolumePercentage / 100);
    curBucketRattleSample++;
    if (curBucketRattleSample == bucketRattleSampleCount) {
      bucketRattleTrigger = false;
      curBucketRattleSample = 0;  // ensure, next sound will start @ first sample
    }
  }
#endif

  // additional sounds *************************************************

#if defined TIRE_SQUEAL
  // Tire squeal sound -----------------------
  if (curTireSquealSample >= tireSquealSampleCount) {
    curTireSquealSample = 0;
  }
  soundVal += (tireSquealSamples[curTireSquealSample] * tireSquealVolumePercentage / 100 * tireSquealVolume / 100);
  curTireSquealSample++;
#endif



  // SOM BATERIA
  if (Bateria_10_Trigger) {


    if (AtivaLeituraTensao == 0) {

      if (curOutOfFuelSample >= SemBateriaSampleCount) {
        curOutOfFuelSample = 0;
      }
      soundVal += (SemBateriaSamples[curOutOfFuelSample] * outOfFuelVolumePercentage / 100);
      curOutOfFuelSample++;
      if (curOutOfFuelSample == SemBateriaSampleCount) {
        curOutOfFuelSample = 0;
        Bateria_10_Trigger = false;
      }

    } else {

      if (bat_percentage > 95) {

        if (curOutOfFuelSample >= Bateria100SampleCount) {
          curOutOfFuelSample = 0;
        }
        soundVal += (Bateria100Samples[curOutOfFuelSample] * outOfFuelVolumePercentage / 100);
        curOutOfFuelSample++;
        if (curOutOfFuelSample == Bateria100SampleCount) {
          curOutOfFuelSample = 0;
          Bateria_10_Trigger = false;
        }
      }


      if ((bat_percentage >= 90) && (bat_percentage <= 95)) {

        if (curOutOfFuelSample >= Bateria90SampleCount) {
          curOutOfFuelSample = 0;
        }
        soundVal += (Bateria90Samples[curOutOfFuelSample] * outOfFuelVolumePercentage / 100);
        curOutOfFuelSample++;
        if (curOutOfFuelSample == Bateria90SampleCount) {
          curOutOfFuelSample = 0;
          Bateria_10_Trigger = false;
        }
      }



      if ((bat_percentage >= 80) && (bat_percentage < 90)) {

        if (curOutOfFuelSample >= Bateria80SampleCount) {
          curOutOfFuelSample = 0;
        }
        soundVal += (Bateria80Samples[curOutOfFuelSample] * outOfFuelVolumePercentage / 100);
        curOutOfFuelSample++;
        if (curOutOfFuelSample == Bateria80SampleCount) {
          curOutOfFuelSample = 0;
          Bateria_10_Trigger = false;
        }
      }



      if ((bat_percentage >= 70) && (bat_percentage < 80)) {

        if (curOutOfFuelSample >= Bateria70SampleCount) {
          curOutOfFuelSample = 0;
        }
        soundVal += (Bateria70Samples[curOutOfFuelSample] * outOfFuelVolumePercentage / 100);
        curOutOfFuelSample++;
        if (curOutOfFuelSample == Bateria70SampleCount) {
          curOutOfFuelSample = 0;
          Bateria_10_Trigger = false;
        }
      }



      if ((bat_percentage >= 60) && (bat_percentage < 70)) {

        if (curOutOfFuelSample >= Bateria60SampleCount) {
          curOutOfFuelSample = 0;
        }
        soundVal += (Bateria60Samples[curOutOfFuelSample] * outOfFuelVolumePercentage / 100);
        curOutOfFuelSample++;
        if (curOutOfFuelSample == Bateria60SampleCount) {
          curOutOfFuelSample = 0;
          Bateria_10_Trigger = false;
        }
      }
