#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define CENTRAL_PORT 5000
#define BUFFER_SIZE 1024
#define NORMAL   0
#define WARNING  1
#define CRITICAL 2

typedef struct
{
    int patient_id;
    unsigned long sequence;
    time_t timestamp;
    int heart_rate;
    int spo2;
    float temperature;
    int priority;
} PatientRecord;


void display_record(PatientRecord *record)
{
    char time_string[64];
    time_t ist_timestamp;
    struct tm *tm_info;
    ist_timestamp = record->timestamp + 19800;
    tm_info = gmtime(&ist_timestamp);

    if (tm_info != NULL)
    {
        strftime(time_string, sizeof(time_string), "%d-%m-%Y %H:%M:%S IST", tm_info);
    }
    else
    {
        strcpy(time_string, "Time unavailable");
    }

    printf("\n----------------------------------\n");
    printf("Patient ID   : %d\n", record->patient_id);
    printf("Sequence     : %lu\n", record->sequence);
    printf("Monitoring Time: %s\n", time_string);
    printf("Heart Rate   : %d BPM\n", record->heart_rate);
    printf("SpO2         : %d %%\n", record->spo2);
    printf("Temperature  : %.1f C\n", record->temperature);

    if (record->priority == CRITICAL)
    {
        printf("Priority     : CRITICAL\n");
    }
    else if (record->priority == WARNING)
    {
        printf("Priority     : WARNING\n");
    }
    else
    {
        printf("Priority     : NORMAL\n");
    }
    printf("----------------------------------\n");
}

void central_alarm(PatientRecord *record)
{
    printf("\n");
    printf("****************************************\n");
    printf("*** CENTRAL CRITICAL ALARM            ***\n");
    printf("*** PATIENT %-3d                       ***\n",record->patient_id);
    printf("*** IMMEDIATE ATTENTION REQUIRED      ***\n");
    printf("****************************************\n");
    printf("\a");
}
int main(void)
{
    int server_socket;
    int client_socket;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    socklen_t client_length;
    PatientRecord record;
    printf("\n========================================\n");
    printf("        QNXMedLink Central Station\n");
    printf("========================================\n");
    printf("TCP Port : %d\n", CENTRAL_PORT);
    printf("Status   : Starting server...\n");
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("socket");
        return EXIT_FAILURE;
    }
    int option = 1;
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(option));
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(CENTRAL_PORT);
    if (bind(server_socket,(struct sockaddr *)&server_address,sizeof(server_address)) < 0)
    {
        perror("bind");
        close(server_socket);
        return EXIT_FAILURE;
    }
    printf("[SERVER] Port %d bound successfully\n",CENTRAL_PORT);
    if (listen(server_socket, 5) < 0)
    {
        perror("listen");
        close(server_socket);
        return EXIT_FAILURE;
    }
    printf("[SERVER] Waiting for Patient Node...\n");
    while (1)
    {
        client_length = sizeof(client_address);
        client_socket = accept(server_socket,(struct sockaddr *)&client_address,&client_length);
        if (client_socket < 0)
        {
            perror("accept");
            continue;
        }
        printf("\n========================================\n");
        printf("[NETWORK] Patient Node connected!\n");
        printf("========================================\n");
        while (1)
        {
            int bytes_received;
            bytes_received = recv(client_socket,&record,sizeof(PatientRecord),0);
            if (bytes_received == 0)
            {
                printf("\n[NETWORK] Patient Node disconnected.\n");
                break;
            }
            if (bytes_received < 0)
            {
                perror("recv");
                break;
            }
            if (bytes_received != sizeof(PatientRecord))
            {
                printf("[ERROR] Incomplete patient record received.\n");
                break;
            }
            display_record(&record);
            if (record.priority == CRITICAL)
            {
                central_alarm(&record);
            }
            else if (record.priority == WARNING)
            {
                printf("[CENTRAL] Warning condition detected.\n");
            }
            else
            {
                printf("[CENTRAL] Patient status normal.\n");
            }
            if (send(client_socket,"ACK",3,0) < 0)
            {
                perror("send ACK");
                break;
            }
            printf("[ACK] Sequence %lu acknowledged.\n",record.sequence);
        }
        close(client_socket);
        printf("[SERVER] Waiting for Patient Node reconnection...\n");
    }
    close(server_socket);
    return EXIT_SUCCESS;
}
