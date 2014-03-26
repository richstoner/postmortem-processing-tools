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

#include <sstream>


#include "itkImageRegistrationMethod.h"

#include "itkMeanSquaresImageToImageMetric.h"
#include "itkNormalizedCorrelationImageToImageMetric.h"
#include "itkMutualInformationImageToImageMetric.h"
#include "itkGradientDifferenceImageToImageMetric.h"

#include "itkRegularStepGradientDescentOptimizer.h"
#include "itkCenteredTransformInitializer.h"
#include "itkAffineTransform.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkTransformFileWriter.h"

#include "itkResampleImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkSubtractImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkNumericSeriesFileNames.h"
#include "itkTimeProbesCollectorBase.h"


// added for ace reg
#include "itkLinearInterpolateImageFunction.h"
#include "itkImage.h"
#include "itkCenteredRigid2DTransform.h"

//
//  The following piece of code implements an observer
//  that will monitor the evolution of the registration process.
//

#pragma mark -
#pragma mark Command iterator

#include "itkCommand.h"
class CommandIterationUpdate : public itk::Command
{
public:
    typedef     CommandIterationUpdate   Self;
    typedef     itk::Command             Superclass;
    typedef     itk::SmartPointer<Self>   Pointer;
    itkNewMacro( Self );
    
protected:
    CommandIterationUpdate() {};
    
public:
    typedef itk::RegularStepGradientDescentOptimizer OptimizerType;
    typedef   const OptimizerType *                  OptimizerPointer;
    
    void Execute(itk::Object *caller, const itk::EventObject & event)
    {
        Execute( (const itk::Object *)caller, event);
    }
    
    void Execute(const itk::Object * object, const itk::EventObject & event)
    {
        OptimizerPointer optimizer =
        dynamic_cast< OptimizerPointer >( object );
        if( ! itk::IterationEvent().CheckEvent( &event ) )
        {
            return;
        }
        
 /*       
        std::ofstream transformLog("transformLog.txt", std::ios::out | std::ios::app );
        if(!transformLog) { 
            std::cout << "Cannot open file.\n"; 
        } 
        
        transformLog << optimizer->GetCurrentIteration() << "\t";
        transformLog << optimizer->GetValue() << "\t";
        transformLog << optimizer->GetCurrentPosition() << std::endl;
        
        transformLog.close(); 
*/        
        std::cout << optimizer->GetCurrentIteration() << "\t";
        std::cout << optimizer->GetValue() << "\t";
        std::cout << optimizer->GetCurrentPosition() << std::endl;

        // Print the angle for the trace plot
        //      vnl_matrix<double> p(2, 2);
             // p[0][0] = (double) optimizer->GetCurrentPosition()[0];
        //      p[0][1] = (double) optimizer->GetCurrentPosition()[1];
        //      p[1][0] = (double) optimizer->GetCurrentPosition()[2];
        //      p[1][1] = (double) optimizer->GetCurrentPosition()[3];
        //      vnl_svd<double> svd(p);
        //      vnl_matrix<double> r(2, 2);
        //      r = svd.U() * vnl_transpose(svd.V());
        //      double angle = vcl_asin(r[1][0]);
        //      std::cout << " AffineAngle: " << angle * 180.0 / vnl_math::pi << std::endl;
    }
};


#pragma mark -
#pragma mark Rigid Body Registration

class RigidBodyRegistration
{
public:
    
    
    
    typedef itk::CenteredRigid2DTransform< double >  TransformType;
    
    RigidBodyRegistration()
    {
        this->m_OutputInterSliceTransform = TransformType::New();
        this->m_transformType = 0;
    }
    
    ~RigidBodyRegistration() {}
    
    void SetFixedImageFileName( const std::string & name )
    {
        this->m_FixedImageFilename = name;
    }
    
    void SetMovingImageFileName( const std::string & name )
    {
        this->m_MovingImageFilename = name;
    }
    
