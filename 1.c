#include <8051.h>

#define RIGHT 0b000100
#define UP 0b001000
#define DOWN 0b010000
#define LEFT 0b100000

__code unsigned char MENU[6][3][33] = 
{{">-1 MAIN SCREEN  -2 SETTINGS    "},
{"OLEKSNDRA BASARBPWM VALUE:   030"}, 
{">- 2.1 PWM       - 2.2 LED      ",
">- 2.3 OTHER     - 2.4 RESET    "},
{">- STATE     OFF - VALUE     030"},
{">- BUZZ      OFF                "},
{">- F1        OFF - F2        OFF",
">- F3        OFF - F4        OFF",
">- OK        OFF - ER        OFF"}};
__xdata unsigned char * LCDWC = (__xdata unsigned char *) 0xFF80; //lcd write command
__xdata unsigned char * LCDWD = (__xdata unsigned char *) 0xFF81; //lcd write data
__xdata unsigned char * LCDRC = (__xdata unsigned char *) 0xFF82; //lcd read command
__xdata unsigned char * CSKB1 = (__xdata unsigned char *) 0xFF22; //klaw multipleksowana
__xdata unsigned char * led_wyb = (__xdata unsigned char *) 0xFF30; // CSDS, wybor jednego (lub wiecej) z 6 wyswietlaczy lub diody
__xdata unsigned char * led_led = (__xdata unsigned char *) 0xFF38; // CSDB, wybor wlaczonych segmentow, lub wlaczonych diod z 6
__xdata unsigned char * CS55B = (__xdata unsigned char *) 0xFF29; //port na ktorym on/off napiecie
__xdata unsigned char * CS55D = (__xdata unsigned char *) 0xFF2B;  // 8255 - registry of control port

__bit t0_flag; //flaga czy poprzedni stan byl wysoki czy niski
__bit can_send; //flaga dozwolu na odsylanie przez transmisje
__bit __at (0x97) DLED; //dioda test

__code unsigned char WZOR[10] = { 0b0111111, 0b0000110,
            0b1011011, 0b1001111, 0b1100110, 0b1101101,
            0b1111101, 0b0000111, 0b1111111, 0b1101111 };
unsigned char KBDS[6] = { 0b000001, 0b000010, 0b000100,  		
                         0b001000, 0b010000, 0b100000 };

__bit __at (0x95) BUZZ;
__bit __at (0x96) SEG_OFF;
__bit __at (0xB5) key_bit;

unsigned char i;
unsigned char pressed_key;
unsigned char isPresed = 0;
unsigned char count_disp = 0;
unsigned char count = 0;
unsigned char min_count = 255;
unsigned char max_count = 2;
unsigned char key,PRESS,tmp,TENH=100, PWM_BUF=0, timer = 0;
unsigned char diod_ponter = 0b000000;

unsigned int TH0_LOW, TL0_LOW;
unsigned int TH0_HIGH, TL0_HIGH;



unsigned char STATE = 1, BUZ = 0, F11 = 0, F2 = 0, F3 = 0, F4 = 0, OK = 0, ER = 0, PWM = 30;
unsigned char STATE_ = 1, BUZ_ = 0, F1_ = 0, F2_ = 0, F3_ = 0, F4_ = 0, OK_ = 0, ER_ = 0, PWM_ = 30; //dublikaty dla pamietania stanow

void WAIT(){
    unsigned char c = 255;
    while (c--){}
}

/*65535 - przepewnienie tajmera. 50hz*/


//Zmienia znaczenia zmiennych starszego i mlodszego bitow wysokiego i niskiego stanow
void CHANGE_INTERUPT(){

    TH0_HIGH =(64980-((184*(PWM-30))/10))/256; //Starszy bajt wysokiego stanu
    TL0_HIGH =(64980-((184*(PWM-30))/10))%256; //Mlodszy bajt wysokiego stanu
    TH0_LOW = (47660+((184*(PWM-30))/10))/256; //Старший байт низького стану
    TL0_LOW = (47660+((184*(PWM-30))/10))%256; //Молодший байт низького стану
}

