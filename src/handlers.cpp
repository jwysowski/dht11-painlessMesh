#include "data.hpp"
#include "handlers.hpp"

#define ASCII_OFFSET    48
#define HANDLERS_OFFSET 2

extern float temp_target;
extern float hum_target;
extern float current_temp;
extern float current_hum;

extern char temp_mode;
extern char hum_mode;

int temp_values_index = 0;
int hum_values_index = 0;

void (*mesh_receive_handlers[6])(char type, float target) = {mesh_receive_temp_handler, mesh_receive_temp_handler, mesh_receive_temp_handler,
                                                                mesh_receive_hum_handler, mesh_receive_hum_handler, mesh_receive_hum_handler};

void (*measurement_handlers[6])() = {temp_norm_handler, temp_rising_handler, temp_falling_handler,
                                        hum_norm_handler, hum_rising_handler, hum_falling_handler};

int get_handler_index(char number) {
    return number - ASCII_OFFSET - HANDLERS_OFFSET;
}

void mesh_receive_temp_handler(char type, float target) {
    temp_mode = type;
    temp_target = target;
}

void mesh_receive_hum_handler(char type, float target) {
    hum_mode = type;
    hum_target = target;
}

void temp_norm_handler() {
    temp_values_index = temp_values_index + 1 == VALUES_SIZE ? 0 : temp_values_index + 1;
    current_temp = get_temp(temp_values_index);
}

void temp_rising_handler() {
    current_temp = current_temp < temp_target ? current_temp + 0.1 : current_temp;
}

void temp_falling_handler() {
    current_temp = current_temp > temp_target ? current_temp - 0.1 : current_temp;
}

void hum_norm_handler() {
    hum_values_index = hum_values_index + 1 == VALUES_SIZE ? 0 : hum_values_index + 1;
    current_hum = get_hum(hum_values_index);
}

void hum_rising_handler() {
    current_hum = current_hum < hum_target ? current_hum + 0.1 : current_hum;
}

void hum_falling_handler() {
    current_hum = current_hum > hum_target ? current_hum - 0.1 : current_hum;
}
