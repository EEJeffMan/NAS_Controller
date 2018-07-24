/*
 * task_scheduler.h
 *
 *  Created on: Jul 21, 2018
 *      Author: Jeff
 */

#ifndef TASK_SCHEDULER_H_
#define TASK_SCHEDULER_H_

extern int VTimer0[4];                   // Virtual Timers slaved off CPU Timer 0
extern int VTimer1[4];                   // Virtual Timers slaved off CPU Timer 1


// -------------------------------- FRAMEWORK --------------------------------------
// State Machine function prototypes
//----------------------------------------------------------------------------------
// Alpha states
void A0(void);  //state A0
void B0(void);  //state B0

// A branch states
void A1(void);  //state A1
void A2(void);  //state A2
void A3(void);  //state A3
void A4(void);  //state A4

// B branch states
void B1(void);  //state B1
void B2(void);  //state B2
void B3(void);  //state B3

// Variable declarations
extern void (*Alpha_State_Ptr)(void);  // Base States pointer
extern void (*A_Task_Ptr)(void);       // State pointer A branch
extern void (*B_Task_Ptr)(void);       // State pointer B branch



#endif /* TASK_SCHEDULER_H_ */
