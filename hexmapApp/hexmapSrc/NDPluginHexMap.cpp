/*
 * NDPluginHexMap.cpp
 *
 * HexMap plugin
 * Author: Matthew Moore
 *
 * Created June 19, 2015
 *
 * Change Log:
 *
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginDriver.h"
#include "NDPluginHexMap.h"


typedef enum {
  HexMapNone,
  HexMapHexToSquare,
  HexMapSquareToHex,
} NDHexMap_t;

template <typename dataTypeIn, typename dataTypeOut> void HexMapNDArray(NDArray *inArray, NDArray *outArray, 
                      int transformType, int colorMode, int newXsize, int newYsize, double hexPitch, NDArrayInfo_t *arrayInfo)
{
  //printf("Begining of function...\n");
  
  dataTypeIn *inData = (dataTypeIn *)inArray->pData;
  dataTypeOut *outData = (dataTypeOut *)outArray->pData;
  int x, y, color;
  int xSize, ySize, colorSize;
  int xStride, yStride, colorStride, xNewStride, yNewStride, colorNewStride;
  int elementSize;  
  NDArrayInfo_t newArrayInfo; 
  
  outArray->getInfo(&newArrayInfo);
  
  xSize = (int)arrayInfo->xSize;
  ySize = (int)arrayInfo->ySize;
  //printf("New Array Size: %dx%d\n",xSize,ySize);
  
  xStride = (int)arrayInfo->xStride;
  yStride = (int)arrayInfo->yStride;
  colorStride = (int)arrayInfo->colorStride;
  elementSize = arrayInfo->bytesPerElement;
  
  
  xNewStride = (int)newArrayInfo.xStride;             
  yNewStride = (int)newArrayInfo.yStride;   
  colorNewStride = (int)newArrayInfo.colorStride;
  double hexPitchX = (double)hexPitch;
  double hexPitchY = (double)hexPitch*sqrt(3)/2;
  int BaseHexX=0, BaseHexY=0;
  double physicalX, physicalY;
  double squarePitchX, squarePitchY;
  double absX, absY;
  int pixelOneX, pixelOneY, pixelTwoX, pixelTwoY;
  
  switch (transformType)
  {
  case (HexMapHexToSquare):
    physicalX = ((double)xSize -0.5) * hexPitchX;
    physicalY = ((double)ySize -1) * hexPitchY;
    squarePitchX = physicalX/((double)newXsize-1.0);
    squarePitchY = physicalY/((double)newYsize-1.0);
    //printf("squarePitchX: %f\tsquarePitchY: %f\n",squarePitchX, squarePitchY);
    //printf("Start of HexMap Case\n");
    if (colorMode == NDColorModeMono)
    {
      for (y = 0; y < newYsize; y++)
      {
        for (x = 0; x < newXsize; x++)
        {
          //printf("X: %d\tY: %d:\t",x,y);
          //Middle of chip case
          if (x>0 && x < (newXsize-1) && y>0 && y <(newYsize-1))
          {
            //printf("Middle of chip\n");
            absY = y*squarePitchY/hexPitchY;
            BaseHexY = rint(absY);
            absX = x * squarePitchX / hexPitchX - 0.5 * (BaseHexY % 2);
            //printf("X: %f\tY: %f\n",absX, absY);
            BaseHexX = rint(absX);
            
            // Is the closest pixel above 
            if (absY<BaseHexY)
            {
              // 
              if (absX > ((BaseHexX + 0.5 *(BaseHexY % 2))))
              {
                pixelOneX = BaseHexX;
                pixelOneY = BaseHexY-1;
                if ((absX- ((BaseHexX-1 + 0.5 *(BaseHexY-1 % 2)))) > (((BaseHexX+1 + 0.5 *(BaseHexY % 2)))-absX))
                {
                  pixelTwoX=BaseHexX+1;
                  pixelTwoY=BaseHexY;
                }
                else
                {
                  pixelTwoX=BaseHexX-1;
                  pixelTwoY=BaseHexY-1;
                }
              }
             else
             {
               pixelOneX = BaseHexX-1;
               pixelOneY = BaseHexY-1;
               if ((absX- ((BaseHexX-1 + 0.5 *(BaseHexY % 2)))) > (((BaseHexX + 0.5 *(BaseHexY-1 % 2)))-absX))
               {
                 pixelTwoX=BaseHexX;
                 pixelTwoY=BaseHexY-1;
               }
               else
               {
                 pixelTwoX=BaseHexX-1;
                 pixelTwoY=BaseHexY;
               }
             }
              
            }
            else
            {
              if (absX > ((BaseHexX + 0.5 *(BaseHexY % 2))))
              {
                pixelOneX = BaseHexX;
                pixelOneY = BaseHexY+1;
                if ((absX- ((BaseHexX-1 + 0.5 *(BaseHexY+1 % 2)))) > (((BaseHexX+1 + 0.5 *(BaseHexY % 2)))-absX))
                {
                  pixelTwoX=BaseHexX+1;
                  pixelTwoY=BaseHexY;
                }
                else
                {
                  pixelTwoX=BaseHexX-1;
                  pixelTwoY=BaseHexY+1;
                }
              }
             else
             {
               pixelOneX = BaseHexX-1;
               pixelOneY = BaseHexY+1;
               if ((absX- ((BaseHexX-1 + 0.5 *(BaseHexY % 2)))) > (((BaseHexX + 0.5 *(BaseHexY+1 % 2)))-absX))
               {
                 pixelTwoX=BaseHexX;
                 pixelTwoY=BaseHexY+1;
               }
               else
               {
                 pixelTwoX=BaseHexX;
                 pixelTwoY=BaseHexY+1;
               }
             }
            }
            //printf("pixelOneX: %d\tpixelOneY: %d\tpixelTwoX: %d\tpixelTwoY: %d\n",pixelOneX,pixelOneY,pixelTwoX,pixelTwoY);
            // New Value = Closest pixel value + 
            /*if (BaseHexX<0) BaseHexX=0;
            if (BaseHexX>newXsize-1) BaseHexX=newXsize-1;
            if (BaseHexY<0) BaseHexY=0;
            if (BaseHexY>newYsize-1) BaseHexY=newYsize-1;
            double tempN, tempD;
            tempN = inData[(BaseHexY * yStride) + (BaseHexX * xStride)]*sqrt(pow((absX-BaseHexX),2)+pow((absY-BaseHexY),2)); 
            tempD = sqrt(pow(absX-BaseHexX,2)+pow(absY-BaseHexY,2));
            if (pixelOneX>=0 and pixelOneX<newXsize and pixelOneY>=0 and pixelOneY<newYsize)
              tempN+=inData[(pixelOneY * yStride) + (pixelOneX * xStride)]*sqrt(pow(absX-pixelOneX,2)+pow(absY-pixelOneY,2));
              tempD+=sqrt(pow(absX-pixelOneX,2)+pow(absY-pixelOneY,2));
            if (pixelTwoX>=0 and pixelTwoX<newXsize and pixelTwoY>=0 and pixelTwoY<newYsize)
              tempN+=inData[(pixelTwoY * yStride) + (pixelTwoX * xStride)]*sqrt(pow(absX-pixelTwoX,2)+pow(absY-pixelTwoY,2)); 
              tempD+=sqrt(pow(absX-pixelTwoX,2)+pow(absY-pixelTwoY,2));
              outData[(y * yNewStride) + (x * xNewStride)] = tempN/tempD;
           */
           outData[(y * yNewStride) + (x * xNewStride)] = (inData[(BaseHexY * yStride) + (BaseHexX * xStride)]*sqrt(pow((absX-BaseHexX),2)+pow((absY-BaseHexY),2)) + inData[(pixelOneY * yStride) + (pixelOneX * xStride)]*sqrt(pow(absX-pixelOneX,2)+pow(absY-pixelOneY,2)) + inData[(pixelTwoY * yStride) + (pixelTwoX * xStride)]*sqrt(pow(absX-pixelTwoX,2)+pow(absY-pixelTwoY,2))) / (sqrt(pow(absX-BaseHexX,2)+pow(absY-BaseHexY,2))+sqrt(pow(absX-pixelOneX,2)+pow(absY-pixelOneY,2)) +sqrt(pow(absX-pixelTwoX,2)+pow(absY-pixelTwoY,2)));
            
            //printf("Location: %d\tValue: %f\n",(y * yNewStride) + (x * xNewStride),outData[(y * yNewStride) + (x * xNewStride)]);
            continue;
            
          }
          else
          {
            if (y == 0)     //handles bottom of chip
            {
              //printf("Bottom of Chip area\n");
              
              BaseHexY = 0;
              if (x == 0) // Handles 0,0 corner
              {
                //printf("0,0 corner\n");
                BaseHexX = 0;
                outData[(y * yNewStride) + (x * xNewStride)] = inData[(BaseHexY * yStride) + (BaseHexX * xStride)];
              }
              else
              {
                if (x == newXsize-1) // Handles max, 0 corner
                {
                  //printf("max,0 corner\n");
                  BaseHexX = (newXsize-1);
                  outData[(y * yNewStride) + (x * xNewStride)] = inData[((xSize-1) * xStride)];
                }
                else
                {
                  //printf("Middle of Bottom\n");
                  // Handles rest of the bottom of the chip
                  absX = x*squarePitchX/hexPitchX;
                  BaseHexX = (int)absX;
                  // linear fit between two closest pixels
                  //printf("best fit of two edge pixels\n");
                  outData[(y * yNewStride) + (x * xNewStride)] = (inData[(BaseHexX* xStride)]*((double)(BaseHexX+1)-absX) + inData[((BaseHexX+1)* xStride)]*(absX-(double)BaseHexX));
                }
              }
              //printf("Location: %d\tValue: %f\n",(y * yNewStride) + (x * xNewStride),outData[(y * yNewStride) + (x * xNewStride)]);
              continue;
            }
            if (y == newYsize-1)      //Handles Top of chip
            {
              //printf("Top of Chip area\n");
              if (x == 0) // Handles 0,max corner
              {
                //printf("0,max corner\n");
                BaseHexX = 0;
                BaseHexY = (newYsize-1);
                outData[(y * yNewStride) + (x * xNewStride)] = inData[((ySize-1) * yStride) + (BaseHexX * xStride)];
              }
              else
              {
                if (x == newXsize-1) // Handles max, max corner
                {
                  //printf("max,max corner\n");
                  BaseHexX = (newXsize-1);
                  BaseHexY = (newYsize-1);
                  outData[(y * yNewStride) + (x * xNewStride)] = inData[((ySize-1) * yStride) + ((xSize-1) * xStride)];
                }
                else
                {
                  // Handles rest of the top of the chip
                  absX = x*squarePitchX/hexPitchX-0.5; //half pixel offset, since the last row is offset horizontially a half pixel
                  BaseHexX = (int)absX;
                  // linear fit between two closest pixels
                  outData[(y * yNewStride) + (x * xNewStride)] = (inData[((ySize-1) * yStride) + (BaseHexX* xStride)]*((double)(BaseHexX+1)-absX) + inData[((ySize-1) * yStride) + ((BaseHexX+1)* xStride)]*(absX-(double)BaseHexX));
                }
              }
              //printf("Location: %d\tValue: %f\n",(y * yNewStride) + (x * xNewStride),outData[(y * yNewStride) + (x * xNewStride)]);
              continue;
            }
            
            if ( x == 0)       //Handles left edge
            {
              //printf("Left edge area\n");
              BaseHexX = 0;
              absY = y*squarePitchY/hexPitchY;
              BaseHexY = (int)round(absY);
              //absX = x * squarePitchX / hexPitchX - 0.5 * (BaseHexY % 2);
              if ((double)BaseHexY == absY)
              {
                outData[(y * yNewStride) + (x * xNewStride)] = inData[(BaseHexY * yStride) + (BaseHexX* xStride)];
                //printf("Location: %d\tValue: %f\n",(y * yNewStride) + (x * xNewStride),outData[(y * yNewStride) + (x * xNewStride)]);
                continue;
              }
              if ((double)BaseHexY > absY)
              {
                // linear fit between 0,BaseHexY and 0,BaseHexY+1
                outData[(y * yNewStride) + (x * xNewStride)] = (inData[(BaseHexY * yStride) + (BaseHexX* xStride)]*(BaseHexY+1-absY) + inData[((BaseHexY+1) * yStride) + (BaseHexX* xStride)]*(absY-BaseHexY));
                //printf("Location: %d\tValue: %f\n",(y * yNewStride) + (x * xNewStride),outData[(y * yNewStride) + (x * xNewStride)]);
                continue;
              }
              if ((double)BaseHexY < absY)
              {
                // linear fit between 0,BaseHexY and 0,BaseHexY-1
                outData[(y * yNewStride) + (x * xNewStride)] = (inData[(BaseHexY * yStride) + (BaseHexX* xStride)]*(absY-BaseHexY) + inData[((BaseHexY-1) * yStride) + (BaseHexX* xStride)]*(BaseHexY+1-absY));
                //printf("Location: %d\tValue: %f\n",(y * yNewStride) + (x * xNewStride),outData[(y * yNewStride) + (x * xNewStride)]);
                continue;
              }
              continue;
            }
            
            if (x == newXsize-1)    //Handles right edge
            {
              //printf("Right edge area\n");
              BaseHexX = (xSize-1);
              absY = y*squarePitchY/hexPitchY;
              BaseHexY = round(absY);
              if (y ==1)
                //printf("BaseHexX: %d\tBaseHexY: %d\tabsY: %f\n",BaseHexX,BaseHexY,absY);
              //absX = x * squarePitchX / hexPitchX - 0.5 * (BaseHexY % 2);
              if ((double)BaseHexY == absY)
              {
                outData[(y * yNewStride) + (x * xNewStride)] = inData[(BaseHexY * yStride) + (BaseHexX* xStride)];
                //printf("Location: %d\tValue: %f\n",(y * yNewStride) + (x * xNewStride),outData[(y * yNewStride) + (x * xNewStride)]);
                continue;
              }
              if ((double)BaseHexY > absY)
              {
                // linear fit between Max,BaseHexY and Max,BaseHexY+1
                outData[(y * yNewStride) + (x * xNewStride)] = (inData[(BaseHexY * yStride) + (BaseHexX* xStride)]*(BaseHexY+1-absY) + inData[((BaseHexY+1) * yStride) + (BaseHexX* xStride)]*(absY-BaseHexY));
                //printf("Location: %d\tValue: %f\n",(y * yNewStride) + (x * xNewStride),outData[(y * yNewStride) + (x * xNewStride)]);
                continue;
              }
              if ((double)BaseHexY < absY)
              {
                // linear fit between Max,BaseHexY and Max,BaseHexY-1
                outData[(y * yNewStride) + (x * xNewStride)] = (inData[(BaseHexY * yStride) + (BaseHexX* xStride)]*(absY-BaseHexY)+inData[((BaseHexY-1) * yStride) + (BaseHexX* xStride)]*(BaseHexY+1-absY));
                //printf("Part1: %f\tPart2: %f\tSum: %f\n",inData[(BaseHexY * yStride) + (BaseHexX* xStride)]*(absY-BaseHexY),inData[((BaseHexY-1) * yStride) + (BaseHexX* xStride)]*(BaseHexY+1-absY), inData[(BaseHexY * yStride) + (BaseHexX* xStride)]*(absY-BaseHexY)+inData[((BaseHexY-1) * yStride) + (BaseHexX* xStride)]*(BaseHexY+1-absY));
                //printf("BaseHexX: %d\tBaseHexY: %d\t",BaseHexX,BaseHexY);
                //printf("Location: %d\tValue: %f\n",(y * yNewStride) + (x * xNewStride),outData[(y * yNewStride) + (x * xNewStride)]);
                continue;
              }
              continue;
            }
            
          }
        }
      }
    }
    
    //Handle other color modes
    
    break;
  
    case (HexMapSquareToHex):
      
      break;
   
  }
    //printf("Done mapping\n");
  
  return;
}