//Оновлення числа пвм на лед спарва
//aktualizacja liczb na led 
void units_refresh(){
    SEG_OFF = 1; //wymyk. wyswietlacz
    *led_wyb = 0b000001;
    *led_led = WZOR[PWM%10];
    SEG_OFF = 0;
}

void tens_refresh(){
    SEG_OFF = 1;
    *led_wyb = 0b000010;
    *led_led = WZOR[(PWM/10)%10];
    SEG_OFF = 0;
}

void hundreds_refresh(){
    SEG_OFF = 1;
    *led_wyb = 0b000100;
    *led_led = WZOR[(PWM/100)%10];
    SEG_OFF = 0;
}

void diods_refresh(){
    SEG_OFF = 1;
    *led_led = diod_ponter; //ktora dioda bedzie swiecila
    *led_wyb = 0b1000000; //wybor segmentu diod
    SEG_OFF = 0;
}

void led_refresh(){
    WAIT();
    units_refresh();
    WAIT();
    tens_refresh();
    WAIT();
    hundreds_refresh();
    WAIT();
    diods_refresh();
}

//zatwierdzenie stanow i dzialan
void ENTER(){
    STATE_ = STATE;
    BUZ_ = BUZ;
    BUZZ = !BUZ;
    PWM_ = PWM;
    F1_ = F11;
    F2_ = F2;
    F3_ = F3;
    F4_ = F4;
    OK_ = OK;
    ER_ = ER;
    led_refresh();
}

void ESC(){
    STATE = STATE_;
    BUZ = BUZ_;
    PWM = PWM_;
    F11 = F1_;
    F2 = F2_;
    F3 = F3_;
    F4 = F4_;
    OK = OK_;
    ER = ER_;
    if (F11)
    {
        diod_ponter |= (1<<0);
    }
    else{
        diod_ponter &= ~(1<<0);
    }

    if (F2)
    {
        diod_ponter |= (1<<1);
    }
    else{
        diod_ponter &= ~(1<<1);
    }
    if (F3)
    {
        diod_ponter |= (1<<2);
    }
    else{
        diod_ponter &= ~(1<<2);
    }
    if (F4)
    {
        diod_ponter |= (1<<3);
    }
    else{
        diod_ponter &= ~(1<<3);
    }
    if (OK)
    {
        diod_ponter |= (1<<4);
    }
    else{
        diod_ponter &= ~(1<<4);
    }
    if (ER)
    {
        diod_ponter |= (1<<5);
    }
    else{
        diod_ponter &= ~(1<<5);
    }
    
    led_refresh();
}



void LCD_WAIT_WHILE_BUSY(){
    while(*LCDRC == 0b10000000){} //0b10000000 - oznacza ze dsm zajeta
}

// odsyla kom w lcd
void LCD_SEND_CMD(unsigned char cmd){
    LCD_WAIT_WHILE_BUSY();
    *LCDWC = cmd;
}

// poczatkowe dane dla lcd
void LCD_INIT(){
    LCD_SEND_CMD(0b00111000);
    LCD_SEND_CMD(0b00001111);
    LCD_SEND_CMD(0b00000110);
    LCD_SEND_CMD(0b00000001); /*4. Wyczyścisz wyświetlacz.*/

}

//przynosi znaczenia pwm przez transmisje kazda sekunde
void TIMER(){
    if (timer > 99) //100 taktiw = 1000ms
        {
            SBUF = (PWM/100)+48;
            WAIT();
            SBUF = ((PWM/10)%10)+48;
            WAIT();
            SBUF = (PWM%10)+48;
            WAIT();
            SBUF = '|';
            timer = 0;
        }
}


void WAIT_WHILE_PRESSED(){
    pressed_key = *CSKB1; //*CSKB1 - adres z ktorego biezemy dane pro klawu

        while (isPresed && pressed_key != 0b11111111){ //0b11111111 - zadny klawisz nie nacisniety
            TIMER();
            pressed_key = *CSKB1; 
            led_refresh();
        }
        isPresed = 0;
}

