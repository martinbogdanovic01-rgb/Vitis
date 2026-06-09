#include "GPIO_MO8.h"

XGpio GDO0_GPIO;
/* ------------------------------------------------------------ */
/* GPIO Init                                                    */
/* ------------------------------------------------------------ */
int Init_GDO0_GPIO(void)
{
	int status;

	status = XGpio_Initialize(&GDO0_GPIO, GDO0_INPUT_DEVICE_ID);
	if (status != XST_SUCCESS) return XST_FAILURE;

	XGpio_SetDataDirection(&GDO0_GPIO, GDO0_CHANNEL, 0xF);

	return XST_SUCCESS;

}