/** Callback function that is called by the NDArray driver with new NDArray data.
  * Grabs the current NDArray and applies the selected transforms to the data.  Apply the transforms in order.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginHexMap::processCallbacks(NDArray *pArray){
  NDArray *pScratch=NULL;
  NDArray *pTransformed=NULL;
  NDArrayInfo arrayInfo;

  unsigned int numRows, rowSize;

  static const char* functionName = "processCallbacks";


  /* Call the base class method */
  NDPluginDriver::processCallbacks(pArray);

  /* Do the computationally expensive code with the lock released */
  this->unlock();
  
  /* Copy Array */
  
  pScratch= this->pNDArrayPool->copy( pArray, NULL, 1);
  //pTransformed= this->pNDArrayPool->copy( pArray, NULL, 1);
  
  pScratch->getInfo(&arrayInfo);

 
  this->lock();
  
  pTransformed=HexMapImage(pScratch, &arrayInfo);
  
  //printf("Done with Mapping...\n");
  
  int arrayCallbacks = 0;
  getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
  if (arrayCallbacks == 1) {
//    inData  = (unsigned char *)detected_edges.data;
//    outData = (unsigned char *)pScratch->pData;
//    memcpy(outData, inData, arrayInfo.nElements * sizeof( *inData));
    //printf("Getting Attritubes\n");
    getAttributes(pTransformed->pAttributeList);
    this->unlock();
    //printf("Doing Callback\n");
    doCallbacksGenericPointer(pTransformed, NDArrayData, 0);
    this->lock();
  }

  if (NULL != pScratch)
    pScratch->release();
  if (pTransformed!=NULL)
    pTransformed->release();
  callParamCallbacks();
}


