#ifndef SENSOR_H
#define SENSOR_H

struct SensorDataPacket {
    unsigned short red[10];
    unsigned short green[10];
    unsigned short blue[10];
    float acc_x[10];
    float acc_y[10];
    float acc_z[10];
};

int init_i2c();
void close_i2c();
int init_color_sensor();
void init_accel_sensor();
void read_sensors(struct SensorDataPacket *packet, int index);

#endif