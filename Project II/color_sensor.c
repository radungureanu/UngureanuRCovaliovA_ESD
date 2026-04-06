#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>

/* File descriptor used for the I2C bus */
int file_i2c;

/* Flag used to stop the program safely with CTRL+C */
volatile int keep_running = 1;

/* TCS34725 I2C address and main register definitions */
#define TCS34725_ADDR         0x29
#define TCS34725_CMD_BIT      0x80

#define TCS34725_ENABLE       0x00
#define TCS34725_ATIME        0x01
#define TCS34725_ID           0x12
#define TCS34725_CDATAL       0x14
#define TCS34725_RDATAL       0x16
#define TCS34725_GDATAL       0x18
#define TCS34725_BDATAL       0x1A

/* Bits used to power on the sensor and enable RGBC measurements */
#define TCS34725_ENABLE_PON   0x01
#define TCS34725_ENABLE_AEN   0x02

/* Signal handler for CTRL+C */
void sigint_handler(int dummy) {
    printf("\n[CTRL-C] detected! Stopping program safely...\n");
    keep_running = 0;
}

/* Writes one byte to a sensor register */
int write_reg(unsigned char reg, unsigned char value) {
    unsigned char buf[2];
    buf[0] = TCS34725_CMD_BIT | reg;
    buf[1] = value;

    if (write(file_i2c, buf, 2) != 2) {
        return -1;
    }
    return 0;
}

/* Reads one 8-bit register from the sensor */
unsigned char read_reg(unsigned char reg) {
    unsigned char reg_addr = TCS34725_CMD_BIT | reg;
    unsigned char data = 0;

    write(file_i2c, &reg_addr, 1);
    read(file_i2c, &data, 1);

    return data;
}

/* Reads a 16-bit value from two consecutive registers */
unsigned short read_word(unsigned char reg) {
    unsigned char reg_addr = TCS34725_CMD_BIT | reg;
    unsigned char buf[2];

    write(file_i2c, &reg_addr, 1);
    read(file_i2c, buf, 2);

    return (unsigned short)(buf[0] | (buf[1] << 8));
}

/* Initializes the sensor: checks the ID, sets integration time, and enables measurements */
int init_sensor() {
    unsigned char id = read_reg(TCS34725_ID);

    if (id != 0x44 && id != 0x10) {
        printf("Unexpected sensor ID: 0x%02X\n", id);
        return -1;
    }

    if (write_reg(TCS34725_ATIME, 0xC0) < 0) {
        printf("Error setting integration time.\n");
        return -1;
    }

    if (write_reg(TCS34725_ENABLE, TCS34725_ENABLE_PON) < 0) {
        printf("Error powering sensor.\n");
        return -1;
    }

    usleep(3000);

    if (write_reg(TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN) < 0) {
        printf("Error enabling RGBC reading.\n");
        return -1;
    }

    usleep(160000);
    return 0;
}

int main(int argc, char *argv[]) {
    /* Default interval between two consecutive readings */
    float interval_seconds = 3.0;

    /* If a command-line argument is provided, use it as the reading interval */
    if (argc > 1) {
        interval_seconds = atof(argv[1]);
        if (interval_seconds <= 0.0) {
            printf("Invalid interval. Using the minimum value of 0.1 seconds.\n");
            interval_seconds = 0.1;
        }
    }

    /* Associate CTRL+C with the safe stop handler */
    signal(SIGINT, sigint_handler);

    /* Open the Raspberry Pi I2C bus */
    if ((file_i2c = open("/dev/i2c-1", O_RDWR)) < 0) {
        printf("Error opening the I2C bus.\n");
        return 1;
    }

    /* Select the I2C device using the sensor address */
    if (ioctl(file_i2c, I2C_SLAVE, TCS34725_ADDR) < 0) {
        printf("Error accessing the color sensor.\n");
        close(file_i2c);
        return 1;
    }

    /* Initialize the sensor before starting the measurements */
    if (init_sensor() < 0) {
        close(file_i2c);
        return 1;
    }

    printf("Color sensor initialized successfully!\n");
    printf("Reading data every %.1f seconds...\n\n", interval_seconds);

    /* Convert the interval into 100 ms steps for easier interruption handling */
    int waiting_steps = (int)(interval_seconds * 10);
    if (waiting_steps < 1) {
        waiting_steps = 1;
    }

    /* Main loop: periodically read Clear, Red, Green, and Blue channel values */
    while (keep_running) {
        unsigned short clear = read_word(TCS34725_CDATAL);
        unsigned short red   = read_word(TCS34725_RDATAL);
        unsigned short green = read_word(TCS34725_GDATAL);
        unsigned short blue  = read_word(TCS34725_BDATAL);

        printf("Color -> Clear: %u | Red: %u | Green: %u | Blue: %u\n",clear, red, green, blue);

        /* Normalize RGB values relative to the Clear channel */
        if (clear > 0) {
            float r_pct = (100.0f * red) / clear;
            float g_pct = (100.0f * green) / clear;
            float b_pct = (100.0f * blue) / clear;

            printf("Normalized -> R: %.1f%% | G: %.1f%% | B: %.1f%%\n\n",r_pct, g_pct, b_pct);
        } else {
            printf("Normalized -> No light detected.\n\n");
        }

        /* Wait between readings */
        for (int w = 0; w < waiting_steps && keep_running; w++) {
            usleep(100000);
        }
    }

    /* Close the I2C bus before exiting */
    close(file_i2c);
    return 0;
}