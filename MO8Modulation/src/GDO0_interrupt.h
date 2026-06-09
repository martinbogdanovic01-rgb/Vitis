#ifndef GDO0_INTERRUPT_H
#define GDO0_INTERRUPT_H

#include "xscugic.h"
#include "xparameters.h"
#include "GPIO_MO8.h"

// GDO0 is on Arduino interrupt-capable pins block
#define AR_INTC_GPIO_INTERRUPT_ID XPAR_FABRIC_ARDUINO_ARDUINO_INTR_EN_PINS_2_3_IP2INTC_IRPT_INTR
#define AR_GDO0_INT XGPIO_IR_CH1_MASK


extern volatile uint8_t PacketReady;
extern XScuGic Inst_GIC ;  	// GIC instance

int Init_GIC(void);
int Init_GDO0_GPIO(void);
int Add_GDO0_To_Interrupt_System(void);
void Init_GDO0_Int_System(void);

#endif