    void SetRegisteredImageFileName( const std::string & name )
    {
        this->m_RegisteredImageFileName = name;
    }
    
    const TransformType * GetOutputInterSliceTransform() const
    {
        return this->m_OutputInterSliceTransform.GetPointer();
    }
    
    void Execute();
    
    void setTransformType(int transformType)
    {
        this->m_transformType = transformType;
    }
    
private:
    
    int m_transformType;
    
    std::string  m_FixedImageFilename;
    std::string  m_MovingImageFilename;
    std::string  m_RegisteredImageFileName;
    
    TransformType::Pointer  m_OutputInterSliceTransform;
};

void RigidBodyRegistration::Execute()
{
    const    unsigned int    Dimension = 2;
    typedef  unsigned char   PixelType;
    
    typedef itk::Image< PixelType, Dimension >  FixedImageType;
    typedef itk::Image< PixelType, Dimension >  MovingImageType;    
    
    
    typedef itk::RegularStepGradientDescentOptimizer    OptimizerType;            
    typedef itk::MeanSquaresImageToImageMetric< FixedImageType, MovingImageType >    MetricType;
    
    
    
    if(this->m_transformType == 0)
    {
        std::cout << "Registration: RegStepGradient Descent + MeanSquaresImageToMetric" << std::endl;
        typedef itk::RegularStepGradientDescentOptimizer    OptimizerType;            
        typedef itk::MeanSquaresImageToImageMetric< FixedImageType, MovingImageType >    MetricType;
        
    }
    else if(this->m_transformType == 1)
    {
        std::cout << "Registration: RegStepGradient Descent + NormalizedCorrelationImage" << std::endl;
        typedef itk::RegularStepGradientDescentOptimizer    OptimizerType;            
        typedef itk::NormalizedCorrelationImageToImageMetric< FixedImageType, MovingImageType >    MetricType;
    }
    else if(this->m_transformType == 2)
    {
        std::cout << "Registration: RegStepGradient Descent + MutualInformationImage" << std::endl;
        typedef itk::RegularStepGradientDescentOptimizer    OptimizerType;            
        typedef itk::MutualInformationImageToImageMetric< FixedImageType, MovingImageType >    MetricType;
    }
    else if(this->m_transformType == 3)
    {
        std::cout << "Registration: RegStepGradient Descent + GradientDifferenceImage" << std::endl;
        typedef itk::RegularStepGradientDescentOptimizer    OptimizerType;            
        typedef itk::GradientDifferenceImageToImageMetric< FixedImageType, MovingImageType >    MetricType;
    }
    else 
    {
        //default
        std::cout << "Registration: default -> should not see this!!" << std::endl;
        typedef itk::RegularStepGradientDescentOptimizer    OptimizerType;            
        typedef itk::MeanSquaresImageToImageMetric< FixedImageType, MovingImageType >    MetricType;
    }
    
    
    typedef itk::LinearInterpolateImageFunction< MovingImageType,double>    InterpolatorType;
    
    typedef itk::ImageRegistrationMethod< FixedImageType, MovingImageType>    RegistrationType;
    
    MetricType::Pointer         metric        = MetricType::New();
    OptimizerType::Pointer      optimizer     = OptimizerType::New();
    InterpolatorType::Pointer   interpolator  = InterpolatorType::New();
    RegistrationType::Pointer   registration  = RegistrationType::New();
    
    registration->SetMetric(        metric        );
    registration->SetOptimizer(     optimizer     );
    registration->SetInterpolator(  interpolator  );
    
    TransformType::Pointer  transform = TransformType::New();
    registration->SetTransform( transform );
    
    typedef itk::ImageFileReader< FixedImageType  > FixedImageReaderType;
    typedef itk::ImageFileReader< MovingImageType > MovingImageReaderType;    
    FixedImageReaderType::Pointer  fixedImageReader  = FixedImageReaderType::New();
    MovingImageReaderType::Pointer movingImageReader = MovingImageReaderType::New();
    
    fixedImageReader->SetFileName(  this->m_FixedImageFilename );
    movingImageReader->SetFileName( this->m_MovingImageFilename );
    
    registration->SetFixedImage(    fixedImageReader->GetOutput()    );
    registration->SetMovingImage(   movingImageReader->GetOutput()   );
    fixedImageReader->Update();
    
    registration->SetFixedImageRegion(fixedImageReader->GetOutput()->GetBufferedRegion() );
    
    fixedImageReader->Update();
    movingImageReader->Update();
    
    typedef FixedImageType::SpacingType    SpacingType;
    typedef FixedImageType::PointType      OriginType;
    typedef FixedImageType::RegionType     RegionType;
    typedef FixedImageType::SizeType       SizeType;
    
    FixedImageType::Pointer fixedImage = fixedImageReader->GetOutput();
    
    const SpacingType fixedSpacing = fixedImage->GetSpacing();
    const OriginType  fixedOrigin  = fixedImage->GetOrigin();
    const RegionType  fixedRegion  = fixedImage->GetLargestPossibleRegion(); 
    const SizeType    fixedSize    = fixedRegion.GetSize();
    
    typedef itk::CenteredTransformInitializer< 
    TransformType, 
    FixedImageType, 
    MovingImageType >  TransformInitializerType;
    
    TransformInitializerType::Pointer initializer = TransformInitializerType::New();
    
    initializer->SetTransform(   transform );
    initializer->SetFixedImage(  fixedImageReader->GetOutput() );
    initializer->SetMovingImage( movingImageReader->GetOutput() );
    initializer->MomentsOn();
    initializer->InitializeTransform();
    
    registration->SetInitialTransformParameters( transform->GetParameters() );
    
    
    
    
    
    
    //    TransformType::InputPointType centerFixed;
    //    
    //    centerFixed[0] = fixedOrigin[0] + fixedSpacing[0] * fixedSize[0] / 2.0;
    //    centerFixed[1] = fixedOrigin[1] + fixedSpacing[1] * fixedSize[1] / 2.0;
    //    
    //    MovingImageType::Pointer movingImage = movingImageReader->GetOutput();
    //    
    //    const SpacingType movingSpacing = movingImage->GetSpacing();
    //    const OriginType  movingOrigin  = movingImage->GetOrigin();
    //    const RegionType  movingRegion  = movingImage->GetLargestPossibleRegion();
    //    const SizeType    movingSize    = movingRegion.GetSize();
    //    
    //    TransformType::InputPointType centerMoving;
    //    
    //    centerMoving[0] = movingOrigin[0] + movingSpacing[0] * movingSize[0] / 2.0;
    //    centerMoving[1] = movingOrigin[1] + movingSpacing[1] * movingSize[1] / 2.0;
    //    
    //    transform->SetCenter( centerFixed );
    //    transform->SetTranslation( centerMoving - centerFixed );
    //    transform->SetAngle( 0.0 );
    //    registration->SetInitialTransformParameters( transform->GetParameters() );
    //    
    //    
    //    
    //    
    //    
    
    
    
    
    typedef OptimizerType::ScalesType       OptimizerScalesType;
    OptimizerScalesType optimizerScales( transform->GetNumberOfParameters() );
    const double translationScale = 1.0 / 1000.0;
    
    optimizerScales[0] = 1.0;
    optimizerScales[1] = translationScale;
    optimizerScales[2] = translationScale;
    optimizerScales[3] = translationScale;
    optimizerScales[4] = translationScale;
    
    optimizer->SetScales( optimizerScales );
    double initialStepLength = 0.1;
    
    
    // optimizer->SetRelaxationFactor( 0.6 );
    
    optimizer->SetMaximumStepLength( initialStepLength ); 
    optimizer->SetMinimumStepLength( 0.001 );
    optimizer->SetNumberOfIterations( 50 );
    
    
    // Create the Command observer and register it with the optimizer.
    //
    CommandIterationUpdate::Pointer observer = CommandIterationUpdate::New();
    optimizer->AddObserver( itk::IterationEvent(), observer );
    
    try 
    { 
        registration->StartRegistration(); 
        std::cout << "Optimizer stop condition: "
        << registration->GetOptimizer()->GetStopConditionDescription()
        << std::endl;
    } 
    catch( itk::ExceptionObject & err ) 
    { 
        std::cerr << "ExceptionObject caught !" << std::endl; 
        std::cerr << err << std::endl; 
        return;
    } 
    
    OptimizerType::ParametersType finalParameters = 
    registration->GetLastTransformParameters();
    
    const double finalAngle           = finalParameters[0];
    const double finalRotationCenterX = finalParameters[1];
    const double finalRotationCenterY = finalParameters[2];
    const double finalTranslationX    = finalParameters[3];
    const double finalTranslationY    = finalParameters[4];
    
    const unsigned int numberOfIterations = optimizer->GetCurrentIteration();
    const double bestValue = optimizer->GetValue();
    
    
    // Print out results
    //
    const double finalAngleInDegrees = finalAngle * 180.0 / vnl_math::pi;
    
    std::cout << "Result = " << std::endl;
    std::cout << " Angle (radians)   = " << finalAngle  << std::endl;
    std::cout << " Angle (degrees)   = " << finalAngleInDegrees  << std::endl;
    std::cout << " Center X      = " << finalRotationCenterX  << std::endl;
    std::cout << " Center Y      = " << finalRotationCenterY  << std::endl;
    std::cout << " Translation X = " << finalTranslationX  << std::endl;
    std::cout << " Translation Y = " << finalTranslationY  << std::endl;
    std::cout << " Iterations    = " << numberOfIterations << std::endl;
    std::cout << " Metric value  = " << bestValue          << std::endl;
    
    transform->SetParameters( finalParameters );
    
    
    
    TransformType::MatrixType matrix = transform->GetRotationMatrix();
    TransformType::OffsetType offset = transform->GetOffset();
    
    // std::cout << "Matrix = " << std::endl << matrix << std::endl;
    // std::cout << "Offset = " << std::endl << offset << std::endl;
    
    
    size_t pos = this->m_RegisteredImageFileName.find(".");
    size_t size = this->m_RegisteredImageFileName.size();
    std::cout << this->m_RegisteredImageFileName.substr(0, size-pos).c_str() << std::endl;
    
    std::string tempstr;
    std::stringstream out;
    out << this->m_transformType;
    tempstr = out.str();
    // std::string typestr = std::string::str(this->m_transformType);
    const std::string finalXForm = this->m_RegisteredImageFileName + "." + tempstr +  ".mox";


    
    //std::ios::out | std::ios::app 
    std::ofstream transformLog(finalXForm.c_str());
    if(!transformLog) { 
        std::cout << "Cannot open file.\n"; 
    } 
    transformLog << "Matrix = " << std::endl << matrix << std::endl;
    transformLog << "Offset = " << std::endl << offset << std::endl;
    
    transformLog.close(); 
    
    
    typedef itk::ResampleImageFilter< 
    MovingImageType, 
    FixedImageType >    ResampleFilterType;
    
    TransformType::Pointer finalTransform = TransformType::New();
    
    
    
    finalTransform->SetParameters( finalParameters );
    finalTransform->SetFixedParameters( transform->GetFixedParameters() );
    
    typedef itk::TransformFileWriter TransformWriterType; 
    TransformWriterType::Pointer transformWriter = TransformWriterType::New(); 
    
    
    
    transformWriter->SetFileName( this->m_RegisteredImageFileName +  "." + tempstr +  ".txt" ); 
    transformWriter->SetInput( finalTransform ); 
    transformWriter->Update(); 
    
    
    ResampleFilterType::Pointer resample = ResampleFilterType::New();
    
    resample->SetTransform( finalTransform );
    resample->SetInput( movingImageReader->GetOutput() );
    
    resample->SetSize(    fixedImage->GetLargestPossibleRegion().GetSize() );
    resample->SetOutputOrigin(  fixedImage->GetOrigin() );
    resample->SetOutputSpacing( fixedImage->GetSpacing() );
    resample->SetOutputDirection( fixedImage->GetDirection() );
    resample->SetDefaultPixelValue( 0 );
    
    typedef itk::ImageFileWriter< FixedImageType >  WriterFixedType;
    
    WriterFixedType::Pointer      writer =  WriterFixedType::New();
    
    writer->SetFileName( this->m_RegisteredImageFileName );
    
    writer->SetInput( resample->GetOutput()   );
    
    try
    {
        writer->Update();
    }
    catch( itk::ExceptionObject & excp )
    { 
        std::cerr << "ExceptionObject while writing the resampled image !" << std::endl; 
        std::cerr << excp << std::endl; 
        return;
    } 
    return;
    
}