/** HexMap the image according to the selected choice.*/  
NDArray* NDPluginHexMap::HexMapImage(NDArray *inArray, NDArrayInfo_t *arrayInfo)
{
  static const char *functionName = "HexMapImage";
  int colorMode;
  int transformType;
  int newX, newY;
  double hexMapPitch; 
  NDArray *outArray=NULL;
  NDDimension_t dimsOut[ND_ARRAY_MAX_DIMS];
  //size_t dimsOut[ND_ARRAY_MAX_DIMS];
  int xDim=0, yDim=1, colorDim=-1, nDims=0;
  size_t dims[ND_ARRAY_MAX_DIMS];
  NDArrayInfo_t OutputArrayInfo;
  
  //printf("Getting Transform Param\n");
  getIntegerParam(NDPluginHexMapType_, &transformType);
    
    
  if (transformType == HexMapNone){
    //printf("Do nothing\n");    
    outArray= this->pNDArrayPool->copy( inArray, NULL, 1);
  }
  else {
    //printf("\nStart of HexMapImage\n");
    colorMode = NDColorModeMono;
    //printf("Getting ColorMode Param\n");
    getIntegerParam(NDColorMode, &colorMode);
    
    //printf("Getting new size Param\n");
    getIntegerParam(NDPluginHexMapMaxNewSizeX, &newX);
    getIntegerParam(NDPluginHexMapMaxNewSizeY, &newY);
    //printf("Getting Pitch Param\n");
    getDoubleParam(NDPluginHexMapPitch, &hexMapPitch);
    //printf("Hex Pitch: %f\n",hexMapPitch);
    
    
    switch (colorMode) {
          case NDColorModeMono:
              xDim = 0;
              yDim = 1;
              break;
          case NDColorModeRGB1:
              colorDim = 0;
              xDim     = 1;
              yDim     = 2;
              break;
          case NDColorModeRGB2:
              colorDim = 1;
              xDim     = 0;
              yDim     = 2;
              break;
          case NDColorModeRGB3:
              colorDim = 2;
              xDim     = 0;
              yDim     = 1;
              break;
      }
    
    inArray->initDimension(&dimsOut[xDim], newX);
    inArray->initDimension(&dimsOut[yDim], newY);
    if (inArray->ndims > 2) inArray->initDimension(&dimsOut[colorDim], 3);
    pNDArrayPool->convert(inArray, &outArray, NDFloat64, dimsOut);
    outArray->pAttributeList->updateValues();
   
    switch (inArray->dataType) {
      case NDInt8:
        HexMapNDArray<epicsInt8, epicsFloat64>(inArray, outArray, transformType, colorMode, newX, newY, hexMapPitch, arrayInfo);
        break;
      case NDUInt8:
        HexMapNDArray<epicsUInt8, epicsFloat64>(inArray, outArray, transformType, colorMode, newX, newY, hexMapPitch, arrayInfo);
        break;
      case NDInt16:
        HexMapNDArray<epicsInt16, epicsFloat64>(inArray, outArray, transformType, colorMode, newX, newY, hexMapPitch, arrayInfo);
        break;
      case NDUInt16:
        HexMapNDArray<epicsUInt16, epicsFloat64>(inArray, outArray, transformType, colorMode, newX, newY, hexMapPitch, arrayInfo);
        break;
      case NDInt32:
        HexMapNDArray<epicsInt32, epicsFloat64>(inArray, outArray, transformType, colorMode, newX, newY, hexMapPitch, arrayInfo);
        break;
      case NDUInt32:
        HexMapNDArray<epicsUInt32, epicsFloat64>(inArray, outArray, transformType, colorMode, newX, newY, hexMapPitch, arrayInfo);
        break;
      case NDFloat32:
        HexMapNDArray<epicsFloat32, epicsFloat64>(inArray, outArray, transformType, colorMode, newX, newY, hexMapPitch, arrayInfo);
        break;
      case NDFloat64:
        HexMapNDArray<epicsFloat64, epicsFloat64>(inArray, outArray, transformType, colorMode, newX, newY, hexMapPitch, arrayInfo);
        break;
    }
  }
  outArray->pAttributeList->updateValues();
  return outArray;
  
}


