/*!****************************************************************************-
 * \file        Application.c
 * \author      Murat SAHIN
 *
 * \brief
 * The real application of tha main device
 ******************************************************************************/

/*-----------------------------------------------------------------------------
 * INCLUDE SECTION
 *----------------------------------------------------------------------------*/
#include "PositionSupervisor.h"

/*-----------------------------------------------------------------------------
 * LOCAL (static) DEFINITIONS
 *----------------------------------------------------------------------------*/
static CarManagement_t carManagement = carIdle;
static DetectCarPosition_t carPositioning = carPositionInitial;


/*-----------------------------------------------------------------------------
 * GLOBAL DEFINITIONS
 *----------------------------------------------------------------------------*/
extern UART_HandleTypeDef huart2;
extern uint32_t appVersionNew, loaderVersionNew;
extern volatile Condition_Handler_t hCondition;

/*-----------------------------------------------------------------------------
 * LOCAL FUNCTION PROTOTYPES
 *----------------------------------------------------------------------------*/
void Inialize_Car_Position(uint16_t timer10msDiff);
NormalMode_t CarControllerStateDecision(uint8_t onlyInspectionProcess);
void RecalOperation(uint16_t timer10msDiff, EnableMovement_t *recallState);
void SendDataToInverter(void);
void EnableMovement(EnableMovement_t *recallState);
void InspectionOperation(uint16_t timer10msDiff, EnableMovement_t *inspectionState);
void Car_Pit_InspectionOperation(uint16_t timer10msDiff, EnableMovement_t *inspectionState);
void Pit_InspectionOperation(uint16_t timer10msDiff, EnableMovement_t *inspectionState);
void Car_InspectionOperation(uint16_t timer10msDiff, EnableMovement_t *inspectionState);
void MovingInspectionUp(uint16_t timer10msDiff, EnableMovement_t *inspectionState);
void MovingInspectionDown(uint16_t timer10msDiff, EnableMovement_t *inspectionState);
void MovingRecallUp(uint16_t timer10msDiff, EnableMovement_t *inspectionState);
void MovingRecallDown(uint16_t timer10msDiff, EnableMovement_t *inspectionState);
void SearchTheFloorToSlowdown(uint16_t timer10msDiff);
void SearchTheCarLevelToStop(uint16_t timer10msDiff);
void ParkingFunction(uint16_t timer10msDiff);
void InspectionStopping(EnableMovement_t *inspectionState);
void InspectionStopImmediately(EnableMovement_t *inspectionState);
void FireEvacuation(void);
void Relevelling(uint16_t timer10msDiff);
void RequestCloseDoor(uint16_t timer10msDiff);
void StartCarMoving(uint16_t timer10msDiff);
void Upgrade(Control *control);
void CarManagement(uint16_t timer10msDiff);
void CarIdle(uint16_t timer10msDiff);
void ReleaseInspectionManagement(uint16_t timer10msDiff);
/*-----------------------------------------------------------------------------
 * LOCAL FUNCTIONS
 *----------------------------------------------------------------------------*/

void ReleaseInspectionManagement(uint16_t timer10msDiff)
{
	carManagement = carIdle;

	if((progOutput.oCarIsInDoorZone.value == true) && (speedRespond == noSpeed)){
		doorCommand.psuRequestDoorA = openTheDoor;
		doorCommand.psuRequestDoorB = openTheDoor;

		if(pProtectionParameter2->packetField.conditionToExitInsp == WITH_KEY_AND_OPEN_DOOR){
			if(IS_DOORA_OPEN_WITHOUT_TORQUE || IS_DOORB_OPEN_WITHOUT_TORQUE){
				if(psuVariables.inspectFlag == true){
					// After 400 millisecond, reset inspection flag
					if(psuVariables.timerAfterInspection < 400){
						psuVariables.timerAfterInspection += timer10msDiff;
					}
					else{
						psuVariables.inspectFlag = false;
					}
				}
				else{
					psuVariables.timerAfterInspection = 0;
				}
			}
		}
		else{
			if(psuVariables.inspectFlag == true){
				// After 400 millisecond, reset inspection flag
				if(psuVariables.timerAfterInspection < 400){
					psuVariables.timerAfterInspection += timer10msDiff;
				}
				else{
					psuVariables.inspectFlag = false;
				}
			}
			else{
				psuVariables.timerAfterInspection = 0;
			}
		}
	}
	else{
		doorCommand.psuRequestDoorA = closeTheDoor;
		doorCommand.psuRequestDoorB = closeTheDoor;
		// After 400 millisecond, reset inspection flag
		if(psuVariables.timerAfterInspection < 400){
			//callFlag = true;
			psuVariables.timerAfterInspection += timer10msDiff;
		}
		else{
			// Reset Car Call
			if(carCurrentFloor < OD_PERSIST_APP_AUTO.x63EC_topAndBottomFloor[1]){
				callList.packetField[carCurrentFloor].carCallA = true;
			}
			else{
				callList.packetField[carCurrentFloor - 2].carCallA = true;
			}
			psuVariables.inspectFlag = false;
		}
	}
}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
	OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void CarIdle(uint16_t timer10msDiff)
{
	// Do not request anything from door function
	if(doorCommand.psuRequestDoorA == openTheDoor){
		if((doorRegister.doorA_Open == true)){
			doorCommand.psuRequestDoorA = noAction;
		}
	}
	else{
		doorCommand.psuRequestDoorA = noAction;
	}
	if(doorCommand.psuRequestDoorB == openTheDoor){
		if((doorRegister.doorB_Open == true)){
			doorCommand.psuRequestDoorB = noAction;
		}
	}
	else{
		doorCommand.psuRequestDoorB = noAction;
	}

	// Reset Direction
	directionIndication.packetField.inspectionUp = 0;
	directionIndication.packetField.inspectionDown = 0;
	directionIndication.packetField.movingDown = 0;
	directionIndication.packetField.movingUp = 0;

	// Reset immediately stop register
	if(hCondition.States.actionflag_gotoparametertarget == true){
		hCondition.States.actionflag_gotoparametertarget = false;
	}

	// No speed
	speedDemand = noSpeed;

	// Don't bridge the door contact
	psuControlWrite.packetValue = 0;
	OD_set_u32(OD_ENTRY_H63E0_PSU_Control, 1, psuControlWrite.packetValue, true);

	if(pPickedTarget->targetFloor == 0xFF){
		// reset motion type
		directionIndication.packetField.moveUp = false;
		directionIndication.packetField.moveDown = false;

		psuVariables.lightCommand = false;

		// There is no floor to go
		ParkingFunction(timer10msDiff);

		if(progOutput.oCarIsInDoorZone.value == true){
			if((IS_DOORA_OPEN_WITHOUT_TORQUE || IS_DOORA_CLOSED_WITHOUT_TORQUE) && (IS_DOORB_OPEN_WITHOUT_TORQUE || IS_DOORB_CLOSED_WITHOUT_TORQUE)){
				// Re leveling will be done, when doors are totally closed or opened
				if((pBasicParameter2->packetField.relevellingFunction == ENABLE) &&
						(statusReleveling == sucessfulReleveling || statusReleveling == startReleveling)){
						// Re-levelling can be done, if re leveling is enabled and prior re-leveling was successful

					// if one of the leveling is missing then leveling will be done
					if((!progInput.iSignal141.value) && (progInput.iSignal142.value)){
						psuVariables.timerReLeveling = 0;
						carManagement = reLeveling;
					}
					else if((progInput.iSignal141.value) && (!progInput.iSignal142.value)){
						psuVariables.timerReLeveling = 0;
						carManagement = reLeveling;
					}
				}
			}
		}
	}
	else{
		// any call received, light will be on
		psuVariables.lightCommand = true;

		// select motion type
		if(pPickedTarget->targetFloor > carCurrentFloor){
			directionIndication.packetField.moveUp = true;
		}
		else{
			directionIndication.packetField.moveUp = false;
		}
		if(pPickedTarget->targetFloor < carCurrentFloor){
			directionIndication.packetField.moveDown = true;
		}
		else{
			directionIndication.packetField.moveDown = false;
		}

		if(carCurrentFloor != pPickedTarget->targetFloor){
			carManagement = requestCloseDoor;
		}
	}
	if((doorCommand.psuRequestDoorA == openTheDoor) || (doorCommand.psuRequestDoorA == openTheDoor)){
		infomessages.infoPassengerLoadUnload.flag = true;
	}
}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
	OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void ParkingFunction(uint16_t timer10msDiff)
{
	if(pBasicParameter2->packetField.parkingFunction == ENABLE){
		if(carCurrentFloor != pBasicParameter2->packetField.parkingFloorNew){
			if(psuVariables.timerParkingTime < (pBasicParameter2->packetField.parkingDelay * 1000)){
				psuVariables.timerParkingTime += timer10msDiff;
			}
			else{
				callListForMovement.packetField[pBasicParameter2->packetField.parkingFloorNew - 1].carCallA = true;
			}
		}
		else{
			psuVariables.timerParkingTime = 0;
		}
	}
	else{
		psuVariables.timerParkingTime = 0;
	}
}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
	OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void SearchTheCarLevelToStop(uint16_t timer10msDiff)
{
	// reset parking time
	psuVariables.timerParkingTime = 0;

	// request turn on the light
	psuVariables.lightCommand = true;
	// The contacts are already bridged

	if(progOutput.oCarIsInDoorZone.value == true){
		if((pPickedTarget->targetFloor == carCurrentFloor) && IS_PICKEDTARGET_DOORA){
			// if the picked target is door A, open door A
			doorCommand.psuRequestDoorA = openTheDoor;
		}
		else{
			doorCommand.psuRequestDoorA = closeTheDoor;
		}

		if((pPickedTarget->targetFloor == carCurrentFloor) && IS_PICKEDTARGET_DOORB){
			// if the picked target is door B, open Door B
			doorCommand.psuRequestDoorB = openTheDoor;
		}
		else{
			doorCommand.psuRequestDoorB = closeTheDoor;
		}
		if(psuStatus.packetField.carIsOnLevelWithDeviation == true){
			// reset the speed, if the car is on level
			directionIndication.packetField.movingUp = false;
			directionIndication.packetField.movingDown = false;

			// stop the car
			speedDemand = noSpeed;

			// stop bridging
			psuControlWrite.packetValue = 0;
			OD_set_u32(OD_ENTRY_H63E0_PSU_Control, 1, psuControlWrite.packetValue, true);

			carManagement = carIdle;
		}
	}
	else{
		doorCommand.psuRequestDoorA = closeTheDoor;
		doorCommand.psuRequestDoorB = closeTheDoor;
	}
	if((doorCommand.psuRequestDoorA == openTheDoor) || (doorCommand.psuRequestDoorA == openTheDoor)){
		infomessages.infoPassengerLoadUnload.flag = true;
	}
}


