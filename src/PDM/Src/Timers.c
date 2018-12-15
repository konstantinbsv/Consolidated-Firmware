/******************************************************************************
 * Includes
 *****************************************************************************/
#include "Timers.h"

/******************************************************************************
 * Module Preprocessor Constants
 *****************************************************************************/

/******************************************************************************
 * Module Preprocessor Macros
 *****************************************************************************/

/******************************************************************************
 * Module Typedefs
 *****************************************************************************/

/******************************************************************************
 * Module Variable Definitions
 *****************************************************************************/

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

/******************************************************************************
 * Private Function Definitions
 *****************************************************************************/

/******************************************************************************
 * Function Definitions
 *****************************************************************************/
void Timers_Init(void)
{
    // Start timer 2
    HAL_TIM_Base_Start_IT(&htim2);

    // Start timer 17
    HAL_TIM_Base_Start_IT(&htim17);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim17)
    {
        FaultHandling_RetryEFuse(e_fuse_fault_states);
    }
}
