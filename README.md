# Contents

 1. [Initialization]
 2. [Inspection Pit Reset]
 3. [Car Controller State Switch Case]
 
## Initialization
This part includes initiation process of both inspection unprogrammable inputs, info messages flags.
- Inspection unprogrammable inputs,
- Info messages flags,

## Inspection Pit Reset
Pit inspection menu configuration (pitInspectionState) or pit inspection input (IS_PIT_INSPECTION_ACTIVE) value should always be [DISABLE](#disable).

Otherwise variable of only inspection process (onlyInspectionProcess) will be 1.

>onlyInspectionProcess is one of the states of the car controller states (carControllerStates).

# Abbreviations

 ##### **DISABLE** 
 > means the input is 0
 ##### **ENABLE**
 > means the input is 1