void LCD_CHANGE(){
    LCD_SEND_CMD(0b00000001);    /*4. Wyczyścisz wyświetlacz.*/
    for (i = 0; i < 32; i++)
        {  
            WAIT();
            //czy wycierac strzalke
            if ((count+1)%2==0 && i == 0) 
            {
                LCD_WAIT_WHILE_BUSY();
                *LCDWD = ' '; //zamiast >
                continue;
            }
            //czy wpisywac strzalke
            if ((count+1)%2==0 && i == 16)
            {
                LCD_SEND_CMD(0b11000000); //przechodzimy na nast linijke

                LCD_WAIT_WHILE_BUSY();
                WAIT();
                *LCDWD = '>';
                continue;
            }

            if (count_disp > 2 && i == 14) //> 2 - meniuszky z on i off, 14 symbol na `n` i pierwszym 'f' w off
            {
                //wpisywanie on w `gorne` zmienne on-off
                if ((count_disp == 4 && BUZ == 1) || (count_disp == 3 && STATE == 1) || (count_disp == 5 && (F11 == 1 && count < 2) || (F3 == 1 && count < 4 && count > 1)|| (OK == 1 && count < 6 && count > 3)))
                {
                    LCD_WAIT_WHILE_BUSY();
                    *LCDWD = 'N';
                    LCD_WAIT_WHILE_BUSY();
                    *LCDWD = ' ';
                    i++;
                    continue;
                }
            }

            //wpisywanie on w `dolne` zmienne on-off w led
             if (count_disp == 5 && i == 30)
            {
                if ((F2 == 1 && count < 2) || (F4 == 1 && count < 4 && count > 1)|| (ER == 1 && count < 6 && count > 3))
                {
                    LCD_WAIT_WHILE_BUSY();
                    *LCDWD = 'N';
                    WAIT();
                    *LCDWD = ' ';
                    break;;
                }
            }

            
            //wyswietla wartosc pwm w dwoch podmenu
            if ((count_disp == 3 && i == 29) ||(count_disp == 1 && i == 29)) //29 - pierwsza liczba w pwm
            {
                LCD_WAIT_WHILE_BUSY();
                *LCDWD = ((PWM/100))+48; //cyfra sotni 
                LCD_WAIT_WHILE_BUSY();
                *LCDWD = ((PWM/10)%10)+48; //cyfra dziesietek
                LCD_WAIT_WHILE_BUSY();
                LCD_WAIT_WHILE_BUSY();
                *LCDWD = (PWM%10)+48; //cyfra jednosti
                break;
            }
            
            //przechodzenie na nast linijke
            if (i==16)
            {
               LCD_SEND_CMD(0b11000000); // komanda przechodzenie na nast linijke
            }
            LCD_WAIT_WHILE_BUSY();

            *LCDWD = MENU[count_disp][count/2][i]; //wybieramy ktore podmenu bedzie swiecilo
        }
}



