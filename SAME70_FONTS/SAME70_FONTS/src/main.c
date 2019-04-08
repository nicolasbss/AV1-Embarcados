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
#define BUT_PIO_IDX       11u
#define BUT_PIO_IDX_MASK (1u << BUT_PIO_IDX)

volatile int flag_button;
volatile int flag_draw;
volatile int flag_speed;
volatile int pulse = 0;
volatile int speed = 0;

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

int calculate_speed() {
	int w = (2 * 3.14 * pulse)/4;
	pulse = 0;
	flag_speed = 0;
	return w * 0.65;
}

void redraw(speed) {
	char b[32];
	sprintf(b, "%d", speed);
	font_draw_text(&calibri_36, b, 50, 100, 1);
	flag_draw = 0;
}

void Button_Handler(void){
	flag_button = 1;
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
}


void pulse_add() {
	pulse += 1;
}

static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
}

void BUT_init(void){
	
	pmc_enable_periph_clk(BUT_PIO_ID);
	pio_set_input(BUT_PIO, BUT_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	
	pio_enable_interrupt(BUT_PIO, BUT_PIO_IDX_MASK);
	pio_handler_set(BUT_PIO, BUT_PIO_ID, BUT_PIO_IDX_MASK, PIO_IT_FALL_EDGE, Button_Handler);

	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 1);
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
	sysclk_init();	
	configure_lcd();
	
	f_rtt_alarme = true;
	
	while(1) {
		
		if (flag_button){
			pulse_add();
		}
		if (f_rtt_alarme){
          
			uint16_t pllPreScale = (int) (((float) 32768) / 2.0);
			uint32_t irqRTTvalue  = 8;
      
			RTT_init(pllPreScale, irqRTTvalue);         
			f_rtt_alarme = false;
		}
		
		if (flag_speed) {
			speed = calculate_speed();
		}
		if (flag_draw) {
			redraw(speed);
		}
		
		
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}