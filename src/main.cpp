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
//Bjørn
#include <iostream>
#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <math.h>

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
void sendSetpoint();
//Bjørn prototyper
uint8_t generateChecksumByte(uint8_t[]);
void decodeMqttMessage(std::string);
void encodeMqttMessage(uint8_t[], std::string);
void buildZBCommand(uint8_t[], std::string);
void stringSplitter(std::string, int[]);

// concat buf for building of command
uint8_t concatBuf[15 + 5 + 2 + 1];  // plus 1 for checksum byte
// global int array for split string mtqq message in
int mqtt_message_in_int[4]= {0};
std::string mqtt_message_out;

int setPoint = 24;

// TODO Only publish msg if the temperature is changing. 

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
    //u_int8_t msg[] =  {0x7e, 0x00, 0x0f, 0x17, 0x01, 0x00, 0x13, 0xa2, 0x00, 0x40, 0xe7, 0xa4, 0xe4, 0xff, 0xfe, 0x02, 0x49, 0x53, 0xe8};
    u_int8_t msg[]{0x7E, 0x00, 0x13, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8B, 0xE7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C};
    printf("Starting publish thread...\n");
    while(1)
    {
        printf("Requesting data...\n");
        int writer = write(fd, msg, sizeof(msg));
        printf("Request send...\n");

        int bytes = read(fd, &read_buf, sizeof(read_buf));
        //std::string msg = "mosquitto_pub -d -h 192.168.240.102  -t testTopic -m '";
        char msg[] = "mosquitto_pub -d -h 192.168.240.102  -t testTopic -m '";
        const char* str = mqtt_message_out.c_str();
        if(bytes > 0){
            //if(bytes == 27 && read_buf[0] == 0x7E){
            if(bytes == 21 && read_buf[0] == 0x7E){
                printf("Received bytes: %i \n", bytes);
                for (size_t i = 0; i < bytes ; i++)
                {
                    printf("%02X ", read_buf[i]);
                }
                printf("\n");

                encodeMqttMessage(read_buf, mqtt_message_out);


                // u_int8_t valA = read_buf[24];
                // u_int8_t valB = read_buf[25];
                // printf("%i    %i\n", valA, valB);
                // uint16_t valC = (valA << 8) + valB;
                // double theTemp = calculateTemp(valC);
                // char conv[20];
                //snprintf(conv, 20, "%f", theTemp);
                // printf("From main conv: %s and msg: %s\n", conv, msg);
                // strcat(msg, conv);
                // strcat(msg, "'");
                // printf("From main converted msg: %s\n", msg);

                //system("mosquitto_pub -d -h 192.168.0.124  -t testTopic -m 'Hello from C once again'");
                strcat(msg, str);
                strcat(msg, "'");
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
    std::string setpointToSend = "139,231,1,";
    setpointToSend.append(charTemp);
    buildZBCommand(concatBuf, setpointToSend);
    sendSetpoint();

}

void sendSetpoint(){
    const char *device="/dev/ttyS0";  //;serial_clear(ss)–for uart on pin 8 and 10  it’s is TTYAMA0 unsigned 
    char rx_buf[30];
    char input;
    char tx_buf[25];
    char command[50];
    //const char * ledONCommand = "LED ON";
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
        printf("Requesting data...\n");
        int writer = write(fd, concatBuf, sizeof(concatBuf));
        printf("Request send...\n");
}





//Bjørn functioner

void buildZBCommand(uint8_t concatBuf[], std::string mqtt_message_in){


  uint8_t msg_10_prefix[13] = {0x7E, 0x00, 0x13, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t dev_adr[2] = {0x00, 0x00}; // 14, 15 ; 0x8B, 0xE7
  uint8_t BR_O[2] = {0x00, 0x00}; // 16, 17
  uint8_t rf_data[5] = {0x00, 0x00, 0x00, 0x00, 0x00}; // 18, 19, 20, 21, 22
  uint8_t gen_checksum[1] = {0x00}; // 23
   

  decodeMqttMessage(mqtt_message_in, rf_data, dev_adr); 
 
  /* copy msg_10_prefix to the start of concatBuf */ 
  std::memcpy(concatBuf, msg_10_prefix, sizeof(msg_10_prefix));
  /* Copy dev_adr to the byte position following it */ 
  std::memcpy(concatBuf + sizeof(msg_10_prefix), dev_adr, sizeof(dev_adr));
    /* Copy BR_O to the byte position following it */ 
  std::memcpy(concatBuf + sizeof(msg_10_prefix) + sizeof(dev_adr), BR_O, sizeof(BR_O));
  /* Copy rf_data to the byte position following it */ 
  std::memcpy(concatBuf + sizeof(msg_10_prefix) + sizeof(dev_adr) + sizeof(BR_O), rf_data, sizeof(rf_data));
  

  uint8_t checksum = generateChecksumByte(concatBuf);
  gen_checksum[0] = checksum;

  /* Copy checksum to the command following the RF_data */ 
  std::memcpy(concatBuf + sizeof(msg_10_prefix) + sizeof(dev_adr) + sizeof(BR_O) + sizeof(rf_data), gen_checksum, sizeof(gen_checksum));

}



uint8_t generateChecksumByte(uint8_t concatBuf[]){

    uint8_t addition_total = 0x00;

    for (int i = 3; i < 23; i++){ // hard coded to 23, sizeof(concatBuf) didn't work
        addition_total = addition_total + concatBuf[i];
    }

    std::cout << "in genchecksum, result: " << (int) addition_total << std::endl;

    return 0xFF - addition_total;

    
}

// Lånt kode: https://www.javatpoint.com/how-to-split-strings-in-cpp
void stringSplitter(std::string input, int mqtt_message_in_int[]){
 
    std::string delim = ","; // delimiter  
    
    std::cout << " Your string with delimiter is: " << input << std::endl;  
    size_t pos = 0;  
    std::string token1; // define a string variable  
    
    int i = 0;

    // use find() function to get the position of the delimiters  
    while (( pos = input.find (delim)) != std::string::npos)  
        {  

        token1 = input.substr(0, pos); // store the substring
        mqtt_message_in_int[i] = std::stoi(token1); // put split token into int array
        i++;

        std::cout << token1 << std::endl;  

        input.erase(0, pos + delim.length());  /* erase() function store the current positon and move to next token. */   


        }  

    std::cout << input << std::endl; // it print last token of the string.
    mqtt_message_in_int[i] = std::stoi(input); // sidste string puttes i split 

}

void decodeMqttMessage(std::string mqtt_message_in, uint8_t rf_data[], uint8_t dev_adr[]){

    
    // reads incoming mqtt message (string), and populates the int array mqtt_message_in_int 
    stringSplitter(mqtt_message_in, mqtt_message_in_int);

    // populate device adress
    dev_adr[0] = (uint8_t) mqtt_message_in_int[0];
    dev_adr[1] = (uint8_t) mqtt_message_in_int[1];


    // depending on incoming command type, populate rf_data accordingly
    // if command type = 1, populate rf_data with cmd, temp_to_be_set 
    if (mqtt_message_in_int[2] == 1){
        rf_data[0] = (uint8_t) mqtt_message_in_int[2]; // cmd
        rf_data[1] = (uint8_t) mqtt_message_in_int[3]; // temperatur to be set
    } 
}

void encodeMqttMessage(uint8_t read_buf[], std::string mqtt_message_out){

    // the received 0x90 frame sent by XBee from ESP32 is 19 bytes long (payload=3Bytes, index 15,16,17)
    // this assumes that the entire frame is written/overwritten into read_buf


     for (int i = 15; i < 18; i++){
        std::string temp = std::to_string((int) read_buf[i]);
        mqtt_message_out.append(temp);

        // appends a comma after first and second byte appended to string
        if (i != 17){
            mqtt_message_out.append(",");
        }

    } 

    std::cout << "dbp@encodeMqttMessage: printing mqtt_message_out: " << mqtt_message_out << std::endl;

}