void main(){
    SCON = 0x50;//ustaw parametry transmisji
                 //tryb 1: 8 bitów, szybkość: T1
	TMOD = 0x21; //ustaw T1 w tryb 2; T0 w tryb 1
	TL1 = 0xFD; //ustawienie młodszego
    TH1 = 0xFD; //i starszego bajtu T1 (19200)
	PCON = 0x40; // power control zegar dla sio, T1 (19200 b/s)

	
	ES = 1; // aktywuj przerwanie od UART
    ET0=1; // aktywuj przerwanie od licznika T0
    EA=1; // aktywuj wszystkie przerwania

    TR1 = 1; // uruchom licznik T1
    TR0=1; // uruchom licznik T0

    *(CS55D) = 0x80;
    led_refresh();
    LCD_INIT();
    LCD_CHANGE();
    CHANGE_INTERUPT();
    
    while (1)
    {
        TIMER();
        CHANGE_INTERUPT();
        led_refresh();
        WAIT_WHILE_PRESSED();

        //sprawdza prawi klawisy zbilsz czy zmensz pwm
        SEG_OFF = 1;
        WAIT();
        key = 0b000000;
        for (i = 0; i < 6; ++i){
			*led_wyb = KBDS[i]; //wybor wskaznika
			if (key_bit == 1){  //1 jesli nazata klawisza jaku peredaly
				key = KBDS[i];
				break;
			}
		}

        if(key == 0b000000 && PRESS == 1){
			PRESS = 0;
		}
        else if (PRESS == 0){
           		if(key == UP){
        			PWM++;
                    PWM_++;
                    if (PWM>120)
                    {
                        PWM=120;
                        PWM_=120;
                    }
                    
        			PRESS = 1;
                    LCD_CHANGE();
			    }   

           		if(key == DOWN){
                    PWM--;
                    PWM_--;
                    if (PWM<30)
                    {
                        PWM=30;
                        PWM_=30;
                    }
        			PRESS = 1;
                    LCD_CHANGE();
			    }

                if(key == RIGHT){
        			PWM+=10;
                    PWM_+=10;
                    if (PWM>120)
                    {
                        PWM=120;
                        PWM_=120;
                    }
                    
        			PRESS = 1;
                    LCD_CHANGE();
			    }      
                if(key == LEFT){
        			PWM-=10;
                    PWM_-=10;
                    if (PWM<30)
                    {
                        PWM=30;
                        PWM_=30;
                    }
                    
        			PRESS = 1;
                    LCD_CHANGE();
			    }  
            
        }
        led_refresh();
        SEG_OFF = 0;
    
        //sprawdza klawisze w multipleksowanej kl., zmienia liczniki przechodzenia po menu
        if (pressed_key == 0b11101111)//gora
        {
            isPresed = 1;

            count--;
            if (count == 255) //jesli 2 razy w gore to na 0 elemencie
            {
                count = 0;
            }
            LCD_CHANGE();
            
        }
        else if (pressed_key == 0b11011111)//vnyz
        {
            count++;
            if (count == max_count)
            {
                count = max_count-1;
            }
            
            
            isPresed = 1;
            LCD_CHANGE();
        }
        else if (pressed_key == 0b01111111)//enter
        {
            isPresed = 1;

            if (count == 0 && count_disp == 0)
            {
                count_disp = 1;
                max_count = 1;
            }
            else if (count == 1 && count_disp == 0)
            {
                count = 0;
                count_disp = 2;
                max_count = 4;
            }
            else if (count == 0 && count_disp == 2)
            {
                count = 0;
                count_disp = 3;
                max_count = 2;
            }
            else if (count == 1 && count_disp == 2)
            {
                count = 0;
                count_disp = 5;
                max_count = 6;
            }
            else if (count == 2 && count_disp == 2)
            {
                count = 0;
                count_disp = 4;
                max_count = 1;
            }
            else if (count == 3 && count_disp == 2)
            {
                STATE = 0;
                BUZ = 0;
                PWM = 30;
                F11 = 0;
                F2 = 0;
                F3 = 0;
                F4 = 0;
                OK = 0;
                ER = 0;
                ENTER();
                count = 0;
                count_disp = 2;
                max_count = 4;
            }
            else if (count_disp > 2)
            {
                ENTER();
                count = 0;
                count_disp = 2;
                max_count = 4;

            }
            
            LCD_CHANGE();
        }
        else if (pressed_key == 0b10111111)//esc
        {
            isPresed = 1;
            if (count_disp == 1)
            {
                count_disp = 0;
                max_count = 2;
            }
            else if (count_disp == 2)
            {
                count = 0;
                count_disp = 0;
                max_count = 2;
            }
            else if (count_disp > 2)
            {
                ESC();
                count = 0;
                count_disp = 2;
                max_count = 4;
            }
            LCD_CHANGE();
        }
        else if (pressed_key == 0b11110111 )//right
        {
            if (count_disp == 4)
            {
                BUZ = 1;
            }
            if (count_disp == 3)
            {
                if (count)
                {
                    PWM+=10;
                    if (PWM > 120)
                    {
                        PWM = 120;
                    } 
                }
                else{
                    STATE = 1;
                }
            }

            if (count_disp == 5)
            {
                switch (count)
                {
                case 0: F11 = 1; diod_ponter |= (1<<count); break;// |= - OR
                case 1: F2 = 1; diod_ponter |= (1<<count); break;
                case 2: F3 = 1; diod_ponter |= (1<<count); break;
                case 3: F4 = 1; diod_ponter |= (1<<count); break;
                case 4: OK = 1; diod_ponter |= (1<<count); break;
                case 5: ER = 1; diod_ponter |= (1<<count); break;
                default: break;
                }
            }
            
            isPresed = 1;
            LCD_CHANGE();
        }
        else if (pressed_key == 0b11111011)//left
        {
            if (count_disp == 4)
            {
                BUZ = 0;
            }
            if (count_disp == 3)
            {
                if (count)
                {
                    PWM-=10;
                    if (PWM < 30)
                    {
                        PWM = 30;
                    } 
                }
                else{
                    STATE = 0;
                }
            }

            if (count_disp == 5)
            {
                switch (count)
                {
                case 0: F11 = 0; diod_ponter &= ~(1<<count); break;
                case 1: F2 = 0; diod_ponter &= ~(1<<count); break;
                case 2: F3 = 0; diod_ponter &= ~(1<<count); break;
                case 3: F4 = 0; diod_ponter &= ~(1<<count); break;
                case 4: OK = 0; diod_ponter &= ~(1<<count); break;
                case 5: ER = 0; diod_ponter &= ~(1<<count); break;
                default: break;
                }
            }

            isPresed = 1;
            LCD_CHANGE();
        }
    }
}

