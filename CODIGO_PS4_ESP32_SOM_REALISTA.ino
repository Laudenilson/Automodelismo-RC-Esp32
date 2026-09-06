

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



      if ((bat_percentage >= 50) && (bat_percentage < 60)) {

        if (curOutOfFuelSample >= Bateria50SampleCount) {
          curOutOfFuelSample = 0;
        }
        soundVal += (Bateria50Samples[curOutOfFuelSample] * outOfFuelVolumePercentage / 100);
        curOutOfFuelSample++;
        if (curOutOfFuelSample == Bateria50SampleCount) {
          curOutOfFuelSample = 0;
          Bateria_10_Trigger = false;
        }
      }



      if ((bat_percentage >= 40) && (bat_percentage < 50)) {

        if (curOutOfFuelSample >= Bateria40SampleCount) {
          curOutOfFuelSample = 0;
        }
        soundVal += (Bateria40Samples[curOutOfFuelSample] * outOfFuelVolumePercentage / 100);
        curOutOfFuelSample++;
        if (curOutOfFuelSample == Bateria40SampleCount) {
          curOutOfFuelSample = 0;
          Bateria_10_Trigger = false;
        }
      }


      if ((bat_percentage >= 30) && (bat_percentage < 40)) {

        if (curOutOfFuelSample >= Bateria30SampleCount) {
          curOutOfFuelSample = 0;
        }
        soundVal += (Bateria30Samples[curOutOfFuelSample] * outOfFuelVolumePercentage / 100);
        curOutOfFuelSample++;
        if (curOutOfFuelSample == Bateria30SampleCount) {
          curOutOfFuelSample = 0;
          Bateria_10_Trigger = false;
        }
      }



      if ((bat_percentage >= 20) && (bat_percentage < 30)) {

        if (curOutOfFuelSample >= Bateria20SampleCount) {
          curOutOfFuelSample = 0;
        }
        soundVal += (Bateria20Samples[curOutOfFuelSample] * outOfFuelVolumePercentage / 100);
        curOutOfFuelSample++;
        if (curOutOfFuelSample == Bateria20SampleCount) {
          curOutOfFuelSample = 0;
          Bateria_10_Trigger = false;
        }
      }


      if ((bat_percentage >= 10) && (bat_percentage < 20)) {

        if (curOutOfFuelSample >= Bateria10SampleCount) {
          curOutOfFuelSample = 0;
        }
        soundVal += (Bateria10Samples[curOutOfFuelSample] * outOfFuelVolumePercentage / 100);
        curOutOfFuelSample++;
        if (curOutOfFuelSample == Bateria10SampleCount) {
          curOutOfFuelSample = 0;
          Bateria_10_Trigger = false;
        }
      }



      if (bat_percentage < 10) {

        if (curOutOfFuelSample > 128716) {
          curOutOfFuelSample = 0;
        }
        soundVal += (BateriaMenos10AtencaoSamples[curOutOfFuelSample] * outOfFuelVolumePercentage / 100);
        curOutOfFuelSample++;
        if (curOutOfFuelSample > BateriaMenos10AtencaoSampleCount) {  // 128716  76000   BateriaMenos10AtencaoSampleCount
          curOutOfFuelSample = 0;
          Bateria_10_Trigger = false;
        }
      }
    }
  }

  uint8_t value = constrain(soundVal * masterVolume / 100 + dacOffset, 0, 255);
  SET_PERI_REG_BITS(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, value, RTC_IO_PDAC2_DAC_S);

  // portEXIT_CRITICAL_ISR(&fixedTimerMux);
}




//
// =======================================================================================================
// mcpwm unit 1 SETUP for ESC (1x during startup)
// =======================================================================================================
//
// See: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/mcpwm.html#configure

void setupMcpwmESC() {

  Serial.printf("Standard ESC mode configured. Connect crawler ESC to ESC header. RZ7886 motor driver not usable!\n");

  // 1. set our ESC output pin
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0A, ESC_PIN);  // Set ESC as PWM0A

  // 2. configure MCPWM parameters
  mcpwm_config_t pwm_config;
  pwm_config.frequency = 50;  // frequency always 50Hz
  pwm_config.cmpr_a = 0;      // duty cycle of PWMxa = 0
  pwm_config.cmpr_b = 0;      // duty cycle of PWMxb = 0
  pwm_config.counter_mode = MCPWM_UP_COUNTER;
  pwm_config.duty_mode = MCPWM_DUTY_MODE_0;  // 0 = not inverted, 1 = inverted

  // 3. configure channels with settings above
  mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_0, &pwm_config);  // Configure PWM0A & PWM0B

  Serial.printf("-------------------------------------\n");
}









//
// =======================================================================================================
// MAIN ARDUINO SETUP (1x during startup)
// =======================================================================================================
//

void setup() {
  // Watchdog timers need to be disabled, if task 1 is running without delay(1)
  disableCore0WDT();
  // disableCore1WDT(); // Core 1 WDT can stay enabled TODO

  // Setup RTC (Real Time Clock) watchdog
  rtc_wdt_protect_off();  // Disable RTC WDT write protection
  rtc_wdt_set_length_of_reset_signal(RTC_WDT_SYS_RESET_SIG, RTC_WDT_LENGTH_3_2us);
  rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_SYSTEM);
  rtc_wdt_set_time(RTC_WDT_STAGE0, 10000);  // set 10s timeout
  rtc_wdt_enable();                         // Start the RTC WDT timer
  // rtc_wdt_disable();            // Disable the RTC WDT timer
  rtc_wdt_protect_on();  // Enable RTC WDT write protection


  ////////////////////////////////////////////// MODIFICADO /////////////////////////////////////////
  // AJUSTE
  // Você deve Comentar a inicialização da serial para que a porta 01 TX do esp32 onde está ligado o led da Luz Auxiliar 4 funcione corretamente
  Serial.begin(115200);  // Inicia a Comunicação Serial entre o computador e o Esp32
  // ****se ativar o serial.begin, comente também o pinMode do pinos das luzes auxiliares 03 e 04 logo abaixo no código



  // Print some system and software info to serial monitor
  delay(1000);  // Give serial port/connection some time to get ready
  Serial.printf("\n==================================================================================================\n");
  Serial.printf("CURSO DE ARDUINO PARA MDOELISMO - CÓDIGO CONTROLE PS4 - SOM REALISTA PARA AUTOMAÇÃO DE MINIATURAS ");
  Serial.printf("https:www.ArduinoParaModelismo.com \n");
  Serial.printf("XTAL Frequency: %i MHz, CPU Clock: %i MHz, APB Bus Clock: %i Hz\n", getXtalFrequencyMhz(), getCpuFrequencyMhz(), getApbFrequency());
  Serial.printf("Internal RAM size: %i Byte, Free: %i Byte\n", ESP.getHeapSize(), ESP.getFreeHeap());
  for (uint8_t coreNum = 0; coreNum < 2; coreNum++) {
    uint8_t resetReason = rtc_get_reset_reason(coreNum);
    if (resetReason <= (sizeof(RESET_REASONS) / sizeof(RESET_REASONS[0]))) {
      Serial.printf("Core %i reset reason: %i: %s\n", coreNum, rtc_get_reset_reason(coreNum), RESET_REASONS[resetReason - 1]);
    }
  }
  Serial.printf("\n==================================================================================================\n\n");



  // Semaphores are useful to stop a Task proceeding, where it should be paused to wait,
  // because it is sharing a resource, such as the PWM variable.
  // Semaphores should only be used whilst the scheduler is running, but we can set it up here.
  if (xPwmSemaphore == NULL)  // Check to confirm that the PWM Semaphore has not already been created.
  {
    xPwmSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage variable access
    if ((xPwmSemaphore) != NULL)
      xSemaphoreGive((xPwmSemaphore));  // Make the PWM variable available for use, by "Giving" the Semaphore.
  }

  if (xRpmSemaphore == NULL)  // Check to confirm that the RPM Semaphore has not already been created.
  {
    xRpmSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage variable access
    if ((xRpmSemaphore) != NULL)
      xSemaphoreGive((xRpmSemaphore));  // Make the RPM variable available for use, by "Giving" the Semaphore.
  }


  // CONFIGURANDO GPIOs COMO SAÍDA DE DADOS PARA LEDs e MOTOR DE VIBRAÇÃO
  pinMode(FAROL, OUTPUT);             // FAROL
  pinMode(FAROL_MILHA, OUTPUT);       // FAROL DE MILHA
  pinMode(LUZ_FREIO, OUTPUT);         // LUZ DE FREIO / LANTERNA TRASEIRA
  pinMode(SETA_DIREITA, OUTPUT);      // SETA DIREITA
  pinMode(SETA_ESQUERDA, OUTPUT);     // SETA ESQUERDA
  pinMode(GIROFLEX_VERMEHO, OUTPUT);  // GIROFLEX VERMELHO
  pinMode(GIROFLEX_AZUL, OUTPUT);     // GIROFLEX AZUL
  pinMode(LUZ_AUX01, OUTPUT);         // LUZ AUXILIAR 01
  pinMode(LUZ_AUX02, OUTPUT);         // LUZ AUXILIAR 02

  // AJUSTE Se você Precisar ver informações no Serial Monitor do IDE você deve comentar as duas lihas abaixo usando //
  // SEM O // PARA ATIVA AS LUZES AUXILIARES 03 E 04 E COM O // PARA ATIVA O MONITOR SERIAL
  //pinMode(LUZ_AUX03, OUTPUT);  // ? RX0 LUZ AUXILIAR 03
  //pinMode(LUZ_AUX04, OUTPUT);  // ? TX0 LUZ AUXILIAR 04 // comentar serial begin para funcionar

  pinMode(LUZ_RE, OUTPUT);          // LUZ DE RÉ
  pinMode(MOTOR_VIBRACAO, OUTPUT);  // MOTOR DE VIBRAÇÃO


  // CONFIGURANDO SERVOS MOTORES E ESC
  Bug.attach(SERVO_DIRECAO);
  servoDirecao.attach(SERVO_DIRECAO);
  servoAux1.attach(SERVO_AUX01);
  servoAux2.attach(SERVO_AUX02);



  // DEFININDO UM VALOR INICIAL PARA OS SERVOS MOTORES
  Bug.write(90);
  servoDirecao.write(90);
  servoAux1.write(0);
  servoAux2.write(0);
  //ESC.write(90);

  // MOTOR DE VIBRAÇÃO
  ledcAttachPin(MOTOR_VIBRACAO, 7);  //  ledcAttachPin(Pinagem do ESP, Nº Canal PWM 0 a 16)
  ledcSetup(7, 500, 8);              //  ledcSetup(Nº Canal PWM, Frequência em Hz, Resolução em Bits)
  //ledcWrite(7, 0);

  // CONFIGURANDO GPIOs DA PONTE H
  // PWM PONTE H
  ledcAttachPin(PONTE_H_IN01, 10);  //  ledcAttachPin(Pinagem do ESP, Nº Canal PWM 0 a 15)
  ledcSetup(10, 500, 8);            //  ledcSetup(Nº Canal PWM, Frequência em Hz, Resolução em Bits)
  // PWM PONTE H
  ledcAttachPin(PONTE_H_IN02, 11);
  ledcSetup(11, 500, 8);


  // CONFIGURANDO PWM PARA O LED DO FAROL
  ledcAttachPin(FAROL, 12);
  ledcSetup(12, 500, 8);


  // CONFIGURANDO PWM PARA O LED DA LUZ DE FREIO
  ledcAttachPin(LUZ_FREIO, 13);
  ledcSetup(13, 500, 8);



  // Refresh sample intervals (important, because MAX_RPM_PERCENTAGE was probably changed above)
  maxSampleInterval = 4000000 / sampleRate;
  minSampleInterval = 4000000 / sampleRate * 100 / MAX_RPM_PERCENTAGE;



  // Task 1 setup (running on core 0)
  TaskHandle_t Task1;
  // create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
    Task1code,  // Task function
    "Task1",    // name of task
    8192,       // Stack size of task (8192)
    NULL,       // parameter of the task
    1,          // priority of the task (1 = low, 3 = medium, 5 = highest)
    &Task1,     // Task handle to keep track of created task
    0);         // pin task to core 0

  // once write with the "normal" way.
  // all further writes are done directly in the register since
  // it's much faster
  dacWrite(DAC1, 0);
  dacWrite(DAC2, 0);

  // Interrupt timer for variable sample rate playback
  variableTimer = timerBegin(0, 20, true);                            // timer 0, MWDT clock period = 12.5 ns * TIMGn_Tx_WDT_CLK_PRESCALE -> 12.5 ns * 20 -> 250 ns = 0.25 us, countUp
  timerAttachInterrupt(variableTimer, &variablePlaybackTimer, true);  // edge (not level) triggered
  timerAlarmWrite(variableTimer, variableTimerTicks, true);           // autoreload true
  timerAlarmEnable(variableTimer);                                    // enable

  // Interrupt timer for fixed sample rate playback
  fixedTimer = timerBegin(1, 20, true);                         // timer 1, MWDT clock period = 12.5 ns * TIMGn_Tx_WDT_CLK_PRESCALE -> 12.5 ns * 20 -> 250 ns = 0.25 us, countUp
  timerAttachInterrupt(fixedTimer, &fixedPlaybackTimer, true);  // edge (not level) triggered
  timerAlarmWrite(fixedTimer, fixedTimerTicks, true);           // autoreload true
  timerAlarmEnable(fixedTimer);                                 // enable


  // MODIFICANDO
  processRawChannels();


  // Calculate RC input signal ranges for all channels
  for (uint8_t i = 1; i < PULSE_ARRAY_SIZE; i++) {
    pulseZero[i] = 1500;  // Always 1500. This is the center position. Auto centering is now done in "processRawChannels()"

    // Input signals
    pulseMaxNeutral[i] = pulseZero[i] + pulseNeutral;
    pulseMinNeutral[i] = pulseZero[i] - pulseNeutral;
    pulseMax[i] = pulseZero[i] + pulseSpan;
    pulseMin[i] = pulseZero[i] - pulseSpan;
    pulseMaxLimit[i] = pulseZero[i] + pulseLimit;
    pulseMinLimit[i] = pulseZero[i] - pulseLimit;
  }

  // ESC output range calibration
  escPulseMaxNeutral = pulseZero[3] + escTakeoffPunch;  // Additional takeoff punch around zero
  escPulseMinNeutral = pulseZero[3] - escTakeoffPunch;

  escPulseMax = pulseZero[3] + escPulseSpan;
  escPulseMin = pulseZero[3] - escPulseSpan + escReversePlus;  // Additional power for ESC with slow reverse

  // ESC setup
  setupMcpwmESC();  // ESC now using mpcpwm

  // CONFIGURAÇÕES DO CONTROLE
  //*************************************************************************************
  //

  Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
  const uint8_t *addr = BP32.localBdAddress();
  Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2],
                addr[3], addr[4], addr[5]);

  // Setup the Bluepad32 callbacks
  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

  BP32.forgetBluetoothKeys();

  //
  //*************************************************************************************
  //
}

