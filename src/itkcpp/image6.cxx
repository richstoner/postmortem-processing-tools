/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

//23202 w
//18402 h
//
//#include "itkImage.h"
//#include "itkImportImageFilter.h"
//#include "itkImageFileReader.h"
//#include "itkImageFileWriter.h"
//#include "itkTIFFImageIO.h"
//#include "itkShrinkImageFilter.h"
//
//
//#include "itkCenteredTransformInitializer.h"
//#include "itkTransformFileReader.h"
//
//#include "itkImageFileReader.h"
//#include "itkImageFileWriter.h"
//
//#include "itkResampleImageFilter.h"
//#include "itkCastImageFilter.h"
//#include "itkRescaleIntensityImageFilter.h"
//#include "itkSubtractImageFilter.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "itkTransformFileReader.h"
#include "itkTransformFileWriter.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkImage.h"
#include "itkVersorRigid3DTransform.h"

#include "itkCenteredRigid2DTransform.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkResampleImageFilter.h"



int main( int argc, char *argv[] )
{
    
    const    unsigned int    Dimension = 2;
    typedef  unsigned char           PixelType;
    
    typedef itk::Image< PixelType, Dimension >  ImageType;
    typedef itk::ImageFileReader< ImageType> ReaderType;
    typedef itk::ImageFileWriter< ImageType> WriterType;
    //    typedef itk::VersorRigid3DTransform< double > VersorRigid3DTransformType;
    
    typedef itk::CenteredRigid2DTransform<double> CenteredRigid2DTransformType;
    
    typedef itk::TransformFileReader TransformReader;
    
    typedef itk:: LinearInterpolateImageFunction< ImageType, double > LinearInterpolatorType;
    typedef itk::ResampleImageFilter<ImageType,ImageType> ResamplerType;
    
    if( argc != 5 ){
        std::cerr << "Usage: " << argv[0];
        std::cerr << " movingImageFile "; // argv[1]
        std::cerr << " transformFile ";   // argv[2]
        std::cerr << " outputImagefile";  // argv[3]
        std::cerr << " scalingfactor"; // argv[4]
        std::cerr << std::endl;
        return -1;
    }
    
    ImageType::Pointer inputImage;
    
    ReaderType::Pointer imageReader = ReaderType::New();
    imageReader->SetFileName(argv[1]);
    
    WriterType::Pointer imageWriter = WriterType::New();
    imageWriter->SetFileName(argv[3]);
    
    TransformReader::Pointer transformReader = TransformReader::New();
    transformReader->SetFileName(argv[2]);
    
    ResamplerType::Pointer resampler = ResamplerType::New();
    
    try{
        imageReader->Update();
        transformReader->Update();
    } catch(itk::ExceptionObject &e){
        std::cerr << "Failed to read file or transform: " << e << std::endl;
        return -1;
    }
    
    inputImage = imageReader->GetOutput();
    resampler->SetInput(inputImage);
    
    typedef itk::TransformFileReader::TransformListType* TransformListType;
    TransformListType transforms = transformReader->GetTransformList();
    itk::TransformFileReader::TransformListType::const_iterator
    it = transforms->begin();
    
    if (!strcmp((*it)->GetNameOfClass(), "CenteredRigid2DTransform")) {
        //        VersorRigid3DTransformType::Pointer  versorrigid_read =
        //        static_cast<VersorRigid3DTransformType*>((*it).GetPointer());
        
        CenteredRigid2DTransformType::Pointer centeredrigid_read =
        static_cast<CenteredRigid2DTransformType*>((*it).GetPointer());
        
        //        resampler->SetTransform( versorrigid_read );
        CenteredRigid2DTransformType::ParametersType finalParameters = centeredrigid_read->GetParameters();
        
        
        std::cout << "Applying this transform: " << centeredrigid_read << std::endl;

//        std::cout << finalParameters[0] << " " << finalParameters[1] << std::endl;
        
        double scalingfactor = atof(argv[4]);
        
        double finalAngle           = finalParameters[0];
        double finalRotationCenterX = finalParameters[1] * scalingfactor;
        double finalRotationCenterY = finalParameters[2] * scalingfactor;
        double finalTranslationX    = finalParameters[3] * scalingfactor;
        double finalTranslationY    = finalParameters[4] * scalingfactor;
        
        finalParameters[0] = finalAngle;
        finalParameters[1] = finalRotationCenterX;
        finalParameters[2] = finalRotationCenterY;
        finalParameters[3] = finalTranslationX;
        finalParameters[4] = finalTranslationY;
        
        centeredrigid_read->SetParameters(finalParameters);
        
        
        
        
        resampler->SetTransform (centeredrigid_read);
        
        //        std::cout << "Applying this transform: " << versorrigid_read << std::endl;
        std::cout << "Applying this transform: " << centeredrigid_read << std::endl;
    } else {
        std::cerr << "Transform " << (*it)->GetNameOfClass()
        << " is not supported" << std::endl;
        return 1;
    }
    
    LinearInterpolatorType::Pointer linearInterpolator =
    LinearInterpolatorType::New();
    resampler->SetInterpolator(linearInterpolator);
    
    resampler->SetSize(inputImage->GetLargestPossibleRegion().GetSize());
    resampler->SetOutputOrigin(inputImage->GetOrigin());
    resampler->SetOutputSpacing(inputImage->GetSpacing());
    resampler->SetDefaultPixelValue(100.0);
    
    imageWriter->SetInput(resampler->GetOutput());
    
    try{
        imageWriter->Update();
    } catch(itk::ExceptionObject &e){
        std::cerr << "Failed to save the resulting image: " << e << std::endl;
        return -1;
    }
    
    return 0;
}
//
//    
//    
//    
//    
//    
//    
//    
//    if( argc < 3 )
//    {
//        std::cerr << "Usage: " << std::endl;
//        std::cerr << argv[0] << " inputfilename outputfilename transform" << std::endl;
//
//        return 1;
//    }
//    
//    
//
//    for (int i = 0; i < argc; i++) {
//        std::cout << i << " " << argv[i] << std::endl;
//    }
//    
//    
//    const    unsigned int    Dimension = 3;
//    typedef  unsigned short  PixelType;
//    
//    typedef itk::Image< PixelType, Dimension >  FixedImageType;
//    typedef itk::Image< PixelType, Dimension >  MovingImageType;
//    
//    
//    typedef itk::ImageFileReader< FixedImageType  > FixedImageReaderType;
//    typedef itk::ImageFileWriter< ImageType> WriterType;
//
//    
//    FixedImageReaderType::Pointer  fixedImageReader  = FixedImageReaderType::New();    
//    fixedImageReader->SetFileName(  argv[1] );
//    fixedImageReader->Update();
//    
//    
//    WriterType::Pointer imageWriter = WriterType::New();
//    imageWriter->SetFileName(argv[2]);
//    
//    
//    
//    typedef itk::CenteredRigid2DTransform< double > TransformType;
//    itk::TransformFileReader::Pointer xformReader;
//    xformReader = itk::TransformFileReader::New();
//    xformReader->SetFileName( argv[3]);
//    xformReader->Update();
//    
//    typedef itk::TransformFileReader::TransformListType * TransformListType;
//    TransformListType transforms = xformReader->GetTransformList();
//    itk::TransformFileReader::TransformListType::const_iterator it;
//    it = transforms->begin();
//    
//    TransformType::Pointer myTransform;
//    
//    if(!strcmp((*it)->GetNameOfClass(),"CenteredRigid2DTransform"))
//    {
//        myTransform = static_cast<TransformType*>((*it).GetPointer());
//        printf("yay it works!\n");
//    }
//
//    
//    
// 
////    
////    
////    typedef itk::ImageFileWriter< ImageType > WriterType;
////    typedef  itk::TIFFImageIO TIFFIOType;
////    WriterType::Pointer writer = WriterType::New();
////    TIFFIOType::Pointer tiffIO2 = TIFFIOType::New();
////    tiffIO2->SetPixelType(itk::ImageIOBase::SCALAR);
////    writer->SetFileName( argv[2] );
////    writer->SetInput(  fixedImageReader->GetOutput()  );    
////    writer->SetImageIO(tiffIO2);
////    
////    try
////    {
////        writer->Update();
////    }
////    catch( itk::ExceptionObject & exp )
////    {
////        std::cerr << "Exception caught !" << std::endl;
////        std::cerr << exp << std::endl;
////    }
////    
////    return 0;
//    
//
//}
