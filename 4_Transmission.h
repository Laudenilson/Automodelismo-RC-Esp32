//#include <Arduino.h>

/* CONFIGURAÇÕES GERAIS DE TRANSMISSÃO ************************************************************************************************************
 *
 * A maioria das configurações de transmissão, como automática, embreagem dupla etc., são feitas nos arquivos de configuração do veículo no diretório /vehicles/.
 *
 */

// Os seguintes modos de transmissão são ignorados em veículos "automáticos" ou "doubleClutch" ==================================================
// Nunca descomente mais de um! Se nenhuma opção estiver definida, você pode usar uma transmissão real de 3 marchas, por exemplo, da TAMIYA

// #define VIRTUAL_3_SPEED permite simular uma transmissão de 3 marchas, se seu veículo não tiver uma real.
// As marchas são simuladas virtualmente, usando o interruptor de 3 posições. Exemplo: seu crawler tem uma transmissão de 2 marchas, que é usada como redutora fora da estrada,
// mas não tem uma transmissão de 3 marchas real. Não descomente para veículos com transmissão elétrica ou hidrostática ou transmissões automáticas!
// Também não use para o MODO_LOCOMOTIVA_A_VAPOR
 //#define VIRTUAL_3_SPEED

// #define VIRTUAL_16_SPEED_SEQUENTIAL ativará uma transmissão sequencial de 16 marchas, mudada por impulsos para cima / para baixo através do interruptor de 3 posições
// #define VIRTUAL_16_SPEED_SEQUENTIAL // Isso ainda é experimental e não funciona corretamente! Não use.

// Opções de transmissão adicionais =========================================================================================================
// Transmissão automática com overdrive (RPM mais baixa na marcha alta, relação de marcha inferior a 1:1, apenas para 4 e 6 marchas)
// Também utilizável em combinação com VIRTUAL_3_SPEED. A 4ª marcha é mudada automaticamente neste caso, se estiver dirigindo na 3ª marcha a toda velocidade
// #define OVERDRIVE // Não use para: doubleClutch. Não funciona com SEMI_AUTOMATIC, mas você pode deixar ativado neste caso.

// Em alguns casos, queremos uma aceleração reversa diferente para veículos com transmissão automática.
uint16_t automaticReverseAccelerationPercentage = 100;

// A porcentagem da faixa baixa é usada para o MODO1_SHIFTING (redutor fora de estrada)
uint16_t lowRangePercentage = 58; // Relações de marcha de 2 velocidades da WPL = 29:1, 17:1 = 58% na faixa baixa. Você pode querer mudar isso para outras transmissões de 2 velocidades

// Opções de controles de transmissão ========================================================================================================
// #define SEMI_AUTOMATIC Isso simulará uma transmissão semi-automática. A mudança não é controlada pelo interruptor de 3 posições neste modo!
 //#define SEMI_AUTOMATIC // Funciona para VIRTUAL_3_SPEED ou transmissão de 3 marchas real. Não selecione isso ao mesmo tempo que VIRTUAL_16_SPEED_SEQUENTIAL

// #define MODE1_SHIFTING A transmissão de 2 marchas é mudada pelo botão "Modo 1" em vez do interruptor de 3 posições.
// Isso é frequentemente usado em veículos WPL com transmissão de 2 marchas, usada como redutora fora de estrada, mudada enquanto dirige lentamente para engatar corretamente.
// #define MODE1_SHIFTING

// #define TRANSMISSION_NEUTRAL Permite colocar a transmissão em ponto morto. Isso não pode ser usado se o botão "Modo 1" for usado para outras coisas!
// Você pode deixar ativado se definir MODE1_SHIFTING. Ele é desativado automaticamente neste caso.
// #define TRANSMISSION_NEUTRAL

// Opções de embreagem ==========================================================================================================================
uint16_t maxClutchSlippingRpm = 250;//250; // A embreagem nunca deslizará acima desse limite! (cerca de 250) 500 para veículos como locomotivas
// e o trator Kirovets com transmissão hidrostática ou elétrica! Principalmente necessário para o modo "VIRTUAL_3_SPEED"

// #define DOUBLE_CLUTCH // Dupla embreagem (Zwischengas) Ative isso para caminhões mais antigos com transmissão manual sem marchas sincronizadas

// #define HIGH_SLIPPINGPOINT // A embreagem se engatará a uma RPM mais alta, se definido. Comente isso para veículos pesados como caminhões semi.
