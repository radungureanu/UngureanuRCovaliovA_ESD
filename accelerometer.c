#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <signal.h>

int file_i2c;
volatile int keep_running = 1;

// 2.3.2 - Stop the program on CTRL-C
void sigint_handler(int dummy) {
    printf("\n[CTRL-C] detected! Stopping program safely...\n");
    keep_running = 0;
}

// Function to read 16-bit data from the sensor's registers
short read_word_2c(int addr) {
    unsigned char buf[2];
    buf[0] = addr;
    write(file_i2c, buf, 1);
    read(file_i2c, buf, 2);
    return (short)((buf[0] << 8) | buf[1]);
}

int main() {
    signal(SIGINT, sigint_handler); // Attach handler for CTRL-C

    // Open the I2C bus
    if ((file_i2c = open("/dev/i2c-1", O_RDWR)) < 0) {
        printf("Error opening the I2C bus.\n"); return 1;
    }
    if (ioctl(file_i2c, I2C_SLAVE, 0x68) < 0) { // 0x68 is the default address for MPU-6000
        printf("Error accessing the sensor.\n"); return 1;
    }

    // Wake up the sensor (write 0 to register 0x6B - Power Management)
    unsigned char wake_buf[2] = {0x6B, 0x00};
    write(file_i2c, wake_buf, 2);

    // 2.3.2 - Offset method (Calibration)
    printf("Calibrating sensor... Please do not move it for 1 second.\n");
    long sum_x = 0, sum_y = 0;
    int samples = 10;
    for(int i = 0; i < samples; i++) {
        sum_x += read_word_2c(0x3B);
        sum_y += read_word_2c(0x3D);
        usleep(100000); // 100ms delay
    }
    short offset_x = sum_x / samples;
    short offset_y = sum_y / samples;
    printf("Calibration finished! Reading data every 3 seconds...\n\n");

    // 2.3.1 - Display readings every 3 seconds
    while (keep_running) {
        short acc_x = read_word_2c(0x3B) - offset_x;
        short acc_y = read_word_2c(0x3D) - offset_y;
        short acc_z = read_word_2c(0x3F); // No massive offset on Z to keep gravity (1g)

        // MPU-6000 has a scale factor of 16384 for +/- 2g
        printf("Acceleration -> X: %.2f g  |  Y: %.2f g  |  Z: %.2f g\n", 
               acc_x / 16384.0, acc_y / 16384.0, acc_z / 16384.0);

        // Wait 3 seconds, but check frequently if the user pressed CTRL-C
        for(int w = 0; w < 30 && keep_running; w++) {
            usleep(100000); 
        }
    }
    close(file_i2c);
    return 0;
}