// Inclusão das bibliotecas utilizadas
#include "mbed.h"
#include "Adafruit_SSD1306.h" // Display Oled
#include "SRF05.h" // Sensor ultrassonico
#include "BME280.h" // Sensor pressao/umidade/temperatura

// Configuração dos pinos como entrada/saida
BME280 sensor(p28, p27); // Sensor pressao/umidade/temperatura
DigitalOut bomba(p22); // 5v da bomba
DigitalOut led(LED1); // Led da própia placa
DigitalOut luz(p23); // Led Iluminação
InterruptIn presenca(p21); // Sensor de presença
AnalogIn analog_value(A0); //Sensor umidade solo
AnalogIn LDR(A1); // LDR
Serial bt(p13, p14); // Bluetooth
SRF05 srf(p9,p10); // Sensor Ultrassônico

// Configuração do display OLed
class SPIPreInit : public SPI
{
public:
    SPIPreInit(PinName mosi, PinName miso, PinName clk) : SPI(mosi,miso,clk) {
        format(8,3);
        frequency(2000000);
    };
};

SPIPreInit gSpi(p5,NC,p7);
Adafruit_SSD1306_Spi gOled1(gSpi,p8,p6,p11);

// Handler da interrupção do timer(1min)
void myhandler()
{
    led=!led;

    //Definição das variáveis obtidas dos sensores
    float y = 100*analog_value.read();
    float ilu = 100*LDR.read();
    float reser = srf.read();
    double umi = sensor.getHumidity();
    double temp = sensor.getTemperature();
    double pres = sensor.getPressure();

// Envio das váriáves por bluetooth
    bt.printf("Umidade do ar: %3.2f%\r\n", umi);
    bt.printf("Temperatura ambiente: %2.1f\r\n", temp);
    bt.printf("Pressao do ambiente: %2.2f\r\n", pres);

//Acender o Led de iluminação caso a luminosidade do local esteja baixa
    if(ilu<=15) {
        luz = 1;
    } else {
        luz = 0;
    }

//Acionar a bomba e indica no display caso a umidade do solo estiver baixa
    if(y>=99) {
        bt.printf("Bomba acionada!\n");
        gOled1.clearDisplay();
        gOled1.printf("Bomba ");
        gOled1.display();
        bomba = 1;
        wait(2);
        bomba = 0;
        gOled1.clearDisplay();
        gOled1.display();
    }

// Obtem o nível de água do reservátorio através do sensor ultrassônico, e envia uma mensagem via BT se o mesmo está baixo
    if(srf.read()>8) {
        bt.printf("Nivel de agua do reservatorio esta baixo\n");
    }


    // clear the TIMER0 interrupt
    LPC_TIM0->IR = 1;
}

// Rotina de Lcd para informar o usuário atraves do display Oled
void RotinaLcd()
{
//Definição e obtenção dos valores das variáveis
    double umi = sensor.getHumidity();
    double temp = sensor.getTemperature();
    double pres = sensor.getPressure();
    float reser = srf.read();

    gOled1.clearDisplay(); // Limpa o display
    gOled1.printf("Umidade: %3.2f%\r", umi); // "Carrega" o printf para o display
    gOled1.display(); // "Atualiza" o display
    bt.printf("Umidade do ar: %.2f%\r\n", umi); // Envia por bluetooth as informações
    wait(2); // delay de 2 segundos

    gOled1.clearDisplay();
    gOled1.printf("Temperatura:%2.1f%\r", temp);
    gOled1.display();
    bt.printf("Temperatura ambiente: %.1f\r\n", temp);
    wait(2);

    gOled1.clearDisplay();
    gOled1.printf("Pressao:%2.2f%\r", pres);
    gOled1.display();
    bt.printf("Pressao do ambiente: %.2f\r\n", pres);
    wait(2);

    gOled1.clearDisplay();
    gOled1.printf("Reserv:%2.1f%\r", reser);
    gOled1.display();
    bt.printf("Reserv: %.1f\r\n", reser);
    wait(2);

    gOled1.clearDisplay();
    gOled1.display();
}

// Configuração do Timer
void timer_config()
{
    // power up TIMER0 (PCONP[1])
    LPC_SC->PCONP |= 1 << 1;

    // reset and set TIMER0 to timer mode
    LPC_TIM0->TCR = 0x2;
    LPC_TIM0->CTCR = 0x0;

    // set no prescaler
    LPC_TIM0->PR = 0;

    // calculate period (1 interrupt every second)
    uint32_t period = SystemCoreClock / 0.0667; //4-1s  0.4-10s

    // set match register and enable interrupt
    LPC_TIM0->MR0 = period;
    LPC_TIM0->MCR |= 1 << 0;    // interrupt on match
    LPC_TIM0->MCR |= 1 << 1;    // reset on match

    // enable the vector in the interrupt controller
    NVIC_SetVector(TIMER0_IRQn, (uint32_t)&myhandler);
    NVIC_EnableIRQ(TIMER0_IRQn);

    // start the timer
    LPC_TIM0->TCR = 1;
}

int main()
{
    char aux;
    bt.baud(9600); // Seleciona o baud rate para comunicação serial
    timer_config(); // Chama a função de configuração do timer

    gOled1.clearDisplay();
    gOled1.display();

    while(1) {
        if(presenca==1) { // Ativa quando o sensor de presença capta movimento
            RotinaLcd(); // Chama a rotina de lcd
        }
        if (bt.readable()) { // Ativa quando BT recebe algum caractere
            aux = (bt.getc());
            if (aux == 'b') { //Aciona a bomba quando o botão "bomba" é acionado no celular
                bomba = 1;
                wait(2);
                bomba = 0;
            }
            if (aux == 'l') { //Acende/Apaga a luz quando o botão "Luz" é acionado no celular
                luz = !luz;
            }
        }


    }

}