/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
	OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void SearchTheFloorToSlowdown(uint16_t timer10msDiff)
{
	static uint8_t enableSlowDown = false, enableSlowUp = false;

	// Request to close the doors
	doorCommand.psuRequestDoorA = closeTheDoor;
	doorCommand.psuRequestDoorB = closeTheDoor;

	// reset parking time
	psuVariables.timerParkingTime = 0;

	// request turn on the light
	psuVariables.lightCommand = true;

	if((IS_DOORA_CLOSED_WITHOUT_TORQUE) &&
			(IS_DOORB_CLOSED_WITHOUT_TORQUE) &&
			(IS_MANUEL_DOOR_CLOSED)){
		// be sure that the doors are closed
		if(progOutput.oCarIsInDoorZone.value == false){
			// To slow down it should be out of the door level
			if(progInput.iSignal142.value == false){
				enableSlowUp = true;
			}
			if(progInput.iSignal141.value == false){
				enableSlowDown = true;
			}
			if((directionIndication.packetField.movingUp == true) &&(((enableSlowUp == true) && (progInput.iSignal142.value) &&
				// To slow down, slow down sensor for moving up should be detected
					((pPickedTarget->targetFloor == 0xFF) || (pPickedTarget->targetFloor <= carCurrentFloor) || (hCondition.States.actionflag_gotoparametertarget == true))) ||
							(psuStatus.packetField.inspLimitSwitchTop == false))){ // todo install flag control with 818
					// if there is no call, or the floor is the target floor, or the car should stop immediately

				// Start to slow down
				speedDemand = slowSpeedUp;

				// set arrival indication
				psuVariables.arrivalIndication = true;

				// Start Bridging
				psuControlWrite.packetValue = 0;
				psuControlWrite.packetField.bridgingFloorNumber = carCurrentFloor;
				psuControlWrite.packetField.bridgeWhileLevelling = true;
				OD_set_u32(OD_ENTRY_H63E0_PSU_Control, 1, psuControlWrite.packetValue, true);

				// Prepare to stop
				carManagement = carStop;
			}
			else if((directionIndication.packetField.movingDown == true) && (((enableSlowDown == true )&&(progInput.iSignal141.value) &&
					// To slow down, slow down sensor for moving down should be detected
						((pPickedTarget->targetFloor == 0xFF) ||
								(pPickedTarget->targetFloor >= carCurrentFloor) ||
								(hCondition.States.actionflag_gotoparametertarget == true))) ||
								(psuStatus.packetField.inspLimitSwitchBottom == false))){ // todo install flag control with 817

				// Start to slow down
				speedDemand = slowSpeedDown;
				// set arrival indication
				psuVariables.arrivalIndication = true;

				// Start Bridging
				psuControlWrite.packetValue = 0;
				psuControlWrite.packetField.bridgingFloorNumber = carCurrentFloor;
				psuControlWrite.packetField.bridgeWhileLevelling = true;
				OD_set_u32(OD_ENTRY_H63E0_PSU_Control, 1, psuControlWrite.packetValue, true);

				// Prepare to stop
				carManagement = carStop;
			}
		}
		else{
			enableSlowDown = false;
			enableSlowUp = false;
		}
	}
}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void StartCarMoving(uint16_t timer10msDiff)
{
	// Request to close the doors
	doorCommand.psuRequestDoorA = closeTheDoor;
	doorCommand.psuRequestDoorB = closeTheDoor;

	// reset parking time
	psuVariables.timerParkingTime = 0;

	// request turn on the light
	psuVariables.lightCommand = true;

	if((IS_DOORA_CLOSED_WITHOUT_TORQUE) && (IS_DOORB_CLOSED_WITHOUT_TORQUE) &&
			(IS_CARDOORA_SAFETY_CONT_CLOSED) && (IS_LANDING_DOORA_LOCKED) &&
			(IS_CARDOORB_SAFETY_CONT_CLOSED) && (IS_LANDING_DOORB_LOCKED) &&
			(IS_MANUEL_DOOR_CLOSED)){
		if(pPickedTarget->targetFloor == 0xFF || pPickedTarget->targetFloor == carCurrentFloor){
			// if there is no call or the call belongs to current floor, then wait
			carManagement = carIdle;
		}
		else{
			if(pPickedTarget->targetFloor > carCurrentFloor){
				// Decide to up
				// start to go up at nominal speed
				directionIndication.packetField.movingUp = true;

				if(speedRespond == noSpeed){
					speedDemand = nominalSpeedUp;
				}
			}
			else if(pPickedTarget->targetFloor < carCurrentFloor){
				// Decide to go down
				// start to go down at nominal speed
				directionIndication.packetField.movingDown = true;

				if(speedRespond == noSpeed){
					speedDemand = nominalSpeedDown;
				}
			}
			carManagement = cruise;
		}
	}
	/*else{
		// reset moving
		directionIndication.packetValue = 0;

		speedDemand = noSpeed;

		carManagement = carIdle;
	}*/

}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void RequestCloseDoor(uint16_t timer10msDiff)
{
	// Request to close the doors
	doorCommand.psuRequestDoorA = closeTheDoor;
	doorCommand.psuRequestDoorB = closeTheDoor;

	// reset parking time
	psuVariables.timerParkingTime = 0;

	// request turn on the light
	psuVariables.lightCommand = true;

	if((IS_DOORA_CLOSED_WITHOUT_TORQUE) && (IS_DOORB_CLOSED_WITHOUT_TORQUE) &&
			(IS_CARDOORA_SAFETY_CONT_CLOSED) && (IS_LANDING_DOORA_LOCKED) &&
			(IS_CARDOORB_SAFETY_CONT_CLOSED) && (IS_LANDING_DOORB_LOCKED) &&
			(IS_MANUEL_DOOR_CLOSED)){
		if(psuVariables.timerRunDelay < 800){
			// Wait 800 millisecond
			psuVariables.timerRunDelay += timer10msDiff;
		}
		else if(pPickedTarget->targetFloor == 0xFF || pPickedTarget->targetFloor == carCurrentFloor){
			// if there is no call or the call belongs to current floor, then wait
			carManagement = carIdle;
		}
		else{
			// if there is any call start to cruise
			carManagement = carStart;
		}
	}
	else{
		psuVariables.timerRunDelay = 0;
	}

}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void Relevelling(uint16_t timer10msDiff)
{
	// increase the timer
	infomessages.infoRelevelling.flag = true;
	psuVariables.timerReLeveling += timer10msDiff;

	// Decide to bridge the door or not
	if((!IS_DOORA_CLOSED_WITHOUT_TORQUE) || (!IS_DOORB_CLOSED_WITHOUT_TORQUE)){
		// bridge the door
		psuControlWrite.packetValue = 0;
		psuControlWrite.packetField.bridgingFloorNumber = carCurrentFloor;
		psuControlWrite.packetField.bridgeWhileRelevelling = true;
		OD_set_u32(OD_ENTRY_H63E0_PSU_Control, 1, psuControlWrite.packetValue, true);
	}
	else{
		// don't bridge the door
		psuControlWrite.packetValue = 0;
		OD_set_u32(OD_ENTRY_H63E0_PSU_Control, 1, psuControlWrite.packetValue, true);
	}

	if((progOutput.oCarIsInDoorZone.value == true) && (psuStatus.packetField.carIsOnLevelWithDeviation == true)){
		// if the car is in door zone and on level

		// stop the car
		speedDemand = noSpeed;

		// reset the timer
		psuVariables.timerReLeveling = 0;

		// inform functions
		statusReleveling = sucessfulReleveling;

		// stop re leveling
		carManagement = carIdle;
	}
	else if((!progInput.iSignal141.value) && (!progInput.iSignal142.value)){
		// if the car is out of the re leveling zone, then there is an error

		// stop the car
		speedDemand = noSpeed;

		// reset the timer
		psuVariables.timerReLeveling = 0;

		// inform functions
		statusReleveling = errorReleveling;

		// stop re leveling
		carManagement = carIdle;
	}

	if(psuVariables.timerReLeveling > 20000){
		// re leveling time exceeded
		speedDemand = noSpeed;

		// reset the timer
		psuVariables.timerReLeveling = 0;

		// inform functions
		statusReleveling = failedReleveling;

		// stop re leveling
		carManagement = carIdle;
	}
	else if((psuVariables.timerReLeveling > 5000) &&
			(progOutput.oCarIsInDoorZone.value == false)){
		speedDemand = noSpeed;
		psuVariables.timerReLeveling = 0;
		statusReleveling = failedReleveling;
		carManagement = carIdle;
	}
	else{
		statusReleveling = processingReleveling;

		if((!progInput.iSignal141.value) && (progInput.iSignal142.value)){
			speedDemand = relevellingSpeedDown;
		}

		if((progInput.iSignal141.value) && (!progInput.iSignal142.value)){
			speedDemand = relevellingSpeedUp;
		}
	}
}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void CarManagement(uint16_t timer10msDiff)
{
	// if 120 is all right
	if(unProgInput.iS20.value){
		switch(carManagement){
			case carIdle:
			{
				CarIdle(timer10msDiff);
				break;
			}
			case reLeveling:
			{
				Relevelling(timer10msDiff);
				break;
			}
			case requestCloseDoor:
			{
				RequestCloseDoor(timer10msDiff);
				break;
			}
			case carStart:
			{
				StartCarMoving(timer10msDiff);
				break;
			}
			case cruise:
			{
				SearchTheFloorToSlowdown(timer10msDiff);
				break;
			}
			case carStop:
			{
				SearchTheCarLevelToStop(timer10msDiff);
				break;
			}
		}
	}
	else{
		// Do not request anything from door function
		doorCommand.psuRequestDoorA = noAction;
		doorCommand.psuRequestDoorB = noAction;

		// No speed
		speedDemand = noSpeed;
	}
}
/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void Upgrade(Control *control)
{
	/*if(menutitle.mainMenu_6.prmt->value > 0){ // parameterMain[softwareUpdate].value
		menutitle.mainMenu_6.prmt->value = 0; // parameterMain[softwareUpdate].value
		control->upgrade = 1;
		if(loaderVersionNew != control->loaderVersion){
			control->loaderFWUpgrade = 1;
		}
		if(appVersionNew != OD_RAM.x1F56_programSoftwareIdentification[0]){
			control->appFWUpgrade = 1;
			JumpApplication(control);
		}
		else if(loaderVersionNew != control->loaderVersion){
			OD_set_u32(OD_ENTRY_H63E2_PSUSafetyControl, precommisioningMode, 1, true);
		}
		else{
			JumpApplication(control);
		}
	}*/
}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void FireEvacuation(void)
{
	// TODO oFireAlarm
	// todo fire mode
	// TODO fire evacuation
/*	if(OD_PERSIST_APP_AUTO.x200F_fireEvacuationState == DISABLE){
		if(((OD_RAM.x2402_functionalInput[G1] & bitG1FireEvacuation1) == bitG1FireEvacuation1)){
			fireEvacuation1 = 1;
		}
		if(((OD_RAM.x2402_functionalInput[G1] & bitG1FireEvacuationSignal2) == bitG1FireEvacuationSignal2)){
			fireEvacuation2 = 1;
		}
	}
	else{
		fireEvacuation1 = 0;
		fireEvacuation2 = 0;
	}*/
	/*		if((fireEvacuation1 == true) || (fireEvacuation2 == true)){
				MainMenu[0].titleRow1 = fireEvacuationState;
				for(int i = 1; i < 65; i++){
					if(fireEvacuation2 == true){
						if(i == OD_PERSIST_APP_AUTO.x2011_fireEvacuationAlternateFloor){

						}
						else{
							OD_set_u8(OD_ENTRY_H2108_processCallList, i, 0, true);
						}
					}
					else if(fireEvacuation1 == true){
						if(i == pFireParameter->packetField.fireEvacuationMainFloor){
//							OD_set_u8(OD_ENTRY_H2108_processCallList, i, 0, true);
						}
						else{
							OD_set_u8(OD_ENTRY_H2108_processCallList, i, 0, true);
						}
					}
				}
			}
			else{*/

//			}

}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void InspectionStopImmediately(EnableMovement_t *inspectionState)
{
	// Reset inspection up and down
	directionIndication.packetField.inspectionCloseDoor = false;
	directionIndication.packetField.inspectionDown = false;
	directionIndication.packetField.inspectionUp = false;

	// reset timer
	inspectionState->timerUp = 0;
	inspectionState->timerDown = 0;

	// reset speed
	speedDemand = noSpeed;
}


