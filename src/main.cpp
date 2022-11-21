#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
//Used for UART
#include <fcntl.h>
//Used for UART
#include <termios.h>
//Used for subscribe
#include <stdlib.h>
#include <mosquitto.h>
#include <pthread.h>

#define GPIO_NUMBER "4"
#define GPIO4_PATH "/sys/class/gpio/gpio4/"
#define GPIO_SYSFS "/sys/class/gpio/"


#define UART_OUT "/dev/ttyS0"

//Calculate the temperature with a termister with the following variables:
#define CONST_A 0.001129148
#define CONST_B 0.000234125
#define CONST_C 0.0000000876741

//MQQT Subscribe variables
#define SUB_ADDRESS "tcp://192.168.240.102:1883"
#define CLIENTID "ExampleClientSub"
#define TOPIC "testTpoic"
#define PAYLOAD "Hello World!"
#define QOS 1
#define TIMEOUT 10000L

// Prototypes
void * publishTemp(void * argA);
void * subscribeSetPoint(void * argB);
void writeGPIO(char filename[], char value[]);
void setup(int uart0_filestream, int max );
double calculateTemp(int adc);
void on_connect(struct mosquitto *mosq, void *obj, int rc);
void on_messages(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);

int setPoint = 24;

int main(int argc, char ** argv){

    pthread_t publish, subscribe;

    if(pthread_create(&publish, NULL, &publishTemp, NULL)){
        printf("Cound not create publish thread\n");
        return 3;
    }
    if(pthread_create(&subscribe, NULL, &subscribeSetPoint, NULL)){
        printf("Cound not create publish thread\n");
        return 4;
    }
    if(pthread_join(publish, NULL)){
        printf("Could not join publish thread");
        return 5;
    }
    if(pthread_join(subscribe, NULL)){
        printf("Could not join subscribe thread");
        return 5;
    }
    return 0;
}

void * publishTemp(void * argA){
    const char *device="/dev/ttyS0";  //;serial_clear(ss)–for uart on pin 8 and 10  it’s is TTYAMA0 unsigned 
    char rx_buf[30];
    char input;
    char tx_buf[20]="en string med text\0";
    char command[50];
    const char * ledONCommand = "LED ON";
    int counter = 0;
    memset(&rx_buf[0], 0, sizeof(rx_buf));
    int n =0;
    int fd;
    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY); printf("%i",fd); //serialOpen ("/dev/ttyAMA0", 9600) ;used when serial uart 
    if (fd == -1) {
        printf("Error init UART");
        exit(-1);
    } 
    else {
        printf("UART initialized.\n");
    setup(fd,1);
    //initial the uart by parameters and set max 19 bytes for recieve buffer in noncanonical mode
    }
    fcntl(fd, F_SETFL, 0); //normal blocking mode re. Link on slide 12
    //Receive and transmit functions
    //transmit 

    u_int8_t read_buf [1024];
    u_int8_t msg[] =  {0x7e, 0x00, 0x0f, 0x17, 0x01, 0x00, 0x13, 0xa2, 0x00, 0x40, 0xe7, 0xa4, 0xe4, 0xff, 0xfe, 0x02, 0x49, 0x53, 0xe8};

    printf("Starting publish thread...\n");
    while(1)
    {
        printf("Requesting data...\n");
        int writer = write(fd, msg, sizeof(msg));
        printf("Request send...\n");

        int bytes = read(fd, &read_buf, sizeof(read_buf));
        char msg[] = "mosquitto_pub -d -h 192.168.240.102  -t testTopic -m '";
        if(bytes > 0){
            if(bytes == 27 && read_buf[0] == 0x7E){
                printf("Received bytes: %i \n", bytes);
                for (size_t i = 0; i < bytes ; i++)
                {
                    printf("%02X ", read_buf[i]);
                }
                printf("\n");
                u_int8_t valA = read_buf[24];
                uint8_t valB = read_buf[25];
                printf("%i    %i\n", valA, valB);
                uint16_t valC = (valA << 8) + valB;
                double theTemp = calculateTemp(valC);
                char conv[20];
                snprintf(conv, 20, "%f", theTemp);
                printf("From main conv: %s and msg: %s\n", conv, msg);
                strcat(msg, conv);
                strcat(msg, "'");
                printf("From main converted msg: %s\n", msg);

                //system("mosquitto_pub -d -h 192.168.0.124  -t testTopic -m 'Hello from C once again'");
                system(msg);
            }
            else{
                printf("Data received corrupted!\n");
                                printf("Received bytes: %i \n", bytes);
                for (size_t i = 0; i < bytes ; i++)
                {
                    printf("%02X ", read_buf[i]);
                }
            }            
            //break;
        }
        else if (bytes < 0)
        {
            printf("An error receiving data has occured. Error number: %i\n", bytes);
        }
        
        else{
            printf("No bytes to read\n");
        }
       
        usleep(2000000);
    }
    close(fd);
};

void * subscribeSetPoint(void * argB){
    int rc, id = 12;

    mosquitto_lib_init();

    struct mosquitto *mosq;

    mosq = mosquitto_new("subscribe-test", true, &id);
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_messages);

    rc = mosquitto_connect(mosq, "192.168.240.102", 1883, 10);
    if(rc){
        printf("Could not connect with broker\n");
        exit(-1);
    }
    printf("Starting subscribe thread...\n");
    mosquitto_loop_start(mosq);
    printf("Press Enter to quit... \n");
    getchar();
    mosquitto_loop_stop(mosq, true);

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}

double calculateTemp(int adc){

    double rntc = (3300*1024)/(adc*1.2)-1000;
    return ((1)/(CONST_A + CONST_B * log(rntc) + CONST_C * log(rntc) * log(rntc)* log(rntc))) - 273.15;
}

void writeGPIO(char filename[], char value[]){ 

    printf("filename: %s\n", filename);
    FILE* fp;                           // create a file pointer fp
    fp = fopen(filename, "w+");         // open file for writing
    if(fp )
    fprintf(fp, "%s", value);           // send the value to the file
    fclose(fp);                         // close the file using fp
}

void setup(int uart0_filestream, int max ){

    printf("Setting up UART connection\n");
    struct termios options;
    tcgetattr (uart0_filestream, &options);
    options.c_cflag= B9600 | CS8 | CLOCAL | CREAD ;
    options.c_iflag= IGNPAR | ICRNL;                                                              
    options.c_lflag &=~(ICANON |ECHO|ECHOE|ISIG);// non-canonical
    options.c_oflag= 1;   //rawdata
    options.c_cc[VMIN]=25;
    options.c_cc[VTIME]=10;         //timeout in 2x 100ms
    tcflush(uart0_filestream,TCIFLUSH);
    tcsetattr(uart0_filestream, TCSANOW, &options);
}

void on_connect(struct mosquitto *mosq, void *obj, int rc){
    printf("ID: %d\n", * (int *) obj);
    if(rc){
        printf("Error with exitcode: %d\n", rc);
        exit(-1);
    }
    mosquitto_subscribe(mosq, NULL, "test/t1", 0);
}

void on_messages(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg){

    printf("New messages with topic: %s and payload: %s\n", msg->topic, (char *) msg->payload);
    char * charTemp = (char *) msg->payload;
    setPoint = atoi(charTemp);
    printf("New set point: %d\n", setPoint);
}


