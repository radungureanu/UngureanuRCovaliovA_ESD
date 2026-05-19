#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>

// --- SETARI THINGSBOARD ---
#define THINGSBOARD_IP "172.20.10.3"
#define ACCESS_TOKEN "sBAjwoDWH4yPNF76tO7w"

int file_i2c;
volatile int keep_running = 1;

// --- REGISTRI SENZOR CULOARE (TCS34725) ---
#define TCS34725_ADDR         0x29
#define TCS34725_CMD_BIT      0x80
#define TCS34725_ENABLE       0x00
#define TCS34725_ATIME        0x01
#define TCS34725_ID           0x12
#define TCS34725_CDATAL       0x14
#define TCS34725_RDATAL       0x16
#define TCS34725_GDATAL       0x18
#define TCS34725_BDATAL       0x1A
#define TCS34725_ENABLE_PON   0x01
#define TCS34725_ENABLE_AEN   0x02

// --- REGISTRI ACCELEROMETRU (MPU-6050) ---
#define MPU6050_ADDR          0x68
#define MPU6050_PWR_MGMT_1    0x6B
#define MPU6050_ACCEL_XOUT_H  0x3B

// Oprire in siguranta cu CTRL-C
void sigint_handler(int dummy) {
    printf("\n[CTRL-C] Detectat! Oprim programul in siguranta...\n");
    keep_running = 0;
}

// ==========================================
// FUNCTII PENTRU SENZORUL DE CULOARE
// ==========================================
int write_reg_color(unsigned char reg, unsigned char value) {
    unsigned char buf[2] = {TCS34725_CMD_BIT | reg, value};
    if (write(file_i2c, buf, 2) != 2) return -1;
    return 0;
}

unsigned char read_reg_color(unsigned char reg) {
    unsigned char reg_addr = TCS34725_CMD_BIT | reg, data = 0;
    write(file_i2c, &reg_addr, 1);
    read(file_i2c, &data, 1);
    return data;
}

unsigned short read_word_color(unsigned char reg) {
    unsigned char reg_addr = TCS34725_CMD_BIT | reg, buf[2];
    write(file_i2c, &reg_addr, 1);
    read(file_i2c, buf, 2);
    return (unsigned short)(buf[0] | (buf[1] << 8));
}

int init_color_sensor() {
    ioctl(file_i2c, I2C_SLAVE, TCS34725_ADDR);
    unsigned char id = read_reg_color(TCS34725_ID);
    if (id != 0x44 && id != 0x10) return -1;
    if (write_reg_color(TCS34725_ATIME, 0xC0) < 0) return -1;
    if (write_reg_color(TCS34725_ENABLE, TCS34725_ENABLE_PON) < 0) return -1;
    usleep(3000);
    if (write_reg_color(TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN) < 0) return -1;
    usleep(160000);
    return 0;
}

// ==========================================
// FUNCTIE PENTRU ACCELEROMETRU
// ==========================================
short read_word_accel(int addr) {
    unsigned char buf[2];
    buf[0] = addr;
    write(file_i2c, buf, 1);
    read(file_i2c, buf, 2);
    return (short)((buf[0] << 8) | buf[1]);
}


// ==========================================
// PROGRAMUL PRINCIPAL
// ==========================================
int main(int argc, char *argv[]) {
    float interval_seconds = 1.0;

    if (argc > 1) {
        interval_seconds = atof(argv[1]);
        if (interval_seconds <= 0.0) {
            printf("Interval invalid. Folosim 0.1 secunde.\n");
            interval_seconds = 0.1;
        }
    }

    signal(SIGINT, sigint_handler);

    if ((file_i2c = open("/dev/i2c-1", O_RDWR)) < 0) {
        printf("Eroare la deschiderea I2C bus.\n"); return 1;
    }

    // --- 1. INITIALIZARE SI CALIBRARE ACCELEROMETRU (MPU-6050) ---
    if (ioctl(file_i2c, I2C_SLAVE, MPU6050_ADDR) < 0) {
        printf("Eroare accesare MPU-6050.\n"); return 1;
    }

    unsigned char wake_buf[2] = {MPU6050_PWR_MGMT_1, 0x00};
    write(file_i2c, wake_buf, 2);

    printf("\nCalibrare accelerometru... Nu il misca 1 secunda.\n");
    long sum_x = 0, sum_y = 0, sum_z = 0;
    int samples = 10;

    for(int i = 0; i < samples; i++) {
        sum_x += read_word_accel(0x3B);
        sum_y += read_word_accel(0x3D);
        sum_z += read_word_accel(0x3F);
        usleep(100000);
    }

    short offset_x = sum_x / samples;
    short offset_y = sum_y / samples;
    long avg_z = sum_z / samples;
    short offset_z;
    if (avg_z > 0) {
        offset_z = avg_z - 16384;
    } else {
        offset_z = avg_z + 16384;
    }

    // --- 2. INITIALIZARE SENZOR DE CULOARE ---
    if (init_color_sensor() < 0) {
        printf("Eroare la initializarea senzorului de culoare!\n");
    } else {
        printf("Senzor de culoare OK!\n");
    }

    printf("\nCalibrare finalizata! Trimit date la ThingsBoard la fiecare %.1f secunde...\n\n", interval_seconds);

    int waiting_steps = (int)(interval_seconds * 10);
    if (waiting_steps < 1) waiting_steps = 1;

    char cmd[1024];

    while (keep_running) {
        // --- CITIRE CULOARE ---
        ioctl(file_i2c, I2C_SLAVE, TCS34725_ADDR);
        unsigned short clear = read_word_color(TCS34725_CDATAL);
        unsigned short red   = read_word_color(TCS34725_RDATAL);
        unsigned short green = read_word_color(TCS34725_GDATAL);
        unsigned short blue  = read_word_color(TCS34725_BDATAL);

        // --- CITIRE ACCELEROMETRU ---
        ioctl(file_i2c, I2C_SLAVE, MPU6050_ADDR);
        short acc_x_raw = read_word_accel(0x3B) - offset_x;
        short acc_y_raw = read_word_accel(0x3D) - offset_y;
        short acc_z_raw = read_word_accel(0x3F) - offset_z;

        // Convertim in valori 'g' (tip float)
        float acc_x = acc_x_raw / 16384.0;
        float acc_y = acc_y_raw / 16384.0;
        float acc_z = acc_z_raw / 16384.0;

        printf("Culoare -> R:%u G:%u B:%u  |  Accel -> X:%.2fg Y:%.2fg Z:%.2fg\n", red, green, blue, acc_x, acc_y, acc_z);

        // --- CONSTRUIRE JSON SI TRIMITERE ---
        // Observatie: Am adaugat %.2f in loc de %u pentru acceleratie deoarece valorile sunt cu zecimale
        snprintf(cmd, sizeof(cmd),
            "mosquitto_pub -h %s -p 1883 -t v1/devices/me/telemetry -u %s -m '{\"red\":%u, \"green\":%u, \"blue\":%u, \"accel_x\":%.2f, \"accel_y\":%.2f, \"accel_z\":%.2f}'",
            THINGSBOARD_IP, ACCESS_TOKEN, red, green, blue, acc_x, acc_y, acc_z);

        system(cmd);

        // Pauza divizata pentru a opri instant cand apasam CTRL-C
        for(int w = 0; w < waiting_steps && keep_running; w++) {
            usleep(100000);
        }
    }

    close(file_i2c);
    return 0;
}