/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void InspectionStopping(EnableMovement_t *inspectionState)
{
	inspectionState->timerDown = 0;
	inspectionState->timerUp = 0;

	directionIndication.packetField.inspectionCloseDoor = true;
	directionIndication.packetField.inspectionUp = false;
	directionIndication.packetField.inspectionDown = false;

	// reset speed
	speedDemand = noSpeed;
}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void MovingRecallDown(uint16_t timer10msDiff, EnableMovement_t *inspectionState)
{
	if(inspectionState->enableRecallDown == true){
		// inform other functions that the car will move
		directionIndication.packetField.inspectionCloseDoor = true;

		if((IS_CARDOORA_SAFETY_CONT_CLOSED) &&  (IS_LANDING_DOORA_LOCKED) &&
				(IS_CARDOORB_SAFETY_CONT_CLOSED) &&  (IS_LANDING_DOORB_LOCKED) &&
				(IS_MANUEL_DOOR_CLOSED)){
			if(inspectionState->timerDown > 1000){
				directionIndication.packetField.inspectionDown = true;
				// Go down at inspection speed
				speedDemand = inspectionSpeedDown;
			}
			else{
				directionIndication.packetField.inspectionDown = false;

				inspectionState->timerDown += timer10msDiff;

				// No speed for 1 second
				speedDemand = noSpeed;
			}
		}
		else{
			// reset timer
			inspectionState->timerDown = 0;
		}
	}
	else{
		directionIndication.packetField.inspectionDown = false;

		// reset timer
		inspectionState->timerDown = 0;

		// reset speed
		speedDemand = noSpeed;
	}
}
/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void MovingInspectionDown(uint16_t timer10msDiff, EnableMovement_t *inspectionState)
{
	if(inspectionState->enableInspectionDown == true){
		// inform other functions that the car will move
		directionIndication.packetField.inspectionCloseDoor = true;

		if((IS_CARDOORA_SAFETY_CONT_CLOSED) &&  (IS_LANDING_DOORA_LOCKED) &&
				(IS_CARDOORB_SAFETY_CONT_CLOSED) &&  (IS_LANDING_DOORB_LOCKED) &&
				(IS_MANUEL_DOOR_CLOSED)){
			if(inspectionState->timerDown > 1000){
				directionIndication.packetField.inspectionDown = true;
				// Go down at inspection speed
				speedDemand = inspectionSpeedDown;
			}
			else{
				directionIndication.packetField.inspectionDown = false;

				inspectionState->timerDown += timer10msDiff;

				// No speed for 1 second
				speedDemand = noSpeed;
			}
		}
		else{
			// reset timer
			inspectionState->timerDown = 0;
		}
	}
	else{
		directionIndication.packetField.inspectionDown = false;

		// reset timer
		inspectionState->timerDown = 0;

		// reset speed
		speedDemand = noSpeed;
	}
}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void MovingInspectionUp(uint16_t timer10msDiff, EnableMovement_t *inspectionState)
{
	if(inspectionState->enableInspectionUp == true){
		// inform other functions that the car will move
		directionIndication.packetField.inspectionCloseDoor = true;

		if((IS_CARDOORA_SAFETY_CONT_CLOSED) &&  (IS_LANDING_DOORA_LOCKED) &&
				(IS_CARDOORB_SAFETY_CONT_CLOSED) &&  (IS_LANDING_DOORB_LOCKED) &&
				(IS_MANUEL_DOOR_CLOSED)){
			if(inspectionState->timerUp > 1000){
				directionIndication.packetField.inspectionUp = true;
				// Go down at inspection speed
				speedDemand = inspectionSpeedUp;
			}
			else{
				directionIndication.packetField.inspectionUp = false;

				inspectionState->timerUp += timer10msDiff;

				// No speed for 1 second
				speedDemand = noSpeed;
			}
		}
		else{
			// reset timer
			inspectionState->timerUp = 0;
		}
	}
	else{
		directionIndication.packetField.inspectionUp = false;

		// reset timer
		inspectionState->timerUp = 0;

		// reset speed
		speedDemand = noSpeed;
	}
}
/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void MovingRecallUp(uint16_t timer10msDiff, EnableMovement_t *inspectionState)
{
	if(inspectionState->enableRecallUp == true){
		// inform other functions that the car will move
		directionIndication.packetField.inspectionCloseDoor = true;

		if((IS_CARDOORA_SAFETY_CONT_CLOSED) &&  (IS_LANDING_DOORA_LOCKED) &&
				(IS_CARDOORB_SAFETY_CONT_CLOSED) &&  (IS_LANDING_DOORB_LOCKED) &&
				(IS_MANUEL_DOOR_CLOSED)){
			if(inspectionState->timerUp > 1000){
				directionIndication.packetField.inspectionUp = true;
				// Go down at inspection speed
				speedDemand = inspectionSpeedUp;
			}
			else{
				directionIndication.packetField.inspectionUp = false;

				inspectionState->timerUp += timer10msDiff;

				// No speed for 1 second
				speedDemand = noSpeed;
			}
		}
		else{
			// reset timer
			inspectionState->timerUp = 0;
		}
	}
	else{
		directionIndication.packetField.inspectionUp = false;

		// reset timer
		inspectionState->timerUp = 0;

		// reset speed
		speedDemand = noSpeed;
	}
}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void Car_InspectionOperation(uint16_t timer10msDiff, EnableMovement_t *inspectionState)
{
	if((inspectionState->inspectionDown) && (inspectionState->inspectionUp)){
		InspectionStopping(inspectionState);
	}
	else if(inspectionState->inspectionDown){
		infomessages.infoHighInspectionDown.flag = true;
		MovingInspectionDown(timer10msDiff,inspectionState);
	}
	else if(inspectionState->inspectionUp){
		infomessages.infoHighInspectionUp.flag = true;
		MovingInspectionUp(timer10msDiff,inspectionState);
	}
	else{
		InspectionStopImmediately(inspectionState);
	}
}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void Pit_InspectionOperation(uint16_t timer10msDiff, EnableMovement_t *inspectionState)
{
	if(pOtherParameter->packetField.pitInspectionState == ENABLE){
		if((inspectionState->pitInspectionDown) && (inspectionState->pitInspectionUp)){
			InspectionStopping(inspectionState);
		}
		else if(inspectionState->pitInspectionDown){
			infomessages.infoHighInspectionDown.flag = true;
			MovingInspectionDown(timer10msDiff,inspectionState);
		}
		else if(inspectionState->pitInspectionUp){
			infomessages.infoHighInspectionUp.flag = true;
			MovingInspectionUp(timer10msDiff,inspectionState);
		}
		else{
			InspectionStopImmediately(inspectionState);
		}
	}
}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void Car_Pit_InspectionOperation(uint16_t timer10msDiff, EnableMovement_t *inspectionState)
{
	if((inspectionState->inspectionDown) && (inspectionState->inspectionUp) && (inspectionState->pitInspectionDown) && (inspectionState->pitInspectionUp)){
		// reset timers
		InspectionStopping(inspectionState);
	}
	else if((inspectionState->inspectionDown) && (inspectionState->pitInspectionDown)){
		// To be able to move up "pit inspection up" and "inspection up" buttons should be pressed together
		infomessages.infoHighInspectionDown.flag = true;
		MovingInspectionDown(timer10msDiff, inspectionState);
	}
	else if((inspectionState->inspectionUp) && (inspectionState->pitInspectionUp)){
		// To be able to move down "pit inspection down" and "inspection down" buttons should be pressed together
		infomessages.infoHighInspectionUp.flag = true;
		MovingInspectionUp(timer10msDiff, inspectionState);
	}
	else{
		// if none of the above situation has achieved, the car will stop
		InspectionStopImmediately(inspectionState);
	}
}


