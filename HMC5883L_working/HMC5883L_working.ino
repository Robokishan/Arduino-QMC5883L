#include <Wire.h> //I2C Arduino Library

#define address 0x0D //0011110b, I2C 7bit address

// Magnetometer variables
int xRaw, yRaw, zRaw;

// Calculated maximum value for each variable
int xMax = -32765;
int xMin = 32765;
int yMax = -32765;
int yMin = 32765;

// Printing variables
bool printStartingInstructions = true;
unsigned long lastTimeUpdatePrint = 0;
unsigned long timeUpdateInterval = 5000; // interval to print time update

// Calibration time in milliseconds
unsigned long calibrationTime = 30000;

unsigned long timeNow;

struct MagnetometerSample{
  int X;
  int Y;
  int Z;
};
uint16_t  mode;
MagnetometerSample sample;

#define COMPASS_CONTINUOUS 0x00
#define COMPASS_SINGLE     0x01
#define COMPASS_IDLE       0x02

//  xxxxxxxxxxxSSSxx
//  SCALE:
//   A lower value indicates a higher precision
//   but "noisier", magentic noise may necessitate
//   you to choose a higher scale.

#define COMPASS_SCALE_088  0x00 << 2
#define COMPASS_SCALE_130  0x01 << 2
#define COMPASS_SCALE_190  0x02 << 2
#define COMPASS_SCALE_250  0x03 << 2
#define COMPASS_SCALE_400  0x04 << 2
#define COMPASS_SCALE_470  0x05 << 2
#define COMPASS_SCALE_560  0x06 << 2
#define COMPASS_SCALE_810  0x07 << 2

//  xxXXXYYYZZZxxxxx
//  ORIENTATION: 
#define COMPASS_NORTH 0x00 
#define COMPASS_SOUTH 0x01
#define COMPASS_WEST  0x02
#define COMPASS_EAST  0x03
#define COMPASS_UP    0x04
#define COMPASS_DOWN  0x05

// When "pointing" north, define the direction of each of the silkscreen'd arrows
// (imagine the Z arrow points out of the top of the device) only N/S/E/W are allowed
#define COMPASS_HORIZONTAL_X_NORTH  ( (COMPASS_NORTH << 6)  | (COMPASS_WEST  << 3)  | COMPASS_UP    ) << 5
#define COMPASS_HORIZONTAL_Y_NORTH  ( (COMPASS_EAST  << 6)  | (COMPASS_NORTH << 3)  | COMPASS_UP    ) << 5
#define COMPASS_VERTICAL_X_EAST     ( (COMPASS_EAST  << 6)  | (COMPASS_UP    << 3)  | COMPASS_SOUTH ) << 5
#define COMPASS_VERTICAL_Y_WEST     ( (COMPASS_UP    << 6)  | (COMPASS_WEST  << 3)  | COMPASS_SOUTH ) << 5


float    declination_offset_radians=0;
void setup(){
  //Initialize Serial and I2C communications
  Serial.begin(9600);
  Wire.begin();
  
  Wire.beginTransmission(address); //open communication with HMC5883
  Wire.write(0x09); //select mode register
  Wire.write(0x0D); //continuous measurement mode
  Wire.endTransmission();

  Wire.beginTransmission(address); //open communication with HMC5883
  Wire.write(0x0B); //select Set/Reset period register
  Wire.write(0x01); //Define Set/Reset period
  Wire.endTransmission();
}

void loop(){
  timeNow = millis();
  // If it is the first iteration, print the intructions to use this calibration program
  if(printStartingInstructions)
  {
    printInstructions();
    printStartingInstructions = false;
  }
  updateMagnetometerData();
  sample.X = xRaw;
  sample.Y = yRaw;
  sample.Z = zRaw;
  
//  Serial.print("X : ");
//  Serial.print(sample.X);
//  Serial.print(" Y : ");
//  Serial.print(sample.Y);
//  Serial.print(" Z : ");
//  Serial.println(sample.Z);
  Serial.println(GetHeadingDegrees(sample));
  while(!Serial.available());
}

