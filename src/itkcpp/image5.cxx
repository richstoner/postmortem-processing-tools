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

#include "itkImage.h"
#include "itkImportImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkTIFFImageIO.h"
#include "itkShrinkImageFilter.h"


#define _USE_MATH_DEFINES
#include <math.h>


typedef itk::Image<unsigned short, 3> ImageType;
//void CreateGauss(ImageType::Pointer image);
void CreateGauss(ImageType::Pointer image, float radius);

void CreateGauss(ImageType::Pointer image, float radius)
{
	ImageType::IndexType start;
	start.Fill(0);

	int diameter = radius*2;

	std::cout << diameter;

	ImageType::SizeType size;
	size[0] = diameter;
	size[1] = diameter;
	size[2] = 1;

	ImageType::RegionType region;
	region.SetSize(size);
	region.SetIndex(start);
	image->SetRegions(region);
	image->Allocate();
	image->FillBuffer(0);

	itk::ImageRegionIterator<ImageType> imageIterator(image, image->GetLargestPossibleRegion());

	while(!imageIterator.IsAtEnd())
	{
		int x_0 = imageIterator.GetIndex()[0];
		int y_0 = imageIterator.GetIndex()[1];

		int x = x_0 - radius;
		int y = y_0 - radius;

		float x2 = x*x;
		float y2 = y*y;

		float x2y2 = x2+y2;

		float distance = sqrt(x2y2)/radius;

		if(distance <= 1.0)
		{
//			std::cout << x_0 << " "<< y_0 << " "<< x2 << " "<< y2 << " "<< distance << std::endl;
//			imageIterator.Set(255);

			unsigned short gau_val = 255* exp(-2*M_PI*distance*distance);
			imageIterator.Set(gau_val);
			//std::cout << gau_val << std::endl;


		}


		++imageIterator;

	}


}