/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void EnableMovement(EnableMovement_t *inspectionState)
{
	if(psuStatus.packetField.inspLimitSwitchBottom == false){
		if((pBasicParameter2->packetField.onInspectionLimitNew == CONTINUE_TO_DRIVE)){
			inspectionState->enableRecallDown = true;
		}
		else if((pBasicParameter2->packetField.onInspectionLimitNew == STOP_AT_NEXT_FLOOR)){
			if(psuStatus.packetField.carIsOnLevelWithDeviation == true){
				inspectionState->enableInspectionDown = false;
				inspectionState->enableRecallDown = false;
			}
			else{
				inspectionState->enableInspectionDown = true;
				inspectionState->enableRecallDown = true;
			}
		}
		else{
			inspectionState->enableInspectionDown = false;
			inspectionState->enableRecallDown = false;
		}
	}
	else{
		inspectionState->enableInspectionDown = true;
		inspectionState->enableRecallDown = true;
	}

	if(psuStatus.packetField.inspLimitSwitchTop == false){
		if((pBasicParameter2->packetField.onInspectionLimitNew == CONTINUE_TO_DRIVE)){
			inspectionState->enableRecallUp = true;
		}
		else if((pBasicParameter2->packetField.onInspectionLimitNew == STOP_AT_NEXT_FLOOR)){
			if(psuStatus.packetField.carIsOnLevelWithDeviation == true){
				inspectionState->enableInspectionUp = false;
				inspectionState->enableRecallUp = false;
			}
			else{
				inspectionState->enableInspectionUp = true;
				inspectionState->enableRecallUp = true;
			}
		}
		else if((pBasicParameter2->packetField.onInspectionLimitNew == STOP_IMMEDIATELY)){
			inspectionState->enableInspectionUp = false;
			inspectionState->enableRecallUp = false;
		}
	}
	else{
		inspectionState->enableInspectionUp = true;
		inspectionState->enableRecallUp = true;
	}
}

/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/

void SendDataToInverter(void)
{
	// prepare inverter register and send to inverter
	controlWord.packetField.switchOn = true;
	controlWord.packetField.enableVoltage = true;
	controlWord.packetField.enableOperation = true;
	OD_set_u16(OD_ENTRY_H6400_controlword, 0, controlWord.packetValue, true);
	OD_set_u16(ODShaft_ENTRY_H6400_controlword, 0, controlWord.packetValue, true);

	messageController.messageStatus[typeDriveUnit] = send;
}
/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void InspectionOperation(uint16_t timer10msDiff, EnableMovement_t *inspectionState)
{
	directionIndication.packetValue = 0;

	EnableMovement(inspectionState);

	if((IS_CAR_INSPECTION_ACTIVE) && (IS_PIT_INSPECTION_ACTIVE)){
		if(pOtherParameter->packetField.pitInspectionState == ENABLE){
			infomessages.infoCarPitInspection.flag = true;
			Car_Pit_InspectionOperation(timer10msDiff, inspectionState);
		}
	}
	else if(IS_CAR_INSPECTION_ACTIVE){
		infomessages.infoCarInspection.flag = true;
		Car_InspectionOperation(timer10msDiff, inspectionState);
	}
	else if(IS_PIT_INSPECTION_ACTIVE){
		infomessages.infoPitInspection.flag = true;
		Pit_InspectionOperation(timer10msDiff, inspectionState);
	}
}


