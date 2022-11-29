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

Car controller states updated before finding current state of the PSU.

PSU modes include,
 - [InitializeCarPosition](#initialize-car-position)
 - [UserCarManagment](#user-car-management)
 - [Inspection](#inspection)
 - [Recall](#recall)
 - [Fault](#fault)
 - [BlockedFault](#blocked-fault)
 - [Installation](#installation)
 - [BypassSelected](#bypass-selected)
 - [Evacuation](#evactuation)
 - [OnlyInspection](#only-inspection)
 - [ReleaseInspection](#release-inspection)

#### Initialize Car Position
#### User Car Management
#### Inspection
#### Recall
#### Fault
#### Installation
#### Blocked Fault
#### Bypass Selected
#### Evactuation
#### Only Inspection
#### Release Inspection

# Abbreviations

 ##### **DISABLE** 
 > means the input is 0
 ##### **ENABLE**
 > means the input is 1
