#include <Arduino.h>
#include <painlessMesh.h>

#include "data.hpp"
#include "handlers.hpp"

void build_data_frame(data_frame &frame, uint32 id, bool is_temp, float val);
void get_message(char *msg, data_frame &frame);
uint16_t checksum(data_frame &frame);
void decode_msg(const char *msg, data_frame &frame);
bool validate(data_frame &frame);
void received_callback(const uint32_t &from, const String &msg);

painlessMesh mesh;

volatile int temp_read = 0;

float temp_target = 0.0;
float hum_target = 0.0;
float current_temp = 21.0;
float current_hum = 41.0;
char temp_mode = TEMPERATURE_NORM_TYPE;
char hum_mode = HUMIDITY_NORM_TYPE;
bool respond = false;

extern void (*mesh_receive_handlers[6])(char type, float target);
extern void (*measurement_handlers[6])();

void setup() {
	// Serial.begin(BAUDRATE);

	//mesh
	mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);   // all types on

	mesh.init(ssid, password, PORT);
	mesh.setContainsRoot(true);
	mesh.onReceive(&received_callback);
}

void loop() {
	mesh.update();

	if (respond) {
		respond = false;
		char message[MESSAGE_SIZE] = {0};
		data_frame frame;
		build_data_frame(frame, mesh.getNodeId(), TEMPERATURE, 10);
		get_message(message, frame);
		mesh.sendBroadcast(String(message));
	}
}

void get_message(char *msg, data_frame &frame) {
	msg[0] = START_SIGN;
	msg[1] = frame.data_type;
	msg[2] = '\0';
	
	if (frame.data_type == TEMPERATURE_TYPE)
		strcat(msg, frame.measurement.temperature);
	else
		strcat(msg, frame.measurement.humidity);

	strcat(msg, frame.node_id);
	
	sprintf(frame.checksum, "%x", checksum(frame));
	strcat(msg, frame.checksum);
	msg[MESSAGE_SIZE - 2] = END_SIGN;
	msg[MESSAGE_SIZE - 1] = '\0';
}

uint16_t checksum(data_frame &frame) {
	uint16_t checksum = 0;

	checksum += START_SIGN;

	checksum += frame.data_type;
	if (frame.data_type == TEMPERATURE_TYPE)
		for (int i = 0; i < DATA_SIZE; i++)
			checksum += frame.measurement.temperature[i];

	if (frame.data_type == HUMIDITY_TYPE)
		for (int i = 0; i < DATA_SIZE; i++)
			checksum += frame.measurement.humidity[i];

	else
		for (int i = 0; i < DATA_SIZE; i++)
			checksum += frame.measurement.target[i];

	for (int i = 0; i < NODE_ID_SIZE; i++)
		checksum += frame.node_id[i];

	return checksum % CHECKSUM_MOD;
}

void build_data_frame(data_frame &frame, uint32 id, bool is_temp, float val) {
	if (is_temp) {
		frame.data_type = TEMPERATURE_TYPE;
		sprintf(frame.measurement.temperature, "%.2f", val);
	} else {
		frame.data_type = HUMIDITY_TYPE;
		sprintf(frame.measurement.humidity, "%.2f", val);
	}

	sprintf(frame.node_id, "%u", id);
}

void decode_msg(const char *msg, data_frame &frame) {
	int start = 0;
	while (start < MESSAGE_SIZE) {
		if (msg[start] == START_SIGN)
			break;

		start++;
	}

	frame.data_type = msg[start + 1];
	if (frame.data_type == TEMPERATURE_TYPE) {
		memcpy(frame.measurement.temperature, msg + start + 2, DATA_SIZE);
		frame.measurement.temperature[DATA_SIZE] = '\0';
	}
	
	if (frame.data_type == HUMIDITY_TYPE) {
		memcpy(frame.measurement.humidity, msg + start + 2, DATA_SIZE);
		frame.measurement.humidity[DATA_SIZE] = '\0';
	}

	else {
		memcpy(frame.measurement.target, msg + start + 2, DATA_SIZE);
		frame.measurement.target[DATA_SIZE] = '\0';
	}

	memcpy(frame.node_id, msg + start + 2 + DATA_SIZE, NODE_ID_SIZE);
	frame.node_id[NODE_ID_SIZE] = '\0';
	
	memcpy(frame.checksum, msg + start + 2 + DATA_SIZE + NODE_ID_SIZE, CHECKSUM_SIZE);
	frame.checksum[CHECKSUM_SIZE] = '\0';
}

bool validate(data_frame &frame) {
	uint16_t frame_checksum = strtoul(frame.checksum, nullptr, 16);
	return frame_checksum == checksum(frame);
}

void received_callback(const uint32_t &from, const String &msg) {
	data_frame frame;
	const char *message = msg.c_str();

	decode_msg(message, frame);
	if (!validate(frame))
		return;

	int handler_index = get_handler_index(frame.data_type);
	if (handler_index < 0)
		return;
		
	char *end_ptr = nullptr;
	float target = strtof(frame.measurement.target, &end_ptr);
	mesh_receive_handlers[handler_index](frame.data_type, target);
}