//
// =======================================================================================================
// DAC OFFSET FADER
// =======================================================================================================
//

static unsigned long dacOffsetMicros;
boolean dacInit;

void dacOffsetFade() {
  if (!dacInit) {
    if (micros() - dacOffsetMicros > 100) {  // Every 0.1ms
      dacOffsetMicros = micros();
      dacOffset++;  // fade DAC offset slowly to prevent it from popping, if ESP32 powered up after amplifierpulseWidthRaw[3]
      if (dacOffset == 128)
        dacInit = true;
    }
  }
}



//
// =======================================================================================================
// PROCESS CHANNELS (Normalize, auto zero and reverse)
// =======================================================================================================
//

void processRawChannels() {


  // MODIFICANDO
  pulseWidthRaw[1] = map(joyDireitaX, 0, 255, 2000, 1000);   // map(SBUSchannels[STEERING - 1], 172, 1811, 1000, 2000);          // CH1 steering
  pulseWidthRaw[2] = 1500;                                   // map(SBUSchannels[GEARBOX - 1], 172, 1811, 1000, 2000);           // CH2 3 position switch for gearbox (left throttle in tracked mode)
  pulseWidthRaw[3] = map(joyEsquerdaY, 0, 255, 1000, 2000);  // map(SBUSchannels[THROTTLE - 1], 172, 1811, 1000, 2000);          // CH3 throttle & brake
  pulseWidthRaw[4] = map(x, 0, 1, 1000, 2000);               // map(SBUSchannels[HORN - 1], 172, 1811, 1000, 2000);              // CH5 jake brake, high / low beam, headlight flasher, engine on / off
  pulseWidthRaw[5] = 1500;                                   // map(SBUSchannels[FUNCTION_R - 1], 172, 1811, 1000, 2000);        // CH5 jake brake, high / low beam, headlight flasher, engine on / off
  pulseWidthRaw[6] = 2000;                                   //map(triangulo, 0, 1, 1000, 2000); // map(SBUSchannels[FUNCTION_L - 1], 172, 1811, 1000, 2000);        // CH6 indicators, hazards
  pulseWidthRaw[7] = 1500;                                   // map(SBUSchannels[POT2 - 1], 172, 1811, 1000, 2000);              // CH7 pot 2
  pulseWidthRaw[8] = 1500;                                   // map(SBUSchannels[MODE1 - 1], 172, 1811, 1000, 2000);             // CH8 mode 1 switch
  pulseWidthRaw[9] = 1500;                                   // map(SBUSchannels[MODE2 - 1], 172, 1811, 1000, 2000);             // CH9 mode 2 switch
  pulseWidthRaw[10] = map(ps, 0, 1, 1000, 2000);             // map(SBUSchannels[MOMENTARY1 - 1], 172, 1811, 1000, 2000);       // CH10
  pulseWidthRaw[11] = 1500;                                  // map(SBUSchannels[HAZARDS - 1], 172, 1811, 1000, 2000);          // CH11
  pulseWidthRaw[12] = 1500;                                  // map(SBUSchannels[INDICATOR_LEFT - 1], 172, 1811, 1000, 2000);   // CH12
  pulseWidthRaw[13] = 1500;                                  // map(SBUSchannels[INDICATOR_RIGHT - 1], 172, 1811, 1000, 2000);  // CH13

  masterVolume = VolumeMaster;
  // verificar
  pulseWidth[3] = map(joyEsquerdaY, 0, 255, 1000, 2000);
}


//
// =======================================================================================================
// DISABLE INTERRUPTS
// =======================================================================================================
//

// it is required to disable interrupts prior to EEPROM access!
void disableAllInterrupts() {

  timerDetachInterrupt(variableTimer);
  timerDetachInterrupt(fixedTimer);

  Serial.print("Interrupts disabled, reboot required!");
}


//
// =======================================================================================================
// MAP PULSEWIDTH TO THROTTLE CH3
// =======================================================================================================
//

void mapThrottle() {

  // Input is around 1000 - 2000us, output 0-500 for forward and backwards

#if defined TRACKED_MODE         // Dual throttle input for caterpillar vehicles ------------------
  int16_t currentThrottleLR[4];  // 2 & 3 is used, so required array size = 4!

  // check if pulsewidths 2 + 3 look like servo pulses
  for (int i = 2; i < 4; i++) {
    if (pulseWidth[i] > pulseMinLimit[i] && pulseWidth[i] < pulseMaxLimit[i]) {
      if (pulseWidth[i] < pulseMin[i])
        pulseWidth[i] = pulseMin[i];  // Constrain the value
      if (pulseWidth[i] > pulseMax[i])
        pulseWidth[i] = pulseMax[i];

      // calculate a throttle value from the pulsewidth signal
      if (pulseWidth[i] > pulseMaxNeutral[i]) {
        currentThrottleLR[i] = map(pulseWidth[i], pulseMaxNeutral[i], pulseMax[i], 0, 500);
      } else if (pulseWidth[i] < pulseMinNeutral[i]) {
        currentThrottleLR[i] = map(pulseWidth[i], pulseMinNeutral[i], pulseMin[i], 0, 500);
      } else {
        currentThrottleLR[i] = 0;
      }
    }
  }

  // Mixing both sides together (take the bigger value)
  currentThrottle = max(currentThrottleLR[2], currentThrottleLR[3]);

  // Print debug infos
  static unsigned long printTrackedMillis;
#ifdef TRACKED_DEBUG                           // can slow down the playback loop!
  if (millis() - printTrackedMillis > 1000) {  // Every 1000ms
    printTrackedMillis = millis();

    Serial.printf("TRACKED DEBUG:\n");
    Serial.printf("currentThrottleLR[2]: %i\n", currentThrottleLR[2]);
    Serial.printf("currentThrottleLR[3]: %i\n", currentThrottleLR[3]);
    Serial.printf("currentThrottle: %i\n", currentThrottle);
  }
#endif  // TRACKED_DEBUG

#elif defined EXCAVATOR_MODE  // Excavator mode ----------------------------------------------

  static bool engineInit = false;  // Only allow to start engine after switch was in up position
  static unsigned long rpmLoweringMillis;
  static uint8_t rpmLowering;

  // calculate a throttle value from the pulsewidth signal (forward only)
  if (pulseWidth[3] > pulseMaxNeutral[3]) {
    currentThrottle = map(pulseWidth[3], pulseMaxNeutral[3], pulseMax[3], 0, (500 - rpmLowering));
  } else {
    currentThrottle = 0;
  }

  // Engine on / off via 3 position switch
  if (pulseWidth[3] < 1200 && currentRpm < 50) {  // Off
    engineInit = true;
    engineOn = false;
    // rpmLoweringMillis = millis();
  } else {  // On
    if (engineInit)
      engineOn = true;
  }

  // Engine RPM lowering, if hydraulic not used for 5s
  if (hydraulicLoad > 1 || pulseWidth[3] < pulseMaxNeutral[3])
    rpmLoweringMillis = millis();
  if (millis() - rpmLoweringMillis > 5000)
    rpmLowering = 250;  // Medium RPM
  else
    rpmLowering = 0;  // Full RPM

#elif defined AIRPLANE_MODE  // Airplane mode ----------------------------------------------

  // Never engage clutchjoyDireitaX
  maxClutchSlippingRpm = 500;
  clutchEngagingPoint = 500;

  // calculate a throttle value from the pulsewidth signal (forward only, throttle zero @1000)
  if (pulseWidth[3] > 1100) {
    currentThrottle = map(pulseWidth[3], 1100, 2000, 0, 500);
  } else {
    currentThrottle = 0;
  }

#else  // Normal mode ---------------------------------------------------------------------------

  // check if the pulsewidth looks like a servo pulse
  if (pulseWidth[3] > pulseMinLimit[3] && pulseWidth[3] < pulseMaxLimit[3]) {
    if (pulseWidth[3] < pulseMin[3])
      pulseWidth[3] = pulseMin[3];  // Constrain the value
    if (pulseWidth[3] > pulseMax[3])
      pulseWidth[3] = pulseMax[3];

    // calculate a throttle value from the pulsewidth signal
    if (pulseWidth[3] > pulseMaxNeutral[3]) {
      currentThrottle = map(pulseWidth[3], pulseMaxNeutral[3], pulseMax[3], 0, 500);
    } else if (pulseWidth[3] < pulseMinNeutral[3]) {
      currentThrottle = map(pulseWidth[3], pulseMinNeutral[3], pulseMin[3], 0, 500);
    } else {
      currentThrottle = 0;
    }
  }
#endif

  // Auto throttle --------------------------------------------------------------------------
#if not defined EXCAVATOR_MODE
  // Auto throttle while gear shifting (synchronizing the Tamiya 3 speed gearbox)
  if (!escIsBraking && escIsDriving && shiftingAutoThrottle && !automatic && !doubleClutch) {
    if (gearUpShiftingInProgress && !doubleClutchInProgress)
      currentThrottle = 0;  // No throttle
    if (gearDownShiftingInProgress || doubleClutchInProgress)
      currentThrottle = 500;                               // Full throttle
    currentThrottle = constrain(currentThrottle, 0, 500);  // Limit throttle range
  }
#endif

  // Volume calculations --------------------------------------------------------------------------

  // As a base for some calculations below, fade the current throttle to make it more natural
  static unsigned long throttleFaderMicros;
  static boolean blowoffLock;
  if (micros() - throttleFaderMicros > 500) {  // Every 0.5ms
    throttleFaderMicros = micros();

    if (currentThrottleFaded < currentThrottle && !escIsBraking && currentThrottleFaded < 499)
      currentThrottleFaded += 2;
    if ((currentThrottleFaded > currentThrottle || escIsBraking) && currentThrottleFaded > 2)
      currentThrottleFaded -= 2;

    // Calculate throttle dependent engine idle volume
    if (!escIsBraking && !brakeDetect && engineRunning)
      throttleDependentVolume = map(currentThrottleFaded, 0, 500, engineIdleVolumePercentage, fullThrottleVolumePercentage);
    // else throttleDependentVolume = engineIdleVolumePercentage; // TODO
    else {
      if (throttleDependentVolume > engineIdleVolumePercentage)
        throttleDependentVolume--;
      else
        throttleDependentVolume = engineIdleVolumePercentage;
    }

    // Calculate throttle dependent engine rev volume
    if (!escIsBraking && !brakeDetect && engineRunning)
      throttleDependentRevVolume = map(currentThrottleFaded, 0, 500, engineRevVolumePercentage, fullThrottleVolumePercentage);
    // else throttleDependentRevVolume = engineRevVolumePercentage; // TODO
    else {
      if (throttleDependentRevVolume > engineRevVolumePercentage)
        throttleDependentRevVolume--;
      else
        throttleDependentRevVolume = engineRevVolumePercentage;
    }

    // Calculate throttle dependent Diesel knock volume
    if (!escIsBraking && !brakeDetect && engineRunning && (currentThrottleFaded > dieselKnockStartPoint))
      throttleDependentKnockVolume = map(currentThrottleFaded, dieselKnockStartPoint, 500, dieselKnockIdleVolumePercentage, 100);
    // else throttleDependentKnockVolume = dieselKnockIdleVolumePercentage;
    else {
      if (throttleDependentKnockVolume > dieselKnockIdleVolumePercentage)
        throttleDependentKnockVolume--;
      else
        throttleDependentKnockVolume = dieselKnockIdleVolumePercentage;
    }

    // Calculate engine rpm dependent jake brake volume
    if (engineRunning)
      rpmDependentJakeBrakeVolume = map(currentRpm, 0, 500, jakeBrakeIdleVolumePercentage, 100);
    else
      rpmDependentJakeBrakeVolume = jakeBrakeIdleVolumePercentage;

#if defined RPM_DEPENDENT_KNOCK  // knock volume also depending on engine rpm
    // Calculate RPM dependent Diesel knock volume
    if (currentRpm > 400)
      rpmDependentKnockVolume = map(currentRpm, knockStartRpm, 500, minKnockVolumePercentage, 100);
    else
      rpmDependentKnockVolume = minKnockVolumePercentage;
#endif

    // Calculate engine rpm dependent turbo volume
    if (engineRunning)
      throttleDependentTurboVolume = map(currentRpm, 0, 500, turboIdleVolumePercentage, 100);
    else
      throttleDependentTurboVolume = turboIdleVolumePercentage;

    // Calculate engine rpm dependent cooling fan volume
    if (engineRunning && (currentRpm > fanStartPoint))
      throttleDependentFanVolume = map(currentRpm, fanStartPoint, 500, fanIdleVolumePercentage, 100);
    else
      throttleDependentFanVolume = fanIdleVolumePercentage;

    // Calculate throttle dependent supercharger volume
    if (!escIsBraking && !brakeDetect && engineRunning && (currentRpm > chargerStartPoint))
      throttleDependentChargerVolume = map(currentThrottleFaded, chargerStartPoint, 500, chargerIdleVolumePercentage, 100);
    else
      throttleDependentChargerVolume = chargerIdleVolumePercentage;

    // Calculate engine rpm dependent wastegate volume
    if (engineRunning)
      rpmDependentWastegateVolume = map(currentRpm, 0, 500, wastegateIdleVolumePercentage, 100);
    else
      rpmDependentWastegateVolume = wastegateIdleVolumePercentage;
  }

  // Calculate engine load (used for torque converter slip simulation)
  engineLoad = currentThrottle - currentRpm;

  if (engineLoad < 0 || escIsBraking || brakeDetect)
    engineLoad = 0;  // Range is 0 - 180
  if (engineLoad > 180)
    engineLoad = 180;

  // Additional sounds volumes -----------------------------

  // Tire squealing ----
  uint8_t steeringAngle = 0;
  uint8_t brakeSquealVolume = 0;

  // Cornering squealing
  if (pulseWidth[1] < 1500)
    steeringAngle = map(pulseWidth[1], 1000, 1500, 100, 0);
  else if (pulseWidth[1] > 1500)
    steeringAngle = map(pulseWidth[1], 1500, 2000, 0, 100);
  else
    steeringAngle = 0;

  tireSquealVolume = steeringAngle * currentSpeed * currentSpeed / 125000;  // Volume = steering angle * speed * speed

  // Brake squealing
  if ((driveState == 2 || driveState == 4) && currentSpeed > 50 && currentThrottle > 250) {
    tireSquealVolume += map(currentThrottle, 250, 500, 0, 100);
  }

  tireSquealVolume = constrain(tireSquealVolume, 0, 100);
}

