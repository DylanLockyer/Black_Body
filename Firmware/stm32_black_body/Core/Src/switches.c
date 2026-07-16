#include "switches.h"
#include "main.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_gpio.h"


// Change output current source.
// Works by closing all switches, then setting the current source resistors
// then connecting the correct current source to the output
bool current_source(current current_level){
    // Open all switches
    HAL_GPIO_WritePin(i_source_sel1_GPIO_Port, i_source_sel1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(i_source_sel3_GPIO_Port, i_source_sel3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(i_source_sel2_GPIO_Port, i_source_sel2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(i_source_sel4_GPIO_Port, i_source_sel4_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sel_10na_neg_GPIO_Port, sel_10na_neg_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sel_10na_pos_GPIO_Port, sel_10na_pos_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sel_100na_neg_GPIO_Port, sel_100na_neg_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sel_100na_pos_GPIO_Port, sel_100na_pos_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sel_v_1ua_GPIO_Port, sel_v_1ua_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sel_t_1ua_GPIO_Port, sel_t_1ua_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sel_v_10ua_GPIO_Port, sel_v_10ua_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sel_t_10ua_GPIO_Port, sel_t_10ua_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sel_v_100ua_GPIO_Port, sel_v_100ua_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sel_t_100ua_GPIO_Port, sel_t_100ua_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sel_v_1ma_GPIO_Port, sel_v_1ma_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sel_t_1ma_GPIO_Port, sel_t_1ma_Pin, GPIO_PIN_RESET);
    
    HAL_Delay(300); // Wait to give switches time to open

    // if current_level = block then close all switches
    if (current_level == 6) return true;

    // Select which current is needed
    switch(current_level){
        case 0:
            //10na current source
            HAL_GPIO_WritePin(sel_10na_neg_GPIO_Port, sel_10na_neg_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(sel_10na_pos_GPIO_Port, sel_10na_pos_Pin, GPIO_PIN_SET);
            break;
        case 1:
            //100na current source
            HAL_GPIO_WritePin(sel_100na_neg_GPIO_Port, sel_100na_neg_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(sel_100na_pos_GPIO_Port, sel_100na_pos_Pin, GPIO_PIN_SET);
            break;
        case 2:
            //1ua current source
            HAL_GPIO_WritePin(sel_v_1ua_GPIO_Port, sel_v_1ua_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(sel_t_1ua_GPIO_Port, sel_t_1ua_Pin, GPIO_PIN_SET);
            break;
        case 3:
            //10ua current source
            HAL_GPIO_WritePin(sel_v_10ua_GPIO_Port, sel_v_10ua_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(sel_t_10ua_GPIO_Port, sel_t_10ua_Pin, GPIO_PIN_SET);
            break;
        case 4:
            //100ua current source
            HAL_GPIO_WritePin(sel_v_100ua_GPIO_Port, sel_v_100ua_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(sel_t_100ua_GPIO_Port, sel_t_100ua_Pin, GPIO_PIN_SET);
            break;
        case 5:
            //1ma current source
            HAL_GPIO_WritePin(sel_v_1ma_GPIO_Port, sel_v_1ma_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(sel_t_1ma_GPIO_Port, sel_t_1ma_Pin, GPIO_PIN_SET);
            break;
        default:
            //do nothing
            return false;
            break;
    }

    HAL_Delay(300); // Wait to give switches time to close

    // Connect correct current source to coresponding output
    if (current_level <= 1){
        // Use na_source
        // Commented out as is connects unused output through resistor
        //HAL_GPIO_WritePin(i_sense_sel1_GPIO_Port, i_sense_sel1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(i_source_sel3_GPIO_Port, i_source_sel3_Pin, GPIO_PIN_SET);
    }
    else {
        // Use ma_source
        HAL_GPIO_WritePin(i_source_sel2_GPIO_Port, i_source_sel2_Pin, GPIO_PIN_SET);
        // Commented out as is connects unused output through resistor
        //HAL_GPIO_WritePin(i_sense_sel4_GPIO_Port, i_sense_sel4_Pin, GPIO_PIN_SET);
    }

    HAL_Delay(300); // Wait to give switches time to open

    return true;
}

// Changes which resistor is used for current measurement
bool current_measurement_resistor(cur_resistor shunt_resistance){
    // Open all switches
    HAL_GPIO_WritePin(i_sense_sel1_GPIO_Port, i_sense_sel1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(i_sense_sel2_GPIO_Port, i_sense_sel2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(i_sense_sel3_GPIO_Port, i_sense_sel3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(i_sense_sel4_GPIO_Port, i_sense_sel4_Pin, GPIO_PIN_RESET);

    HAL_Delay(300); // Delay to give switches time

    switch(shunt_resistance){
        case 0:
            // Resistance of 0
            HAL_GPIO_WritePin(i_sense_sel1_GPIO_Port, i_sense_sel1_Pin, GPIO_PIN_SET);
            break;
        case 1:
            // Resistance of 5
            HAL_GPIO_WritePin(i_sense_sel2_GPIO_Port, i_sense_sel2_Pin, GPIO_PIN_SET);
            break;
        case 2:
            // Resistance of 500
            HAL_GPIO_WritePin(i_sense_sel3_GPIO_Port, i_sense_sel3_Pin, GPIO_PIN_SET);
            break;
        case 3:
            // Resistance of 33.2k
            HAL_GPIO_WritePin(i_sense_sel4_GPIO_Port, i_sense_sel4_Pin, GPIO_PIN_SET);
            break;
        case 4:
            //Turn off all switches (they're already all off)
            break;
        default:
            //do nothing
            return false;
            break;
    }

    HAL_Delay(300); // Delay to give switches time
    return true;
}

// Sets direction for current
// Left is backwards (flows from I- to I+)
bool current_direction(cur_direction direction){

    // Turn all switches off
    HAL_GPIO_WritePin(hbridge_sel1_GPIO_Port, hbridge_sel1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(hbridge_sel2_GPIO_Port, hbridge_sel2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(hbridge_sel3_GPIO_Port, hbridge_sel3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(hbridge_sel4_GPIO_Port, hbridge_sel4_Pin, GPIO_PIN_RESET);

    HAL_Delay(10); // Give switches time to open
    switch(direction){
        case 0:
            //Current flows left
            HAL_GPIO_WritePin(hbridge_sel2_GPIO_Port, hbridge_sel2_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(hbridge_sel4_GPIO_Port, hbridge_sel4_Pin, GPIO_PIN_SET);
            break;
        case 1:
            //Current flows right
            HAL_GPIO_WritePin(hbridge_sel1_GPIO_Port, hbridge_sel1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(hbridge_sel3_GPIO_Port, hbridge_sel3_Pin, GPIO_PIN_SET);
            break;
        case 2:
            // Turn off all switches (already all open)
            break;
        default:
            //do nothing
            return false;
            break;
    }
    HAL_Delay(10); // Wait for switches to close
    return true;
}