int main(int argc, char * argv[])
{
    if( argc < 11 )
    {
        std::cerr << "Usage: " << std::endl;
//        std::cerr << argv[0] << "  inputcsv outputImageFile width height scale"<< std::endl;
        std::cerr << argv[0] << "  pointslist.csv outputfilename angle c_x c_y d_x d_y native_x native_y resize_scale radius" << std::endl;
        //    std::cerr << argv[1] << "  outputImageFile" << std::endl;        
        return 1;
    }

    for (int i = 0; i < argc; i++) {
        std::cout << i << " " << argv[i] << std::endl;
    }


    float angle = atof(argv[3]);
    float cx = atof(argv[4]);
    float cy = atof(argv[5]);
    float dx = atof(argv[6]);
    float dy = atof(argv[7]);
    
    float original_width = atof(argv[8]);
    float original_height = atof(argv[9]);

    float left_offset = ( 32000 - original_width) / 2;
    float top_offset = ( 24000 - original_height) / 2;

    float radius = atof(argv[11]);

    //std::cout << radius;

 


    ImageType::Pointer gaussImage = ImageType::New();
    CreateGauss(gaussImage, radius);

    ImageType::RegionType gaussianBox;
    gaussianBox.SetSize(gaussImage->GetLargestPossibleRegion().GetSize());
    ImageType::IndexType bbOrigin; // set region origin
    bbOrigin[0] = 0;
    bbOrigin[1] = 0;
    bbOrigin[2] = 0;
    gaussianBox.SetIndex(bbOrigin);

    float max_x = 0.0;
    float max_y = 0.0;
    float temp_x;
    float temp_y;
    
    std::vector<int> x_positions;
    std::vector<int> y_positions;
    std::vector<int> registered_x_positions;
    std::vector<int> registered_y_positions;

    float baseWidth = 32000.0;
    float baseHeight = 24000.0;
    float targetScale = atof(argv[10]);
    
    int newx, newy, good_x, good_y;

    float c_x_new, c_y_new, r_x, r_y;

    // printf("loading pointlist\n");
    std::ifstream inFile (argv[1]);
    std::string line;
    int linenum = 0;
    while (getline (inFile, line))
    {
        //        std::cout << "\nLine #" << linenum << ":" << std::endl;
        std::istringstream linestream(line);
        std::string item;
        int itemnum = 0;
        while (getline (linestream, item, '\t'))
        {
//            std::cout << "Item #" << itemnum << ": " << item << std::endl;
            switch(itemnum)
            {
                case 0: 
                    //                    printf("%d", atoi(item.c_str()));
                    break;
                    
                case 2:
                    temp_x = atoi(item.c_str());

                    x_positions.push_back(temp_x);
                    

                    break;
                    
                case 3:
                    temp_y = atoi(item.c_str());
                    y_positions.push_back(temp_y);

                    
                    // first things first - we now have points from ImageJ analysis with origin = top left
                    // ITK transform was applied with origin = bottom left
                    // therefore, flip y axis
                    if(x_positions.size() < 2)
                    {
                        printf("original coords from IJ: %d %d, original dims: %f %f\n", x_positions[x_positions.size()-1], y_positions[x_positions.size()-1], original_width, original_height);                    
                    }

                    // have angle, small centers, and small offsets

                    // center of rotation
                    c_x_new = baseWidth * cx / 2000;
                    c_y_new = baseHeight * cy / 1500;

                    // IJ position in padded fullsize image
                    newx = x_positions[x_positions.size()-1] + left_offset;
                    newy = y_positions[x_positions.size()-1] + top_offset;

                    // flipping for ITK
                    newy = baseHeight - newy;

                    // subtract centers
                    newx -= c_x_new;
                    newy -= c_y_new;


                    r_x = newx*cos(angle) + newy*-sin(angle);
                    r_y = newx*sin(angle) + newy*cos(angle);

                    good_x = r_x + c_x_new + dx;
                    good_y = baseHeight - (r_y + c_y_new +dy);







                
                //     //newy = baseHeight - (top_offset + y_positions[x_positions.size()-1]);

                //     // now have coordinates in ITK frame
                //     if(x_positions.size() < 2)
                //     {
                //         printf("padded IJ Coords: %d %d %f %f\n", newx, newy, left_offset, top_offset);
                //     }

                //     newy = baseHeight - newy;

                //     // now subtract center

                //     // (p - C)
                //     newx -= baseWidth/2;
                //     newy -= baseHeight/2;

                //     if(x_positions.size() < 2)
                //     {
                //     printf("centered Coords: %d %d\n", newx, newy);
                // }

                //     // now rotate

                //     // R*(p-c)
                //     r_x = newx * m00 + newy * m01;
                //     r_y = newx * m10 + newy * m11;
                    
                //     if(x_positions.size() < 2)
                //     {
                //     printf("rotated Coords: %d %d\n", r_x, r_y);
                // }

                //     // now return to center and translate by transform

                //     // R*(p-c) + C + T
                //     good_x = r_x + baseWidth/2 ;
                //     good_y = baseHeight - (r_y + baseHeight/2 );

                    


                //     if(x_positions.size() < 2)
                //     {
                //     printf("registered Coords: %d %d from %f %f\n", good_x, good_y, m02, m12);
                //     }

                    // 
                    registered_x_positions.push_back(good_x);
                    registered_y_positions.push_back(good_y);

                    if (newx > max_x) {
                       max_x = newx;
                    }

                    if (newy > max_y) {
                        max_y = newy;
                    }

                    if ((int)x_positions[x_positions.size()-1] > max_x) {
                       max_x = (int)x_positions[x_positions.size()-1];
                    }

                    if (temp_y > max_y) {
                        max_y = temp_y;
                    }

                    //printf("centered at origin: %d %d, final: %d %d\n", newx, newy, good_x, good_y);
                    // return 1;

                    break;
                    
            }
            itemnum++;                        
        }
        
        linenum++;        
    }
    
    //printf("%d %d, %d %d\n", (int)max_x, (int)max_y, (int)x_positions.size(), (int)y_positions.size());
    printf("found %d points\n", (int)x_positions.size());

    
    // set size of sprite image
    typedef itk::Image< unsigned short, 3 > ImageType;
    ImageType::Pointer image = ImageType::New();
    
    ImageType::IndexType start;
    start.Fill(0);
    
    ImageType::SizeType size;
    size[0] = baseWidth + 2*radius;
    size[1] = baseHeight + 2*radius;
    size[2] = 1;
    
    ImageType::RegionType entireImage;
    entireImage.SetSize(size);
    entireImage.SetIndex(start);
    image->SetRegions(entireImage);
    image->Allocate(); 
    
    // source iterator
    itk::ImageRegionIterator<ImageType> sourceIterator(gaussImage, gaussianBox);

    for(unsigned int i =0; i< registered_x_positions.size(); i++)
    {
        std::cout << std::flush;
      //  printf("%d of %d complete ( %2.0f%% )\n", i, (int)x_positions.size(), ((float)i / (float)x_positions.size()) * 100);

        ImageType::RegionType targetRegion;
        targetRegion.SetSize(gaussImage->GetLargestPossibleRegion().GetSize());

        ImageType::IndexType newOrigin;
        newOrigin[0] = (int)registered_x_positions[i];
        newOrigin[1] = (int)registered_y_positions[i];
        newOrigin[2] = 0;
        targetRegion.SetIndex(newOrigin);
        itk::ImageRegionIterator<ImageType> imageIterator(image, targetRegion);

        imageIterator.GoToBegin();
        sourceIterator.GoToBegin();

        while(!sourceIterator.IsAtEnd())
        {

            // Set the current pixel to white
            imageIterator.Set(imageIterator.Get() + sourceIterator.Get());

            ++sourceIterator;
            ++imageIterator;
        }
    }



    typedef itk::ShrinkImageFilter <ImageType, ImageType> ShrinkImageFilterType;
    ShrinkImageFilterType::Pointer shrinkFilter = ShrinkImageFilterType::New();
    shrinkFilter->SetInput(image);

    float shrinkFactor = 1/targetScale;

    shrinkFilter->SetShrinkFactor(0, shrinkFactor);
    shrinkFilter->SetShrinkFactor(1, shrinkFactor);
    shrinkFilter->Update();
    
    //    typedef unsigned char  PixelType;
    //    const unsigned int Dimension = 3;
    
    typedef itk::ImageFileWriter< ImageType > WriterType;
    typedef  itk::TIFFImageIO TIFFIOType;
    WriterType::Pointer writer = WriterType::New();
    TIFFIOType::Pointer tiffIO2 = TIFFIOType::New();
    tiffIO2->SetPixelType(itk::ImageIOBase::SCALAR);
    writer->SetFileName( argv[2] );
    writer->SetInput(  shrinkFilter->GetOutput()  );    
    writer->SetImageIO(tiffIO2);
    

    
    
    try
    {
        writer->Update();
    }
    catch( itk::ExceptionObject & exp )
    {
        std::cerr << "Exception caught !" << std::endl;
        std::cerr << exp << std::endl;
    }
    
    return 0;
}