//
// =======================================================================================================
// ENGINE MASS SIMULATION (running on core 0)
// =======================================================================================================
//

void engineMassSimulation() {

  static int32_t targetRpm = 0;    // The engine RPM target
  static int32_t _currentRpm = 0;  // Private current RPM (to prevent conflict with core 1)
  static int32_t _currentThrottle = 0;
  static int32_t lastThrottle;
  uint16_t converterSlip;
  static unsigned long throtMillis;
  static unsigned long wastegateMillis;
  static unsigned long blowoffMillis;
  uint8_t timeBase;

#ifdef SUPER_SLOW
  timeBase = 6;  // super slow running, heavy engines, for example locomotive diesels
#else
  timeBase = 2;
#endif

  _currentThrottle = currentThrottle;

  if (millis() - throtMillis > timeBase) {  // Every 2 or 6ms
    throtMillis = millis();

    if (_currentThrottle > 500)
      _currentThrottle = 500;

      // Virtual clutch **********************************************************************************
#if defined EXCAVATOR_MODE  // Excavator mode ---
    clutchDisengaged = true;

    targetRpm = _currentThrottle - hydraulicLoad;
    targetRpm = constrain(targetRpm, 0, 500);

#else  // Normal mode ---
    // if ((currentSpeed < clutchEngagingPoint && _currentRpm < maxClutchSlippingRpm) || gearUpShiftingInProgress || gearDownShiftingInProgress || neutralGear || _currentRpm < 200) { // TODO Bug?
    if ((currentSpeed < clutchEngagingPoint && _currentRpm < maxClutchSlippingRpm) || gearUpShiftingInProgress || gearDownShiftingInProgress || neutralGear) {
      clutchDisengaged = true;
    } else {
      clutchDisengaged = false;
    }

    // Transmissions ***********************************************************************************

    // automatic transmission ----
    if (automatic) {
      // Torque converter slip calculation
      if (selectedAutomaticGear < 2)
        converterSlip = engineLoad * torqueconverterSlipPercentage / 100 * 2;  // more slip in first and reverse gear
      else
        converterSlip = engineLoad * torqueconverterSlipPercentage / 100;

      if (!neutralGear)
        targetRpm = currentSpeed * gearRatio[selectedAutomaticGear] / 10 + converterSlip;  // Compute engine RPM
      else
        targetRpm = reMap(curveLinear, _currentThrottle);
    } else if (doubleClutch) {
      // double clutch transmission
      if (!neutralGear)
        targetRpm = currentSpeed * gearRatio[selectedAutomaticGear] / 10;  // Compute engine RPM
      else
        targetRpm = reMap(curveLinear, _currentThrottle);
    } else {
      // Manual transmission ----
      if (clutchDisengaged) {  // Clutch disengaged: Engine revving allowed
#if defined VIRTUAL_16_SPEED_SEQUENTIAL
        targetRpm = _currentThrottle;
#else
        targetRpm = reMap(curveLinear, _currentThrottle);

#endif
      } else {                                                                                       // Clutch engaged: Engine rpm synchronized with ESC power (speed)

#if defined VIRTUAL_3_SPEED || defined VIRTUAL_16_SPEED_SEQUENTIAL  // Virtual 3 speed or sequential 16 speed transmission
        targetRpm = reMap(curveLinear, (currentSpeed * virtualManualGearRatio[selectedGear] / 10));  // Add virtual gear ratios
        if (targetRpm > 500)
          targetRpm = 500;

#elif defined STEAM_LOCOMOTIVE_MODE
        targetRpm = currentSpeed;

#else  // Real 3 speed transmission
        targetRpm = reMap(curveLinear, currentSpeed);
#endif
      }
    }
#endif

    // Engine RPM **************************************************************************************

    if (escIsBraking && currentSpeed < clutchEngagingPoint)
      targetRpm = 0;  // keep engine @idle rpm, if braking at very low speed
    if (targetRpm > 500)
      targetRpm = 500;

    // Accelerate engine
    if (targetRpm > (_currentRpm + acc) && (_currentRpm + acc) < maxRpm && engineState == RUNNING && engineRunning) {
      if (!airBrakeTrigger) {  // No acceleration, if brake release noise still playing
        if (!gearDownShiftingInProgress)
          _currentRpm += acc;
        else
          _currentRpm += acc / 2;  // less aggressive rpm rise while downshifting
        if (_currentRpm > maxRpm)
          _currentRpm = maxRpm;
      }
    }

    // Decelerate engine
    if (targetRpm < _currentRpm) {
      _currentRpm -= dec;
      if (_currentRpm < minRpm)
        _currentRpm = minRpm;
    }

#if (defined VIRTUAL_3_SPEED || defined VIRTUAL_16_SPEED_SEQUENTIAL) and not defined STEAM_LOCOMOTIVE_MODE
    // Limit top speed, depending on manual gear ratio. Ensures, that the engine will not blow up!
    if (!automatic && !doubleClutch)
      speedLimit = maxRpm * 10 / virtualManualGearRatio[selectedGear];
#endif

    // Speed (sample rate) output
    engineSampleRate = map(_currentRpm, minRpm, maxRpm, maxSampleInterval, minSampleInterval);  // Idle

    // if ( xSemaphoreTake( xRpmSemaphore, portMAX_DELAY ) )
    //{
    currentRpm = _currentRpm;
    // xSemaphoreGive( xRpmSemaphore ); // Now free or "Give" the semaphore for others.
    // }
  }

  // Prevent Wastegate from being triggered while downshifting
  if (gearDownShiftingInProgress)
    wastegateMillis = millis();

  // Trigger Wastegate, if throttle rapidly dropped
  if (lastThrottle - _currentThrottle > 70 && !escIsBraking && millis() - wastegateMillis > 1000) {
    wastegateMillis = millis();
    wastegateTrigger = true;
  }

#if defined JAKEBRAKE_ENGINE_SLOWDOWN && defined JAKE_BRAKE_SOUND
  // Use jake brake to slow down engine while releasing throttle in neutral or during upshifting while applying throttle
  // for some vehicles like Volvo FH open pipe. See example: https://www.youtube.com/watch?v=MU1iwzl33Zw&list=LL&index=4
  if (!wastegateTrigger)
    blowoffMillis = millis();
  blowoffTrigger = ((gearUpShiftingInProgress || neutralGear) && millis() - blowoffMillis > 20 && millis() - blowoffMillis < 250);
#endif

  lastThrottle = _currentThrottle;
}

//
// =======================================================================================================
// SWITCH ENGINE ON OR OFF (for automatic mode)
// =======================================================================================================
//

void engineOnOff() {

  // static unsigned long pulseDelayMillis; // TODO
  static unsigned long idleDelayMillis;

  // Engine automatically switched on or off depending on throttle position and 15s delay timne
  if (currentThrottle > 80 || driveState != 0)
    idleDelayMillis = millis();  // reset delay timer, if throttle not in neutral

#ifdef AUTO_ENGINE_ON_OFF
  if (millis() - idleDelayMillis > 15000) {
    engineOn = false;  // after delay, switch engine off
  }
#endif

#ifdef AUTO_LIGHTS
  if (millis() - idleDelayMillis > 10000) {
    lightsOn = false;  // after delay, switch light off
  }
#endif

  // Engine start detection
  if (currentThrottle > 100 && !airBrakeTrigger) {
    engineOn = true;

#ifdef AUTO_LIGHTS
    lightsOn = true;
#endif
  }
}


//
// =======================================================================================================
// SHAKER (simulates engine vibrations)
// =======================================================================================================
//
// MOTOR DE VIBRAÇÃO
// MODIFICADO MOTOR DE VIBRAÇÃO
void shaker() {
  int32_t shakerRpm = 0;

  // Set desired shaker rpm
  if (engineRunning) shakerRpm = map(currentRpm, minRpm, maxRpm, VibracaoMarchaLenta, VibracaoAceleracaoMaxima);
  if (engineStart) shakerRpm = VibracaoPartida;
  if (engineStop) shakerRpm = VibracaoDesligar;

  // Shaker on / off
  if (engineRunning || engineStart || engineStop) {
    ledcWrite(7, shakerRpm);
    //Serial.print("shakerRpm: ");
    //Serial.println(shakerRpm);
  } else {
    ledcWrite(7, 0);
    //Serial.print("shakerRpm else: ");
    //Serial.println(shakerRpm);
  }
}


//
// =======================================================================================================
// MANUAL GEARBOX DETECTION (Real 3 speed, virtual 3 speed, virtual 16 speed, semi automatic)
// =======================================================================================================
//

void gearboxDetection() {

  static uint8_t previousGear = 1;
  static bool previousReverse;
  static bool sequentialLock;
  static bool overdrive = false;
  static unsigned long upShiftingMillis;
  static unsigned long downShiftingMillis;
  static unsigned long lastShiftingMillis;  // This timer is used to prevent transmission from oscillating!

#if defined TRACKED_MODE or defined STEAM_LOCOMOTIVE_MODE  // CH2 is used for left throttle in TRACKED_MODE --------------------------------
  selectedGear = 2;

#else  // only active, if not in TRACKED_MODE -------------------------------------------------------------

#if defined OVERDRIVE && defined VIRTUAL_3_SPEED  // Additional 4th gear mode for virtual 3 speed ********************************
  if (!crawlerMode) {
    // The 4th gear (overdrive) is engaged automatically, if driving @ full throttle in 3rd gear
    if (currentRpm > 490 && selectedGear == 3 && engineLoad < 5 && currentThrottle > 490 && millis() - lastShiftingMillis > 2000) {
      overdrive = true;
    }
    if (!escIsBraking) {  // Lower downshift point, if not braking
      if (currentRpm < 200 && millis() - lastShiftingMillis > 2000) {
        overdrive = false;
      }
    } else {  // Higher downshift point, if braking
      if ((currentRpm < 400 || engineLoad > 150) && millis() - lastShiftingMillis > 2000) {
        overdrive = false;
      }
    }
    if (selectedGear < 3)
      overdrive = false;
  }
#endif                                            // End of overdrive ******************************************************************************************************

#if not defined VIRTUAL_16_SPEED_SEQUENTIAL && not defined SEMI_AUTOMATIC  // 3 gears, selected by 3 position switch **************
  // Gear detection
  if (pulseWidth[2] > 1700)
    selectedGear = 3;
  else if (pulseWidth[2] < 1300)
    selectedGear = 1;
  else
    selectedGear = 2;
  if (overdrive && selectedGear == 3)
    selectedGear = 4;
#endif                                                                     // End of manual 3 speed *************************************************************************************************

#if defined VIRTUAL_16_SPEED_SEQUENTIAL  // 16 gears, selected by up / down impulses *********************************************
  if (pulseWidth[2] > 1700 && selectedGear < 16 && !sequentialLock) {
    sequentialLock = true;
    selectedGear++;
  } else if (pulseWidth[2] < 1300 && selectedGear > 1 && !sequentialLock) {
    sequentialLock = true;
    selectedGear--;
  }
  if (pulseWidth[2] > 1400 && pulseWidth[2] < 1600)
    sequentialLock = false;
#endif                                   // End of VIRTUAL_16_SPEED_SEQUENTIAL *************************************************************************************

#if defined SEMI_AUTOMATIC  // gears not controlled by the 3 position switch but by RPM limits ************************************
  if (currentRpm > 490 && selectedGear < 3 && engineLoad < 5 && currentThrottle > 490 && millis() - lastShiftingMillis > 2000) {
    selectedGear++;
  }
  if (!escIsBraking) {  // Lower downshift point, if not braking
    if (currentRpm < 200 && selectedGear > 1 && millis() - lastShiftingMillis > 2000) {
      selectedGear--;  //
    }
  } else {  // Higher downshift point, if braking
    if ((currentRpm < 400 || engineLoad > 150) && selectedGear > 1 && millis() - lastShiftingMillis > 2000) {
      selectedGear--;  // Higher downshift point, if braking
    }
  }
  if (neutralGear || escInReverse)
    selectedGear = 1;
#endif                      // End of SEMI_AUTOMATIC **************************************************************************************************

  // Gear upshifting detection
  if (selectedGear > previousGear) {
    gearUpShiftingInProgress = true;
    gearUpShiftingPulse = true;
    shiftingTrigger = true;
    previousGear = selectedGear;
    lastShiftingMillis = millis();
  }

  // Gear upshifting duration
  static uint16_t upshiftingDuration = 700;
  if (!gearUpShiftingInProgress)
    upShiftingMillis = millis();
  if (millis() - upShiftingMillis > upshiftingDuration) {
    gearUpShiftingInProgress = false;
  }
  // Double-clutch (Zwischengas während dem Hochschalten)
#if defined DOUBLE_CLUTCH
  upshiftingDuration = 900;
  doubleClutchInProgress = (millis() - upShiftingMillis >= 500 && millis() - upShiftingMillis < 600);  // Apply full throttle
#endif

  // Gear downshifting detection
  if (selectedGear < previousGear) {
    gearDownShiftingInProgress = true;
    gearDownShiftingPulse = true;
    shiftingTrigger = true;
    previousGear = selectedGear;
    lastShiftingMillis = millis();
  }

  // Gear downshifting duration
  if (!gearDownShiftingInProgress)
    downShiftingMillis = millis();
  if (millis() - downShiftingMillis > 300) {
    gearDownShiftingInProgress = false;
  }

  // Som da marcha a ré
  // Reverse gear engaging / disengaging detection
  // if (escInReverse != previousReverse) {
  //   previousReverse = escInReverse;
  //   shiftingTrigger = true;  // Play shifting sound
  // }

#ifdef MANUAL_TRANS_DEBUG
  static unsigned long manualTransDebugMillis;
  if (millis() - manualTransDebugMillis > 100) {
    manualTransDebugMillis = millis();
    Serial.printf("MANUAL_TRANS_DEBUG:\n");
    Serial.printf("currentThrottle: %i\n", currentThrottle);
    Serial.printf("selectedGear: %i\n", selectedGear);
    Serial.printf("overdrive: %i\n", overdrive);
    Serial.printf("engineLoad: %i\n", engineLoad);
    Serial.printf("sequentialLock: %s\n", sequentialLock ? "true" : "false");
    Serial.printf("currentRpm: %i\n", currentRpm);
    Serial.printf("currentSpeed: %i\n", currentSpeed);
    Serial.printf("---------------------------------\n");
  }
#endif  // MANUAL_TRANS_DEBUG

#endif  // End of not TRACKED_MODE -----------------------------------------------------------------------
}