/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
void RecalOperation(uint16_t timer10msDiff, EnableMovement_t *inspectionState)
{
	// reset direction indication
	directionIndication.packetValue = 0;

	EnableMovement(inspectionState);

	if((inspectionState->recallDown) && (inspectionState->recallUp)){
		// if both buttons has been pressed together stop movement
		InspectionStopping(inspectionState);
	}
	else if(inspectionState->recallDown == true){
		infomessages.infoRecallDown.flag = true;
		MovingRecallDown(timer10msDiff, inspectionState);
	}
	else if(inspectionState->recallUp == true){
		infomessages.infoRecallUp.flag = true;
		MovingRecallUp(timer10msDiff, inspectionState);
	}
	else{
		InspectionStopImmediately(inspectionState);
	}
}



/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
NormalMode_t CarControllerStateDecision (uint8_t onlyInspectionProcess)
{
	NormalMode_t newState;

	if(psuStatus.packetField.errorStatus == PERMANENT_FAULT){
		newState = BlockedFault;
	}
	else if(psuStatus.packetField.errorStatus == FAULT){
		newState = Fault;
	}
	else if(psuStatus.packetField.inspectionControl == true){
		newState = Inspection;
	}
	else if(psuStatus.packetField.recallOperation == true){
		newState = Recall;
	}
	else if((pBasicParameter1->packetField.operationMode == WITH_INSPECTION_BOX) ||
			(pBasicParameter1->packetField.operationMode == WITHOUT_INSPECTION_BOX)){
		newState = Installation;
	}
	else if(!progInput.iBypassKey.value){
		newState = BypassSelected;
	}
	else if(onlyInspectionProcess == 1){
		newState = OnlyInspection;
	}
	else if((carCurrentFloor == 0) || (carPositioning != carPositionInitial)){
		newState = InitializeCarPosition;
	}
	else if(psuVariables.inspectFlag == true){
		newState = ReleaseInspection;
	}
	else{
		newState = UserCarManagement;
	}
	return newState;
}



/*------------------------------------------------------------------*-

	void Inialize_Car_Position(void)


-*------------------------------------------------------------------*/
void Inialize_Car_Position(uint16_t timer10msDiff)
{
	static uint8_t counterFlag817 = 0, enableIncrease = true;
	static uint32_t timerRunDelay = 0;

	// initialize door to close the door
	doorCommand.psuRequestDoorA = closeTheDoor;
	doorCommand.psuRequestDoorB = closeTheDoor;

	// Wait for door controller response
	switch(carPositioning){
		case carPositionInitial:
		{
			if((IS_DOORA_CLOSED_WITHOUT_TORQUE) && (IS_DOORB_CLOSED_WITHOUT_TORQUE) &&
					(IS_CARDOORA_SAFETY_CONT_CLOSED) && (IS_LANDING_DOORA_LOCKED) &&
					(IS_CARDOORB_SAFETY_CONT_CLOSED) && (IS_LANDING_DOORB_LOCKED) &&
					(IS_MANUEL_DOOR_CLOSED)){
				if(carCurrentFloor == 0){
					carPositioning = prepareToMoveUp;
				}
			}
			timerRunDelay = 0;
			break;
		}
		case prepareToMoveUp:
		{
			if(timerRunDelay > 800){
				// Before start running wait for 800 millisecond

				if(psuStatus.packetField.inspLimitSwitchBottom == true){
					// if the car is above the inspection sensor, start to find ground level
					// reset direction indication
					directionIndication.packetValue = 0;

					// reset speed register
					speedDemand = noSpeed;

					if(speedRespond == noSpeed){
						// jump
						timerRunDelay = 0;
						carPositioning = findGroundLevel;
					}
				}
				else if(psuStatus.packetField.inspLimitSwitchBottom == false){
					// if the car is already below the inspection sensor, go above inspection sensor
					// inform direction indication to go up
					directionIndication.packetField.movingUp = true;

					// prepare speed register for going up in nominal speed
					if(speedRespond == noSpeed){
						speedDemand = nominalSpeedUp;
					}
				}
			}
			else{
				// reset direction indication
				directionIndication.packetValue = 0;

				// reset speed register
				speedDemand = noSpeed;
				timerRunDelay += timer10msDiff;
			}
			break;
		}
		case findGroundLevel:
		{
			if(timerRunDelay > 800){
				if((psuStatus.packetField.inspLimitSwitchBottom == false)){
					if((directionIndication.packetField.movingDown == true) && (!IS_CAR_IN_DOORZONE)){
						if(enableIncrease == true){
							++counterFlag817;
							enableIncrease = false;
						}
					}
					else{
						enableIncrease = true;
					}

					if(counterFlag817 >= pShaftParameter->packetField.flagNumberBelow817){
						if((unProgInput.iML1.value) &&
								(unProgInput.iML2.value) &&
								(progInput.iSignal141.value) &&
								(progInput.iSignal142.value)){
						// Floor level has been detected
							if(speedRespond == slowSpeedDown){
								// reset direction indication
								directionIndication.packetValue = 0;

								// reset speed register
								speedDemand = noSpeed;

								// Ground floor has been detected
								doorCommand.psuRequestDoorA = noAction;
								doorCommand.psuRequestDoorB = noAction;

								carPositioning = floorLevel;
								timerRunDelay = 0;
							}
							else{
								// inform direction indication to go down
								directionIndication.packetField.movingDown = true;

								// decrease the speed
								speedDemand = slowSpeedDown;
							}
						}
						else{
							// bridge the contact
							psuControlWrite.packetValue = 0;
							psuControlWrite.packetField.bridgingFloorNumber = carCurrentFloor;
							psuControlWrite.packetField.bridgeWhilePreliminaryOp = true;
							OD_set_u32(OD_ENTRY_H63E0_PSU_Control, 1, psuControlWrite.packetValue, true);

							// inform direction indication to go down
							directionIndication.packetField.movingDown = true;

							// decrease the speed
							speedDemand = slowSpeedDown;
						}
					}
				}
				else{
					counterFlag817 = 0;
					enableIncrease = true;
					// if the car is above the inspection sensor, start to find ground level
					// inform direction indication to go down
					directionIndication.packetField.movingDown = true;

					// move down with nominal speed
					if(speedRespond == noSpeed){
						speedDemand = nominalSpeedDown;
					}
				}
			}
			else{
				timerRunDelay += timer10msDiff;
			}
			break;
		}
		case floorLevel:
		{

			if(speedRespond == noSpeed){
				if(timerRunDelay > 1000){
					carPositioning = carPositionInitial;
					timerRunDelay = 0;
				}
				else{
					timerRunDelay += timer10msDiff;
				}
			}
			break;
		}
	}
}




/*------------------------------------------------------------------*-

	void UpdateLiftParameter(void)
	In validation mode, this function will update 63E1PsuStatus for validation
OD_RAM.x63E1_PSUStatus[1]
-*------------------------------------------------------------------*/
/*
void UpdateLiftParameter(void)
{

	ODR_t odRet;
	uint32_t data32;
	odRet = OD_get_u32(OD_ENTRY_H63E1_PSUStatus, 2, &data32, true);
	if (odRet == ODR_OK) {
		if(pDoorParameter1->packetField.doorPreOpening == ENABLE){
			data32 |= CarInDoorZone;
		}
		else{
			data32 &= ~CarInDoorZone;
		}
		// Or if EN81-20 and EN81-1-A3 is selected. We will put this option to the parameters
		if(pBasicParameter2->packetField.relevellingFunction == ENABLE){
			data32 |= CarOnLevel;
		}
		else{
			data32 &= ~CarOnLevel;
		}
		data32 |= FinalLimitSwitchBottom;
		data32 |= FinalLimitSwitchTop;
		data32 |= InspectionLimitSwitchBottom;
		data32 |= InspectionLimitSwitchTop;
		data32 &= ~InspectionLimitSwitchBottomExt;
		data32 &= ~InspectionLimitSwitchTopExt;
		if(1){// if EN81-20 and EN81-1-A3 is selected. We will put this option to the parameters
			data32 |= UnintendeCarMovement;
		}
		else{
			data32 &= ~UnintendeCarMovement;
		}
		data32 |= OverSpeed;
		if((uint8_t) parameterBasic[driverType].value != DRIVER_VVVF){ // This parameter will be set when the paralel driver is selected // Maybe at hydraulic setting
			data32 |= DecelerationMonitoring;
		}
		else{
			data32 &= ~DecelerationMonitoring;
		}
		data32 |= RecallOperationAllowed;
		if(pBasicParameter1->packetField.operationModeNew == WITHOUT_INSPECTION_BOX){
			data32 |= InspectionOperationAllowed;
		}
		else{
			data32 &= ~InspectionOperationAllowed;
		}
		data32 |= DoorInterLockLoop;
		data32 &= ~ShelterTopInput;
		data32 &= ~ShelterBottomInput;
		data32 |= RelayContact1;
		data32 &= ~RelayContact2;
		data32 &= ~RelayContact3;
		data32 &= ~EmergencySafetyGearActive;
		data32 &= ~RelayTestRequest;
		data32 &= ~RelayTestActive;
		data32 |= CurrentOperationMode;
		data32 |= ErrorStatusBitField;
		data32 &= ~Reserved;
		odRet = OD_set_u32(OD_ENTRY_H63E1_PSUStatus, 2, data32, true);
	}
	return;
}*/

