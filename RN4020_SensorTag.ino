
// RN4020 central connect peripheral, TI CC2541 SensorTag for test.
// by PCT / tom hsieh.
// Board : Arduino M0 PRO ( MCU : ATSAMD21G18A, ARM Cortex M0+ ). 
// IDE : Arduino 1.7.10 ( Arduino.ORG ).
#include <Wire.h>


// Connect to Microchip RN4020 bluetooth LE module, presetting :
// - buadrate 9600pbs.
// - Central.
// - Not support MLDP.
#define MLDP     13  // RN4020 MLDP control pin.
#define HW_WAKE  12  // RN4020 hardware wake control pin.
#define SW_WAKE  11  // RN4020 software wake control pin.
#define CONN_LED 8   // RN4020 connect LED.
#define TEST_OUT 6   // Test output.

// RN4020 connection command + SensorTag peripheral MAC.
// MAC : BC6A29C3541E
const char cmd_connect[17] = {'E',',','0',',','B','C','6','A','2','9','C','3','5','4','1','E','\r'};
// RN4020 write custom service, turn on IR temperature sensor on SensorTag,
// RN4020 command : CUWV
// UUID : F000AA02-0451-4000-B000-000000000000 
// Write data : 01
const char cmd_ir_on[41] = {'C','U','W','V',',','F','0','0','0','A','A','0','2','0','4','5','1','4','0','0','0','B','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0',',','0','1','\r'};
// RN4020 write custom service, turn off IR temperature sensor on SensorTag,
// RN4020 command : CUWV
// UUID : F000AA02-0451-4000-B000-000000000000 
// Write data : 00
const char cmd_ir_off[41] = {'C','U','W','V',',','F','0','0','0','A','A','0','2','0','4','5','1','4','0','0','0','B','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0',',','0','0','\r'}; 
// RN4020 read custom service, measurement value of IR temperature sensor on SensorTag,
// RN4020 command : CURV
// UUID : F000AA01-0451-4000-B000-000000000000
// Read data from UART : R,XXXXXXXX.\r\n
const char cmd_ir_read[38] = {'C','U','R','V',',','F','0','0','0','A','A','0','1','0','4','5','1','4','0','0','0','B','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','\r'}; 

//char test_out_text[12];           // Test out.
unsigned char temp1;              // Temporary #1.
unsigned char temp2;              // Temporary #2.
unsigned char sensor_status;      // Sensor status flag.
unsigned char uart_rec_data[32];  // UART received data.
unsigned char uart_rec_cnt;       // UART received data counter.
unsigned char display_data[11];   // LCM display data.
unsigned long sensor_delay;       // Sensor data read delay counter.  
unsigned long ms_cur;             // Minisecond time counter current.
unsigned long ms_pre;             // Minisecond time counter previous.
unsigned long ms_10ms_past;       // Minisecond time 10ms past.
unsigned long temp3;              // Temporary #3.
unsigned long measure_value;

void setup() {
  // put your setup code here, to run once:
  // IO port initialize.
  pinMode( MLDP, OUTPUT );
  pinMode( HW_WAKE, OUTPUT );
  pinMode( SW_WAKE, OUTPUT );  
  pinMode( TEST_OUT, OUTPUT );
  pinMode( CONN_LED, INPUT );
    
  Serial5.begin( 9600 ); // Arduino UART interface.  
  //Serial.begin( 9600 ); // Atmel EDBG CDC port, for test output.

  Wire.begin();

  // RN4020 control.
  digitalWrite( MLDP, LOW );        // Command mode.
  digitalWrite( HW_WAKE, HIGH );    // Hardware wake.
  digitalWrite( SW_WAKE, HIGH );    // Software wake.
  digitalWrite( TEST_OUT, LOW );
  
  // Power on delay.
  delay( 100 );
  // Connect to peripheral.  
  if( digitalRead( CONN_LED ) == LOW ){
    Serial5.write( cmd_connect );
  }  
  // Waiting for connection.
  while( digitalRead( CONN_LED ) == LOW ){} 
  delay( 1000 );
  
  while( Serial5.available() ){
     temp1 = Serial5.read();
  } 
    
  // Turn IR temperature sensor.  
  Serial5.write( cmd_ir_on ); 
  delay( 2000 ); 

  while( Serial5.available() ){
     temp1 = Serial5.read();
  }
  
  uart_rec_cnt = 0;
  sensor_delay = 0;
  ms_pre = 0;
  sensor_status = 0;
  
  // For test out.
  //test_out_text[10] = '\r';
  //test_out_text[11] = '\n';
}