//
// =======================================================================================================
// SIMULATED AUTOMATIC TRANSMISSION GEAR SELECTOR (running on core 0)
// =======================================================================================================
//

void automaticGearSelector() {

  static unsigned long gearSelectorMillis;
  static unsigned long lastUpShiftingMillis;
  static unsigned long lastDownShiftingMillis;
  uint16_t downShiftPoint = 200;
  uint16_t upShiftPoint = 490;
  static int32_t _currentRpm = 0;  // Private current RPM (to prevent conflict with core 1)

  // if ( xSemaphoreTake( xRpmSemaphore, portMAX_DELAY ) )
  //{
  _currentRpm = currentRpm;
  // xSemaphoreGive( xRpmSemaphore ); // Now free or "Give" the semaphore for others.
  // }

  if (millis() - gearSelectorMillis > 100) {  // Waiting for 100ms is very important. Otherwise gears are skipped!
    gearSelectorMillis = millis();

    // compute load dependent shift points (less throttle = less rpm before shifting up, kick down will shift back!)
    upShiftPoint = map(engineLoad, 0, 180, 390, 490);    // 390, 490
    downShiftPoint = map(engineLoad, 0, 180, 150, 250);  // 150, 250

    if (escInReverse) {  // Reverse (only one gear)
      selectedAutomaticGear = 0;
    } else {  // Forward (multiple gears)

      // Adaptive shift points
      if (millis() - lastDownShiftingMillis > 500 && _currentRpm >= upShiftPoint && engineLoad < 5) {  // 500ms locking timer!
        selectedAutomaticGear++;                                                                       // Upshifting (load maximum is important to prevent gears from oscillating!)
        lastUpShiftingMillis = millis();
      }
      if (millis() - lastUpShiftingMillis > 600 && selectedAutomaticGear > 1 && (_currentRpm <= downShiftPoint || engineLoad > 100)) {  // 600ms locking timer! TODO was 1000
        selectedAutomaticGear--;                                                                                                        // Downshifting incl. kickdown
        lastDownShiftingMillis = millis();
      }

      selectedAutomaticGear = constrain(selectedAutomaticGear, 1, NumberOfAutomaticGears);
    }

#ifdef AUTO_TRANS_DEBUG
    Serial.printf("AUTO_TRANS_DEBUG:\n");
    Serial.printf("currentThrottle: %i\n", currentThrottle);
    Serial.printf("selectedAutomaticGear: %i\n", selectedAutomaticGear);
    Serial.printf("engineLoad: %i\n", engineLoad);
    Serial.printf("upShiftPoint: %i\n", upShiftPoint);
    Serial.printf("_currentRpm: %i\n", _currentRpm);
    Serial.printf("downShiftPoint: %i\n", downShiftPoint);
    Serial.printf("-----------------------------------\n");
#endif
  }
}

//
// =======================================================================================================
// ESC CONTROL (including optional battery protection)
// =======================================================================================================
//

static uint16_t escPulseWidth = 1500;
static uint16_t escPulseWidthOut = 1500;
static uint16_t escSignal = 1500;
static uint8_t motorDriverDuty = 0;
static unsigned long escMillis;
static unsigned long lastStateTime;
// static int8_t pulse; // -1 = reverse, 0 = neutral, 1 = forward
// static int8_t escPulse; // -1 = reverse, 0 = neutral, 1 = forward
static int8_t driveRampRate;
static int8_t driveRampGain;
static int8_t brakeRampRate;
uint16_t escRampTime;

// ESC sub functions =============================================
// We always need the data up to date, so these comparators are programmed as sub functions!
int8_t pulse() {  // Throttle direction
  int8_t pulse;
  if (pulseWidth[3] > pulseMaxNeutral[3] && pulseWidth[3] < pulseMaxLimit[3])
    pulse = 1;  // 1 = Forward
  else if (pulseWidth[3] < pulseMinNeutral[3] && pulseWidth[3] > pulseMinLimit[3])
    pulse = -1;  // -1 = Backwards
  else
    pulse = 0;  // 0 = Neutral
  return pulse;
}
int8_t escPulse() {  // ESC direction
  int8_t escPulse;
  if (escPulseWidth > pulseMaxNeutral[3] && escPulseWidth < pulseMaxLimit[3])
    escPulse = 1;  // 1 = Forward
  else if (escPulseWidth < pulseMinNeutral[3] && escPulseWidth > pulseMinLimit[3])
    escPulse = -1;  // -1 = Backwards
  else
    escPulse = 0;  // 0 = Neutral
  return escPulse;
}

// If you connect your ESC to pin 33, the vehicle inertia is simulated. Direct brake (crawler) ESC required
// *** WARNING!! Do it at your own risk!! There is a falisafe function in case, the signal input from the
// receiver is lost, but if the ESP32 crashes, the vehicle could get out of control!! ***

void esc() {  // ESC main function ================================



#if not defined TRACKED_MODE && not defined AIRPLANE_MODE  // No ESC control in TRACKED_MODE or in AIRPLANE_MODE
  // Gear dependent ramp speed for acceleration & deceleration
#if defined VIRTUAL_3_SPEED
  escRampTime = escRampTimeThirdGear * 10 / virtualManualGearRatio[selectedGear];

#elif defined VIRTUAL_16_SPEED_SEQUENTIAL
  escRampTime = escRampTimeThirdGear * virtualManualGearRatio[selectedGear] / 5;

#elif defined STEAM_LOCOMOTIVE_MODE
  escRampTime = escRampTimeSecondGear;

#else  // TAMIYA 3 speed shifting transmission
  if (selectedGear == 1)
    escRampTime = escRampTimeFirstGear;  // about 20
  if (selectedGear == 2)
    escRampTime = escRampTimeSecondGear;  // about 50
  if (selectedGear == 3)
    escRampTime = escRampTimeThirdGear;  // about 75
#endif

  if (automatic || doubleClutch) {
    escRampTime = escRampTimeSecondGear;  // always use 2nd gear acceleration for automatic transmissions
    if (escInReverse)
      escRampTime = escRampTime * 100 / automaticReverseAccelerationPercentage;  // faster acceleration in automatic reverse, EXPERIMENTAL, TODO!
  }

  // Allows to scale vehicle file dependent acceleration
  escRampTime = escRampTime * 100 / globalAccelerationPercentage;

  // ESC ramp time compensation in low range
  if (lowRange)
    escRampTime = escRampTime * lowRangePercentage / 100;

  // Drive mode -------------------------------------------
  // Crawler mode for direct control -----
  crawlerMode = (masterVolume <= masterVolumeCrawlerThreshold);  // Direct control, depending on master volume

  if (crawlerMode) {  // almost no virtual inertia (just for drive train protection), for crawling competitions
    escRampTime = crawlerEscRampTime;
    brakeRampRate = map(currentThrottle, 0, 500, 1, 10);
    driveRampRate = 10;
  } else {  // Virtual inertia mode -----
    // calulate throttle dependent brake & acceleration steps
    brakeRampRate = map(currentThrottle, 0, 500, 1, escBrakeSteps);
    driveRampRate = map(currentThrottle, 0, 500, 1, escAccelerationSteps);
  }  // ----------------------------------------------------

  // Emergency ramp rates for falisafe
  if (failSafe) {
    brakeRampRate = escBrakeSteps;
    driveRampRate = escBrakeSteps;
  }

  // Additional brake detection signal, applied immediately. Used to prevent sound issues, if braking very quickly
  brakeDetect = ((pulse() == 1 && escPulse() == -1) || (pulse() == -1 && escPulse() == 1));

#ifdef ESC_DEBUG
  if (millis() - lastStateTime > 300) {  // Print the data every 300ms
    lastStateTime = millis();
    Serial.printf("ESC_DEBUG:\n");
    Serial.printf("driveState:            %i\n", driveState);
    Serial.printf("pulse():               %i\n", pulse());
    Serial.printf("escPulse():            %i\n", escPulse());
    Serial.printf("brakeDetect:           %s\n", brakeDetect ? "true" : "false");
    Serial.printf("escPulseMin:           %i\n", escPulseMin);
    Serial.printf("escPulseMinNeutral:    %i\n", escPulseMinNeutral);
    Serial.printf("escPulseMaxNeutral:    %i\n", escPulseMaxNeutral);
    Serial.printf("escPulseMax:           %i\n", escPulseMax);
    Serial.printf("brakeRampRate:         %i\n", brakeRampRate);
    Serial.printf("lowRange:              %s\n", lowRange ? "true" : "false");
    Serial.printf("currentRpm:            %i\n", currentRpm);
    Serial.printf("escPulseWidth:         %i\n", escPulseWidth);
    Serial.printf("escPulseWidthOut:      %i\n", escPulseWidthOut);
    Serial.printf("escSignal:             %i\n", escSignal);
    Serial.printf("motorDriverDuty:       %i\n", motorDriverDuty);
    Serial.printf("currentSpeed:          %i\n", currentSpeed);
    Serial.printf("speedLimit:            %i\n", speedLimit);
    //Serial.printf("batteryProtection:     %s\n", batteryProtection ? "true" : "false");
    //Serial.printf("batteryVoltage:        %.2f\n", batteryVoltage);
    Serial.printf("--------------------------------------\n");
  }
#endif  // ESC_DEBUG

  if (millis() - escMillis > escRampTime) {  // About very 20 - 75ms
    escMillis = millis();

    // Drive state state machine **********************************************************************************
    switch (driveState) {

      case 0:  // Standing still ---------------------------------------------------------------------
        escIsBraking = false;
        escInReverse = false;
        escIsDriving = false;
        escPulseWidth = pulseZero[3];  // ESC to neutral position
#ifdef VIRTUAL_16_SPEED_SEQUENTIAL
        selectedGear = 1;
#endif

        if (pulse() == 1 && engineRunning && !neutralGear)
          driveState = 1;  // Driving forward
        if (pulse() == -1 && engineRunning && !neutralGear)
          driveState = 3;  // Driving backwards
        break;

      case 1:  // Driving forward ---------------------------------------------------------------------
        escIsBraking = false;
        escInReverse = false;
        escIsDriving = true;
        if (escPulseWidth < pulseWidth[3] && currentSpeed < speedLimit) {
          if (escPulseWidth >= escPulseMaxNeutral)
            escPulseWidth += (driveRampRate * driveRampGain);  // Faster
          else
            escPulseWidth = escPulseMaxNeutral;  // Initial boost
        }
        if ((escPulseWidth > pulseWidth[3]) && escPulseWidth > pulseZero[3])
          escPulseWidth -= (driveRampRate * driveRampGain);  // Slower

        if (gearUpShiftingPulse && shiftingAutoThrottle && !automatic && !doubleClutch) {  // lowering RPM, if shifting up transmission
#if not defined VIRTUAL_3_SPEED && not defined VIRTUAL_16_SPEED_SEQUENTIAL                 // Only, if we have a real 3 speed transmission
          escPulseWidth -= currentSpeed / 4;                                               // Synchronize engine speed
                                                                                           // escPulseWidth -= currentSpeed * 40 / 100; // Synchronize engine speed TODO
#endif
          gearUpShiftingPulse = false;
          escPulseWidth = constrain(escPulseWidth, pulseZero[3], pulseMax[3]);
        }
        if (gearDownShiftingPulse && shiftingAutoThrottle && !automatic && !doubleClutch) {  // increasing RPM, if shifting down transmission
#if not defined VIRTUAL_3_SPEED && not defined VIRTUAL_16_SPEED_SEQUENTIAL                   // Only, if we have a real 3 speed transmission
          escPulseWidth += 50;                                                               // Synchronize engine speed
                                                                                             // escPulseWidth += currentSpeed;// * 40 / 100; // Synchronize engine speed TODO
#endif
          gearDownShiftingPulse = false;
          escPulseWidth = constrain(escPulseWidth, pulseZero[3], pulseMax[3]);
        }

        if (pulse() == -1 && escPulse() == 1)
          driveState = 2;  // Braking forward
        if (pulse() == -1 && escPulse() == 0)
          driveState = 3;  // Driving backwards, if ESC not yet moving. Prevents state machine from hanging! v9.7.0
        if (pulse() == 0 && escPulse() == 0)
          driveState = 0;  // standing still
        break;

      case 2:  // Braking forward ---------------------------------------------------------------------
        escIsBraking = true;
        escInReverse = false;
        escIsDriving = false;
        if (escPulseWidth > pulseZero[3])
          escPulseWidth -= brakeRampRate;  // brake with variable deceleration
        if (escPulseWidth < pulseZero[3] + brakeMargin && pulse() == -1)
          escPulseWidth = pulseZero[3] + brakeMargin;  // Don't go completely back to neutral, if brake applied
        if (escPulseWidth < pulseZero[3] && pulse() == 0)
          escPulseWidth = pulseZero[3];  // Overflow prevention!

        if (pulse() == 0 && escPulse() == 1 && !neutralGear) {
          driveState = 1;  // Driving forward
          airBrakeTrigger = true;
        }
        if (pulse() == 0 && escPulse() == 0) {
          driveState = 0;  // standing still
          airBrakeTrigger = true;
        }
        break;

      case 3:  // Driving backwards ---------------------------------------------------------------------
        escIsBraking = false;
        escInReverse = true;
        escIsDriving = true;
        if (escPulseWidth > pulseWidth[3] && currentSpeed < speedLimit) {
          if (escPulseWidth <= escPulseMinNeutral)
            escPulseWidth -= (driveRampRate * driveRampGain);  // Faster
          else
            escPulseWidth = escPulseMinNeutral;  // Initial boost
        }
        if ((escPulseWidth < pulseWidth[3]) && escPulseWidth < pulseZero[3])
          escPulseWidth += (driveRampRate * driveRampGain);  // Slower

        if (gearUpShiftingPulse && shiftingAutoThrottle && !automatic && !doubleClutch) {  // lowering RPM, if shifting up transmission
#if not defined VIRTUAL_3_SPEED && not defined VIRTUAL_16_SPEED_SEQUENTIAL                 // Only, if we have a real 3 speed transmission
          escPulseWidth += currentSpeed / 4;                                               // Synchronize engine speed
#endif
          gearUpShiftingPulse = false;
          escPulseWidth = constrain(escPulseWidth, pulseMin[3], pulseZero[3]);
        }
        if (gearDownShiftingPulse && shiftingAutoThrottle && !automatic && !doubleClutch) {  // increasing RPM, if shifting down transmission
#if not defined VIRTUAL_3_SPEED && not defined VIRTUAL_16_SPEED_SEQUENTIAL                   // Only, if we have a real 3 speed transmission
          escPulseWidth -= 50;                                                               // Synchronize engine speed
#endif
          gearDownShiftingPulse = false;
          escPulseWidth = constrain(escPulseWidth, pulseMin[3], pulseZero[3]);
        }

        if (pulse() == 1 && escPulse() == -1)
          driveState = 4;  // Braking backwards
        if (pulse() == 1 && escPulse() == 0)
          driveState = 1;  // Driving forward, if ESC not yet moving. Prevents state machine from hanging! v9.7.0
        if (pulse() == 0 && escPulse() == 0)
          driveState = 0;  // standing still
        break;

      case 4:  // Braking backwards ---------------------------------------------------------------------
        escIsBraking = true;
        escInReverse = true;
        escIsDriving = false;
        if (escPulseWidth < pulseZero[3])
          escPulseWidth += brakeRampRate;  // brake with variable deceleration
        if (escPulseWidth > pulseZero[3] - brakeMargin && pulse() == 1)
          escPulseWidth = pulseZero[3] - brakeMargin;  // Don't go completely back to neutral, if brake applied
        if (escPulseWidth > pulseZero[3] && pulse() == 0)
          escPulseWidth = pulseZero[3];  // Overflow prevention!

        if (pulse() == 0 && escPulse() == -1 && !neutralGear) {
          driveState = 3;  // Driving backwards
          airBrakeTrigger = true;
        }
        if (pulse() == 0 && escPulse() == 0) {
          driveState = 0;  // standing still
          airBrakeTrigger = true;
        }
        break;

    }  // End of state machine **********************************************************************************

    // Gain for drive ramp rate, depending on clutchEngagingPoint
    if (currentSpeed < clutchEngagingPoint) {
      if (!automatic && !doubleClutch)
        driveRampGain = 2;  // prevent clutch from slipping too much (2)
      else
        driveRampGain = 4;  // Automatic transmission needs to catch immediately (4)
    } else
      driveRampGain = 1;

      // ESC linearity compensation ---------------------
#ifdef QUICRUN_FUSION
    escPulseWidthOut = reMap(curveQuicrunFusion, escPulseWidth);
#elif defined QUICRUN_16BL30
    escPulseWidthOut = reMap(curveQuicrun16BL30, escPulseWidth);
#else
    escPulseWidthOut = escPulseWidth;
#endif  // --------------------------------------------

    // ESC control

    if (ReESC == 1) {
      // TRATANDO ESC FRENTE

      // AJUSTE DE MARCHA - FRENTE PARA O ESC
      // AQUI VOCÊ PODE AJUSTAR O QUÃO RÁPIDO VAI SER A VELOCIDADE DO MOTOR NAS 3 MARCHAS DISPONIVEIS QUANDO A MINIATURA ESTIVER INDO PARA FRENTE
      // PADRÃO: 1ª Velocidade Lenta 1300 a 1700   -   2ª Velocidade Média 1200 a 1800   -   3ª Velocidade Rápida 1000 a 2000 - Configure conforme sua necessidade
      // Controle de velocidades

      // ESC range & direction calibration -------------
#ifndef ESC_DIR
      // escSignal = escPulseWidthOut;
      //escSignal = map(escPulseWidthOut, escPulseMin, escPulseMax, 1000, 2000);

      if (Velocidade == 1) {
        escSignal = map(escPulseWidthOut, escPulseMin, escPulseMax, 1400, 1600);
      }
      if (Velocidade == 2) {
        escSignal = map(escPulseWidthOut, escPulseMin, escPulseMax, 1300, 1700);
      }
      if (Velocidade == 3) {
        escSignal = map(escPulseWidthOut, escPulseMin, escPulseMax, 1000, 2000);
      }


#else
      //escSignal = map(escPulseWidthOut, escPulseMax, escPulseMin, 1000, 2000);  // direction inversed
      if (Velocidade == 1) {
        escSignal = map(escPulseWidthOut, escPulseMax, escPulseMin, 1400, 1600);
      }
      if (Velocidade == 2) {
        escSignal = map(escPulseWidthOut, escPulseMax, escPulseMin, 1300, 1700);
      }
      if (Velocidade == 3) {
        escSignal = map(escPulseWidthOut, escPulseMax, escPulseMin, 1000, 2000);
      }
#endif  // --------------------------------------------


    } else {
      // TRATANDO ESC RÉ

      // AJUSTE DE MARCHA - RÉ PARA O ESC
      // AQUI VOCÊ PODE AJUSTAR O QUÃO RÁPIDO VAI SER A VELOCIDADE DO MOTOR NAS 3 MARCHAS DISPONIVEIS QUANDO A MINIATURA ESTIVER INDO PARA TRÁS
      // PADRÃO: Deixei todas as velocidades no Médio de 1200 a 1800 ---  o máximo seria de 1000 a 2000 - Configure conforme sua necessidade
      // Controle de velocidade

#ifndef ESC_DIR
      if (Velocidade == 1) {
        escSignal = map(escPulseWidthOut, escPulseMin, escPulseMax, 1300, 1700);
      }
      if (Velocidade == 2) {
        escSignal = map(escPulseWidthOut, escPulseMin, escPulseMax, 1300, 1700);
      }
      if (Velocidade == 3) {
        escSignal = map(escPulseWidthOut, escPulseMin, escPulseMax, 1300, 1700);
      }
#else
      if (Velocidade == 1) {
        escSignal = map(escPulseWidthOut, escPulseMax, escPulseMin, 1300, 1700);
      }
      if (Velocidade == 2) {
        escSignal = map(escPulseWidthOut, escPulseMax, escPulseMin, 1300, 1700);
      }
      if (Velocidade == 3) {
        escSignal = map(escPulseWidthOut, escPulseMax, escPulseMin, 1300, 1700);
      }
#endif
    }



    // modificado ativando neutro através do botão l3 (Botão do Joystick Esquerdo)
    if (l3 == true) {
      escSignal = 1500;
      // Serial.println(" NEUTRO ATIVADO ESC E PONTE H DESATIVADOS ");
    }


    if (Freio) {
      mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, 1500);  // ESC now using MCPWM
    } else {
      // Classic crawler style RC ESC mode ----
      mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, escSignal);  // ESC now using MCPWM
    }

