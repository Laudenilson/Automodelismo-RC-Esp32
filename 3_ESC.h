//#include <Arduino.h>

/* Configurações Recomendadas para ESC e Ajustes

*****************************************************************************
  ESC: HOBBYWING 1080 QUICRUN WP Crawler Brushed com as seguintes configurações:
  Veículo: Caminhões TAMIYA com transmissão de 3 marchas
  Motor: Tamanho 540, 35 voltas, pinhão padrão

  Para mais detalhes, consulte o manual do ESC.
  Configurações não comentadas permanecem nos ajustes de fábrica
  1: 3
  2: 1 (1= LiPo, 2 = bateria NIMH)
  3: 3
  4: 3
  5: 4
  6: 2
  7: 9
  8: 1
  9: 8 (mude, é importante)
  10: 4
  11: 4
  12: 5
  13: 5 (16KHz = menos chiado)
  14: 1 (tenha cuidado aqui, isso mudará a voltagem do BEC!!)
  15: 1 (mude, é importante)

*****************************************************************************
  ESC: Combo motor/ESC HOBBYWING QUICRUN FUSION com as seguintes configurações:
  Veículo: RGT EX86100 Jeep Crawler, pinhão padrão, LiPo 3S
  Motor: Brushless integrado

  Certifique-se de descomentar (remova //) "#define QUICRUN_FUSION" abaixo!

  Para mais detalhes, consulte o manual do ESC.
  Configurações não comentadas permanecem nos ajustes de fábrica
  1: 2 (mude, 1 é áspero)
  2: 1
  3: 3
  4: 1
  5: 2 (mude, se o veículo estiver indo na direção errada)
  6: 1 (tenha cuidado aqui, isso mudará a voltagem do BEC!!)
  7: 5
  8: 1
  9: 4 (mude a velocidade reversa conforme preferir)

*****************************************************************************
  HOBBYWING 1060 também funciona, mas o 1080 é melhor ainda.

*****************************************************************************
  O AS-12/6RW EASY da Modellbau-Regler.de é adequado para veículos menores (WPL, MN Model etc.),
  mas não tem a melhor frenagem. É melhor usar um CI de driver de motor RZ7886 (veja abaixo)
*****************************************************************************

*/

// AJUSTES DO ESC ******************************************************************************************************

// Opções gerais
 #define QUICRUN_FUSION // Compensação de linearidade para combo motor/ESC HOBBYWING Quicrun Fusion
// #define QUICRUN_16BL30 // Compensação de linearidade para ESC HOBBYWING Quicrun 16BL30 (experimental, não use)
// #define ESC_DIR // descomente isso, se seu motor estiver girando na direção errada

/* CI de driver de motor RZ7886 em vez de um ESC. Não em combinação com #define THIRD_BRAKELIGHT ou #define TRAILER_LIGHTS_TRAILER_PRESENCE_SWITCH_DEPENDENT
  Conexões:
  Pino 1 a 33 "ESC"
  Pino 2 a 32
  Pino 3 para GND (0V)
  Pino 4 para Bateria +
  Pinos 5 e 6 ao motor +
  Pinos 7 e 8 ao motor -
  Observações:
  - Certifique-se de ter área de cobre suficiente ao redor do motor + e - para dissipação de calor do CI!
  - Conecte um capacitor eletrolítico de 100 - 470uF e um capacitor de tântalo ou cerâmica de 100nF entre os pinos 2 - 3
*/
//#define RZ7886_DRIVER_MODE // Um CI de driver de motor RZ7886 é usado em vez de um ESC RC Crawler padrão. adequado para motores até o tamanho 370, por exemplo, veículos WPL.
const uint16_t RZ7886_FREQUENCY = 500;     // 500 Hz é recomendado. Não é audível se o som do motor virtual estiver sendo reproduzido. Frequências mais altas podem superaquecer o CI de driver!
const uint8_t RZ7886_DRAGBRAKE_DUTY = 100; // 0 - 100%. 100% = potência máxima de freio enquanto está parado. 100% é recomendado para crawlers.

