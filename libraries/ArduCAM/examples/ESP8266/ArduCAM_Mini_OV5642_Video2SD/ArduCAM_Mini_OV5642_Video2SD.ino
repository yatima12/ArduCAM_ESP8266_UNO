#include <SD.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"
// set pin 1 as the slave select for SD:
#define SD_CS 1
// set the num of picture
#define pic_num 500
//set pin 16 as the slave select for SPI:
const int SPI_CS = 16;

#define AVIOFFSET 240
unsigned long movi_size = 0;
unsigned long jpeg_size = 0;
const char zero_buf[4] = {0x00, 0x00, 0x00, 0x00};
const char avi_header[AVIOFFSET] PROGMEM ={
  0x52, 0x49, 0x46, 0x46, 0xD8, 0x01, 0x0E, 0x00, 0x41, 0x56, 0x49, 0x20, 0x4C, 0x49, 0x53, 0x54,
  0xD0, 0x00, 0x00, 0x00, 0x68, 0x64, 0x72, 0x6C, 0x61, 0x76, 0x69, 0x68, 0x38, 0x00, 0x00, 0x00,
  0xA0, 0x86, 0x01, 0x00, 0x80, 0x66, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x49, 0x53, 0x54, 0x84, 0x00, 0x00, 0x00,
  0x73, 0x74, 0x72, 0x6C, 0x73, 0x74, 0x72, 0x68, 0x30, 0x00, 0x00, 0x00, 0x76, 0x69, 0x64, 0x73,
  0x4D, 0x4A, 0x50, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x66,
  0x28, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x18, 0x00, 0x4D, 0x4A, 0x50, 0x47, 0x00, 0x84, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x49, 0x53, 0x54,
  0x10, 0x00, 0x00, 0x00, 0x6F, 0x64, 0x6D, 0x6C, 0x64, 0x6D, 0x6C, 0x68, 0x04, 0x00, 0x00, 0x00,
  0x64, 0x00, 0x00, 0x00, 0x4C, 0x49, 0x53, 0x54, 0x00, 0x01, 0x0E, 0x00, 0x6D, 0x6F, 0x76, 0x69,
};
ArduCAM myCAM( OV5642, SPI_CS );
void print_quartet(unsigned long i,File fd){
  fd.write(i % 0x100);  i = i >> 8;   //i /= 0x100;
  fd.write(i % 0x100);  i = i >> 8;   //i /= 0x100;
  fd.write(i % 0x100);  i = i >> 8;   //i /= 0x100;
  fd.write(i % 0x100);
}
void Video2SD(){
   char quad_buf[4] = {};
  char str[8];
  File outFile;
  byte buf[256];
 
  static int i = 0;
  static int k = 0;
  uint8_t temp, temp_last;
  unsigned long position = 0;
  uint16_t frame_cnt = 0;
  uint8_t remnant = 0;
  //Create a avi file
  k = k + 1;
  itoa(k, str, 10);
  strcat(str, ".avi");
  //Open the new file
  outFile = SD.open(str, O_WRITE | O_CREAT | O_TRUNC);
  if (! outFile)
  {
    Serial.println("open file failed");
    while (1);
    return;
  }
  //Write AVI Header

  for ( i = 0; i < AVIOFFSET; i++)
  {
    char ch = pgm_read_byte(&avi_header[i]);
    buf[i] = ch;
  }
  outFile.write(buf, AVIOFFSET);
  //Write video data
  for ( frame_cnt = 0; frame_cnt < pic_num; frame_cnt ++)
  {
        yield(); 
    //Capture a frame            
    //Flush the FIFO
    myCAM.flush_fifo();
    //Clear the capture done flag
    myCAM.clear_fifo_flag();
    //Start capture
    myCAM.start_capture();
    //Serial.println("Start Capture");
    while (!myCAM.get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK));
    //Serial.println("Capture Done!");
    outFile.write("00dc");
    outFile.write(zero_buf, 4);
    i = 0;
    jpeg_size = 0;
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();
    temp = SPI.transfer(0x00);
    temp = (byte)(temp >> 1) | (temp << 7); // correction for bit rotation from readback    
    //Read JPEG data from FIFO
    
    while ( (temp != 0xD9) | (temp_last != 0xFF))
    {
      temp_last = temp;
      temp = SPI.transfer(0x00);
       temp = (byte)(temp >> 1) | (temp << 7); // correction for bit rotation from readback
      
      //Write image data to buffer if not full
      if (i < 256)
        buf[i++] = temp;
      else
      {
        //Write 256 bytes image data to file
        myCAM.CS_HIGH();
        outFile.write(buf, 256);
        i = 0;
        buf[i++] = temp;
        myCAM.CS_LOW();
        myCAM.set_fifo_burst();
        jpeg_size += 256;
      }
    }
    //Write the remain bytes in the buffer
    if (i > 0)
    {
      myCAM.CS_HIGH();
      outFile.write(buf, i);
      jpeg_size += i;
    }
    remnant = (4 - (jpeg_size & 0x00000003)) & 0x00000003;
    jpeg_size = jpeg_size + remnant;
    movi_size = movi_size + jpeg_size;
    if (remnant > 0)
      outFile.write(zero_buf, remnant);
    //Serial.println(movi_size, HEX);

    position = outFile.position();
    outFile.seek(position - 4 - jpeg_size);
    print_quartet(jpeg_size, outFile);
    position = outFile.position();
    outFile.seek(position + 6);
    outFile.write("AVI1", 4);
    position = outFile.position();
    outFile.seek(position + jpeg_size - 10);
  }

  //Modify the MJPEG header from the beginning of the file

  outFile.seek(4);
  print_quartet(movi_size + 0xd8, outFile);//riff file size
  //Serial.println(movi_size, HEX);

  //overwrite hdrl
  unsigned long us_per_frame = 1000000 / 4; //(per_usec); //hdrl.avih.us_per_frame
  outFile.seek(0x20);
  print_quartet(us_per_frame, outFile);
  unsigned long max_bytes_per_sec = movi_size * 4/ frame_cnt; //hdrl.avih.max_bytes_per_sec
  outFile.seek(0x24);
  print_quartet(max_bytes_per_sec, outFile);
  //unsigned long tot_frames = framecnt;    //hdrl.avih.tot_frames
  outFile.seek(0x30);
  print_quartet(max_bytes_per_sec, outFile);
  //unsigned long frames =framecnt;// (TOTALFRAMES); //hdrl.strl.list_odml.frames
  outFile.seek(0xe0);
  print_quartet(max_bytes_per_sec, outFile);
  outFile.seek(0xe8);
  print_quartet(movi_size, outFile);// size again
  //Close the file
  outFile.close();
  Serial.println("OK");
}
void setup(){
  uint8_t vid, pid;
  uint8_t temp;
  Wire.begin();
  Serial.begin(115200);
  Serial.println("ArduCAM Start!");
  // set the SPI_CS as an output:
  pinMode(SPI_CS, OUTPUT);

  // initialize SPI:
  SPI.begin();
  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55)
  {
    Serial.println("SPI interface Error!");
    while (1);
  }

  myCAM.clear_bit(ARDUCHIP_GPIO,GPIO_PWDN_MASK); //disable low power
 delay(100);
//Check if the camera module type is OV5642
  myCAM.wrSensorReg16_8(0xff, 0x01);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
   if((vid != 0x56) || (pid != 0x42))
   Serial.println("Can't find OV5642 module!");
   else
   Serial.println("OV5642 detected.");
   myCAM.set_format(JPEG);
   myCAM.InitCAM();
   myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
   myCAM.OV5642_set_JPEG_size(OV5642_320x240);

  //Initialize SD Card
  if (!SD.begin(SD_CS))
  {
    //while (1);    //If failed, stop here
    Serial.println("SD Card Error");
  }
  else
    Serial.println("SD Card detected!");
}
void loop(){
  
  Video2SD();
  myCAM.set_bit(ARDUCHIP_GPIO,GPIO_PWDN_MASK);  //enable low power
  delay(3000);
  myCAM.clear_bit(ARDUCHIP_GPIO,GPIO_PWDN_MASK); //disable low power
  delay(2000);
}