#endif

    // Calculate a speed value from the pulsewidth signal (used as base for engine sound RPM while clutch is engaged)
    if (escPulseWidth > pulseMaxNeutral[3]) {
      currentSpeed = map(escPulseWidth, pulseMaxNeutral[3], pulseMax[3], 0, 500);
    } else if (escPulseWidth < pulseMinNeutral[3]) {
      currentSpeed = map(escPulseWidth, pulseMinNeutral[3], pulseMin[3], 0, 500);
    } else {
      currentSpeed = 0;
    }
    //#endif
  }
}



//
// =======================================================================================================
// LOOP TIME MEASUREMENT
// =======================================================================================================
//

unsigned long loopDuration() {
  static unsigned long timerOld;
  unsigned long loopTime;
  unsigned long timer = millis();
  loopTime = timer - timerOld;
  timerOld = timer;
  return loopTime;
}

//
// =======================================================================================================
// HORN, BLUELIGHT & SIREN TRIGGERING BY CH4 (POT), WINCH CONTROL
// =======================================================================================================
//

void triggerHorn() {


  // detect horn trigger ( impulse length > 1900us) -------------
  if (SomBuzina == true) {
    hornTrigger = true;
    hornLatch = true;
  } else {
    hornTrigger = false;
  }

  //#if not defined EXCAVATOR_MODE
  //#ifndef NO_SIREN
  // detect siren trigger ( impulse length < 1100us) ----------
  if (SomSirene == true) {
    sirenTrigger = true;
    sirenLatch = true;
  } else {
    sirenTrigger = false;
  }
}


//
// =======================================================================================================
// EXCAVATOR CONTROL
// =======================================================================================================
//

void excavatorControl() {

  static uint32_t lastFrameTime = millis();
  static uint16_t hydraulicPumpVolumeInternal[9];
  static uint16_t hydraulicPumpVolumeInternalUndelayed;
  static uint16_t hydraulicFlowVolumeInternalUndelayed;
  static uint16_t trackRattleVolumeInternal[9];
  static uint16_t trackRattleVolumeInternalUndelayed;
  static uint16_t lastBucketPulseWidth = pulseWidth[1];
  static uint16_t lastDipperPulseWidth = pulseWidth[2];

  if (millis() - lastFrameTime > 4) {  // 3
    lastFrameTime = millis();

    // Calculate zylinder speed and engine RPM dependent hydraulic pump volume ----
    // Bucket ---
    if (pulseWidth[1] > pulseMaxNeutral[1])
      hydraulicPumpVolumeInternal[1] = map(pulseWidth[1], pulseMaxNeutral[1], pulseMax[1], 0, 100);
    else if (pulseWidth[1] < pulseMinNeutral[1])
      hydraulicPumpVolumeInternal[1] = map(pulseWidth[1], pulseMinNeutral[1], pulseMin[1], 0, 100);
    else
      hydraulicPumpVolumeInternal[1] = 0;

    // Dipper ---
    if (pulseWidth[2] > pulseMaxNeutral[2])
      hydraulicPumpVolumeInternal[2] = map(pulseWidth[2], pulseMaxNeutral[2], pulseMax[2], 0, 100);
    else if (pulseWidth[2] < pulseMinNeutral[2])
      hydraulicPumpVolumeInternal[2] = map(pulseWidth[2], pulseMinNeutral[2], pulseMin[2], 0, 100);
    else
      hydraulicPumpVolumeInternal[2] = 0;

    // Boom (upwards only) ---
    if (pulseWidth[5] < pulseMinNeutral[5])
      hydraulicPumpVolumeInternal[5] = map(pulseWidth[5], pulseMinNeutral[5], (pulseMin[5] + 200), 0, 100);
    else
      hydraulicPumpVolumeInternal[5] = 0;

    // Swing ---
    if (pulseWidth[8] > pulseMaxNeutral[8])
      hydraulicPumpVolumeInternal[8] = map(pulseWidth[8], pulseMaxNeutral[8], (pulseMax[8] - 150), 0, 100);
    else if (pulseWidth[8] < pulseMinNeutral[8])
      hydraulicPumpVolumeInternal[8] = map(pulseWidth[8], pulseMinNeutral[8], (pulseMin[8] + 150), 0, 100);
    else
      hydraulicPumpVolumeInternal[8] = 0;

    hydraulicPumpVolumeInternalUndelayed = constrain(hydraulicPumpVolumeInternal[1] + hydraulicPumpVolumeInternal[2] + hydraulicPumpVolumeInternal[5] + hydraulicPumpVolumeInternal[8], 0, 100) * map(currentRpm, 0, 500, 30, 100) / 100;

    if (hydraulicPumpVolumeInternalUndelayed < hydraulicPumpVolume)
      hydraulicPumpVolume--;
    if (hydraulicPumpVolumeInternalUndelayed > hydraulicPumpVolume)
      hydraulicPumpVolume++;

    // Calculate zylinder speed dependent hydraulic flow volume ----
    // Boom (downwards) ---
    if (pulseWidth[5] > pulseMaxNeutral[5])
      hydraulicFlowVolumeInternalUndelayed = map(pulseWidth[5], pulseMaxNeutral[5], (pulseMax[5] - 200), 0, 100);
    else
      hydraulicFlowVolumeInternalUndelayed = 0;

    if (hydraulicFlowVolumeInternalUndelayed < hydraulicFlowVolume)
      hydraulicFlowVolume--;
    if (hydraulicFlowVolumeInternalUndelayed > hydraulicFlowVolume)
      hydraulicFlowVolume++;

    // Calculate speed dependent track rattle volume ----
    // Left ---
    if (pulseWidth[6] > pulseMaxNeutral[6])
      trackRattleVolumeInternal[6] = map(pulseWidth[6], pulseMaxNeutral[6], (pulseMax[6] - 150), 0, 100);
    else if (pulseWidth[6] < pulseMinNeutral[6])
      trackRattleVolumeInternal[6] = map(pulseWidth[6], pulseMinNeutral[6], (pulseMin[6] + 150), 0, 100);
    else
      trackRattleVolumeInternal[6] = 0;

    // Right
    if (pulseWidth[7] > pulseMaxNeutral[7])
      trackRattleVolumeInternal[7] = map(pulseWidth[7], pulseMaxNeutral[7], (pulseMax[7] - 100), 0, 100);
    else if (pulseWidth[7] < pulseMinNeutral[7])
      trackRattleVolumeInternal[7] = map(pulseWidth[7], pulseMinNeutral[7], (pulseMin[7] + 100), 0, 100);
    else
      trackRattleVolumeInternal[7] = 0;

    if (engineRunning)
      trackRattleVolumeInternalUndelayed = constrain(trackRattleVolumeInternal[6] + trackRattleVolumeInternal[7], 0, 100) * map(currentRpm, 0, 500, 100, 150) / 100;
    else
      trackRattleVolumeInternalUndelayed = 0;

    if (trackRattleVolumeInternalUndelayed < trackRattleVolume)
      trackRattleVolume--;
    if (trackRattleVolumeInternalUndelayed > trackRattleVolume)
      trackRattleVolume++;

    // Calculate hydraulic load dependent Diesel knock volume
    hydraulicDependentKnockVolume = map(hydraulicPumpVolume, 0, 100, 50, 100);

    // Calculate hydraulic load dependent engine RMP drop
    hydraulicLoad = map(hydraulicPumpVolume, 0, 100, 0, 40);

    // Bucket rattle sound triggering
    if (engineRunning && currentRpm > 400) {
      // If bucket stick is moved fast
      if (abs(pulseWidth[1] - lastBucketPulseWidth > 100)) {
        bucketRattleTrigger = true;
      }
      lastBucketPulseWidth = pulseWidth[1];

      // If dipper stick is moved fast
      if (abs(pulseWidth[2] - lastDipperPulseWidth > 100)) {
        bucketRattleTrigger = true;
      }
      lastDipperPulseWidth = pulseWidth[2];
    }
  }
}

