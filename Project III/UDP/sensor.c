#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "sensor.h"

int file_i2c;

#define TCS34725_ADDR 0x29
#define MPU6050_ADDR  0x68
#define TCS34725_CMD_BIT 0x80

int init_i2c() {
    if ((file_i2c = open("/dev/i2c-1", O_RDWR)) < 0) return -1;
    return 0;
}

void close_i2c() {
    close(file_i2c);
}

void write_reg(unsigned char addr, unsigned char reg, unsigned char value) {
    ioctl(file_i2c, I2C_SLAVE, addr);
    unsigned char buf[2] = {reg, value};
    write(file_i2c, buf, 2);
}

unsigned short read_word_color(unsigned char reg) {
    ioctl(file_i2c, I2C_SLAVE, TCS34725_ADDR);
    unsigned char reg_addr = TCS34725_CMD_BIT | reg;
    unsigned char buf[2];
    write(file_i2c, &reg_addr, 1);
    read(file_i2c, buf, 2);
    return (unsigned short)(buf[0] | (buf[1] << 8));
}

short read_word_accel(unsigned char reg) {
    ioctl(file_i2c, I2C_SLAVE, MPU6050_ADDR);
    unsigned char buf[2];
    write(file_i2c, &reg, 1);
    read(file_i2c, buf, 2);
    return (short)((buf[0] << 8) | buf[1]);
}

int init_color_sensor() {
    ioctl(file_i2c, I2C_SLAVE, TCS34725_ADDR);
    unsigned char reg_addr = TCS34725_CMD_BIT | 0x12;
    unsigned char id;
    write(file_i2c, &reg_addr, 1);
    read(file_i2c, &id, 1);
    if (id != 0x44 && id != 0x10) return -1;

    write_reg(TCS34725_ADDR, TCS34725_CMD_BIT | 0x01, 0xC0);
    write_reg(TCS34725_ADDR, TCS34725_CMD_BIT | 0x00, 0x01);
    usleep(3000);
    write_reg(TCS34725_ADDR, TCS34725_CMD_BIT | 0x00, 0x03);
    return 0;
}

void init_accel_sensor() {
    write_reg(MPU6050_ADDR, 0x6B, 0x00);
}

void read_sensors(struct SensorDataPacket *packet, int index) {
    packet->red[index]   = read_word_color(0x16);
    packet->green[index] = read_word_color(0x18);
    packet->blue[index]  = read_word_color(0x1A);

    packet->acc_x[index] = read_word_accel(0x3B) / 16384.0;
    packet->acc_y[index] = read_word_accel(0x3D) / 16384.0;
    packet->acc_z[index] = read_word_accel(0x3F) / 16384.0;
}