/** Constructor for NDPluginHexMap; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * HexMap parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *      NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *      at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *      0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *      of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *      allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *      allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginHexMap::NDPluginHexMap(const char *portName, int queueSize, int blockingCallbacks,
             const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory,
             int priority, int stackSize)
  /* Invoke the base class constructor */
  : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_HEXMAP_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
  static const char *functionName = "NDPluginHexMap";
  int i;
  
  createParam(NDPluginHexMapTypeString,          asynParamInt32,   &NDPluginHexMapType_);
  //createParam(NDPluginHexMapMaxSizeXString,      asynParamInt32,   &NDPluginHexMapMaxSizeX);
  //createParam(NDPluginHexMapMaxSizeYString,      asynParamInt32,   &NDPluginHexMapMaxSizeY);
  createParam(NDPluginHexMapMaxNewSizeXString,   asynParamInt32,   &NDPluginHexMapMaxNewSizeX);
  createParam(NDPluginHexMapMaxNewSizeYString,   asynParamInt32,   &NDPluginHexMapMaxNewSizeY);
  createParam(NDPluginHexMapHexPitchString,      asynParamFloat64, &NDPluginHexMapPitch);
  
  for (i = 0; i < ND_ARRAY_MAX_DIMS; i++) {
    this->userDims_[i] = i;
  }
  
  /* Set the plugin type string */
  setStringParam(NDPluginDriverPluginType, "NDPluginHexMap");
  setIntegerParam(NDPluginHexMapType_, HexMapNone);

  // Enable ArrayCallbacks.  
  // This plugin currently ignores this setting and always does callbacks, so make the setting reflect the behavior
  setIntegerParam(NDArrayCallbacks, 1);

  /* Try to connect to the array port */
  connectToArrayPort();
}

/** Configuration command */
extern "C" int NDHexMapConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                    const char *NDArrayPort, int NDArrayAddr,
                                    int maxBuffers, size_t maxMemory,
                                    int priority, int stackSize)
{
  new NDPluginHexMap(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
              maxBuffers, maxMemory, priority, stackSize);
  return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8};
static const iocshFuncDef initFuncDef = {"NDHexMapConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDHexMapConfigure(args[0].sval, args[1].ival, args[2].ival,
                       args[3].sval, args[4].ival, args[5].ival,
                       args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDHexMapRegister(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
  epicsExportRegistrar(NDHexMapRegister);
}
