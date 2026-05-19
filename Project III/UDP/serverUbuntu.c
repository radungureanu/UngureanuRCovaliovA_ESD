#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>

#define PORT 8080

struct SensorDataPacket {
    unsigned short red[10];
    unsigned short green[10];
    unsigned short blue[10];
    float acc_x[10];
    float acc_y[10];
    float acc_z[10];
};

void print_stats_int(const char* name, unsigned short data[10]) {
    double sum = 0;
    unsigned short min = data[0], max = data[0];
    for(int i = 0; i < 10; i++) {
        sum += data[i];
        if(data[i] < min) min = data[i];
        if(data[i] > max) max = data[i];
    }
    double mean = sum / 10.0;
    double var_sum = 0;
    for(int i = 0; i < 10; i++) var_sum += pow(data[i] - mean, 2);
    printf("%s - Mean: %6.2f | Min: %5u | Max: %5u | StdDev: %5.2f\n", name, mean, min, max, sqrt(var_sum / 10.0));
}

void print_stats_float(const char* name, float data[10]) {
    double sum = 0;
    float min = data[0], max = data[0];
    for(int i = 0; i < 10; i++) {
        sum += data[i];
        if(data[i] < min) min = data[i];
        if(data[i] > max) max = data[i];
    }
    double mean = sum / 10.0;
    double var_sum = 0;
    for(int i = 0; i < 10; i++) var_sum += pow(data[i] - mean, 2);
    printf("%s - Mean: %6.2f | Min: %5.2f | Max: %5.2f | StdDev: %5.2f\n", name, mean, min, max, sqrt(var_sum / 10.0));
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    struct SensorDataPacket packet;
    socklen_t client_len = sizeof(client_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Eroare socket"); exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Eroare bind"); exit(1);
    }
    
    printf("Server pornit! Asteptam pachete de cate 10s pe portul %d...\n", PORT);

    while (1) {
        int n = recvfrom(sockfd, &packet, sizeof(packet), MSG_WAITALL, (struct sockaddr *)&client_addr, &client_len);
        if (n == sizeof(packet)) {
            printf("\n--- PACHET 10 SECUNDE PRIMIT. STATISTICI: ---\n");
            print_stats_int("Rosu    ", packet.red);
            print_stats_int("Verde   ", packet.green);
            print_stats_int("Albastru", packet.blue);
            print_stats_float("Accel X ", packet.acc_x);
            print_stats_float("Accel Y ", packet.acc_y);
            print_stats_float("Accel Z ", packet.acc_z);
            printf("---------------------------------------------\n");
        } else {
            printf("Am primit un pachet, dar dimensiunea nu corespunde!\n");
        }
    }
    return 0;
}