void loop() {
  // put your main code here, to run repeatedly:
  ms_cur = millis();  
  ms_10ms_past = ms_cur;
  ms_10ms_past -= ms_pre;
  if( ms_10ms_past > 10 ){
    // About 10ms past.    
    // Increase sensor delay counter.
    
    temp1 = sensor_status;
    temp1 &= 0x01;
    if( temp1 == 0 ){
      ++sensor_delay;
    }
    
    if( sensor_delay == 100 ){
      sensor_delay = 0;
      // set flag bit, sensor read lock.
      sensor_status |= 0x01;
      // About 2000ms past.
      if( digitalRead( CONN_LED ) == HIGH ){
        // Read sensor data trigger on.
        // IR temperature sensor data.
        Serial5.write( cmd_ir_read );               
      }
    }    
    // Keep current value as previous.
    ms_pre = ms_cur;
  }
  
  // UART received data.
  if( Serial5.available() ){ 
    // Keep received data.
    // Check received data.
    Result_IR_Data();
  }    
}

//--------------------------------------------------------------------------------------------------------
// Data read command.
void Cmd_Data( void ){  
  // IR temperature sensor data.
  Serial5.write( cmd_ir_read );  
}

//--------------------------------------------------------------------------------------------------------
// IR measure data resoult.
void Result_IR_Data( void ){
  // Check received counter.
  temp1 = Serial5.read();
  
  switch( uart_rec_cnt ){
   
   case 0: // Start byte #1.
     if( temp1 == 'R'){
       ++uart_rec_cnt;
       return;
     }
     return;
   
   case 1: // Start byte #2.
     if( temp1 == ','){
       ++uart_rec_cnt;
       return;
     }
     // Error.
     uart_rec_cnt = 0;
     return;    
    
    case 2: // Data #0.
    case 3: // Data #1.
    case 4: // Data #2.
    case 5: // Data #3.
    case 6: // Data #4.
    case 7: // Data #5.
    case 8: // Data #6.
    case 9: // Data #7.
      uart_rec_data[uart_rec_cnt] = temp1;
      ++uart_rec_cnt;
      return;
      
    case 10: // End byte.   
      uart_rec_cnt = 0;   
      if( temp1 != '.' ){
        // Error.
        return;
      }
      break;           
  }
          
  // Get measurement value.
  // MSB. 
  temp2 = uart_rec_data[8];  
  ToValue();
  temp3 = temp1;
  temp3 <<= 4;
  temp3 &= 0x000000f0;
  temp2 = uart_rec_data[9];  
  ToValue();
  temp3 |= temp1;
  measure_value = temp3;
  measure_value <<= 8;
  measure_value &= 0x0000ff00;
  // LSB.
  temp2 = uart_rec_data[6];  
  ToValue();
  temp3 = temp1;
  temp3 <<= 4;
  temp3 &= 0x000000f0;  
  temp2 = uart_rec_data[7];  
  ToValue();
  temp3 |= temp1;
  measure_value += temp3;
  // 2^14 bits data.  
  measure_value >>= 2;
  
  // Beep control byte, useless.
  display_data[0] = '0';
    
  // To character for display.
  // Sign check.
  display_data[1] = ' ';  
  temp3 = measure_value;
  temp3 &= 0x00002000;
  if( temp3 == 0x00002000 ){
    temp3 = ~(measure_value);
    temp3 += 1;
    measure_value = temp3;
    // Minus sign.
    display_data[1] = '-';  
  }
  measure_value *= 3125;
  // For test out.
  //test_out_text[0] = (char)display_data[1];
    
  temp3 = measure_value;
  temp3 /= 10000000;
  measure_value %= 10000000;  
  display_data[2] = (unsigned char) temp3;
  display_data[2] += 0x30;
  // For test out.
  //test_out_text[1] = (char)display_data[2];
  
  temp3 = measure_value;
  temp3 /= 1000000;
  measure_value %= 1000000;  
  display_data[3] = (unsigned char) temp3;
  display_data[3] += 0x30;
  // For test out.
  //test_out_text[2] = (char)display_data[3];
  
  temp3 = measure_value;
  temp3 /= 100000;
  measure_value %= 100000;  
  display_data[4] = (unsigned char) temp3;
  display_data[4] += 0x30;
  // For test out.
  //test_out_text[3] = (char)display_data[4];  
  
  display_data[5] = '.';
  // For test out.
  //test_out_text[4] = (char)display_data[5];  
    
  temp3 = measure_value;
  temp3 /= 10000;
  measure_value %= 10000;  
  display_data[6] = (unsigned char) temp3;
  display_data[6] += 0x30;
  // For test out.
  //test_out_text[5] = (char)display_data[6];  
  
  temp3 = measure_value;
  temp3 /= 1000;
  measure_value %= 1000;  
  display_data[7] = (unsigned char) temp3;
  display_data[7] += 0x30;
  // For test out.
  //test_out_text[6] = (char)display_data[7];  
  
  temp3 = measure_value;
  temp3 /= 100;
  measure_value %= 100;  
  display_data[8] = (unsigned char) temp3;
  display_data[8] += 0x30;
  // For test out.
  //test_out_text[7] = (char)display_data[8];  
  
  temp3 = measure_value;
  temp3 /= 10;
  measure_value %= 10;  
  display_data[9] = (unsigned char) temp3;
  display_data[9] += 0x30;
  // For test out.
  //test_out_text[8] = (char)display_data[9];  
  
  temp3 = measure_value;
  display_data[10] = (unsigned char) temp3;
  display_data[10] += 0x30;
  // For test out.
  //test_out_text[9] = (char)display_data[10];  
  
  //test_out_text[0] = uart_rec_data[6];
  //test_out_text[1] = uart_rec_data[7];
  //test_out_text[2] = uart_rec_data[8];
  //test_out_text[3] = uart_rec_data[9];
  //test_out_text[4] = ' ';
  //test_out_text[5] = ' ';
  //test_out_text[6] = ' ';
  //test_out_text[7] = ' ';
  //test_out_text[8] = ' ';
  //test_out_text[9] = ' ';
    
  // For test out.
  //Serial.write( test_out_text );
  //digitalWrite( TEST_OUT, HIGH );  
  
  // send LCM display data via I2C.
  Wire.beginTransmission( 0x26 );
  Wire.write( display_data, 11 );
  Wire.endTransmission();      

  // Clear flag bit, sensor read unlock.
  sensor_status &= 0xfe;
}
//--------------------------------------------------------------------------------------------------------
// Character to value.
void ToValue( void ){  
  // '0' ~ '9'.
  temp1 = temp2;
  temp1 &= 0x10;
  if( temp1 ){
    temp1 = temp2;
    temp1 &= 0x0f;
    return;
  }
  // 'A' ~ 'F' or 'a' ~ 'f'.
  temp1 = temp2;
  temp1 &= 0x40;
  if( temp1 ){
    temp1 = temp2;
    temp1 &= 0x0f;
    temp1 += 9;
  }
}
//--------------------------------------------------------------------------------------------------------


