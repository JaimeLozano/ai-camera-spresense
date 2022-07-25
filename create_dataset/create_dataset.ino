/*
 * @file create_dataset.ino
 * @author Jaime Lozano
 * @brief Records images to train neural network models
 *
 * How to use this code?
 * After running the firmware, find region.jpg on the SD card and find the region of interest 
 * that suits your project requirements and spresense memory requirements.
 * Modify parameters lefttop_x, lefttop_y, rightbottom_x and rightbottom_y.
 * Run again the firmware with the new parameters to create the dataset
 * to train your model with neural network console.
 * 
 * To inference refer to inference.ino
 */

#include <Camera.h>
#include <SDHCI.h>
#include <BmpImage.h>

#define lefttop_x  (145)
#define lefttop_y  (141)
#define rightbottom_x (200)
#define rightbottom_y (196)
#define out_width  (28)
#define out_height  (28)

SDClass SD;
BmpImage bmp;
char fname[16];

void saveGrayBmpImage(int width, int height, uint8_t* grayImg) 
{
  static int g_counter = 0;  // file name
  sprintf(fname, "%d.bmp", g_counter);

  /* Remove the old file with the same file name as new created file,
   * and create new file.
   */
  if (SD.exists(fname)) SD.remove(fname);
  
  // Open file in write mode
  File bmpFile = SD.open(fname, FILE_WRITE);
  if (!bmpFile) {
    Serial.println("Fail to create file: " + String(fname));
    while(1);
  }
  
  // Generate bitmap image
  bmp.begin(BmpImage::BMP_IMAGE_GRAY8, out_width, out_height, grayImg);
  
  // Write bitmap
  bmpFile.write(bmp.getBmpBuff(), bmp.getBmpSize());
  bmpFile.close();
  bmp.end();
  ++g_counter;
  
  // Show file name
  Serial.println("Saved an image as " + String(fname));
}

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
  uint8_t grayImg[out_width*out_height];
  for (int n = 0; n < out_width*out_height; ++n) {
    grayImg[n] = (uint8_t)(((imgbuf[n] & 0xf000) >> 8) 
                         | ((imgbuf[n] & 0x00f0) >> 4));
  }
  
  // Save image in bmp format
  saveGrayBmpImage(out_width, out_height, grayImg);
}

void takeOnePicture() {
  
  // Set parameters about still picture.
  theCamera.setStillPictureImageFormat(
     CAM_IMGSIZE_QVGA_H,
     CAM_IMGSIZE_QVGA_V,
     CAM_IMAGE_PIX_FMT_JPG);

  // Take picture
  CamImage img = theCamera.takePicture();
  
  // Check availability of the img instance
  if (img.isAvailable())
    {
      // Create file name
      char filename[16] = {0};
      sprintf(filename, "region.jpg");
  
      /* Remove the old file with the same file name as new created file,
       * and create new file.
       */
  
      SD.remove(filename);
      File myFile = SD.open(filename, FILE_WRITE);
      myFile.write(img.getImgBuff(), img.getImgSize());
      myFile.close();
      Serial.println("Saved an image as " + String(filename));
    }
  else
    {
      /* The size of a picture may exceed the allocated memory size.
       * Then, allocate the larger memory size and/or decrease the size of a picture.
       * [How to allocate the larger memory]
       * - Decrease jpgbufsize_divisor specified by setStillPictureImageFormat()
       * - Increase the Memory size from Arduino IDE tools Menu
       * [How to decrease the size of a picture]
       * - Decrease the JPEG quality by setJPEGQuality()
       */
  
      Serial.println("Failed to take picture");
    }
}

void setup() {
  
  // Initialize serial port
  Serial.begin(115200);

  // Initialize SD card
  while (!SD.begin()) {
    Serial.println("Insert SD card"); 
  }

  // Initialize camera
  theCamera.begin();
  theCamera.startStreaming(true, CamCB);
  Serial.println("CamCB started");

  // Take picture to define interest region
  takeOnePicture();
}

void loop() {
  // put your main code here, to run repeatedly:
}
