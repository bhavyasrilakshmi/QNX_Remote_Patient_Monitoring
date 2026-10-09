#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/neutrino.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define NUMBER_OF_PATIENTS  4
#define SAMPLE_INTERVAL_SEC  1
#define CENTRAL_PORT  5000
#define CENTRAL_IP  "127.0.0.1"
#define BUFFER_SIZE  200
#define NORMAL  0
#define WARNING  1
#define CRITICAL  2
#define TIMER_PULSE (_PULSE_CODE_MINAVAIL + 1)
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
typedef struct
{
    PatientRecord records[BUFFER_SIZE];
    int count;
} PriorityBuffer;
PriorityBuffer buffer;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_cond = PTHREAD_COND_INITIALIZER;
volatile int network_connected = 0;
unsigned long sequence_number = 0;
int timer_chid;
int timer_coid;
int network_socket = -1;
void *network_thread(void *arg);
void *timer_thread(void *arg);
void generate_vitals(PatientRecord *record);
int determine_priority(PatientRecord *record);
void add_to_buffer(PatientRecord *record);
int get_highest_priority_record(PatientRecord *record);
void remove_record_from_buffer(unsigned long sequence);
void print_record(PatientRecord *record);
void activate_local_alarm(PatientRecord *record);
int connect_to_central_station(void);
int send_record(PatientRecord *record);
void print_buffer_status(void);
int determine_priority(PatientRecord *record)
{
    if (record->spo2 < 85 || record->heart_rate > 140 || record->heart_rate < 40 || record->temperature >= 40.0)
    {
        return CRITICAL;
    }
    if (record->spo2 < 90 || record->heart_rate > 120 || record->heart_rate < 50 || record->temperature >= 38.0)
    {
        return WARNING;
    }
    return NORMAL;
}
void generate_vitals(PatientRecord *record)
{
    record->heart_rate = 60 + rand() % 61;
    record->spo2 = 94 + rand() % 6;
    record->temperature = 36.0 + ((float)(rand() % 20) / 10.0);
    int event = rand() % 10;
    if (event == 7)
    {
        record->heart_rate = 125;
        record->spo2 = 89;
    }
    if (event == 8)
    {
        record->heart_rate = 155;
        record->spo2 = 82;
    }
    if (event == 9)
    {
        record->temperature = 40.5;
    }
    record->priority = determine_priority(record);
}
void add_to_buffer(PatientRecord *record)
{
    int status;
    status = pthread_mutex_lock(&buffer_mutex);
    if (status != EOK)
    {
        printf("Mutex lock failed\n");
        return;
    }
    if (buffer.count >= BUFFER_SIZE)
    {
        printf("\nBUFFER FULL!\n");
        pthread_mutex_unlock(&buffer_mutex);
        return;
    }
    buffer.records[buffer.count] = *record;
    buffer.count++;
    printf("\n[BUFFER] Patient %d stored " "Sequence=%lu Priority=%d\n", record->patient_id, record->sequence, record->priority);
    pthread_cond_signal(&buffer_cond);
    pthread_mutex_unlock(&buffer_mutex);
}
int get_highest_priority_record(PatientRecord *record)
{
    int i;
    int best_index = -1;
    int best_priority = -1;
    pthread_mutex_lock(&buffer_mutex);
    if (buffer.count == 0)
    {
        pthread_mutex_unlock(&buffer_mutex);
        return 0;
    }
    for (i = 0;i < buffer.count;i++)
    {
        if (buffer.records[i].priority > best_priority)
        {
            best_priority = buffer.records[i].priority;
            best_index = i;
        }
    }
    if (best_index >= 0)
    {
        *record = buffer.records[best_index];
        pthread_mutex_unlock(&buffer_mutex);
        return 1;
    }
    pthread_mutex_unlock(&buffer_mutex);
    return 0;
}
void remove_record_from_buffer(unsigned long sequence)
{
    int i;
    int j;
    pthread_mutex_lock(&buffer_mutex);
    for (i = 0;i < buffer.count;i++)
    {
        if (buffer.records[i].sequence == sequence)
        {
            for (j = i;j < buffer.count - 1;j++)
            {
                buffer.records[j] = buffer.records[j + 1];
            }
            buffer.count--;
            printf("[BUFFER] Sequence %lu " "removed after ACK\n", sequence);
            break;
        }
    }
    pthread_mutex_unlock(&buffer_mutex);
}
void print_record(PatientRecord *record)
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
    printf("Time         : %s\n", time_string);
    printf("Heart Rate   : %d BPM\n", record->heart_rate);
    printf("SpO2         : %d %%\n", record->spo2);
    printf("Temperature  : %.1f C\n", record->temperature);
    if (record->priority == NORMAL)
    {
        printf("Priority     : NORMAL\n");
    }
    else if (record->priority == WARNING)
    {
        printf("Priority     : WARNING\n");
    }
    else
    {
        printf("Priority     : CRITICAL\n");
    }
    printf("----------------------------------\n");
}
void activate_local_alarm(PatientRecord *record)
{
    printf("\n\n");
    printf("****************************************\n");
    printf("*** LOCAL CRITICAL ALARM              ***\n");
    printf("*** PATIENT %d                         ***\n",record->patient_id);
    printf("*** NETWORK UNAVAILABLE               ***\n");
    printf("*** IMMEDIATE ATTENTION REQUIRED      ***\n");
    printf("****************************************\n");
    printf("\a");
}
int connect_to_central_station(void)
{
    struct sockaddr_in server;
    int sock;
    sock = socket(AF_INET,SOCK_STREAM,0);
    if (sock < 0)
    {
        perror("socket");
        return -1;
    }
    memset(&server,0,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(CENTRAL_PORT);
    if (inet_aton(CENTRAL_IP,&server.sin_addr) == 0)
    {
        printf("Invalid Central Station IP\n");
        close(sock);
        return -1;
    }
    printf("\n[NETWORK] Connecting to ""Central Station...\n");
    if (connect(sock,(struct sockaddr *)&server,sizeof(server)) < 0)
    {
        close(sock);
        return -1;
    }
    printf("[NETWORK] Connected to ""Central Station\n");
    return sock;
}
int send_record(PatientRecord *record)
{
    int bytes;
    char ack[32];
    bytes = send(network_socket,record,sizeof(PatientRecord),0);
    if (bytes != sizeof(PatientRecord))
    {
        return -1;
    }
    memset(ack,0,sizeof(ack));
    bytes = recv(network_socket,ack,sizeof(ack) - 1,0);
    if (bytes <= 0)
    {
        return -1;
    }
    if (strncmp(ack,"ACK",3) != 0)
    {
        return -1;
    }
    return 0;
}
void *network_thread(void *arg)
{
    PatientRecord record;
    (void)arg;
    while (1)
    {
        if (!network_connected)
        {
            network_socket = connect_to_central_station();
            if (network_socket >= 0)
            {
                network_connected = 1;
                printf("[NETWORK] " "NETWORK UP\n");
            }
            else
            {
                printf("[NETWORK] " "Central Station unavailable\n");
                sleep(2);
                continue;
            }
        }
        if (!get_highest_priority_record(
                &record))
        {
            usleep(100000);
            continue;
        }
        if (send_record(&record) == 0)
        {
            printf("\n[NETWORK] Sent " "Patient %d Sequence %lu\n",record.patient_id,record.sequence);
            remove_record_from_buffer(record.sequence);
        }
        else
        {
            printf("\n[NETWORK] Transmission " "failed\n");
            printf("[NETWORK] NETWORK DOWN\n");
            close(network_socket);
            network_socket = -1;
            network_connected = 0;
        }
    }
    return NULL;
}
void *timer_thread(void *arg)
{
    struct sigevent event;
    timer_t timer_id;
    struct itimerspec timer_spec;
    int rcvid;
    struct _pulse pulse;
    int i;
    PatientRecord record;
    (void)arg;
    timer_chid = ChannelCreate(_NTO_CHF_PRIVATE);
    if (timer_chid == -1)
    {
        perror("ChannelCreate");
        return NULL;
    }
    timer_coid = ConnectAttach(0,0, timer_chid, _NTO_SIDE_CHANNEL,0);
    if (timer_coid == -1)
    {
        perror("ConnectAttach");
        return NULL;
    }
    SIGEV_PULSE_INIT(&event,timer_coid,10,TIMER_PULSE,0);
    if (timer_create(CLOCK_REALTIME,&event,&timer_id) == -1)
    {
        perror("timer_create");
        return NULL;
    }
    timer_spec.it_value.tv_sec = SAMPLE_INTERVAL_SEC;
    timer_spec.it_value.tv_nsec = 0;
    timer_spec.it_interval.tv_sec = SAMPLE_INTERVAL_SEC;
    timer_spec.it_interval.tv_nsec = 0;
    if (timer_settime(timer_id,0,&timer_spec,NULL) == -1)
    {
        perror("timer_settime");
        return NULL;
    }
    printf("\n");
    printf("========================================\n");
    printf("       QNXMedLink Patient Node\n");
    printf("========================================\n");
    printf("Patients       : %d\n",NUMBER_OF_PATIENTS);
    printf("Sample period  : %d seconds\n",SAMPLE_INTERVAL_SEC);
    printf("Central IP     : %s\n",CENTRAL_IP);
    printf("Central Port   : %d\n",CENTRAL_PORT);
    printf("========================================\n");
    while (1)
    {
        rcvid = MsgReceive(timer_chid,&pulse,sizeof(pulse),NULL);
        if (rcvid == -1)
        {
            perror("MsgReceive");
            continue;
        }
        if (rcvid == 0)
        {
            if (pulse.code == TIMER_PULSE)
            {
                printf("\n\n");
                printf("========== NEW SAMPLE ==========\n");
                for (i = 1;i <= NUMBER_OF_PATIENTS;i++)
                {
                    memset(&record,0,sizeof(record));
                    record.patient_id = i;
                    record.sequence = ++sequence_number;
                    record.timestamp = time(NULL);
                    generate_vitals(&record);
                    print_record(&record);
                    add_to_buffer(&record);
                    if (record.priority == CRITICAL && !network_connected)
                    {
                        activate_local_alarm(&record);
                    }
                }
                print_buffer_status();
            }
        }
    }
    return NULL;
}
void print_buffer_status(void)
{
    pthread_mutex_lock(&buffer_mutex);
    printf("\n[BUFFER STATUS] " "Records waiting = %d\n", buffer.count);
    pthread_mutex_unlock(&buffer_mutex);
}
int main(void)
{
    pthread_t network_tid;
    pthread_t timer_tid;
    srand(time(NULL));
    memset(&buffer,0,sizeof(buffer));
    if (pthread_create(&network_tid,NULL,network_thread,NULL) != EOK)
    {
        perror("pthread_create network");
        return EXIT_FAILURE;
    }
    if (pthread_create(&timer_tid,NULL,timer_thread,NULL) != EOK)
    {
        perror("pthread_create timer");
        return EXIT_FAILURE;
    }
    pthread_join(network_tid,NULL);
    pthread_join(timer_tid,NULL);
    return EXIT_SUCCESS;
}
