/*
 * @file inference.ino
 * @author Jaime Lozano
 * @brief Inference using a trained neural network model
 *
 * How to use this code?
 * Copy model.nnb to the SD card.
 * Use the same parameters lefttop_x, lefttop_y, rightbottom_x and rightbottom_y that you used
 * at create_dataset.ino
 * Run the new firmware to start inferencing
 * 
 * To create model.nnb refer to create_dataset.ino
 */

#include <Camera.h>
#include <SDHCI.h>
#include <DNNRT.h>
#include <RunningAverage.h>

#define lefttop_x  (145)
#define lefttop_y  (141)
#define rightbottom_x (200)
#define rightbottom_y (196)
#define out_width  (28)
#define out_height  (28)

DNNRT dnnrt;
SDClass SD;
DNNVariable input(out_width*out_height);
RunningAverage myRA(5);
float output[5] = {0};
int last = 0;
int actual = 0;

void CamCB(CamImage img)
{
  // Check availability of the img instance
  if (!img.isAvailable()) {
    Serial.println("Image is not available. Try again");
    return;
  }

  // Clip and resize img to interest region
  CamImage small;
  CamErr err = img.clipAndResizeImageByHW(small, lefttop_x, lefttop_y,
                    rightbottom_x, rightbottom_y, out_height, out_width);
  if (!small.isAvailable()){
    Serial.println("Clip and Reize Error:" + String(err));
    return;
  }

  // Change image to greyscale map
  uint16_t* imgbuf = (uint16_t*)small.getImgBuff();
  float *dnnbuf = input.data();
  int n = 0;
  for (n = 0; n < out_height*out_width; ++n) {
    dnnbuf[n] = (float)(((imgbuf[n] & 0xf000) >> 8) 
                      | ((imgbuf[n] & 0x00f0) >> 4)) / 255.0; // Normalize
  }

  // Inference
  dnnrt.inputVariable(input, 0);
  dnnrt.forward();
  DNNVariable output = dnnrt.outputVariable(0);

  int index = output.maxIndex();
  myRA.addValue(output[index]);
  float outputRA = myRA.getAverage();
  if (outputRA < 0.2)
  {
    Serial.print("First action recognized!");
    Serial.println(output[index]);
    digitalWrite(LED0, HIGH);
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
  }
  else if (outputRA > 0.8)
  {
    Serial.print("Second action recognized");
    Serial.println(output[index]);
    digitalWrite(LED0, LOW);
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, LOW);
  }
  else
  {
    Serial.println("No action recognized");
    digitalWrite(LED0, LOW);
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, HIGH);
  }
}

void setup() {

  // Initialize output pins
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  // Initialize serial port
  Serial.begin(115200);

  // Initialize SD card
  while (!SD.begin()) { 
    Serial.println("Insert SD card"); 
  }

  // Initialize model
  File nnbfile = SD.open("model.nnb");
  if (!nnbfile) {
    Serial.print("nnb not found");
    return;
  }
  int ret = dnnrt.begin(nnbfile);
  if (ret < 0) {
    Serial.println("Runtime initialization failure.");
    if (ret == -16) {
      Serial.print("Please install bootloader!");
      Serial.println(" or consider memory configuration!");
    } else {
      Serial.println(ret);
    }
    return;
  }

  // Clear running average
  myRA.clear();

  // Initialize camera
  theCamera.begin();
  theCamera.startStreaming(true, CamCB);
  Serial.println("CamCB started");

}

void loop() {
  // put your main code here, to run repeatedly:
}