#pragma mark -
#pragma mark Affine Registration

class AffineRegistration
{
public:
    
    typedef itk::AffineTransform< double, 2 >  TransformType;
    
    AffineRegistration()
    {
        this->m_OutputInterSliceTransform = TransformType::New();
    }
    
    ~AffineRegistration() {}
    
    void SetFixedImageFileName( const std::string & name )
    {
        this->m_FixedImageFilename = name;
    }
    
    void SetMovingImageFileName( const std::string & name )
    {
        this->m_MovingImageFilename = name;
    }
    
    void SetRegisteredImageFileName( const std::string & name )
    {
        this->m_RegisteredImageFileName = name;
    }
    
    const TransformType * GetOutputInterSliceTransform() const
    {
        return this->m_OutputInterSliceTransform.GetPointer();
    }
    
    void Execute();
    
private:
    
    std::string  m_FixedImageFilename;
    std::string  m_MovingImageFilename;
    std::string  m_RegisteredImageFileName;
    
    TransformType::Pointer  m_OutputInterSliceTransform;
    
};


void AffineRegistration::Execute()
{
    
    std::cout << "AffineRegistration of " << std::endl;
    std::cout << this->m_FixedImageFilename  << std::endl;
    std::cout << this->m_MovingImageFilename << std::endl;
    std::cout << std::endl;
    
    itk::TimeProbesCollectorBase  chronometer;
    
    const    unsigned int    Dimension = 2;
    typedef  unsigned char   PixelType;
    
    typedef itk::Image< PixelType, Dimension >  FixedImageType;
    typedef itk::Image< PixelType, Dimension >  MovingImageType;
    
    typedef itk::RegularStepGradientDescentOptimizer       OptimizerType;
    
    typedef itk::MeanSquaresImageToImageMetric<
    FixedImageType,
    MovingImageType >    MetricType;
    
    typedef itk:: LinearInterpolateImageFunction<
    MovingImageType,
    double          >    InterpolatorType;
    
    typedef itk::ImageRegistrationMethod<
    FixedImageType,
    MovingImageType >    RegistrationType;
    
    MetricType::Pointer         metric        = MetricType::New();
    OptimizerType::Pointer      optimizer     = OptimizerType::New();
    InterpolatorType::Pointer   interpolator  = InterpolatorType::New();
    RegistrationType::Pointer   registration  = RegistrationType::New();
    
    registration->SetMetric(        metric        );
    registration->SetOptimizer(     optimizer     );
    registration->SetInterpolator(  interpolator  );
    
    registration->SetTransform( this->m_OutputInterSliceTransform );
    
    typedef itk::ImageFileReader< FixedImageType  > FixedImageReaderType;
    typedef itk::ImageFileReader< MovingImageType > MovingImageReaderType;
    
    FixedImageReaderType::Pointer  fixedImageReader  = FixedImageReaderType::New();
    MovingImageReaderType::Pointer movingImageReader = MovingImageReaderType::New();
    
    fixedImageReader->SetFileName(  this->m_FixedImageFilename );
    movingImageReader->SetFileName( this->m_MovingImageFilename );
    
    chronometer.Start("Reading");
    fixedImageReader->Update();
    movingImageReader->Update();
    chronometer.Stop("Reading");
    
    registration->SetFixedImage(    fixedImageReader->GetOutput()    );
    registration->SetMovingImage(   movingImageReader->GetOutput()   );
    
    registration->SetFixedImageRegion(
                                      fixedImageReader->GetOutput()->GetBufferedRegion() );
    
    typedef itk::CenteredTransformInitializer<
    TransformType,
    FixedImageType,
    MovingImageType >  TransformInitializerType;
    
    TransformInitializerType::Pointer initializer = TransformInitializerType::New();
    
    initializer->SetTransform( this->m_OutputInterSliceTransform );
    initializer->SetFixedImage(  fixedImageReader->GetOutput() );
    initializer->SetMovingImage( movingImageReader->GetOutput() );
    initializer->GeometryOn();
    
    chronometer.Start("Initialization");
    initializer->InitializeTransform();
    chronometer.Stop("Initialization");
    
    registration->SetInitialTransformParameters( this->m_OutputInterSliceTransform->GetParameters() );
    
    double translationScale = 1.0 / 1000.0;
    
    typedef OptimizerType::ScalesType       OptimizerScalesType;
    OptimizerScalesType optimizerScales( this->m_OutputInterSliceTransform->GetNumberOfParameters() );
    
    optimizerScales[0] =  1.0;
    optimizerScales[1] =  1.0;
    optimizerScales[2] =  1.0;
    optimizerScales[3] =  1.0;
    optimizerScales[4] =  translationScale;
    optimizerScales[5] =  translationScale;
    
    optimizer->SetScales( optimizerScales );
    
    double steplength = 0.1;
    
    unsigned int maxNumberOfIterations = 300;
    
    optimizer->SetMaximumStepLength( steplength );
    optimizer->SetMinimumStepLength( 0.0001 );
    optimizer->SetNumberOfIterations( maxNumberOfIterations );
    
    optimizer->MinimizeOn();
    
    CommandIterationUpdate::Pointer observer = CommandIterationUpdate::New();
    optimizer->AddObserver( itk::IterationEvent(), observer );
    
    try
    {
        chronometer.Start("Registration");
        registration->StartRegistration();
        chronometer.Stop("Registration");
    }
    catch( itk::ExceptionObject & err )
    {
        std::cerr << "ExceptionObject caught !" << std::endl;
        std::cerr << err << std::endl;
        return;
    }
    
    std::cout << "Optimizer stop condition: "
    << registration->GetOptimizer()->GetStopConditionDescription()
    << std::endl;
    
    OptimizerType::ParametersType finalParameters =
    registration->GetLastTransformParameters();
    
    const double finalRotationCenterX = this->m_OutputInterSliceTransform->GetCenter()[0];
    const double finalRotationCenterY = this->m_OutputInterSliceTransform->GetCenter()[1];
    const double finalTranslationX    = finalParameters[4];
    const double finalTranslationY    = finalParameters[5];
    
    const unsigned int numberOfIterations = optimizer->GetCurrentIteration();
    const double bestValue = optimizer->GetValue();
    
    std::cout << "Result = " << std::endl;
    std::cout << " Center X      = " << finalRotationCenterX  << std::endl;
    std::cout << " Center Y      = " << finalRotationCenterY  << std::endl;
    std::cout << " Translation X = " << finalTranslationX  << std::endl;
    std::cout << " Translation Y = " << finalTranslationY  << std::endl;
    std::cout << " Iterations    = " << numberOfIterations << std::endl;
    std::cout << " Metric value  = " << bestValue          << std::endl;
    
    vnl_matrix<double> p(2, 2);
    p[0][0] = (double) finalParameters[0];
    p[0][1] = (double) finalParameters[1];
    p[1][0] = (double) finalParameters[2];
    p[1][1] = (double) finalParameters[3];
    vnl_svd<double> svd(p);
    vnl_matrix<double> r(2, 2);
    r = svd.U() * vnl_transpose(svd.V());
    double angle = vcl_asin(r[1][0]);
    
    const double angleInDegrees = angle * 180.0 / vnl_math::pi;
    
    std::cout << " Scale 1         = " << svd.W(0)        << std::endl;
    std::cout << " Scale 2         = " << svd.W(1)        << std::endl;
    std::cout << " Angle (degrees) = " << angleInDegrees  << std::endl;
    
    typedef itk::ResampleImageFilter<
    MovingImageType,
    FixedImageType >    ResampleFilterType;
    
    TransformType::Pointer finalTransform = TransformType::New();
    
    finalTransform->SetParameters( finalParameters );
    finalTransform->SetFixedParameters( this->m_OutputInterSliceTransform->GetFixedParameters() );
    
    ResampleFilterType::Pointer resampler = ResampleFilterType::New();
    
    resampler->SetTransform( finalTransform );
    resampler->SetInput( movingImageReader->GetOutput() );
    
    FixedImageType::Pointer fixedImage = fixedImageReader->GetOutput();
    
    resampler->SetSize( fixedImage->GetLargestPossibleRegion().GetSize() );
    resampler->SetOutputOrigin(  fixedImage->GetOrigin() );
    resampler->SetOutputSpacing( fixedImage->GetSpacing() );
    resampler->SetOutputDirection( fixedImage->GetDirection() );
    
    resampler->SetDefaultPixelValue( 100 );
    
    chronometer.Start("Resampling");
    resampler->Update();
    chronometer.Stop("Resampling");
    
    typedef  unsigned char  OutputPixelType;
    
    typedef itk::ImageFileWriter< FixedImageType >  WriterType;
    
    WriterType::Pointer      writer =  WriterType::New();
    
    writer->SetFileName( m_RegisteredImageFileName );
    
    writer->SetInput( resampler->GetOutput() );
    
    chronometer.Start("Writing");
    writer->Update();
    chronometer.Stop("Writing");
    
    chronometer.Report( std::cout );
}


