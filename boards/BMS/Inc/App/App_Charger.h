#pragma once

#include <stdbool.h>

struct Charger;

// This library is modelled after the BRUSA NLG513 charger. It has the concept
// of a master switch (Pin 3: PON). In short, if the BRUSA NLG513 s powered on
// but the master switch is disabled, it can never start the charging sequence.
// For the charging sequence to start, the BRUSA NLG513 must be powered on and
// its master switch enabled.
//
// Read more: BMS-9, BMS-10, BMS-34, and BRUSA NLG513 datasheet

/**
 * Allocate and initialize a charger
 * @param enable_charger A function that can be called to enable the charger
 * @param disable_charger A function that can be called to disable the charger
 * @param is_charger_connected A function that returns whether the charger is
 *                             is connected
 * @return The created charger, whose ownership is given to the caller
 */
struct Charger *App_Charger_Create(
    void (*enable_charger)(void),
    void (*disable_charger)(void),
    bool (*is_charger_connected)(void));

/**
 * Deallocate the memory used by the given charger
 * @param charger The charger to deallocate
 */
void App_Charger_Destroy(struct Charger *charger);

/**
 * Enable the given charger
 * @param charger The charger to enable
 */
void App_Charger_Enable(struct Charger *charger);

/**
 * Disable the given charger
 * @param charger The charger to disable
 */
void App_Charger_Disable(struct Charger *charger);

/**
 * Check if the given charger is connected
 * @param charger The charger to check
 * @return true if the charger is connected, else false
 */
bool App_Charger_IsConnected(struct Charger *charger);