// Ajuste de velocidade máxima:
// Normalmente 500 ( = 1000 - 2000 microssegundos de saída ou ângulo de servo de -45° a 45°) Aumente se seu veículo estiver muito rápido
// - O Hobbywing 1060 alcança o máximo de aceleração (para frente) em cerca de 1800 microssegundos, então precisamos de cerca de 640 para aceleração total
// - Hobbywing 1060 ESC & RBR/C 370 motor & transmissão de 2 marchas = 1600
// - Hobbywing 1060 ESC & motor WPL 370 padrão & transmissão de 2 marchas = 650
// - Hobbywing 1040 ESC & motor WPL 20T 540 padrão no RGT EX86100 = 640
// - Hobbywing 1080 ESC & motor 35T 540 para caminhões TAMIYA com transmissão de 3 marchas = 1200
// - Hobbywing 1080 ESC & motor RBR/C 370 na caminhonete de corrida Mercedes da Carson (velocidade máxima = 160km/h) = 900
// - Modellbau-Regler.de AS-12/6RW EASY ESC = 600
// - Meccano Dumper = 500
const uint16_t escPulseSpan = 500; // 500 = potência total do ESC disponível, 1000 metade da potência do ESC disponível etc.

// Punch de decolagem adicional:
// Normalmente 0. Aumente se seu motor for muito fraco perto do ponto morto.
// - Hobbywing 1060 ESC & RBR/C 370 motor & transmissão de 2 marchas = 80
// - Hobbywing 1060 ESC & motor WPL 370 padrão & transmissão de 2 marchas = 10
// - Hobbywing 1080 ESC & motor 35T 540 para caminhões TAMIYA com transmissão de 3 marchas = 40 (era 0)
// - Hobbywing 1080 ESC & motor 35T 540 para caminhões HERCULES HOBBY com transmissão de 3 marchas = 150
// - Hobbywing 1080 ESC & motor RBR/C 370 na caminhonete de corrida Mercedes da Carson = 50
// - Meccano Dumper = 0
// - RZ7886 Driver = 0
const uint16_t escTakeoffPunch = 0;

// Velocidade reversa adicional (desconecte e reconecte a bateria após mudar este ajuste):
// Normalmente 0. Aumente se sua velocidade reversa for muito lenta.
// - Hobbywing 1060 ESC & RBR/C 370 motor & transmissão de 2 marchas = 220
// - Hobbywing 1060 ESC & motor WPL 370 padrão & transmissão de 2 marchas = 150
// - Hobbywing 1080 ESC & motor 35T 540 para caminhões TAMIYA com transmissão de 3 marchas = 40 (era 0)
// - Hobbywing 1080 ESC & motor 35T 540 para caminhões HERCULES HOBBY com transmissão de 3 marchas = 80
// - Meccano Dumper = 0
// - RZ7886 Driver = 0
const uint16_t escReversePlus = 0;

// Margem de frenagem: (Experimental!)
// Esta configuração impede que o ESC volte completamente para zero / neutro enquanto o gatilho de freio estiver acionado.
// Isso impede que o veículo recue enquanto o freio estiver aplicado. 0 = sem efeito, cerca de 20 = efeito forte.
// Como funciona? Impede que o ESC entre na "faixa de frenagem de arrasto"
// Aviso: o veículo pode ser incapaz de parar se estiver muito alto, especialmente ao dirigir morro abaixo! NUNCA mais do que 20!
const uint16_t brakeMargin = 5; // 10 / Para RZ7886 motor driver e motor 370 = 10











// crawlerEscRampTime (veja "8_Sound.h") do modo rastejador. AVISO: uma configuração muito baixa pode danificar sua transmissão!
const uint8_t crawlerEscRampTime = 0; //10; // cerca de 10 (15 para Jeep), menos = controle mais direto = menos inércia virtual















