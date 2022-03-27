#include <Arduino.h>
#include <painlessMesh.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Ticker.h>

#define BAUDRATE        9600

//DHT
#define DHTPIN          	2
#define DHTTYPE         	DHT11
#define OVERFLOW_LIMIT  	0x7FFFFF

//mesh
#define PORT            5555

const char *ssid = "esp_mesh";
const char *password = "123456789";

const char *temp_msg = "Temperature is: ";
const char *hum_msg = "Humidity is: ";

#define	START_SIGN		':'
#define	END_SIGN		'\n'
#define	NODE_ID_SIZE	10
#define	DATA_SIZE		5
#define	CHECKSUM_SIZE	2
#define	MESSAGE_SIZE	21
#define ZERO			48

#define	TEMPERATURE_TYPE	'0'
#define	HUMIDITY_TYPE		'1'
#define	CHECKSUM_MOD		0x100

#define TEMPERATURE		true
#define HUMIDITY		false

struct data_frame {
	char data_type;
	union data {
		char temperature[DATA_SIZE + 1];
		char humidity[DATA_SIZE + 1];
	} measurement;
	char node_id[NODE_ID_SIZE + 1];
	char checksum[CHECKSUM_SIZE + 1];
};

void build_data_frame(data_frame &frame, uint32 id, bool is_temp, float val);
void get_message(char *msg, data_frame &frame);
uint16 checksum(data_frame &frame);
void IRAM_ATTR timer_overflow();

volatile int overflows = 0;
volatile int temp_read = 0;

DHT dht(DHTPIN, DHTTYPE);
painlessMesh mesh;

void setup() {
	Serial.begin(BAUDRATE);

	//mesh
	mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);   // all types on

	mesh.init(ssid, password, PORT);
	mesh.setContainsRoot(true);

	//dht
	timer1_attachInterrupt(timer_overflow);
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);         //5MHz
	timer1_write(OVERFLOW_LIMIT);

	dht.begin();
}

void loop() {
	mesh.update();

	if (!temp_read &&  overflows == 6) {
		temp_read = 1;
		float temp = dht.readTemperature();
		if (!isnan(temp)) {
			char message[MESSAGE_SIZE] = {0};
			data_frame frame;
			build_data_frame(frame, mesh.getNodeId(), TEMPERATURE, temp);
			get_message(message, frame);
			mesh.sendBroadcast(String(message));
		}
	}

	else if (overflows == 12) {
		overflows = 0;
		temp_read = 0;
		float hum = dht.readHumidity();
		if (!isnan(hum)) {
			char message[MESSAGE_SIZE] = {0};
			data_frame frame;
			build_data_frame(frame, mesh.getNodeId(), HUMIDITY, hum);
			get_message(message, frame);
			mesh.sendBroadcast(String(message));
		}
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

uint16 checksum(data_frame &frame) {
	uint16 checksum = 0;

	checksum += START_SIGN;

	checksum += frame.data_type;
	if (frame.data_type == TEMPERATURE_TYPE)
		for (int i = 0; i < DATA_SIZE; i++)
			checksum += frame.measurement.temperature[i];
	else
		for (int i = 0; i < DATA_SIZE; i++)
			checksum += frame.measurement.humidity[i];

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

void IRAM_ATTR timer_overflow() {
	overflows++;
	timer1_write(OVERFLOW_LIMIT);
}