//
// =======================================================================================================
// STEAM LOCOMOTIVE CONTROL
// =======================================================================================================
//

void steamLocomotiveControl() {
#if defined STEAM_LOCOMOTIVE_MODE
  static uint32_t lastFrameTime = millis();
  if (millis() - lastFrameTime > 4) {  // every 4ms
    lastFrameTime = millis();

    // Calculate speed dependent track rattle volume ----
    if (currentSpeed > 1)
      trackRattleVolume = map(currentSpeed, 1, 500, 10, trackRattleVolumePercentage);
    else
      trackRattleVolume = 0;
  }
#endif
}


//
// =======================================================================================================
// MAIN LOOP, RUNNING ON CORE 1
// =======================================================================================================
//

void loop() {

  CargaBateria();

  SimulaBateriaFracaNaPartida();

  // Horn triggering
  triggerHorn();


  if (xSemaphoreTake(xRpmSemaphore, portMAX_DELAY)) {
    // Map pulsewidth to throttle
    mapThrottle();


    // Excavator specific controls
#if defined EXCAVATOR_MODE
    excavatorControl();
#endif

    // Steam locomotive specific controls
#if defined STEAM_LOCOMOTIVE_MODE
    steamLocomotiveControl();
#endif

    xSemaphoreGive(xRpmSemaphore);  // Now free or "Give" the semaphore for others.
  }


  // Core ID debug
#if defined CORE_DEBUG
  Serial.print("Running on core ");
  Serial.println(coreId);
#endif

  // Feeding the RTC watchtog timer is essential!
  rtc_wdt_feed();
}

//
// =======================================================================================================
// 1st MAIN TASK, RUNNING ON CORE 0 (Interrupts are running on this core as well)
// =======================================================================================================
//

void Task1code(void *pvParameters) {
  for (;;) {

    // coreId = xPortGetCoreID(); // Running on core 0
    ControleRemoto();
    // DAC offset fader
    dacOffsetFade();

    if (xSemaphoreTake(xRpmSemaphore, portMAX_DELAY)) {
      // Simulate engine mass, generate RPM signal
      engineMassSimulation();

      // Call gear selector
      if (automatic || doubleClutch)
        automaticGearSelector();
      xSemaphoreGive(xRpmSemaphore);  // Now free or "Give" the semaphore for others.
    }

    // Switch engine on or off
    engineOnOff();

    processRawChannels();
    // Gearbox detection
    gearboxDetection();

    // ESC control & low discharge protection
    esc();


    // MOTOR DE VIBRAÇÃO
    shaker();


    // measure loop time
    loopTime = loopDuration();  // for debug only

    // Feeding the RTC watchtog timer is essential!
    rtc_wdt_feed();  // TODO, test only
  }
}



// FUNÇÃO RESPONSÁVEL PELO CONTROLE TOTAL DA MINIATURA: FRENTE, RÉ, ACELERAÇÃO, DIREÇÃO, LUZES E SERVOS
//***************************************************************
//


