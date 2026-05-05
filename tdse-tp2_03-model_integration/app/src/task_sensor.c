/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 * ... (Licencia omitida por brevedad, mantener en archivo final)
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"
#include "task_sensor_attribute.h"


/********************** macros and definitions *******************************/
#define DEL_BTN_MIN		0ul
#define DEL_BTN_MED		25ul
#define DEL_BTN_MAX		50ul

/* El tamaño se calcula dinámicamente. Como solo hay 1 config, será 1 */
#define SENSOR_CFG_QTY		(sizeof(task_sensor_cfg_list)/sizeof(task_sensor_cfg_t))
#define SENSOR_DTA_QTY		SENSOR_CFG_QTY

/********************** internal data declaration ****************************/




/********************** internal data declaration ****************************/
const task_sensor_cfg_t task_sensor_cfg_list[] = {
	{ID_BTN_B, BTN_B_PORT, BTN_B_PIN, BTN_B_PRESSED, DEL_BTN_MAX, EV_SYS_BTN_UP, EV_SYS_CAMERA},
	{ID_BTN_C, BTN_C_PORT, BTN_C_PIN, BTN_C_PRESSED, DEL_BTN_MAX, EV_SYS_BTN_UP, EV_SYS_BUTTON},
	{ID_BTN_D, BTN_D_PORT, BTN_D_PIN, BTN_D_PRESSED, DEL_BTN_MAX, EV_SYS_BTN_UP, EV_SYS_SENSOR_COIL}
};

task_sensor_dta_t task_sensor_dta_list[SENSOR_DTA_QTY]; // Automáticamente será de tamaño 3

/********************** internal functions declaration ***********************/
void task_sensor_statechart(uint32_t index);

/********************** internal data definition *****************************/
const char *p_task_sensor 		= "Task Sensor (Sensor Statechart)";
const char *p_task_sensor_ 		= "Non-Blocking Code";
const char *p_task_sensor__ 	= "(Update by Time Code, period = 1mS)";

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
void task_sensor_init(void *parameters)
{
	uint32_t index;
	task_sensor_dta_t *p_task_sensor_dta;
	task_sensor_st_t state;
	task_sensor_ev_t event;

	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", GET_NAME(task_sensor_init), HAL_GetTick());
	LOGGER_INFO("   %s is a %s", GET_NAME(task_sensor), p_task_sensor);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_sensor), p_task_sensor_);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_sensor), p_task_sensor__);

	for (index = 0; SENSOR_DTA_QTY > index; index++)
	{
		p_task_sensor_dta = &task_sensor_dta_list[index];

		state = ST_BTN_UP;
		p_task_sensor_dta->state = state;

		event = EV_BTN_UP;
		p_task_sensor_dta->event = event;

        p_task_sensor_dta->tick = DEL_BTN_MIN;

		LOGGER_INFO(" ");
		LOGGER_INFO("   %s = %lu   %s = %lu   %s = %lu",
				    GET_NAME(index), index,
					GET_NAME(state), (uint32_t)state,
					GET_NAME(event), (uint32_t)event);
	}
}

void task_sensor_update(void *parameters)
{
	uint32_t index;

	for (index = 0; SENSOR_DTA_QTY > index; index++)
	{
		task_sensor_statechart(index);
	}
}

void task_sensor_statechart(uint32_t index)
{
	const task_sensor_cfg_t *p_task_sensor_cfg;
	task_sensor_dta_t *p_task_sensor_dta;

	p_task_sensor_cfg = &task_sensor_cfg_list[index];
	p_task_sensor_dta = &task_sensor_dta_list[index];

    /* Inyección de evento bruto mediante lectura de hardware */
	if (p_task_sensor_cfg->pressed == HAL_GPIO_ReadPin(p_task_sensor_cfg->gpio_port, p_task_sensor_cfg->pin))
	{
		p_task_sensor_dta->event = EV_BTN_DOWN;
	}
	else
	{
		p_task_sensor_dta->event = EV_BTN_UP;
	}

    /* Statechart estricto */
	switch (p_task_sensor_dta->state)
	{
		case ST_BTN_UP:
			if (p_task_sensor_dta->event == EV_BTN_DOWN)
			{
				p_task_sensor_dta->tick = p_task_sensor_cfg->tick_max;
				p_task_sensor_dta->state = ST_BTN_FALLING;
			}
			break;

		case ST_BTN_FALLING:
			if (p_task_sensor_dta->tick > 0)
			{
				p_task_sensor_dta->tick--;
			}
			else
			{
				if (p_task_sensor_dta->event == EV_BTN_DOWN)
				{
					p_task_sensor_dta->state = ST_BTN_DOWN;
                    /* Depositamos el evento filtrado en la capa de sistema */
					put_event_task_system(p_task_sensor_cfg->signal_down);
				}
				else
				{
					p_task_sensor_dta->state = ST_BTN_UP;
				}
			}
			break;

		case ST_BTN_DOWN:
			if (p_task_sensor_dta->event == EV_BTN_UP)
			{
				p_task_sensor_dta->tick = p_task_sensor_cfg->tick_max;
				p_task_sensor_dta->state = ST_BTN_RISING;
			}
			break;

		case ST_BTN_RISING:
			if (p_task_sensor_dta->tick > 0)
			{
				p_task_sensor_dta->tick--;
			}
			else
			{
				if (p_task_sensor_dta->event == EV_BTN_UP)
				{
					p_task_sensor_dta->state = ST_BTN_UP;
                    /* Evento opcional de liberación (release) al sistema */
					put_event_task_system(p_task_sensor_cfg->signal_up);
				}
				else
				{
					p_task_sensor_dta->state = ST_BTN_DOWN;
				}
			}
			break;

		default:
			p_task_sensor_dta->tick  = DEL_BTN_MIN;
			p_task_sensor_dta->state = ST_BTN_UP;
			p_task_sensor_dta->event = EV_BTN_UP;
			break;
	}
}

/********************** end of file ******************************************/
