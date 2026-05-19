#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "sensor.h"

#define SERVER_IP "172.20.10.3"
#define PORT 8080

volatile int keep_running = 1;

void sigint_handler(int dummy) {
    printf("\n[CTRL-C] detectat! Oprire...\n");
    keep_running = 0;
}

int main() {
    signal(SIGINT, sigint_handler);

    if (init_i2c() < 0) { printf("Eroare I2C\n"); return 1; }
    if (init_color_sensor() < 0) { printf("Eroare senzor culoare\n"); return 1; }
    init_accel_sensor();
    printf("Senzorii au fost initializati cu succes.\n");

    int sockfd;
    struct sockaddr_in server_addr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { perror("Eroare socket"); return 1; }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    struct SensorDataPacket packet;
    int sample_count = 0;

    printf("Incepem! Se va trimite un pachet la fiecare 10 secunde...\n");

    while (keep_running) {
        read_sensors(&packet, sample_count);
        printf("Secunda %2d citita...\n", sample_count + 1);

        sample_count++;

        if (sample_count == 10) {
            printf(">> Am adunat 10 secunde. Trimit pachetul prin UDP...\n");
            sendto(sockfd, &packet, sizeof(packet), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
            sample_count = 0;
            printf("----------------------------------------\n");
        }

        sleep(1);
    }

    close(sockfd);
    close_i2c();
    return 0;
}
