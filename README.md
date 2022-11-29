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

Otherwise variable of only inspection process (onlyInspectionProcess) will be 1.

>onlyInspectionProcess is one of the states of the car controller states (carControllerStates).

## Car Controller State Switch Case

Car controller states (carControllerStates) updated before finding current state of the PSU.

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
- Car lights goes on.
- Display shows "Finding car position".
- Timer after inspection becomes 0.
- Find first floor.
- Car idling sequence (carIdle) will be executed on next car management function.
#### User Car Management
If car controller states (carControllerStates) is UserCarManagement:
- Reset variables of finding floors.
- Timer after inspection becomes 0.
- Display shows "Waiting calls"
- Execute car management function (CarManagement)
#### Inspection
If car controller states (carControllerStates) is Inspection:
- Reset variables of finding floors.
- Inspection flag will be true.
- Timer after inspection becomes 0.
- Car lights goes on.
- Variable for screen to show "Lift in Maintenance" true.
- execute inspection function (InspectionOperation).
#### Recall
If car controller states (carControllerStates) is Recall:
- Car idling sequence (carIdle) will be executed on next car management function.
- Reset variables of finding floors.
- Inspection flag will be true.
- Timer after inspection becomes 0.
- Car lights goes on.
- Variable for screen to show "Lift in Maintenance" true.
- Execute recall operation function (RecalOperation)
#### Fault
If car controller states (carControllerStates) is Fault:
- Car idling sequence (carIdle) will be executed on next car management function.
- Reset variables of finding floors.
- Car lights goes on.
- Display shows "System Fault".
- Timer after inspection becomes 0.
- No direction selected.
- Variable for screen to show "Out of Order" true.
- Set car speed to no speed.
#### Blocked Fault
If car controller states (carControllerStates) is BlockedFault:
- Car idling sequence (carIdle) will be executed on next car management function.
- Reset variables of finding floors.
- Display shows "System Blocked Fault".
- Car lights goes on.
- Timer after inspection becomes 0.
- No direction selected.
- Variable for screen to show "Out of Order" true.
- Set car speed to no speed.
#### Installation
If car controller states (carControllerStates) is Installation:
- No direction selected.
- Timer after inspection becomes 0.
- Variable for screen to show "Lift in Maintenance" true.
- Car idling sequence (carIdle) will be executed on next car management function.
- Reset variables of finding floors.
#### Bypass Selected
#### Evactuation
#### Only Inspection
#### Release Inspection

# Abbreviations

 ##### **DISABLE** 
 > means the input is 0
 ##### **ENABLE**
 > means the input is 1