void ControleRemoto() {

  indicatorSoundOn = SomSeta;




  if (AtivaSistemaAnteFalhas == 1) {

    if (BP32.update()) {
      Cont = 0;
    } else {
      Cont++;
    }
    if (Incio == false) {
      Cont = 0;
    }

    if (Cont > TempoSemSinal) {
      ledcWrite(10, 255);
      ledcWrite(11, 255);
      mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, 1500);  // ESC now using MCPWM
      PWM = 0;
      joyEsquerdaY = 128;
      engineOn = false;
      PiscaAlerta();
      Serial.println("       DESCONECTADO");
      ESP.restart();
    }
  } else {
    BP32.update();
  }

  if (Cont > TempoSemSinal + 10000) {
    Cont = TempoSemSinal + 5000;
  }



  GamepadPtr myGamepad = myGamepads[0];
  if (myGamepad && myGamepad->isConnected()) {
    Incio = true;
    // myGamepad->setColorLED(0, 0, 255);  // Comando para ativar a luz do controle na cor desejada - Não funciona em controles paralelos

    // BOTÃO PS
    // Ativa e Desativa Som de Partida
    if (myGamepad->miscSystem()) {
      if (Auxps == LOW) {
        Auxps = HIGH;
        ps = !ps;
        Serial.print("Botao PS pressionado: ");
        Serial.println(ps);
        if (myGamepad->r2()) {
          Bateria_10_Trigger = true;
          Serial.println("       BOTÃO X E BOTÃO R2 PRESSIONADOS JUNTOS");
          Serial.println("       CONSULTAR NIVEL DE CARGA DA BATERIA DO RECEPTOR");
          Serial.print("CARGA DA BATERIA DO RECEPTOR: ");
          Serial.print(bat_percentage);
          Serial.println("%");
        } else {
          engineOn = !engineOn;  // SOM PARTIDA
        }
      }
    } else {
      Auxps = LOW;
    }




    // BOTÃO OPTIONS
    // ATIVA 03 MODOS DE SIRENE E GIROFLEX
    if (myGamepad->miscHome()) {
      if (Auxoptions == LOW) {
        Auxoptions = HIGH;
        options = !options;
        //delay(30);
        Serial.print("Botao options pressionado: ");
        Serial.println(options);
        ConteGiroflex++;
        if (ConteGiroflex > 3) {
          ConteGiroflex = 0;
        }
        Millis_Giroflex = millis();
        digitalWrite(GIROFLEX_AZUL, LOW);
        digitalWrite(GIROFLEX_VERMEHO, LOW);
      }
    } else {
      Auxoptions = LOW;
    }

    Giroflex(ConteGiroflex);

    // BOTÃO share
    // CONTROLA 4 POSIÇÕES DO VOLUME DO SOM (MUDO, BAIXO, MÉDIO E ALTO)
    if (myGamepad->miscBack()) {
      if (Auxshare == LOW) {
        Auxshare = HIGH;
        share = !share;
        //delay(30);
        Serial.print("Botao share pressionado: ");
        Serial.println(share);
        Volume++;
        if (Volume > 3) {
          Volume = 0;
        }
      }
    } else {
      Auxshare = LOW;
    }


    if (Volume == 0) {
      VolumeMaster = 0;
      //Serial.println("********************************** 0 ");
    }
    if (Volume == 1) {
      VolumeMaster = 33;
      //Serial.println("********************************** 33 ");
    }
    if (Volume == 2) {
      VolumeMaster = 60;
      //Serial.println("********************************** 70 ");
    }
    if (Volume == 3) {
      VolumeMaster = 100;
      //Serial.println("********************************** 100 ");
    }




    // BOTÃO X  (sem tratamento)
    // ACIONA BUZINA
    if (myGamepad->a()) {
      x = 1;
      Serial.print("Botao X pressionado: ");
      Serial.println(x);
    } else {
      x = 0;
    }

    if (x == 1) {
      SomBuzina = true;
    } else {
      SomBuzina = false;
    }


    // BOTÃO QUADRADO
    // ACIONA FAROL DE MILHA
    if (myGamepad->x()) {
      if (Auxquadrado == LOW) {
        Auxquadrado = HIGH;
        quadrado = !quadrado;
        Serial.print("Botao Quadrado pressionado: ");
        Serial.println(quadrado);
        digitalWrite(FAROL_MILHA, quadrado);
      }
    } else {
      Auxquadrado = LOW;
    }



    // BOTÃO TRIANGULO
    // ACIONA PISCA-ALERTA
    if (myGamepad->y()) {
      if (Auxtriangulo == LOW) {
        Auxtriangulo = HIGH;
        triangulo = !triangulo;
        Serial.print("Botao Triangulo pressionado: ");
        Serial.println(triangulo);

        digitalWrite(SETA_DIREITA, LOW);
        digitalWrite(SETA_ESQUERDA, LOW);
        SomSeta = false;
      }
    } else {
      Auxtriangulo = LOW;
    }

    if (triangulo == 1) {
      PiscaAlerta();
      l1 = 0;
      r1 = 0;
    } else {
    }




    // BOTÃO CÍRCULO
    // ACIONA FAROL ALTO E BAIXO
    if (myGamepad->b()) {
      if (Auxcirculo == LOW) {
        Auxcirculo = HIGH;
        circulo = !circulo;
        Serial.print("Botao Circulo pressionado: ");
        Serial.println(circulo);
        Farol++;
        if (Farol > 2) {
          Farol = 0;
        }
        if (Farol == 1) {
          ledcWrite(12, 255);  // Farol Alto
          ledcWrite(13, 80);   // Acende Lanterna
        }
        if (Farol == 2) {
          ledcWrite(12, 100);  // Farol Baixo
          ledcWrite(13, 80);   // Acende Lanterna
        }
        if (Farol == 0) {
          ledcWrite(12, 0);  // Farol Desligado
          ledcWrite(13, 0);  // Apaga Lanterna
        }
      }
    } else {
      Auxcirculo = LOW;
    }





    if (myGamepad->r2()) {  // Se o gatilho R2 ESTIVER  pressionado as setas do controle são usadas para a movimentação dos Servos auxiliares


      //TRATANDO MOVIMENTAÇÃO DOS SERVOS AUXILIARES

      // ACIONANDO SERVO AUXILIAR 01

      // BOTÃO SETA PARA CIMA
      // ACIONA SERVO AUXILIAR 01
      if (myGamepad->dpad() == 1) {

        if (millis() - Millis_ServoAux01 < VelSerAux01) {
        } else {
          ServoAux01++;
          if (ServoAux01 > 180) {
            ServoAux01 = 180;
          }
          servoAux1.write(ServoAux01);
          //Serial.print("SERVO AUX01: ");
          //Serial.println(ServoAux01);
          Millis_ServoAux01 = millis();
        }
      }

      // BOTÃO SETA PARA BAIXO
      // ACIONA SERVO AUXILIAR 01
      if (myGamepad->dpad() == 2) {
        if (millis() - Millis_ServoAux01 < VelSerAux01) {
        } else {
          ServoAux01--;
          if (ServoAux01 < 1) {
            ServoAux01 = 0;
          }
          servoAux1.write(ServoAux01);
          //Serial.print("SERVO AUX01: ");
          //Serial.println(ServoAux01);
          Millis_ServoAux01 = millis();
        }
      }

      // ACIONANDO SERVO AUXILIAR 02

      // BOTÃO SETA PARA ESQUERDA
      // ACIONA SERVO AUXILIAR 02
      if (myGamepad->dpad() == 8) {

        if (millis() - Millis_ServoAux02 < VelSerAux02) {
        } else {
          ServoAux02--;
          if (ServoAux02 < 1) {
            ServoAux02 = 0;
          }
          servoAux2.write(ServoAux02);
          //Serial.print("SERVO AUX02: ");
          //Serial.println(ServoAux02);
          Millis_ServoAux02 = millis();
        }
      }



      // BOTÃO SETA PARA DIREITA
      // ACIONA SERVO AUXILIAR 02
      if (myGamepad->dpad() == 4) {

        if (millis() - Millis_ServoAux02 < VelSerAux02) {
        } else {
          ServoAux02++;
          if (ServoAux02 > 180) {
            ServoAux02 = 180;
          }
          servoAux2.write(ServoAux02);
          //Serial.print("SERVO AUX02: ");
          //Serial.println(ServoAux02);
          Millis_ServoAux02 = millis();
        }
      }


    } else {  // Se o gatilho R2 não estiver pressionado as setas do controle são usadas para as luzes auxiliares

      // BOTÃO SETA PARA CIMA
      // ACIONA LUZ AUXILIAR 01
      if (myGamepad->dpad() == 1) {
        if (AuxsetaCima == LOW) {
          AuxsetaCima = HIGH;
          setaCima = !setaCima;
          //delay(30);
          Serial.print("Botao Seta Cima pressionado: ");
          Serial.println(setaCima);
          digitalWrite(LUZ_AUX01, setaCima);
        }
      } else {
        AuxsetaCima = LOW;
      }



      // BOTÃO SETA PARA ESQUERDA
      // ACIONA LUZ AUXILIAR 02
      if (myGamepad->dpad() == 8) {
        if (AuxsetaEsquerda == LOW) {
          AuxsetaEsquerda = HIGH;
          setaEsquerda = !setaEsquerda;
          //delay(30);
          Serial.print("Botao Seta Esquerda pressionado: ");
          Serial.println(setaEsquerda);
          digitalWrite(LUZ_AUX02, setaEsquerda);
        }
      } else {
        AuxsetaEsquerda = LOW;
      }



      // BOTÃO SETA PARA DIREITA
      // ACIONA LUZ AUXILIAR 03
      if (myGamepad->dpad() == 4) {
        if (AuxsetaDireita == LOW) {
          AuxsetaDireita = HIGH;
          setaDireita = !setaDireita;
          //delay(30);
          Serial.print("Botao Seta Direita pressionado: ");
          Serial.println(setaDireita);
          digitalWrite(LUZ_AUX03, setaDireita);
        }
      } else {
        AuxsetaDireita = LOW;
      }



      // BOTÃO SETA PARA BAIXO
      // ACIONA LUZ AUXILIAR 04
      if (myGamepad->dpad() == 2) {
        if (AuxsetaBaixo == LOW) {
          AuxsetaBaixo = HIGH;
          setaBaixo = !setaBaixo;
          //delay(30);
          Serial.print("Botao Seta Baixo pressionado: ");
          Serial.println(setaBaixo);
          digitalWrite(LUZ_AUX04, setaBaixo);
        }
      } else {
        AuxsetaBaixo = LOW;
      }
    }




    // BOTÃO R1
    // ACIONA SETA DIREITA
    if (myGamepad->r1()) {
      if (Auxr1 == LOW) {
        Auxr1 = HIGH;
        if (triangulo == false) {  // Somente se o Pisca-Alerta estiver desligado a seta é assionada
          r1 = !r1;
          //delay(30);
          Serial.print("Botao R1 pressionado: ");
          Serial.println(r1);

          digitalWrite(SETA_DIREITA, LOW);
          digitalWrite(SETA_ESQUERDA, LOW);
          SomSeta = false;
        }
      }
    } else {
      Auxr1 = LOW;
    }

    if (r1 == 1) {
      if (triangulo == 0) {
        SetaDireita();
        l1 = 0;
      }
    }



    // BOTÃO L1
    // ACIONA SETA ESQUERDA
    if (myGamepad->l1()) {
      if (Auxl1 == LOW) {
        Auxl1 = HIGH;
        if (triangulo == false) {  // Somente se o Pisca-Alerta estiver desligado a seta é assionada
          l1 = !l1;
          //delay(30);
          Serial.print("Botao L1 pressionado: ");
          Serial.println(l1);

          digitalWrite(SETA_DIREITA, LOW);
          digitalWrite(SETA_ESQUERDA, LOW);
          SomSeta = false;
        }
      }
    } else {
      Auxl1 = LOW;
    }

    if (l1 == 1) {
      if (triangulo == 0) {
        SetaEsquerda();
        r1 = 0;
      }
    }



    //BOTÃO GATILHO L2
    //ACIONA TROCA DE VELOCIDADES - MARCHAS SIMULADAS
    if (myGamepad->l2()) {
      if (Auxl2 == LOW) {
        Auxl2 = HIGH;
        l2 = !l2;
        //delay(30);
        Serial.print("Botao L2 pressionado: ");
        Serial.println(l2);

        Velocidade++;
        if (Velocidade > 3) {
          Velocidade = 1;
        }
        shiftingTrigger = true;
        // Alterna cores da luz do controle de ps4 de acordo com a marcha
        if (l3 == LOW) {
          if (Velocidade == 1) {
            myGamepad->setColorLED(0, 255, 0);  // VERDE
          }
          if (Velocidade == 2) {
            myGamepad->setColorLED(0, 0, 255);  // AZUL
          }
          if (Velocidade == 3) {
            myGamepad->setColorLED(255, 0, 0);  // VERMELHO
          }
        } else {
          myGamepad->setColorLED(255, 255, 255);  // BRANCO
        }
      }
    } else {
      Auxl2 = LOW;
    }



    // BOTÃO GATILHO R2
    // USADO APENAS PARA TIVAR SEGUNDA FUNÇÃO DOS BOTÕES SETAS DO CONTROLE PARA MOVIMENTAÇÃO DOS SERVOS AUXILIARES E DO BOTÃO PS PARA CONFERIR A CARGA DA BATERIA
    if (myGamepad->r2()) {
      if (Auxr2 == LOW) {
        Auxr2 = HIGH;
        r2 = !r2;
        //delay(30);
        Serial.print("Botao R2 pressionado: ");
        Serial.println(r2);
      }
    } else {
      Auxr2 = LOW;
    }



    // BOTÃO r3 (PUSH BUTTON DO JOYSTICK DIREITO)
    // ATIVA E DESATIVA O ACIONAMENTO DAS SETAS ATRAVÉS DO MOVIMENTO DO JOYSTICK DE DIREÇÃO
    if (myGamepad->thumbR()) {
      if (Auxr3 == LOW) {
        Auxr3 = HIGH;
        r3 = !r3;
        //delay(30);
        Serial.print("Botao r3 JoyD pressionado: ");
        Serial.println(r3);
      }
    } else {
      Auxr3 = LOW;
    }



    // BOTÃO l3 (PUSH BUTTON DO JOYSTICK ESQUERDO)
    // ATIVA DESATIVA NEUTRO - PONTO MORTO
    if (myGamepad->thumbL()) {
      if (Auxl3 == LOW) {
        Auxl3 = HIGH;
        l3 = !l3;
        //delay(30);
        Serial.print("Botao l3 JoyE pressionado: ");
        Serial.println(l3);
        shiftingTrigger = true;

        // Alterna cores da luz do controle de ps4 de acordo com a marcha
        if (l3 == LOW) {
          if (Velocidade == 1) {
            myGamepad->setColorLED(0, 255, 0);  // VERDE
          }
          if (Velocidade == 2) {
            myGamepad->setColorLED(0, 0, 255);  // AZUL
          }
          if (Velocidade == 3) {
            myGamepad->setColorLED(255, 0, 0);  // VERMELHO
          }
        } else {
          myGamepad->setColorLED(255, 255, 255);  // BRANCO
        }
      }
    } else {
      Auxl3 = LOW;
    }



    // *** JOYSTICKS ANALÓGICOS

    //Recebendo valores dos joysticks analógicos do controle de ps3
    joyEsquerdaX = map(myGamepad->axisX(), -508, 512, 0, 255);  // NÃO UTILIZADO
    joyEsquerdaY = map(myGamepad->axisY(), -508, 512, 255, 0);  // ACELERAÇÃO, FRENTE E RÉ
    joyDireitaX = map(myGamepad->axisRX(), -508, 512, 0, 255);  // SERVO DE DIREÇÃO
    joyDireitaY = map(myGamepad->axisRY(), -508, 512, 0, 255);  // NÃO UTILIZADO

    GD = map(myGamepad->throttle(), 0, 1020, 0, 255);  // r2
    GE = map(myGamepad->brake(), 0, 1020, 0, 255);     // l2


    // Serial.print("joyEX: ");
    // Serial.print(joyEsquerdaX);
    // Serial.print("  joyEY: ");
    // Serial.print(joyEsquerdaY);
    // Serial.print("  joyDX: ");
    // Serial.print(joyDireitaX);
    // Serial.print("  joyDX: ");
    // Serial.print(joyDireitaY);
    // Serial.print("  GE: ");
    // Serial.print(GE);
    // Serial.print("  GD: ");
    // Serial.println(GD);



    // PROGRAMANDO CONTROLE DE ACELERAÇÃO FRENTE RÉ E FREIO DO VEÍCULO


    // *** FRENTE (BASTÃO DO JOYSTICK EMPURRADO PARA FRENTE)
    if (joyEsquerdaY >= 135) {
      Freio = false;
      // TRATANDO PONTE H

      // AJUSTE DE VELOCIDADE - FRENTE
      // AQUI VOCÊ PODE AJUSTAR O QUÃO RÁPIDO VAI SER A VELOCIDADE DO MOTOR NAS 3 MARCHAS DISPONIVEIS QUANDO A MINIATURA ESTIVER INDO PARA FRENTE
      // PADRÃO: 1ª VELOCIDADE PWM 100   -   2ª VELOCIDADE PWM 200   -   3ª VELOCIDADE PWM 255 - Configure conforme sua necessidade
      // Controle para simulação de Marchas - 03 velocidades

      // MARCHA 1
      if (Velocidade == 1) {
        PWM = map(escSignal, 1500, 2000, 0, 100);
        if (joyEsquerdaY > 230) {
          PWM = 100;
        }
      }

      // MARCHA 2
      if (Velocidade == 2) {
        PWM = map(escSignal, 1500, 2000, 0, 200);
        if (joyEsquerdaY > 230) {
          PWM = 200;
        }
      }

      // MARCHA 3
      if (Velocidade == 3) {
        PWM = map(escSignal, 1500, 2000, 0, 255);
        if (joyEsquerdaY > 230) {
          PWM = 255;
        }
      }

      Serial.print(" Aceleração Re: ");
      Serial.println(PWM);


      if (escIsBraking == false) {  // Para verificar se deve ser acionado Motor para frente ou Freiar

        if (l3 == HIGH) {
          ledcWrite(10, LOW);
          ledcWrite(11, LOW);
          // Serial.println(" NEUTRO ATIVADO ESC E PONTE H DESATIVADOS ");
        } else {
          if (engineStart == false) {
            ledcWrite(10, LOW);
            ledcWrite(11, PWM);
          }
        }

        digitalWrite(LUZ_RE, LOW);  // APAGA LUZ DE RÉ
        if (Farol == 0) {
          ledcWrite(13, 0);  // Apaga luz de freio
        } else {
          ledcWrite(13, 80);  // Diminui a intensidade da luz para Lanterna
        }
        Re = false;  //jOYSTICK PARA CIMA
      }

      if (escIsBraking == 1) {
        //Serial.println("FREIO ACIONADO");
        ledcWrite(10, 255);
        ledcWrite(11, 255);
        ledcWrite(13, 255);         // Acende luz de freio
        digitalWrite(LUZ_RE, LOW);  // Apaga Luz de Ré
        PWM = 0;
        FreioReal = true;
      }
      ReESC = 1;

      Sinal_LuzFreio = millis();
      Millis_LuzRe = millis();

      FreioReal = false;
    }



    // ************************************************************ //



    // *** MEIO (BASTÃO DO JOYSTICK EM REPOUSO NO MEIO DO PERCURSO)
    if ((joyEsquerdaY < 135) && (joyEsquerdaY > 120)) {

      // TRATANDO PONTE H
      ledcWrite(10, LOW);
      ledcWrite(11, LOW);

      digitalWrite(LUZ_RE, LOW);  // APAGA LUZ DE RÉ

      if (escIsBraking == 1) {
        //Serial.println("FREIO ACIONADO");
        ledcWrite(10, 255);
        ledcWrite(11, 255);
        ledcWrite(13, 255);         // Acende luz de freio
        digitalWrite(LUZ_RE, LOW);  // Apaga Luz de Ré
        PWM = 0;
        FreioReal = true;
      }


      // ACIONA LUZ E SOM DE FREIO SIMULADOS AO DEIXAR O BASTÃO DO JOYSITCK NA POSIÇÃO CENTRAL DEPOIS DE ACELERAR PARA FRENTE OU RÉ
      if (AtivaSimulaFreio) {

        if (!FreioReal) {
          Freio = true;

          if (engineOn) {
            if (engineStart == false) {
              if (millis() - Sinal_LuzFreio < 400) {
              } else if (millis() - Sinal_LuzFreio < 1200) {

                Freio = true;
                ledcWrite(10, 0);
                ledcWrite(11, 0);
                ledcWrite(13, 255);         // Acende luz de freio
                                            //Serial.println("LUZ DE FREIO ACIONADA! ");
                digitalWrite(LUZ_RE, LOW);  // Apaga Luz de Ré
                PWM = 0;
                escSignal = 1500;

              } else if (millis() - Sinal_LuzFreio < 1300) {
                airBrakeTrigger = true;
              } else {
                if (escIsBraking == 0) {
                  if (Farol == 0) {
                    ledcWrite(13, 0);  // Apaga luz de freio
                  } else {
                    ledcWrite(13, 80);  // Diminui a intensidade da luz para Lanterna
                  }
                }
              }
            }
          }
        } else {
          if (Farol == 0) {
            ledcWrite(13, 0);  // Apaga luz de freio
          } else {
            ledcWrite(13, 80);  // Diminui a intensidade da luz para Lanterna
          }
        }
      } else {
        if (Farol == 0) {
          ledcWrite(13, 0);  // Apaga luz de freio
        } else {
          ledcWrite(13, 80);  // Diminui a intensidade da luz para Lanterna
        }
      }
    }


    // ************************************************************ //


    // ***RÉ (BASTÃO DO JOYSTICK EMPURRADO PARA TRÁS)
    if (joyEsquerdaY <= 120) {

      Freio = false;
      // TRATANDO PONTE H

      // AJUSTE DE VELOCIDADE - RÉ
      // AQUI VOCÊ PODE AJUSTAR O QUÃO RÁPIDO VAI SER A VELOCIDADE DO MOTOR NAS 3 MARCHAS DISPONIVEIS QUANDO A MINIATURA ESTIVER INDO PARA TRÁS
      // PADRÃO: Deixei todas as velocidades em um nível médio PWM 200 - Modifique a seu gosto
      // Controle de velocidade


      // MARCHA 1
      if (Velocidade == 1) {
        PWM = map(escSignal, 1500, 1000, 0, 200);
        if (joyEsquerdaY < 25) {
          PWM = 200;
        }
      }

      // MARCHA 2
      if (Velocidade == 2) {
        PWM = map(escSignal, 1500, 1000, 0, 200);
        if (joyEsquerdaY < 25) {
          PWM = 200;
        }
      }

      // MARCHA 3
      if (Velocidade == 3) {
        PWM = map(escSignal, 1500, 1000, 0, 200);
        if (joyEsquerdaY < 25) {
          PWM = 200;
        }
      }

      Serial.print(" Aceleração Frente: ");
      Serial.println(PWM);

      if (escIsBraking == false) {  // Para verificar se deve ser acionado Motor para trás ou Freiar

        if (l3 == HIGH) {
          ledcWrite(10, LOW);
          ledcWrite(11, LOW);
          // Serial.println(" NEUTRO ATIVADO ESC E PONTE H DESATIVADOS ");
        } else {
          if (engineStart == false) {
            ledcWrite(10, PWM);
            ledcWrite(11, LOW);
            //Serial.println(" RE ");
          }
        }
        if (Farol == 0) {
          ledcWrite(13, 0);  // Apaga luz de freio
        } else {
          ledcWrite(13, 80);  // Diminui a intensidade da luz para Lanterna
        }
        if (millis() - Millis_LuzRe > 500) {  // tempo necessário para que a luz de ré só seja acinada se o freio não for acionado, caso contrário ela pode sempre piscar ao se frear
          digitalWrite(LUZ_RE, HIGH);         // ACENDE LUZ DE RÉ
        }
        Re = true;  // RÉ JOYSTICK PARA BAIXO
      }


      if (escIsBraking == 1) {
        //Serial.println("FREIO ACIONADO");
        ledcWrite(10, 255);
        ledcWrite(11, 255);
        ledcWrite(13, 255);         // Acende luz de freio
        digitalWrite(LUZ_RE, LOW);  // Apaga Luz de Ré
        PWM = 0;
        FreioReal = true;
      }

      FreioReal = false;
      ReESC = 0;

      Sinal_LuzFreio = millis();
    }




    // *** TRATANDO MOVIMENTAÇÃO DO SERVO MOTOR DE DIREÇÃO
    // AJUSTE DE DIREÇÃO

    // Caso queira LIMITAR os movimentos do braço do servo de direção altere os valores 0, 180

    GrauServo = map(joyDireitaX, 0, 255, 0, 100);  // Movimentação total do Servo 180 graus

    //GrauServo = map(joyDireitaX, 0, 255, 30, 150); // Movimentação Limitada em 120 graus
    //GrauServo = map(joyDireitaX, 0, 255, 50, 130); // Movimentação Limitada em 80 graus
    //GrauServo = map(joyDireitaX, 0, 255, 70, 110); // Movimentação Limitada em 40 graus

    // Para INVERTER o movimento do braço do servo basta inverter os valor 0,180 para 180, 0
    //GrauServo = map(joyDireitaX, 0, 255, 180, 0);  // Invertendo movimento do braço do servo



    // Ajusta a velocidade de resposta do servo para deixa-lo mais suave
    if (millis() - Millis_ServoDirecao < VelServoDirecao) {

    } else {

      if (AuxGrauServo < GrauServo) {
        AuxGrauServo++;
        if (AuxGrauServo > GrauServo) {
          AuxGrauServo = GrauServo;
        }
      }

      if (AuxGrauServo > GrauServo) {
        AuxGrauServo--;
        if (AuxGrauServo < GrauServo) {
          AuxGrauServo = GrauServo;
        }
      }
      servoDirecao.write(AuxGrauServo);
      Millis_ServoDirecao = millis();
    }



    // MOSTRA NO MONITOR SERIAL OS VALORES DE COMANDO DO SERVO DE DIREÇÃO APENAS QUANDO O JOYSTICK É MOVIMENTADO
    if (AuxGrauServo != 90) {
      Serial.print(" Servo: ");
      Serial.println(AuxGrauServo);
    }


    // ATIVA ACIONAMENTO DAS SETAS AO MOVIMENTAR JOYSTICK DE DIREÇÃO PARA CURVAS
    if (r3 == 0) {

      if (triangulo == false) {

        if (joyDireitaX >= 150) {
          r1 = 1;
          l1 = 0;
          digitalWrite(SETA_DIREITA, 0);  // Apaga led seta direita
          SinalSeta = false;
        } else if (joyDireitaX <= 105) {
          l1 = 1;
          r1 = 0;
          digitalWrite(SETA_ESQUERDA, 0);  // Apaga led seta esquerda
          SinalSeta = false;
        } else {
          if (SinalSeta == false) {
            SinalSeta = true;
            r1 = 0;
            l1 = 0;
            digitalWrite(SETA_DIREITA, 0);   // Apaga led seta direita
            digitalWrite(SETA_ESQUERDA, 0);  // Apaga led seta esquerda
            SomSeta = false;
          }
        }
      }
    }



    // Serial.print("escIsBraking: ");
    // Serial.print(escIsBraking);
    // Serial.print(" escSignal: ");
    // Serial.print(escSignal);
    // Serial.print(" PWM: ");
    // Serial.print(PWM);
    // Serial.print(" Joystick: ");
    // Serial.print(joyEsquerdaY);
    // Serial.print(" Freio: ");
    // Serial.print(Freio);
    // Serial.print(" FreioReal: ");
    // Serial.print(FreioReal);
    // Serial.print(" ServoAux01: ");
    // Serial.print(ServoAux01);
    // Serial.print(" ServoAux02: ");
    // Serial.print(ServoAux02);
    // Serial.print(" AuxGrauServo: ");
    // Serial.print(AuxGrauServo);
    // Serial.print(" dpad(): ");
    // Serial.print(myGamepad->dpad());
    // Serial.print(" Conectado: ");
    // Serial.print(Conectado);
    // Serial.print(" bateria ");
    // Serial.println(myGamepad->battery());

    if (sinalDesconectado) {
      digitalWrite(SETA_ESQUERDA, LOW);
      digitalWrite(SETA_ESQUERDA, LOW);
      SomSeta = false;
      sinalDesconectado = false;
    }
  } else {


    if (millis() - Millis_Desconectado > 500) {  // Se o controle permanecer desconectado por mais de 100 milessegundos o motor é desligado, as luzes são apagadas e o pisca-alerta é ativado até que o controle seja novamente conectado

      Serial.println("CONTROLE DESCONECTADO! ");

      // Apaga todas as luzes
      ledcWrite(12, LOW);
      ledcWrite(13, LOW);
      digitalWrite(FAROL_MILHA, LOW);
      digitalWrite(LUZ_AUX01, LOW);
      digitalWrite(LUZ_AUX02, LOW);
      digitalWrite(LUZ_AUX03, LOW);
      digitalWrite(LUZ_AUX04, LOW);

      Millis_Desconectado = millis();
    }
    ledcWrite(10, 255);
    ledcWrite(11, 255);
    mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, 1500);  // ESC now using MCPWM
    PWM = 0;
    joyEsquerdaY = 128;
    engineOn = false;
    PiscaAlerta();

    sinalDesconectado = true;
  }
  vTaskDelay(1);
}





