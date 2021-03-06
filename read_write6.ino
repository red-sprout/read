/*
  Motor : XL320
  DEVICENAME "1" -> Serial1
  DEVICENAME "2" -> Serial2
  DEVICENAME "3" -> Serial3
*/

#include <DynamixelSDK.h>
#define _TASK_SLEEP_ON_IDLE_RUN
#define _TASK_PRIORITY
#define _TASK_WDT_IDS
#define _TASK_TIMECRITICAL
#include <TaskScheduler.h>

Scheduler Print_Priority;
Scheduler Control_Priority;

#define BOARD_BUTTON_PIN1               15
#define BOARD_BUTTON_PIN2               16
#define BOARD_BUTTON_PIN3               17

#define BOARD_LED_PIN                  18

// Control table address (XL320)
#define ADDR_PRO_TORQUE_ENABLE          64                 // Control table address is different in Dynamixel model
#define ADDR_PRO_GOAL_POSITION          116
#define ADDR_PRO_GOAL_CURRENT           102
#define ADDR_PRO_PRESENT_POSITION       132

// Protocol version
#define PROTOCOL_VERSION                2.0                 // See which protocol version is used in the Dynamixel

// Default setting
#define DEVICENAME                      "3"                 // Check which port is being used on your controller
                                                            // ex) Windows: "COM1"   Linux: "/dev/ttyUSB0"

#define TORQUE_ENABLE                   1                   // Value for enabling the torque
#define TORQUE_DISABLE                  0                   // Value for disabling the torque
#define DXL_MINIMUM_POSITION_VALUE      0                 // Dynamixel will rotate between this value
#define DXL_MAXIMUM_POSITION_VALUE      4095                 // and this value (note that the Dynamixel would not move when the position value is out of movable range. Check e-manual about the range of the Dynamixel you use.)
#define DXL_MAXIMUM_CURRENT_VALUE       2047
#define DXL_MOVING_STATUS_THRESHOLD     20                  // Dynamixel moving status threshold

#define ESC_ASCII_VALUE                 0x1b

// Initialize PortHandler instance
// Set the port path
// Get methods and members of PortHandlerLinux or PortHandlerWindows
dynamixel::PortHandler *portHandler;

// Initialize PacketHandler instance
// Set the protocol version
// Get methods and members of Protocol1PacketHandler or Protocol2PacketHandler
dynamixel::PacketHandler *packetHandler;

uint8_t dxl_error = 0;                          // Dynamixel error
int16_t dxl_present_position = 0;               // Present position

int DXL_ID=1;

int dxl_comm_result = COMM_TX_FAIL;             // Communication result
int dxl_goal_position[2] = {DXL_MINIMUM_POSITION_VALUE, DXL_MAXIMUM_POSITION_VALUE};         // Goal position
int dxl_goal_current = DXL_MAXIMUM_CURRENT_VALUE;
int target_position = 0;
int target_current = 40;
char input_character;
int i = 0;

bool new_value_check;
bool new_value_check2;
int change_value = 40;
int toggle = -1;



// PIN 15: 
void btn_IQR1(){change_value*=(digitalRead(BOARD_BUTTON_PIN1)==LOW)?toggle:toggle;  
  
  new_value_check = false;
}

// PIN 16: position control
void btn_IQR2(){target_position+=(digitalRead(BOARD_BUTTON_PIN2)==HIGH)?change_value:0;  
  if(target_position<0)target_position=0;
  
  new_value_check = true;
}

// PIN 17: current control
void btn_IQR3(){target_current+=(digitalRead(BOARD_BUTTON_PIN3)==HIGH)?change_value:0;  
  if(target_current<40)target_current=40;
  
  new_value_check2 = true;
}

Task Control_Task(1000, TASK_FOREVER, &fControl, &Control_Priority);
Task Print_Task(100, TASK_FOREVER, &fPrint, &Print_Priority);

void fControl(){
 if(change_value>0){
  digitalWrite(BOARD_LED_PIN,LOW);
 }
 else{
  digitalWrite(BOARD_LED_PIN,HIGH);
 }
}

int incomingByte=0;// for incoming serial data
void fPrint(){
  Serial.print("position : ");
  Serial.print(target_position);
  Serial.print("\tcurrent : ");
  Serial.print(target_current);
  Serial.println("\n");
  Serial.print("\tchange value: ");
  Serial.println(change_value);
}