#pragma mark -
#pragma mark main






#pragma mark -
#pragma mark main


int main( int argc, char *argv[] )
{
    // expecting 4 parameters         
    //fixedImageFiles, outputImagefiles, files (from 0), type
    //argv[1] argv[2] argv[3], argv[4]
    
    // 0 -> rigid xform, MeanSquaresImageToMetric, 
    // 1 -> rigid xform, NormalizedCorrelationImageToMetric
    // 2 -> rigid xform, MutualInformationImageToImageMetric
    // 3 -> rigid xform, GradientDifferenceImageToImageMetric
    // 4 -> rigid xform, CorrelationCoefficientHistogramImageToImageMetric
    
    //    std::ofstream transformLog("transformLog.txt");
    //    
    //    if(!transformLog) { 
    //        std::cout << "Cannot open file.\n"; 
    //        return 1; 
    //    } 
    //
    //    transformLog << 10 << " " << 123.23 << std::endl; 
    //    transformLog << "This is a short text file."; 
    //    
    //    transformLog.close(); 
    
    if( argc < 4 )
    {
        std::cerr << "Invalid parameters" << std::endl;
        return EXIT_FAILURE;
    }
    
    std::ofstream transformLog("transformLog.txt");
    if(!transformLog) { 
        std::cout << "Cannot open file.\n"; 
    } 
    
    transformLog << "Transform Log: Rigid Registration V: " << argv[4] << std::endl;
    transformLog.close(); 
    
    const int transformType = atoi(argv[4]);
    
    std::cout << "Rigid registration, type: " << argv[4] << std::endl;

    typedef RigidBodyRegistration::TransformType TransformType;
    

    typedef itk::NumericSeriesFileNames    NameGeneratorType;
    NameGeneratorType::Pointer nameGenerator = NameGeneratorType::New();
    
    nameGenerator->SetSeriesFormat( argv[1] );
    nameGenerator->SetStartIndex( 0 );
    nameGenerator->SetEndIndex( atoi( argv[3] ) - 1 );
    nameGenerator->SetIncrementIndex( 1 );
    
    typedef std::vector< std::string > FileNamesType;
    const FileNamesType & nameList = nameGenerator->GetFileNames();
    FileNamesType::const_iterator nameFixed  = nameList.begin();
    FileNamesType::const_iterator nameMoving = nameList.begin();
    FileNamesType::const_iterator nameEnd    = nameList.end();
    nameMoving++;
    
    std::string format = argv[2];
    
    // output file writer
    typedef itk::NumericSeriesFileNames     OutputNameGeneratorType;
    typedef std::vector< std::string >      OutputFileNameType;
    OutputNameGeneratorType::Pointer outputGenerator = NameGeneratorType::New();
    outputGenerator->SetSeriesFormat( format.c_str() );
    outputGenerator->SetStartIndex( 0 );
    outputGenerator->SetEndIndex( atoi( argv[3] ) - 1 );
    outputGenerator->SetIncrementIndex( 1 );
    
    const OutputFileNameType & outputNameList = outputGenerator->GetFileNames();
    FileNamesType::const_iterator outputMoving = outputNameList.begin();
    
    RigidBodyRegistration registration;
    registration.setTransformType(transformType);
    
    
    outputMoving++;
    
    
    while( nameMoving != nameEnd )
    {
        
        
        // if at the beginning, use from namelist to register against
        if(nameFixed == nameList.begin())
        {
            std::cout << "Registering " << *nameMoving << " to " << *nameFixed << ", saving as " << *outputMoving << std::endl;
            
            std::ofstream transformLog("transformLog.txt", std::ios::out | std::ios::app );
            if(!transformLog) { 
                std::cout << "Cannot open file.\n"; 
            } 
            
            transformLog << *nameMoving << "\t" << *nameFixed << "\t" << *outputMoving << std::endl;
            
            
            registration.SetFixedImageFileName( *nameFixed );
            registration.SetMovingImageFileName( *nameMoving );
            
            registration.SetRegisteredImageFileName( *outputMoving );
            registration.Execute();
            
            
            nameFixed = outputMoving;
            nameMoving++;
            outputMoving++;
            
            
            
            
            
            transformLog << std::endl;
            
            transformLog.close(); 
            
        }
        else
        {
            
            std::cout << "Registering " << *nameMoving << " to " << *nameFixed << ", saving as " << *outputMoving << std::endl;            
            
            std::ofstream transformLog("transformLog.txt", std::ios::out | std::ios::app );
            if(!transformLog) { 
                std::cout << "Cannot open file.\n"; 
            } 
            
            transformLog << *nameMoving << "\t" << *nameFixed << "\t" << *outputMoving << std::endl;
            
            // else, use the previously registered image
            registration.SetFixedImageFileName( *nameFixed );
            registration.SetMovingImageFileName( *nameMoving );
            registration.SetRegisteredImageFileName( *outputMoving );
            registration.Execute();
            
            //            const TransformType* intersliceTransform = registration.GetOutputInterSliceTransform();
            
            nameFixed = outputMoving;
            nameMoving++;
            outputMoving++;
            
            //            std::ofstream transformLog("transformLog.txt", std::ios::out | std::ios::app );
            //            if(!transformLog) { 
            //                std::cout << "Cannot open file.\n"; 
            //            } 
            
            transformLog << std::endl;
            transformLog.close(); 
            
        }
    }
    
}