//funkcja przerwania ukladu czasowo-licznikowego
void t0_int( void ) __interrupt( 1 ) //przerwanie od licznika t0
{
    
    
    
    if(STATE_){ 
        timer++; // inc tajmer kazdy tik 2 tika 20 ms, 100 = 1000ms
    
    t0_flag = !t0_flag;  //czy wysoki czy niski byl poprzedni
    
    TF0=0; //1 to przerwanie z tej przyczyny ustawiamy na 0
    if(t0_flag)
    {
    	*(CS55B) = 0xFF; //wlanczamy wszystkie porty wyjscia B
		DLED = 0; 
		
		TL0 = TL0_HIGH;
		TH0 = TH0_HIGH;
	}
    else
    {
        *(CS55B) = 0x00;
        DLED = 1;
        
		TL0 = TL0_LOW;
		TH0 = TH0_LOW;
    }
    }
    
}

//transmisija szeregowa
void sio_int(void) __interrupt(4){ //przerwanie od portu transmisji szeregowej
	if (TI){//kiedy 1 to mozemy wysylac dane
        can_send = 1;
 
		TI = 0; 
        /*bit flagowy stanu nadawania - moŜe spowodować zgłoszenie
przerwania; bit jest ustawiany sprzętowo w momencie zakończe-
nia wysyłania danej; kasowanie bitu wyłącznie programowe;*/

	}
	else {
        PWM_BUF += (SBUF-48)*TENH; //PWM_BUF - wyslana nam liczba
        if (TENH == 1)
        {
            if (PWM_BUF>120)
            {
               PWM_BUF=120;
            }
            else if (PWM_BUF<30)
            {
                PWM_BUF=30;
            }
        
            PWM_ = PWM_BUF;
            PWM=PWM_BUF;
            PWM_BUF = 0;
            TENH = 100;
        }
        else{
            TENH/=10;
		}
        RI = 0; //zakonczenie odbioru
        /*bit flagowy stanu odbioru - moŜe spowodować zgłoszenie przer-
wania; bit jest ustawiany sprzętowo w momencie zakończenia od-
bioru danej; kasowanie bitu wyłącznie programowe.*/
        
	}
}