/*-----------------------------------------------------------------------------
 * GLOBAL FUNCTIONS - see descriptions in header file
 *----------------------------------------------------------------------------*/
/*------------------------------------------------------------------*-

	void ContactorSupervisor(uint16_t timer10msDiff)
	it is setting Main Contactor and Overspeed Governer Controler

-*------------------------------------------------------------------*/
void SpeedSupervisor(uint16_t timer10msDiff)
{
	static uint32_t timerNoSpeed = 0, timerMainContactor = 0;
	static uint8_t flagOSG = 0;
	int32_t noSpeedVelocity, slowSpeedUpVelocity, slowSpeedDownVelocity;
	int32_t inspectionSpeedUpVelocity, inspectionSpeedDownVelocity;
	int32_t intermediateSpeedUpVelocity, intermediateSpeedDownVelocity;
	int32_t nominalSpeedUpVelocity, nominalSpeedDownVelocity;
	int32_t relevelingSpeedUpVelocity, relevelingSpeedDownVelocity;

	if(OD_PERSIST_APP_AUTO.x2002_driverType == 1){
		noSpeedVelocity = NO_SPEED_E200;
		slowSpeedUpVelocity = SLOW_SPEED_UP_E200;
		slowSpeedDownVelocity = SLOW_SPEED_DOWN_E200;
		inspectionSpeedUpVelocity = INSPECTION_SPEED_UP_E200;
		inspectionSpeedDownVelocity = INSPECTION_SPEED_DOWN_E200;
		intermediateSpeedUpVelocity = INTERMEDIATE_SPEED_UP_E200;
		intermediateSpeedDownVelocity = INTERMEDIATE_SPEED_DOWN_E200;
		nominalSpeedUpVelocity = NOMINAL_SPEED_UP_E200;
		nominalSpeedDownVelocity = NOMINAL_SPEED_DOWN_E200;
		relevelingSpeedUpVelocity = RELEVELLING_SPEED_UP_E200;
		relevelingSpeedDownVelocity = RELEVELLING_SPEED_DOWN_E200;
	}
	else{
		noSpeedVelocity = 0;
		slowSpeedUpVelocity = 1;
		slowSpeedDownVelocity = -1;
		inspectionSpeedUpVelocity = 2;
		inspectionSpeedDownVelocity = -2;
		intermediateSpeedUpVelocity = 3;
		intermediateSpeedDownVelocity = -3;
		nominalSpeedUpVelocity = 4;
		nominalSpeedDownVelocity = -4;
		relevelingSpeedUpVelocity = 5;
		relevelingSpeedDownVelocity = -5;
	}

	switch(speedDemand){
		case noSpeed:
		{
			flagOSG = false;
			if((psuStatus.packetField.errorStatus == PERMANENT_FAULT) || (psuStatus.packetField.errorStatus == FAULT)){
				progOutput.oMainContactorOut.value = false;
				speedRespond = noSpeed;

				if(timerNoSpeed >= 2000){
					unProgOutput.oOsg.value = false;
				}
				else{
					timerNoSpeed += timer10msDiff;
				}
			}
			else{
				if(pBasicParameter1->packetField.driverType == DRIVER_VVVF_PARALLEL){
					if(pDriverParameter->packetField.contactorDrop == DROP_WITH_FEEDBACK){
						if(progInput.iDriverRun.value == false){
							speedRespond = noSpeed;
							progOutput.oMainContactorOut.value = false;
						}
					}
					else if(pDriverParameter->packetField.contactorDrop == DROP_TIME_DELAYED){
						if(timerMainContactor < (pDriverParameter->packetField.contactorDropByTime * 100)){
							timerMainContactor += timer10msDiff;
						}
						else{
							speedRespond = noSpeed;
							progOutput.oMainContactorOut.value = false;
						}
					}
				}
				else{
					progOutput.oMainContactorOut.value = false;
				}

				if(timerNoSpeed >= 2000){
					unProgOutput.oOsg.value = false;
				}
				else{
					timerNoSpeed += timer10msDiff;
				}
			}
			OD_set_i32(OD_ENTRY_H6430_targetVelocity, 0, noSpeedVelocity, true);
			OD_set_i32(ODShaft_ENTRY_H6430_targetVelocity, 0, noSpeedVelocity, true);
			break;
		}
		case slowSpeedUp:
		{
			timerMainContactor = 0;
			timerNoSpeed = 0;
			infomessages.infoHighNormalUp.flag = true;
			progOutput.oMainContactorOut.value = true;

			if(flagOSG == false){
				unProgOutput.oOsg.value = true;
				flagOSG = true;
			}
			else if(progOutput.oCarIsInDoorZone.value == true){
				unProgOutput.oOsg.value = false;
			}

			speedRespond = slowSpeedUp;
			OD_set_i32(OD_ENTRY_H6430_targetVelocity, 0, slowSpeedUpVelocity, true);
			OD_set_i32(ODShaft_ENTRY_H6430_targetVelocity, 0, slowSpeedUpVelocity, true);
			break;
		}
		case slowSpeedDown:
		{
			timerMainContactor = 0;
			timerNoSpeed = 0;
			infomessages.infoHighNormalDown.flag = true;
			progOutput.oMainContactorOut.value = true;
			if(flagOSG == false){
				flagOSG = true;
				unProgOutput.oOsg.value = true;
			}
			else if(progOutput.oCarIsInDoorZone.value == true){
				unProgOutput.oOsg.value = false;
			}

			speedRespond = slowSpeedDown;
			OD_set_i32(OD_ENTRY_H6430_targetVelocity, 0, slowSpeedDownVelocity, true);
			OD_set_i32(ODShaft_ENTRY_H6430_targetVelocity, 0, slowSpeedDownVelocity, true);
			break;
		}
		case inspectionSpeedUp:
		{
			timerMainContactor = 0;
			timerNoSpeed = 0;

			progOutput.oMainContactorOut.value = true;
			unProgOutput.oOsg.value = true;
			speedRespond = inspectionSpeedUp;
			OD_set_i32(OD_ENTRY_H6430_targetVelocity, 0, inspectionSpeedUpVelocity, true);
			OD_set_i32(ODShaft_ENTRY_H6430_targetVelocity, 0, inspectionSpeedUpVelocity, true);
			break;
		}
		case inspectionSpeedDown:
		{
			timerMainContactor = 0;
			timerNoSpeed = 0;

			progOutput.oMainContactorOut.value = true;
			unProgOutput.oOsg.value = true;

			speedRespond = inspectionSpeedDown;
			OD_set_i32(OD_ENTRY_H6430_targetVelocity, 0, inspectionSpeedDownVelocity, true);
			OD_set_i32(ODShaft_ENTRY_H6430_targetVelocity, 0, inspectionSpeedDownVelocity, true);
			break;
		}
		case intermediateSpeedUp:
		{
			timerMainContactor = 0;
			timerNoSpeed = 0;

			progOutput.oMainContactorOut.value = true;
			unProgOutput.oOsg.value = true;

			speedRespond = intermediateSpeedUp;
			OD_set_i32(OD_ENTRY_H6430_targetVelocity, 0, intermediateSpeedUpVelocity, true);
			OD_set_i32(ODShaft_ENTRY_H6430_targetVelocity, 0, intermediateSpeedUpVelocity, true);
			break;
		}
		case intermediateSpeedDown:
		{
			timerMainContactor = 0;
			timerNoSpeed = 0;

			progOutput.oMainContactorOut.value = true;
			unProgOutput.oOsg.value = true;
			speedRespond = intermediateSpeedDown;
			OD_set_i32(OD_ENTRY_H6430_targetVelocity, 0, intermediateSpeedDownVelocity, true);
			OD_set_i32(ODShaft_ENTRY_H6430_targetVelocity, 0, intermediateSpeedDownVelocity, true);
			break;
		}
		case nominalSpeedUp:
		{
			timerMainContactor = 0;
			timerNoSpeed = 0;
			infomessages.infoHighSpeedUp.flag = true;
			progOutput.oMainContactorOut.value = true;
			unProgOutput.oOsg.value = true;
			speedRespond = nominalSpeedUp;
			OD_set_i32(OD_ENTRY_H6430_targetVelocity, 0, nominalSpeedUpVelocity, true);
			OD_set_i32(ODShaft_ENTRY_H6430_targetVelocity, 0, nominalSpeedUpVelocity, true);
			break;
		}
		case nominalSpeedDown:
		{
			timerMainContactor = 0;
			timerNoSpeed = 0;
			infomessages.infoHighSpeedDown.flag = true;
			progOutput.oMainContactorOut.value = true;
			unProgOutput.oOsg.value = true;
			speedRespond = nominalSpeedDown;
			OD_set_i32(OD_ENTRY_H6430_targetVelocity, 0, nominalSpeedDownVelocity, true);
			OD_set_i32(ODShaft_ENTRY_H6430_targetVelocity, 0, nominalSpeedDownVelocity, true);
			break;
		}
		case relevellingSpeedUp:
		{
			timerMainContactor = 0;
			timerNoSpeed = 0;
			progOutput.oMainContactorOut.value = true;
			unProgOutput.oOsg.value = true;
			speedRespond = relevellingSpeedUp;
			OD_set_i32(OD_ENTRY_H6430_targetVelocity, 0, relevelingSpeedUpVelocity, true);
			OD_set_i32(ODShaft_ENTRY_H6430_targetVelocity, 0, relevelingSpeedUpVelocity, true);
			break;
		}
		case relevellingSpeedDown:
		{
			timerMainContactor = 0;
			timerNoSpeed = 0;

			progOutput.oMainContactorOut.value = true;
			unProgOutput.oOsg.value = true;
			speedRespond = relevellingSpeedDown;
			OD_set_i32(OD_ENTRY_H6430_targetVelocity, 0, relevelingSpeedDownVelocity, true);
			OD_set_i32(ODShaft_ENTRY_H6430_targetVelocity, 0, relevelingSpeedDownVelocity, true);
			break;
		}
	}
	SendDataToInverter();
}

