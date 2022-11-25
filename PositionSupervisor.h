/*!*****************************************************************************



 ******************************************************************************/

/*!*****************************************************************************
 * \file        PositionSupervisor.h
 * \author      Murat SAHIN
 * \date        24.09.2021
 *
 * \brief
 * It controls the position of the car.
 ******************************************************************************/


#ifndef INC_POSITIONSUPERVISOR_H_
#define INC_POSITIONSUPERVISOR_H_

/*-----------------------------------------------------------------------------
 * INCLUDE FILES
 *----------------------------------------------------------------------------*/
#include <CallController.h>
#include <HMInterface.h>
#include "conditions.h"

/*-----------------------------------------------------------------------------
 * EXPORTED DEFINITIONS
 *----------------------------------------------------------------------------*/
typedef enum enumPSUState {
	precommisioningMode	= 2,
	teachMode			= 3,
	configurationMode	= 4,
	validationMode		= 5,
	normalMode			= 6
}PSUState;

typedef enum enumValidation {
	initValidate 		= 0,
	validateCar			= 1,
	validateShaft		= 2,
	releaseValidate 	= 3
}Validation;

typedef enum enumNormalModeCarCall{
	carIdle						= 0,
	requestCloseDoor			= 1,
	carStart					= 2,
	cruise						= 3,
	carStop						= 5,
	reLeveling					= 7
}CarManagement_t;

typedef struct{
	uint8_t enableInspectionUp		:1;
	uint8_t enableInspectionDown	:1;
	uint8_t enableRecallUp			:1;
	uint8_t enableRecallDown		:1;
	uint8_t recallUp				:1;
	uint8_t recallDown				:1;
	uint8_t inspectionUp			:1;
	uint8_t inspectionDown			:1;
	uint8_t pitInspectionUp			:1;
	uint8_t pitInspectionDown		:1;
	uint16_t timerUp;
	uint16_t timerDown;
	uint32_t timerRunDelay;
	uint32_t timerReLeveling;
	uint32_t timerParkingTime;
	uint32_t timerLight;
}EnableMovement_t;

typedef enum{
	carPositionInitial			= 0,
	prepareToMoveUp				= 1,
	findGroundLevel				= 2,
	floorLevel					= 3
}DetectCarPosition_t;

typedef enum {
	InitializeCarPosition		= 0,
	Inspection					= 1,
	Recall						= 2,
	UserCarManagement			= 3,
	Fault						= 4,
	BlockedFault				= 5,
	Installation				= 6,
	Evacuation					= 7,
	BypassSelected				= 8,
	OnlyInspection				= 9,
	ReleaseInspection			= 10
}NormalMode_t;



/*-----------------------------------------------------------------------------
 * EXPORTED FUNCTIONS
 *----------------------------------------------------------------------------*/
PSUState	PSUStateController(void);
void		ProcessConfiguration(uint16_t timer1msDiff);
void		PSUController(uint16_t timer10msDiff, Control *control);
void		ProcessTeach(uint16_t timer1msDiff);
void		ProcessValidation(CO_t *co, CO_t *coShaft, uint16_t timer1msDiff);
void		Precommisioning(CO_t *co, CO_t *coShaft, uint16_t CO_timer_10ms, Control *control);
void		SpeedSupervisor(uint16_t timer1msDiff);
#endif /* INC_POSITIONSUPERVISOR_H_ */
/*-----------------------------------------------------------------*-
  -------- END OF FILE -------------------------------------------
--*----------------------------------------------------------------*/
