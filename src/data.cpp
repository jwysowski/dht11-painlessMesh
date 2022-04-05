#include "data.hpp"

const char *ssid = "esp_mesh";
const char *password = "123456789";

const char *temp_msg = "Temperature is: ";
const char *hum_msg = "Humidity is: ";

float normal_temp[] = {21.00, 21.10, 21.20, 21.30, 21.40, 21.50, 21.60, 21.70, 21.80, 21.90,
						22.00, 22.10, 22.20, 22.30, 22.40, 22.50, 22.60, 22.70, 22.80, 22.90,
						23.00};

float normal_hum[] = {41.00, 41.10, 41.20, 41.30, 41.40, 41.50, 41.60, 41.70, 41.80, 41.90,
						42.00, 42.10, 42.20, 42.30, 42.40, 42.50, 42.60, 42.70, 42.80, 42.90,
						43.00};       

float get_temp(int index) {
    return normal_temp[index];
}

float get_hum(int index) {
    return normal_hum[index];
}