/*------------------------------------------------------------------*-

	void FunctionalOutputsControl(void)
	It void ProcessConfiguration(uint16_t timer10msDiff)is setting programmable outputs according to the settings of programmable
	outputs	and functional outputs

-*------------------------------------------------------------------*/

void ProcessConfiguration(uint16_t timer10msDiff)
{
	if(saveDefault == true){
		SaveDefaultParameter();
		OD_set_u32(OD_ENTRY_H1011_restoreDefaultParameters, 3, 1, true);
		saveDefault = false;
	}
	else{
		SaveParameters();
	}
	OD_set_u32(OD_ENTRY_H63E2_PSUSafetyControl, validationMode, 1, true);
}




/*------------------------------------------------------------------*-

	void FunctionalOutputsControl(void)
	It void ProcessConfiguration(uint16_t timer10msDiff)is setting programmable outputs according to the settings of programmable
	outputs	and functional outputs

-*------------------------------------------------------------------*/

void PSUController(uint16_t timer10msDiff, Control *control)
{
	NormalMode_t carControllerStates = InitializeCarPosition;
	static EnableMovement_t psuControl;
	static uint8_t onlyInspectionProcess = 0, waitInspectionReset = 0;

#if	fairWithInspection
	uint8_t state[1];
#endif

	// initialize inspectionState
	psuControl.enableInspectionDown = 0;
	psuControl.enableInspectionUp = 0;
	psuControl.pitInspectionUp = progInput.iSignal505.value;
	psuControl.pitInspectionDown = progInput.iSignal504.value;
	psuControl.recallUp = progInput.iSignal503.value;
	psuControl.recallDown = progInput.iSignal502.value;
	psuControl.inspectionUp = unProgInput.iSignal501.value;
	psuControl.inspectionDown = unProgInput.iSignal500.value;

    infomessages.infoBlockedFault.flag = false;
    infomessages.infoFault.flag = false;
    infomessages.infoFindCarPosition.flag = false;
	infomessages.infoRelevelling.flag = false;
	infomessages.infoHighInspectionUp.flag = false;
	infomessages.infoHighInspectionDown.flag = false;
	infomessages.infoRecallUp.flag = false;
	infomessages.infoRecallDown.flag = false;
    infomessages.infoEvacuation.flag = false;
	infomessages.infoCarInspection.flag = false;
	infomessages.infoCarPitInspection.flag = false;
	infomessages.infoPitInspection.flag = false;
	infomessages.infoRecall.flag = false;
    infomessages.infoInstallation.flag = false;
	infomessages.infoBypassSelected.flag = false;
    infomessages.infoOnlyInspection.flag = false;
	infomessages.infoReleaseInspection.flag = false;
	infomessages.infoHighSpeedUp.flag = false;
	infomessages.infoHighSpeedDown.flag = false;
	infomessages.infoHighNormalUp.flag = false;
	infomessages.infoHighNormalDown.flag = false;
	infomessages.infoPassengerLoadUnload.flag = false;
	infomessages.infoWaitingCalls.flag = false;
	infomessages.infoS20_Off.flag = !unProgInput.iS20.value;

	progOutput.oOutOfOrderSignal.value = false;
	progOutput.oLiftInMaintenance.value = false;

	if((doorA_Variables.closeTrialNumber > 0) || (doorB_Variables.closeTrialNumber > 0)){
		infomessages.infoDoorsAreAwaited.flag = true;
	}
	else{
		infomessages.infoDoorsAreAwaited.flag = false;
	}

	Upgrade(control);
	// todo earthquake mode
	// This function is locking only inspection state, if pit inspection is used
	if(pOtherParameter->packetField.pitInspectionState == ENABLE){
		if(IS_PIT_INSPECTION_ACTIVE){
			onlyInspectionProcess = 1;

			if(progInput.iInspectionPitReset.value){
				waitInspectionReset = 1;
			}
		}
	}
	else{
		waitInspectionReset = 0;
	}

	carControllerStates = CarControllerStateDecision(onlyInspectionProcess);
	switch(carControllerStates){
		case InitializeCarPosition:
		{
			psuVariables.lightCommand = true;
			infomessages.infoFindCarPosition.flag = true;
			psuVariables.timerAfterInspection = 0;
			carManagement = carIdle;
			Inialize_Car_Position(timer10msDiff);
			break;
		}
		case UserCarManagement:
		{
			carPositioning = carPositionInitial;
			psuVariables.timerAfterInspection = 0;
			infomessages.infoWaitingCalls.flag = true;
			CarManagement(timer10msDiff);
#if	fairWithInspection
			state[0] = 'N';
			HAL_UART_Transmit(&huart2, state, sizeof(state), 1000);				////////////
#endif
			break;
		}
		case Inspection:
		{
			carManagement = carIdle;
			carPositioning = carPositionInitial;
			psuVariables.inspectFlag = true;
			psuVariables.timerAfterInspection = 0;
			psuVariables.lightCommand = true;
			psuVariables.arrivalIndication = false;
			progOutput.oLiftInMaintenance.value = true;
			InspectionOperation(timer10msDiff, &psuControl);
#if	fairWithInspection
			state[0] = 'I';
			HAL_UART_Transmit(&huart2, state, sizeof(state), 1000);				////////////
#endif
			break;
		}
		case Recall:
		{
			// Reset Car Call
			carManagement = carIdle;
			carPositioning = carPositionInitial;
			psuVariables.inspectFlag = true;
			psuVariables.timerAfterInspection = 0;
			psuVariables.lightCommand = true;
			psuVariables.arrivalIndication = false;
			infomessages.infoRecall.flag = true;
			progOutput.oLiftInMaintenance.value = true;
			RecalOperation(timer10msDiff, &psuControl);
#if	fairWithInspection
			state[0] = 'R';
			HAL_UART_Transmit(&huart2, state, sizeof(state), 1000);				////////////
#endif
			break;
		}
		case Fault:
		{
			// Reset Car Call
			carManagement = carIdle;
			carPositioning = carPositionInitial;
			psuVariables.lightCommand = true;
		    infomessages.infoFault.flag = true;
			// reset variables
			psuVariables.timerAfterInspection = 0;
			psuControl.timerDown = 0;
			psuControl.timerUp = 0;
			directionIndication.packetValue = 0;
			psuVariables.arrivalIndication = false;
			progOutput.oOutOfOrderSignal.value = true;
			// Reset Speed
			speedDemand = noSpeed;
#if	fairWithInspection
			state[0] = 'F';
			HAL_UART_Transmit(&huart2, state, sizeof(state), 1000);				////////////
#endif
			break;
		}
		case BlockedFault:
		{
			// Reset Car Call
			carManagement = carIdle;
			carPositioning = carPositionInitial;
		    infomessages.infoBlockedFault.flag = true;
			psuVariables.lightCommand = true;
			psuVariables.timerAfterInspection = 0;
			// reset variables
			psuControl.timerDown = 0;
			psuControl.timerUp = 0;
			psuVariables.arrivalIndication = false;
			directionIndication.packetValue = 0;
			progOutput.oOutOfOrderSignal.value = true;
			// Reset Speed
			speedDemand = noSpeed;
#if	fairWithInspection
			state[0] = 'B';
			HAL_UART_Transmit(&huart2, state, sizeof(state), 1000);				////////////
#endif
			break;
		}
		case Installation:
		{
			directionIndication.packetValue = 0;
			psuVariables.timerAfterInspection = 0;
			progOutput.oLiftInMaintenance.value = false;
			// Reset Car Call
			carManagement = carIdle;
			carPositioning = carPositionInitial;
		    infomessages.infoInstallation.flag = true;
			break;
		}
		case BypassSelected:
		{
			psuVariables.timerAfterInspection = 0;
			progOutput.oLiftInMaintenance.value = false;
			// Reset Car Call
			carManagement = carIdle;
			carPositioning = carPositionInitial;
			infomessages.infoBypassSelected.flag = true;
			break;
		}
		case Evacuation:
		{
			psuVariables.timerAfterInspection = 0;
			psuVariables.lightCommand = true;
			// Reset Car Call
			carManagement = carIdle;
			carPositioning = carPositionInitial;
			infomessages.infoEvacuation.flag = true;
			FireEvacuation();
			break;
		}
		case OnlyInspection:
		{
			psuVariables.timerAfterInspection = 0;
			progOutput.oLiftInMaintenance.value = true;
			// Reset Car Call
			carManagement = carIdle;
			carPositioning = carPositionInitial;
		    infomessages.infoOnlyInspection.flag = true;
		    if((waitInspectionReset == 0) && (progInput.iInspectionPitReset.value)){
				onlyInspectionProcess = 0;
			}
			else if(!progInput.iInspectionPitReset.value){
				waitInspectionReset = 0;
			}
			break;
		}
		case ReleaseInspection:
		{
			progOutput.oLiftInMaintenance.value = true;
			carPositioning = carPositionInitial;
			infomessages.infoReleaseInspection.flag = false;
			ReleaseInspectionManagement(timer10msDiff);
			break;
		}
	}
	return;
}