// Permite escalonar a aceleração dependente do arquivo do veículo
uint16_t globalAccelerationPercentage = 100; //100 cerca de 100 - 200% (200 para Jeep, 150 para Land Rover 1/8) Experimental, pode causar problemas de troca de marcha automática!

/* Proteção de descarga da bateria (apenas para placas com resistores de divisor de tensão):
 *  IMPORTANTE: Insira os valores dos resistores usados em Ohms (Ω) e DEPOIS ajuste DIODE_DROP, até que suas leituras correspondam à voltagem real da bateria! */
//#define BATTERY_PROTECTION // Isso desativará a saída do ESC se a voltagem de corte da bateria for atingida. 2 flashes rápidos = erro na bateria!
const float CUTOFF_VOLTAGE = 3.3;        // Normalmente 3.3 V por célula LiPo. NUNCA abaixo de 3.2 V!
const float FULLY_CHARGED_VOLTAGE = 4.2; // Normalmente 4.2 V por célula LiPo, NUNCA acima!
const float RECOVERY_HYSTERESIS = 0.2;   // cerca de 0.2 V
/* Nota sobre valores de resistores: Esses valores serão usados para calcular a razão real entre esses dois resistores (que também é chamada de "divisor de tensão").
 * Ao selecionar resistores, sempre use dois do mesmo valor: Como, por exemplo, 10k/2k, 20k/4k ou 100k/20k. NUNCA exceda uma razão MENOR que (4:1 = 4)!
 * AVISO: Se a razão for muito BAIXA, como 10k/5k (2:1 = 2), a voltagem da bateria provavelmente DANIFICARÁ o controlador permanentemente!
 * Cálculo de exemplo: 2000 / (2000 + 10000) = 0.166 666 666 7; 7.4 V * 0.167 = 1.2358 V (de 3.3 V máximo no pino GPIO). */
uint32_t RESISTOR_TO_BATTTERY_PLUS = 10000; // Valor em Ohms (Ω), por exemplo, 10000
uint32_t RESISTOR_TO_GND = 1000;           // Valor em Ohms (Ω), por exemplo, 2000. Medir os valores exatos dos resistores antes de soldar, se possível, é recomendado!
float DIODE_DROP = 0;                   // Ajuste fino do valor medido e/ou considere a queda de tensão do diodo (cerca de 0.34V para diodo SS34)
/* É recomendável adicionar um adesivo ao seu ESP32 que inclua os 3 valores de calibração acima */
volatile int outOfFuelVolumePercentage = 80; // Ajuste o volume da mensagem em %
// Selecione a mensagem de falta de combustível que você deseja:
//#include "vehicles/sounds/OutOfFuelEnglish.h"
// #include "vehicles/sounds/OutOfFuelGerman.h"
// #include "vehicles/sounds/OutOfFuelFrench.h"
// #include "vehicles/sounds/OutOfFuelDutch.h"
// #include "vehicles/sounds/OutOfFuelSpanish.h"
// #include "vehicles/sounds/OutOfFuelPortuguese.h"
// #include "vehicles/sounds/OutOfFuelJapanese.h"
// #include "vehicles/sounds/OutOfFuelChinese.h"
// #include "vehicles/sounds/OutOfFuelTurkish.h"
// #include "vehicles/sounds/OutOfFuelRussian.h"

#include "vehicles/sounds/OutOfFuelPortuguese.h"
#include "vehicles/sounds/100_Bateria.h"
#include "vehicles/sounds/90_Bateria.h"
#include "vehicles/sounds/80_Bateria.h"
#include "vehicles/sounds/70_Bateria.h"
#include "vehicles/sounds/60_Bateria.h"
#include "vehicles/sounds/50_Bateria.h"
#include "vehicles/sounds/40_Bateria.h"
#include "vehicles/sounds/30_Bateria.h"
#include "vehicles/sounds/20_Bateria.h"
#include "vehicles/sounds/10_Bateria.h"
#include "vehicles/sounds/Bateria_Menos10Atencao.h"
#include "vehicles/sounds/Sem_Bateria.h"