///////////////////////////////////////////////////////
void SetaDireita() {
  if (millis() - Millis_SetaEsquerda > 340) {
    digitalWrite(SETA_ESQUERDA, !digitalRead(SETA_ESQUERDA));
    Millis_SetaEsquerda = millis();
    SomSeta = true;
  }
}
//////////////////////////////////////////////////////////
void SetaEsquerda() {
  if (millis() - Millis_SetaDireita > 340) {
    digitalWrite(SETA_DIREITA, !digitalRead(SETA_DIREITA));
    Millis_SetaDireita = millis();
    SomSeta = true;
  }
}
///////////////////////////////////////////////////////
void PiscaAlerta() {
  if (millis() - Millis_PiscaAlerta > 340) {
    digitalWrite(SETA_DIREITA, !digitalRead(SETA_DIREITA));
    digitalWrite(SETA_ESQUERDA, !digitalRead(SETA_ESQUERDA));
    Millis_PiscaAlerta = millis();
    SomSeta = true;
  }
}
///////////////////////////////////////////////////////
void Giroflex(int Modo) {

  // Tratando som
  if (Modo == 0) {
    SomSirene = false;
  } else {
    SomSirene = true;
  }

  // Tratando luzes
  if (Modo == 1) {

    if ((millis() - Millis_Giroflex > 100) && (millis() - Millis_Giroflex < 200)) {
      digitalWrite(GIROFLEX_AZUL, LOW);
      digitalWrite(GIROFLEX_VERMEHO, HIGH);
    }
    if ((millis() - Millis_Giroflex > 200) && (millis() - Millis_Giroflex < 300)) {
      digitalWrite(GIROFLEX_AZUL, LOW);
      digitalWrite(GIROFLEX_VERMEHO, LOW);
    }
    if ((millis() - Millis_Giroflex > 300) && (millis() - Millis_Giroflex < 400)) {
      digitalWrite(GIROFLEX_AZUL, LOW);
      digitalWrite(GIROFLEX_VERMEHO, HIGH);
    }
    if ((millis() - Millis_Giroflex > 400) && (millis() - Millis_Giroflex < 500)) {
      digitalWrite(GIROFLEX_AZUL, LOW);
      digitalWrite(GIROFLEX_VERMEHO, LOW);
    }
    if ((millis() - Millis_Giroflex > 500) && (millis() - Millis_Giroflex < 600)) {
      digitalWrite(GIROFLEX_AZUL, LOW);
      digitalWrite(GIROFLEX_VERMEHO, HIGH);
    }
    if ((millis() - Millis_Giroflex > 600) && (millis() - Millis_Giroflex < 700)) {
      digitalWrite(GIROFLEX_VERMEHO, LOW);
      digitalWrite(GIROFLEX_AZUL, HIGH);
    }
    if ((millis() - Millis_Giroflex > 700) && (millis() - Millis_Giroflex < 800)) {
      digitalWrite(GIROFLEX_VERMEHO, LOW);
      digitalWrite(GIROFLEX_AZUL, LOW);
    }
    if ((millis() - Millis_Giroflex > 800) && (millis() - Millis_Giroflex < 900)) {
      digitalWrite(GIROFLEX_VERMEHO, LOW);
      digitalWrite(GIROFLEX_AZUL, HIGH);
    }
    if ((millis() - Millis_Giroflex > 900) && (millis() - Millis_Giroflex < 1000)) {
      digitalWrite(GIROFLEX_VERMEHO, LOW);
      digitalWrite(GIROFLEX_AZUL, LOW);
    }
    if ((millis() - Millis_Giroflex > 1000) && (millis() - Millis_Giroflex < 1100)) {
      digitalWrite(GIROFLEX_VERMEHO, LOW);
      digitalWrite(GIROFLEX_AZUL, HIGH);

      Millis_Giroflex = millis();
    }

    // ATIVA MODO 2 DE PISCAR OS LEDS
  } else if (Modo == 2) {

    if ((millis() - Millis_Giroflex > 100) && (millis() - Millis_Giroflex < 200)) {
      digitalWrite(GIROFLEX_VERMEHO, LOW);
      digitalWrite(GIROFLEX_AZUL, HIGH);
    }
    if ((millis() - Millis_Giroflex > 200) && (millis() - Millis_Giroflex < 300)) {
      digitalWrite(GIROFLEX_VERMEHO, HIGH);
      digitalWrite(GIROFLEX_AZUL, LOW);

      Millis_Giroflex = millis();
    }

    // ATIVA MODO 3 DE PISCAR OS LEDS
  } else if (Modo == 3) {

    if ((millis() - Millis_Giroflex > 100) && (millis() - Millis_Giroflex < 200)) {
      digitalWrite(GIROFLEX_VERMEHO, HIGH);
      digitalWrite(GIROFLEX_AZUL, HIGH);
    }
    if ((millis() - Millis_Giroflex > 200) && (millis() - Millis_Giroflex < 300)) {
      digitalWrite(GIROFLEX_VERMEHO, LOW);
      digitalWrite(GIROFLEX_AZUL, LOW);

      Millis_Giroflex = millis();
    }
  }
}
///////////////////////////////////////////////////////
//////////////////////////// LEITURA DA CARGA BATERIA //


// LEITURA TENSÃO DA BATERIA
void CargaBateria() {
  // Se você Ativar a Leitura da carga da bateria
  if (AtivaLeituraTensao == 1) {

    if (Bateria_10_Trigger == 0) {
      sensorValue = analogRead(BAT_Pin);
      delay(1);
    }
    // Se usar apenas 01 Bateria 4.2v Usar dois resisitores de 10k no divisor de tensão e Multiplicar por 2 na formula abaixo
    if (Numero_de_Baterias == 1) {
      voltage = (((sensorValue * 3.3) / 4095) * 2 + calibration);  // Multiplique por 2 para ajustar a tensão quando usar apenas uma bateria
      bat_percentage = mapfloat(voltage, 3.3, 4.2, 0, 100);        //3.3v é a tensão mínima da bateria 0% e 4.2V é a tensão máxima 100%
    }

    // Se usar 02 Baterias em série 8.4v Usar 01 resistor de 10k no GND e um de 30k no Vcc (8.4v) e Multiplicar por 4 na formula abaixo
    if (Numero_de_Baterias == 2) {
      voltage = (((sensorValue * 3.3) / 4095) * 4 + calibration);  //// Multiplique por 4 para ajustar a tensão quando usar duas baterias em série
      bat_percentage = mapfloat(voltage, 6.6, 8.4, 0, 100);        //6.6v é a tensão mínima das 2 baterias em série (0%) e 8.4V é a tensão máxima das 2 baterias em série (100%)
    }


    if (bat_percentage >= 100) {
      bat_percentage = 100;
    }
    if (bat_percentage <= 0) {
      bat_percentage = 0;
    }

    // AJUSTE
    // PARA CONSEGUIR CALIBRAR A TENSÃO DA BATERIA VOCÊ DEVE RETIRAR OS COMENTÁRIOS ABAIXO PARA VISUALIZAR A LEITURA DE TENSÃO FEITA PELO ESP32 NO MONITOR SERIAL

    //Serial.print("TENSAO BATERIA: ");
    //Serial.print(voltage);
    //Serial.println("v");

    //Serial.print(" CARGA BATERIA: ");
    //Serial.print(bat_percentage);
    //Serial.println("%");


    //bat_percentage = 70;  // APENS PARA TESTES

    if (bat_percentage < 10) {

      if (millis() - Millis_Alerta_Bateria > 50000) {

        Bateria_10_Trigger = true;
        Serial.print("Atenção! Carga da Bateria Baixa: ");
        Serial.print(bat_percentage);
        Serial.println("%");

        Millis_Alerta_Bateria = millis();
      }
    }
  }
}


///////////////////////////////////////////////////////////
// Faz o brilho do led do farol diminuir durante a ignição para sumular um situação real de bateria baixa
void SimulaBateriaFracaNaPartida() {

  if (engineStart == true) {
    if (Farol != 0) {
      ledcWrite(12, 35);
      ledcWrite(13, 35);
    }

    if (quadrado == 1) {
      digitalWrite(FAROL_MILHA, LOW);
    }
    if (setaCima == 1) {
      digitalWrite(LUZ_AUX01, LOW);
    }
    if (setaEsquerda == 1) {
      digitalWrite(LUZ_AUX02, LOW);
    }
    if (setaDireita == 1) {
      digitalWrite(LUZ_AUX03, LOW);
    }
    if (setaBaixo == 1) {
      digitalWrite(LUZ_AUX04, LOW);
    }

    delay(50);

    if (Farol == 1) {
      ledcWrite(12, 255);
      ledcWrite(13, 255);
    }
    if (Farol == 2) {
      ledcWrite(12, 100);  // Diminui a intensidade da luz do farol
      ledcWrite(13, 80);   // Diminui a intensidade da luz de Freio/Lanterna
    }
    if (quadrado == 1) {
      digitalWrite(FAROL_MILHA, HIGH);
    }
    if (setaCima == 1) {
      digitalWrite(LUZ_AUX01, HIGH);
    }
    if (setaEsquerda == 1) {
      digitalWrite(LUZ_AUX02, HIGH);
    }
    if (setaDireita == 1) {
      digitalWrite(LUZ_AUX03, HIGH);
    }
    if (setaBaixo == 1) {
      digitalWrite(LUZ_AUX04, HIGH);
    }
  }
}

/////////////////////////////////////////////////////
// FUNÇÃO PARA AUXILIAR NA CONVERSÃO DOS DADOS DA LEITURA DA BATERIA, FAZ O MAPEAMENTE COM VARIÁVEIS DO TIPO FLOAT
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