/*------------------------------------------------------------------*-

	void ProcessValidation(CO_t *co, CO_t *coShaft, uint16_t timer10msDiff)
	It void ProcessConfiguration(uint16_t timer10msDiff)is setting programmable outputs according to the settings of programmable
	outputs	and functional outputs

-*------------------------------------------------------------------*/

void ProcessValidation(CO_t *co, CO_t *coShaft, uint16_t timer10msDiff)
{
	static Validation validation = initValidate;

	switch(validation){
		case initValidate:
		{
			validation = validateCar;
			detectingDevices = initDetectingDevices;
			break;
		}
		case validateCar:
		{
			if(UpdateFoundDeviceListAccordingToProductcode(co, carSlaveList) == succes){
				validation = validateShaft;
				detectingDevices = initDetectingDevices;
			}
			break;
		}
		case validateShaft:
		{
			if(UpdateFoundDeviceListAccordingToProductcode(coShaft, shaftSlaveList) == succes){
				validation = releaseValidate;
			}
			break;
		}
		case releaseValidate:
		{
			ActivateDeviceTracking(co, coShaft);
			C200_PortSetup();
			OD_set_u32(OD_ENTRY_H63E2_PSUSafetyControl, normalMode, 1, true);
			validation = initValidate;
			break;
		}
	}
}
/*------------------------------------------------------------------*-

	void ProcessTeach(uint16_t timer10msDiff)
	It void ProcessConfiguration(uint16_t timer10msDiff)is setting programmable outputs according to the settings of programmable
	outputs	and functional outputs

-*------------------------------------------------------------------*/

void ProcessTeach(uint16_t timer10msDiff)
{
	OD_set_u32(OD_ENTRY_H63E2_PSUSafetyControl, teachMode, 1, true);
}
/*------------------------------------------------------------------*-

	PSUState PSUStateController(void)
	It void ProcessConfiguration(uint16_t timer10msDiff)is setting programmable outputs according to the settings of programmable
	outputs	and functional outputs

-*------------------------------------------------------------------*/
PSUState PSUStateController(void)
{
	ODR_t odRet;
	uint32_t data32;
	static PSUState psuState = precommisioningMode;

	if(OD_RAM.x63E2_PSUSafetyControl[precommisioningMode-1] == 1) {
		OD_set_u32(OD_ENTRY_H63E2_PSUSafetyControl, precommisioningMode, 0, true);
		psuState = precommisioningMode;
		odRet = OD_get_u32(OD_ENTRY_H63E1_PSUStatus, 1, &data32, true);
		if (odRet == ODR_OK) {
			data32 &= ~CleaningMode;
			odRet = OD_set_u32(OD_ENTRY_H63E1_PSUStatus, 1, data32, true);
		}
	}
	if(OD_RAM.x63E2_PSUSafetyControl[teachMode - 1] == 1) {
		OD_set_u32(OD_ENTRY_H63E2_PSUSafetyControl, teachMode, 0, true);
		psuState = teachMode;
		odRet = OD_get_u32(OD_ENTRY_H63E1_PSUStatus, 1, &data32, true);
		if (odRet == ODR_OK) {
			data32 &= ~CleaningMode;
			data32 |=  TeachingMode;
			odRet = OD_set_u32(OD_ENTRY_H63E1_PSUStatus, 1, data32, true);
		}
	}
	if(OD_RAM.x63E2_PSUSafetyControl[configurationMode - 1] == 1) {
		if((speedDemand == noSpeed) && (speedRespond == noSpeed)   /*carManagement == carIdle*/){
			OD_set_u32(OD_ENTRY_H63E2_PSUSafetyControl, configurationMode, 0, true);
			psuState = configurationMode;
			odRet = OD_get_u32(OD_ENTRY_H63E1_PSUStatus, 1, &data32, true);
			if (odRet == ODR_OK) {
				data32 &= ~CleaningMode;
				data32 |=  ConfigurationMode;
				odRet = OD_set_u32(OD_ENTRY_H63E1_PSUStatus, 1, data32, true);
			}
		}
	}
	if(OD_RAM.x63E2_PSUSafetyControl[validationMode - 1] == 1) {
		OD_set_u32(OD_ENTRY_H63E2_PSUSafetyControl, validationMode, 0, true);
		psuState = validationMode;
		odRet = OD_get_u32(OD_ENTRY_H63E1_PSUStatus, 1, &data32, true);
		if (odRet == ODR_OK) {
			data32 &= ~CleaningMode;
			data32 |=  ValidationMode;
			odRet = OD_set_u32(OD_ENTRY_H63E1_PSUStatus, 1, data32, true);
		}
	}
	if(OD_RAM.x63E2_PSUSafetyControl[normalMode - 1] == 1) {
		OD_set_u32(OD_ENTRY_H63E2_PSUSafetyControl, normalMode, 0, true);
		psuState = normalMode;
		odRet = OD_get_u32(OD_ENTRY_H63E1_PSUStatus, 1, &data32, true);
		if (odRet == ODR_OK) {
			data32 &= ~CleaningMode;
			data32 |=  NormalMode;
			odRet = OD_set_u32(OD_ENTRY_H63E1_PSUStatus, 1, data32, true);
		}
	}
	return psuState;
}

/*------------------------------------------------------------------*-

	void Precommisioning(CO_t *co, CO_t *coShaft, Control *control)
	It void ProcessConfiguration(uint16_t timer10msDiff)is setting programmable outputs according to the settings of programmable
	outputs	and functional outputs

-*------------------------------------------------------------------*/
void Precommisioning(CO_t *co, CO_t *coShaft, uint16_t CO_timer_10ms, Control *control)
{
	if(InitNetwork(co, coShaft, CO_timer_10ms, control) == true){
		OD_set_u32(OD_ENTRY_H63E2_PSUSafetyControl, validationMode, 1, true);
	}
}


/*------------------------------------------------------------------------------------------*-
  -------------------------------------------END OF FILE -----------------------------------
-*------------------------------------------------------------------------------------------*/


