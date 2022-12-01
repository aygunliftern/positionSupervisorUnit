# Contents

 1. [Initialization](#initialization)
 2. [Inspection Pit Reset](#inspection-pit-reset)
 3. [Car Controller State Switch Case](#car-controller-state-switch-case)
 
## Initialization
This part includes initiation process of both inspection unprogrammable inputs, info messages flags.
>Inspection unprogrammable inputs indicates inpection up and down variables.
>Info messages flags indicates messages to be shown on LCD.

## Inspection Pit Reset
Pit inspection menu configuration (pitInspectionState) or pit inspection input (IS_PIT_INSPECTION_ACTIVE) value should always be [DISABLE](#disable).

Otherwise variable of [onlyInspectionProcess](#onlyinspectionprocess) will be 1.

## Car Controller State Switch Case

Car controller states (carControllerStates) updated before switch cases of PSU.

PSU modes include,
1.  [InitializeCarPosition](#initialize-car-position)
2.  [UserCarManagment](#user-car-management)
3.  [Inspection](#inspection)
4.  [Recall](#recall)
5.  [Fault](#fault)
6.  [BlockedFault](#blocked-fault)
7.  [Installation](#installation)
8.  [BypassSelected](#bypass-selected)
9.  [Evacuation](#evactuation)
10.  [OnlyInspection](#only-inspection)
11.  [ReleaseInspection](#release-inspection)

#### Initialize Car Position
If car controller states (carControllerStates) is InitializeCarPosition:
- Car lights set to [TRUE](#true).
- Display shows "Finding car position".
- Timer after inspection becomes 0.
- Find first floor.
- [carManagement](#carmanagement) set to carIdle. 
#### User Car Management
If car controller states (carControllerStates) is UserCarManagement:
- Reset variables of finding floors.
- Timer after inspection becomes 0.
- Display shows "Waiting calls"
- Execute [CarManagement](#carmanagement-1) function
#### Inspection
If car controller states (carControllerStates) is Inspection:
- Reset variables of finding floors.
- Inspection flag set to [TRUE](#true).
- Timer after inspection becomes 0.
- Car lights set to [TRUE](#true).
- Programmable output of Lift in Maintenance set to [TRUE](#true).
- execute [InspectionOperation](#inspectionoperation) function.
#### Recall
If car controller states (carControllerStates) is Recall:
- [carManagement](#carmanagement) set to carIdle.
- Reset variables of finding floors.
- Inspection flag set to [TRUE](#true).
- Timer after inspection becomes 0.
- Car lights set to [TRUE](#true).
- Display shows "Recall"
- Programmable output of Lift in Maintenance set to [TRUE](#true).
- Execute [RecalOperation](#recaloperation) function.
#### Fault
If car controller states (carControllerStates) is Fault:
- [carManagement](#carmanagement) set to carIdle.
- Reset variables of finding floors.
- Car lights set to [TRUE](#true).
- Display shows "System Fault".
- Timer after inspection becomes 0.
- No direction selected.
- Programmable output of Out of Order set to [TRUE](#true).
- Set car speed to no speed.
#### Blocked Fault
If car controller states (carControllerStates) is BlockedFault:
- [carManagement](#carmanagement) set to carIdle.
- Reset variables of finding floors.
- Display shows "System Blocked Fault".
- Car lights set to [TRUE](#true).
- Timer after inspection becomes 0.
- No direction selected.
- Programmable output of Out of Order set to [TRUE](#true).
- Set car speed to no speed.
#### Installation
If car controller states (carControllerStates) is Installation:
- No direction selected.
- Timer after inspection becomes 0.
- Programmable output of Lift in Maintenance set to [TRUE](#true).
- [carManagement](#carmanagement) set to carIdle.
- Reset variables of finding floors.
- Display shows "Installation"
#### Bypass Selected
If car controller states (carControllerStates) is BypassSelected:
- Timer after inspection becomes 0.
- Programmable output of Lift in Maintenance set to [FALSE](#false).
- [carManagement](#carmanagement) set to carIdle.
- Reset variables of finding floors.
- Display shows "Bypass"
#### Evactuation
If car controller states (carControllerStates) is Evacation:
- Timer after inspection becomes 0.
- Car lights set to [TRUE](#true).
- [carManagement](#carmanagement) set to carIdle.
- Reset variables of finding floors.
- Display shows "Evacuation"
#### Only Inspection
If car controller states (carControllerStates) is OnlyInspection:
- Timer after inspection becomes 0.
- Programmable output of Lift in Maintenance set to [TRUE](#true).
- [carManagement](#carmanagement) set to carIdle.
- Reset variables of finding floors.
- Display shows "OnlyInspection"
- If Inspection Pit Reset is equal to 1, onlyInspectionProcess is equal to 0. Else if, Inspection Pit Reset is not equal to 1, waitInspectionReset is equal to 0;
#### Release Inspection
- Programmable output of Lift in Maintenance set to [TRUE](#true).
- Reset variables of finding floors.
- Configure display not to show "Relesease Inspection".
- Execute ReleaseInspectionManagement function.

# Expressions

 ##### **onlyInspectionProcess**
 > onlyInspectionProcess is one of the states of the car controller states (carControllerStates)
 ##### **carControllerStates**
 > updated by [CarControllerStateDecision](#carcontrollerstatedecision) function.
 ##### **CarControllerStateDecision**
 > decides the current mode of the PSU.
 ##### **carManagement**
 > indicates car current state.
 ##### **CarManagement**
 > is a function decide which function (carIdle,releveling,requestCloseDoor,carStart,cruise,carStop) should be executed. It takes [carManagement](#carmanagement) as input. 
 ##### **psuControl**
 > indicates movement of inspection.
 ##### **InspectionOperation**
 > is a function decide which inspection mode is active. It takes [psuControl](#psucontrol) address as input.
 ##### **RecalOperation**
 > is a function, which takes [psuControl](#psucontrol) address as input.

# Abbreviations

 ##### **DISABLE** 
 > means the input is 0
 ##### **ENABLE**
 > means the input is 1
 ##### **FALSE** 
 > means the input is 0
 ##### **TRUE**
 > means the input is 1