void setup(){
  int BAUD[4] = {9600,115200,1000000,2000000};
  int BAUDRATE;

  pinMode(BOARD_LED_PIN, OUTPUT);
  pinMode(BOARD_BUTTON_PIN1, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(BOARD_BUTTON_PIN1),btn_IQR1,FALLING);
  pinMode(BOARD_BUTTON_PIN2, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(BOARD_BUTTON_PIN2),btn_IQR2,RISING);
  pinMode(BOARD_BUTTON_PIN3, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(BOARD_BUTTON_PIN3),btn_IQR3,RISING);
  // put your setup code here, to run once:
  Serial.begin(115200);

  // put your setup code here, to run once:
  while(!Serial);
  Serial.println("Start..");
  portHandler = dynamixel::PortHandler::getPortHandler(DEVICENAME);
  packetHandler = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

  // Open port
  if (portHandler->openPort()){
    Serial.print("Succeeded to open the port!\n");
  }
  else{
    Serial.print("Failed to open the port!\n");
    Serial.print("Press any key to terminate...\n");
    return;
  }
  
  // find ID, baudrate
  BAUDRATE = BAUD[3];
  if (portHandler->setBaudRate(BAUDRATE)){
    Serial.print("Succeeded to change the baudrate!\n");
  }
  else{
    Serial.print("Failed to change the baudrate!\n");
    Serial.print("Press any key to terminate...\n");
  }

   
  Serial.print("DXL_ID is ");
  Serial.println(DXL_ID);
  
  Serial.print("BAUDRATE is ");
  Serial.println(BAUDRATE);


  dxl_comm_result = packetHandler->write1ByteTxRx(portHandler, DXL_ID, ADDR_PRO_TORQUE_ENABLE, TORQUE_ENABLE, &dxl_error);
  if (dxl_comm_result != COMM_SUCCESS){
    packetHandler->getTxRxResult(dxl_comm_result);
  }
  else if (dxl_error != 0){
    packetHandler->getRxPacketError(dxl_error);
  }

  
  dxl_comm_result = packetHandler->write1ByteTxRx(portHandler, DXL_ID, 11, 5, &dxl_error);
  if (dxl_comm_result != COMM_SUCCESS){
    packetHandler->getTxRxResult(dxl_comm_result);
  }
  else if (dxl_error != 0){
    packetHandler->getRxPacketError(dxl_error);
  }




 // Change Position P gain
  dxl_comm_result = packetHandler->write2ByteTxRx(portHandler, DXL_ID, 84, 6000, &dxl_error);
  if (dxl_comm_result != COMM_SUCCESS){
    packetHandler->getTxRxResult(dxl_comm_result);
  }
  else if (dxl_error != 0){
    packetHandler->getRxPacketError(dxl_error);
  }
  else{
    Serial.print("Change P gain \n");
  }
  
  //GOAL_CURRENT
  dxl_comm_result = packetHandler->write2ByteTxRx(portHandler, DXL_ID, ADDR_PRO_GOAL_CURRENT, target_current, &dxl_error);
  if (dxl_comm_result != COMM_SUCCESS){
    packetHandler->getTxRxResult(dxl_comm_result);
  }
  else if (dxl_error != 0){
    packetHandler->getRxPacketError(dxl_error);
  }

  //GOAL_POSITION
  dxl_comm_result = packetHandler->write4ByteTxRx(portHandler, DXL_ID, ADDR_PRO_GOAL_POSITION, target_position, &dxl_error);
  if (dxl_comm_result != COMM_SUCCESS){
    packetHandler->getTxRxResult(dxl_comm_result);
  }
  else if (dxl_error != 0){
    packetHandler->getRxPacketError(dxl_error);
  }

  Control_Task.setId(10);
  Print_Task.setId(20);
  Print_Priority.setHighPriorityScheduler(&Control_Priority);
  Print_Priority.enableAll(true);

}


void loop() {
  Print_Priority.execute();
  
  if(new_value_check){
    dxl_comm_result = packetHandler->write4ByteTxRx(portHandler, DXL_ID, ADDR_PRO_GOAL_POSITION, target_position, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS){
      packetHandler->getTxRxResult(dxl_comm_result);
    }
    else if (dxl_error != 0){
      packetHandler->getRxPacketError(dxl_error);
    }
    do{
      // Read present position
      dxl_comm_result = packetHandler->read2ByteTxRx(portHandler, DXL_ID, ADDR_PRO_PRESENT_POSITION, (uint16_t*)&dxl_present_position, &dxl_error);
      Serial.print(target_position);
      Serial.print(", present :  ");
      Serial.println(dxl_present_position);
      if (dxl_comm_result != COMM_SUCCESS){
        packetHandler->getTxRxResult(dxl_comm_result);
      }
      else if (dxl_error != 0){
        packetHandler->getRxPacketError(dxl_error);
      }
    }while((abs(target_position - dxl_present_position) > DXL_MOVING_STATUS_THRESHOLD));
  }  
  new_value_check=false;
  
  if(new_value_check2){
    dxl_comm_result = packetHandler->write2ByteTxRx(portHandler, DXL_ID, ADDR_PRO_GOAL_CURRENT, target_current, &dxl_error);
    if (dxl_comm_result != COMM_SUCCESS){
      packetHandler->getTxRxResult(dxl_comm_result);
    }
    else if (dxl_error != 0){
      packetHandler->getRxPacketError(dxl_error);
    }
  }
  new_value_check2=false;
}
