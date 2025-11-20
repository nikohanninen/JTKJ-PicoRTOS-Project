#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <math.h>
//#include <pins.h>

#include "tkjhat/sdk.h"



#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX      1
#define BUFFER_SIZE 1000
char message[BUFFER_SIZE];
char last_char;
uint16_t message_length = 0;
float position_data[7];


enum state { WAITING=1,
             READ_POS,
             ADD_CHAR,
             ADD_SPACE,
             CHECK_READY,
             SEND
             };
enum state programState = WAITING;

static void btn_fxn(uint gpio, uint32_t eventMask) {
    if (gpio == BUTTON1 && programState == WAITING){
        programState = READ_POS;
    }
    else if (gpio == BUTTON2 && programState == WAITING){
        programState = ADD_SPACE;
    }
}

static void read_position(void *arg){
    (void)arg;

    
    ICM42670_start_with_default_values();

    float ax; float ay; float az;
    float gx; float gy; float gz;
    float t;

    while(1){
        

        if (programState == READ_POS){
            ICM42670_read_sensor_data(&ax, &ay, &az, 
                                  &gx, &gy, &gz,
                                  &t);
            
        
            if(fabs(ay) < 0.6){
                last_char = '-';
                printf("-");
            }
            else{
                last_char = '.';
                printf(".");
            }

                
            
            programState = ADD_CHAR;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}



static void char_add_task(void *arg){
    (void)arg;

    while(1){

        if (programState == ADD_CHAR){
            message[message_length] = last_char;
            message_length++;

            programState = WAITING;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


static void space_add_task(void *arg){
    (void)arg;

    while(1){
        
        if (programState == ADD_SPACE){
            message[message_length] = ' ';
            message_length += 1;
            printf(" ");

            programState = CHECK_READY;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void check_ready_task(void *arg){
    (void)arg;

    while (1){

        if (programState == CHECK_READY){
            if (message[message_length - 1] == ' ' && message[message_length - 2] == ' ' && message[message_length - 3] == ' '){
                programState = SEND;
            }
            else{
                programState = WAITING;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

}

static void send_task(void *arg){
    (void)arg;

    while(1){
        if (programState == SEND){
            printf("\nMessage: %s\n", message);
            memset(message, 0, BUFFER_SIZE);
            message_length = 0;
            programState = WAITING;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {

    stdio_init_all();
    

    // Uncomment this lines if you want to wait till the serial monitor is connected
    /*while (!stdio_usb_connected()){
        sleep_ms(10);
    }*/ 
    
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.
    init_ICM42670();

    gpio_init(BUTTON1);
    gpio_set_dir(BUTTON1, GPIO_IN);

    gpio_init(BUTTON2);
    gpio_set_dir(BUTTON2, GPIO_IN);

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true, btn_fxn);
    gpio_set_irq_enabled(BUTTON2, GPIO_IRQ_EDGE_RISE, true);
    
    TaskHandle_t hreadPosition, hCharAddTask, hSpaceAddTask, hCheckReadyTask, hSendTask = NULL;

    // Create the tasks with xTaskCreate
    
    BaseType_t result = xTaskCreate(char_add_task,  // (en) Task function
                "char_add",              // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hCharAddTask);         // (en) A handle to control the execution of this task
    
                
    if(result != pdPASS) {
        printf("char_add Task creation failed\n");
        return 0;
    }

    result = xTaskCreate(space_add_task,  // (en) Task function
                "space_add",              // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hSpaceAddTask);         // (en) A handle to control the execution of this task
    
                
    if(result != pdPASS) {
        printf("space_add Task creation failed\n");
        return 0;
    }

    result = xTaskCreate(check_ready_task,  // (en) Task function
                "check_ready",              // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hCheckReadyTask);         // (en) A handle to control the execution of this task
    
                
    if(result != pdPASS) {
        printf("check_ready Task creation failed\n");
        return 0;
    }

    result = xTaskCreate(send_task,  // (en) Task function
                "send",              // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hSendTask);         // (en) A handle to control the execution of this task
    
                
    if(result != pdPASS) {
        printf("send Task creation failed\n");
        return 0;
    }

    result = xTaskCreate(read_position,  // (en) Task function
                "read",              // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hreadPosition);         // (en) A handle to control the execution of this task
    
                
    if(result != pdPASS) {
        printf("Print Task creation failed\n");
        return 0;
    }

    // Start the scheduler (never returns)
    vTaskStartScheduler();
    
    // Never reach this line.
    return 0;
}