void updateMagnetometerData()
{
  //Tell the magnetometer where to begin reading data
  Wire.beginTransmission(address);
  Wire.write(0x00); //select register 0, X MSB register
  Wire.endTransmission();
  
  //Read data from each axis, 2 registers per axis
  Wire.requestFrom(address, 6);
  if(Wire.available()>=6){
    xRaw = Wire.read(); //x lsb
    xRaw |= Wire.read()<<8; //x msb
    yRaw = Wire.read(); //y lsb
    yRaw |= Wire.read()<<8;; //y msb
    zRaw = Wire.read(); //z lsb
    zRaw |= Wire.read()<<8; //z msb
  }
}

void printInstructions()
{
  // Print the instructions to use the calibration
  Serial.println("Rotate your robot in a slow rate to one direction until it has been rotate 360 degrees.");
  Serial.println("Rotate it to the other direction for another 360 degrees.");
  Serial.println("Keep rotating it until calibration time is up.");
  Serial.println("Make sure you keep the robot leveled during the calibration!");
}

void printTimeUpdate()
{
  // Print elapsed time in the claibration process
  Serial.println();
  Serial.print("Elapsed time: ");
  Serial.print(timeNow/1000);
  Serial.print("s of ");
  Serial.print(calibrationTime/1000);
  Serial.println("s");
}

void printResults()
{
  // Print the results of the calibration
  Serial.println();
  Serial.println();
  Serial.println("Finished calibration!");
  Serial.println("Copy the following numbers to your main magnetometer code:");
  Serial.print("xMin: ");
  Serial.print(xMin);
  Serial.print("\txMax: ");
  Serial.print(xMax);
  Serial.print("\tyMin: ");
  Serial.print(yMin);
  Serial.print("\tyMax: ");
  Serial.println(yMax);
}

float GetHeadingDegrees(MagnetometerSample sample)
{     

  mode = COMPASS_SINGLE | COMPASS_SCALE_130 | COMPASS_HORIZONTAL_X_NORTH;
  
  // Obtain a sample of the magnetic axes
  float heading;    
  
  // Determine which of the Axes to use for North and West (when compass is "pointing" north)
  float mag_north, mag_west;
   
  // Z = bits 0-2
  switch((mode >> 5) & 0x07 )
  {
    case COMPASS_NORTH: mag_north = sample.Z; break;
    case COMPASS_SOUTH: mag_north = 0-sample.Z; break;
    case COMPASS_WEST:  mag_west  = sample.Z; break;
    case COMPASS_EAST:  mag_west  = 0-sample.Z; break;
      
    // Don't care
    case COMPASS_UP:
    case COMPASS_DOWN:
     break;
  }
  
  // Y = bits 3 - 5
  switch(((mode >> 5) >> 3) & 0x07 )
  {
    case COMPASS_NORTH: mag_north = sample.Y;  break;
    case COMPASS_SOUTH: mag_north = 0-sample.Y; ;  break;
    case COMPASS_WEST:  mag_west  = sample.Y;  break;
    case COMPASS_EAST:  mag_west  = 0-sample.Y;  break;
      
    // Don't care
    case COMPASS_UP:
    case COMPASS_DOWN:
     break;
  }
  
  // X = bits 6 - 8
  switch(((mode >> 5) >> 6) & 0x07 )
  {
    case COMPASS_NORTH: mag_north = sample.X; break;
    case COMPASS_SOUTH: mag_north = 0-sample.X; break;
    case COMPASS_WEST:  mag_west  = sample.X; break;
    case COMPASS_EAST:  mag_west  = 0-sample.X; break;
      
    // Don't care
    case COMPASS_UP:
    case COMPASS_DOWN:
     break;
  }
    
  // calculate heading from the north and west magnetic axes
  heading = atan2(mag_west, mag_north);
  
  // Adjust the heading by the declination
  heading += declination_offset_radians;
  
  // Correct for when signs are reversed.
  if(heading < 0)
    heading += 2*M_PI;
    
  // Check for wrap due to addition of declination.
  if(heading > 2*M_PI)
    heading -= 2*M_PI;
   
  // Convert radians to degrees for readability.
  return heading * 180/M_PI; 
}
