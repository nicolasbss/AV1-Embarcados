/*
 * main.c
 *
 * Created: 05/03/2019 18:00:58
 *  Author: eduardo
 */ 

#include <asf.h>
#include "tfont.h"
#include "sourcecodepro_28.h"
#include "calibri_36.h"
#include "arial_72.h"

#define BUT_PIO           PIOA
#define BUT_PIO_ID        10
#define BUT_PIO_IDX       19u
#define BUT_PIO_IDX_MASK (1u << BUT_PIO_IDX)

#define BUT2_PIO           PIOA
#define BUT2_PIO_ID        10
#define BUT2_PIO_IDX       11u
#define BUT2_PIO_IDX_MASK (1u << BUT_PIO_IDX)

#define YEAR        2018
#define MOUNTH      3
#define DAY         19
#define WEEK        12
#define HOUR        15
#define MINUTE      45
#define SECOND      0

volatile int flag_button;
volatile int flag_draw;
volatile int flag_speed;
volatile int flag_distance;
volatile int flag_redraw2;
volatile int flag_stop = 1;
volatile int hora;
volatile int min;
volatile int sec;
volatile float distance = 0;
volatile int pulse = 0;
volatile float speed = 0;
volatile int flag_ciclo;


volatile Bool f_rtt_alarme = false;

void BUT_init(void);
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq);
void RTC_init(void);
static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses);

struct ili9488_opt_t g_ili9488_display_opt;

void font_draw_text(tFont *font, const char *text, int x, int y, int spacing) {
	char *p = text;
	while(*p != NULL) {
		char letter = *p;
		int letter_offset = letter - font->start_char;
		if(letter <= font->end_char) {
			tChar *current_char = font->chars + letter_offset;
			ili9488_draw_pixmap(x, y, current_char->image->width, current_char->image->height, current_char->image->data);
			x += current_char->image->width + spacing;
		}
		p++;
	}
}

float calculate_distance(){
	return 2.0*3.14*0.325*(float)pulse;
}

float calculate_speed() {
	int w = (2.0 * 3.14 * (float)pulse)/4;
	flag_speed = 0;
	
	return w * 0.325 / 3.6;
}

void redraw2() {
	char b[90];
	char a[90];
	char c[90];
	
	
	sprintf(b, "%d:%d:%d", hora, min, sec);
	font_draw_text(&calibri_36, b, 50, 150, 1);

}

void redraw() {
	pulse = 0;
	char b[90];
	char a[90];
	
	
	sprintf(b, "%f", speed);
	sprintf(a, "%f", distance);
	font_draw_text(&calibri_36, b, 50, 50, 1);
	font_draw_text(&calibri_36, a, 50, 100, 1);
	flag_draw = 0;
}

void Button_Handler(void){
	flag_button = 1;
}

void Button_Handler2(void){
	if (flag_stop) {
		flag_stop = 0;
	} else {
		flag_stop = 1;
	}	
}

void RTC_Handler(void)
{
	uint32_t ul_status = rtc_get_status(RTC);

	/*
	*  Verifica por qual motivo entrou
	*  na interrupcao, se foi por segundo
	*  ou Alarm
	*/
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		rtc_clear_status(RTC, RTC_SCCR_SECCLR);
		rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
		rtc_set_date_alarm(RTC, 1 , MOUNTH, 1, DAY);
		
		rtc_get_time(RTC, &hora, &min, &sec);
		rtc_set_time_alarm(RTC, 1, hora, 1, min,1, sec+1);
		

		flag_redraw2 = 1;

	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
			
	}
	
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
	
}

void RTT_Handler(void)
{
	uint32_t ul_status;

	/* Get RTT status */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {  }

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		f_rtt_alarme = true;                  // flag RTT alarme
	}
	flag_speed = 1;
	flag_draw = 1;
	flag_distance = 1;
}


void pulse_add() {
	pulse += 1;
}

static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
}

void RTC_init(){
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(RTC, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(RTC, YEAR, MOUNTH, DAY, WEEK);
	rtc_set_time(RTC, HOUR, MINUTE, SECOND);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(RTC_IRQn);
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_SetPriority(RTC_IRQn, 0);
	NVIC_EnableIRQ(RTC_IRQn);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(RTC,  RTC_IER_SECEN);

}

void BUT_init(void){
	
	pmc_enable_periph_clk(BUT_PIO_ID);
	pio_set_input(BUT_PIO, BUT_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	//pio_set_debounce_filter()
	
	pio_enable_interrupt(BUT_PIO, BUT_PIO_IDX_MASK);

	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 1);
	pio_handler_set(BUT_PIO, BUT_PIO_ID, BUT_PIO_IDX_MASK, PIO_IT_FALL_EDGE, Button_Handler);

};

void BUT2_init(void){
	
	pmc_enable_periph_clk(BUT2_PIO_ID);
	pio_set_input(BUT2_PIO, BUT2_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT2_PIO, BUT2_PIO_IDX_MASK, 50);
	
	pio_enable_interrupt(BUT2_PIO, BUT2_PIO_IDX_MASK);

	NVIC_EnableIRQ(BUT2_PIO_ID);
	NVIC_SetPriority(BUT2_PIO_ID, 1);
	pio_handler_set(BUT2_PIO, BUT2_PIO_ID, BUT2_PIO_IDX_MASK, PIO_IT_FALL_EDGE, Button_Handler2);

};

static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses)
{
	uint32_t ul_previous_time;

	/* Configure RTT for a 1 second tick interrupt */
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	ul_previous_time = rtt_read_timer_value(RTT);
	while (ul_previous_time == rtt_read_timer_value(RTT));
	
	rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);

	/* Enable RTT interrupt */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 0);
	NVIC_EnableIRQ(RTT_IRQn);
	rtt_enable_interrupt(RTT, RTT_MR_ALMIEN);
}


void configure_lcd(void){
	/* Initialize display parameter */
	g_ili9488_display_opt.ul_width = ILI9488_LCD_WIDTH;
	g_ili9488_display_opt.ul_height = ILI9488_LCD_HEIGHT;
	g_ili9488_display_opt.foreground_color = COLOR_CONVERT(COLOR_WHITE);
	g_ili9488_display_opt.background_color = COLOR_CONVERT(COLOR_WHITE);

	/* Initialize LCD */
	ili9488_init(&g_ili9488_display_opt);
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
	
}



int main(void) {
	board_init();
	BUT_init();
	BUT2_init();
	sysclk_init();	
	configure_lcd();
	RTC_init();
	
	f_rtt_alarme = true;
	rtc_set_date_alarm(RTC, 1, MOUNTH, 1, DAY);
	rtc_set_time_alarm(RTC, 1, HOUR, 1, MINUTE, 1, SECOND+1);
	
	
	while(1) {
		if (flag_stop) {
			if (flag_button){
				pulse_add();
				flag_button = 0;
			}
			if (f_rtt_alarme){
				
				uint16_t pllPreScale = (int) (((float) 32768) / 2.0);
				uint32_t irqRTTvalue  = 8;
				
				RTT_init(pllPreScale, irqRTTvalue);
				f_rtt_alarme = false;
			}
			if (flag_distance) {
				distance += calculate_distance();
			}
			if (flag_speed) {
				speed = calculate_speed();
			}
			if (flag_ciclo) {
				
				flag_ciclo = 0;
			}
			
			if (flag_redraw2) {
				redraw2();
			}
			if (flag_draw) {
				redraw();
			}
			
			
		}
		
		
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}