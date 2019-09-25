#include "tclap/CmdLine.h"
#include "ioutils.h"
#include "itkHistogramMatchingImageFilter.h"
#include "itkExtractImageFilter.h"
#include "itkSliceBySliceImageFilter.h"

typedef class CmdLineType
{
public:
  std::string InIm, OutputIm;
  float Thresh;
  int RefSlice;
} CmdLineType;

void ParseCmdLine(int argc, char* argv[],
                  CmdLineType &CmdLineObj
                  )
{
  using namespace TCLAP;
  try
    {
    // Define the command line object.
    CmdLine cmd("mkBrainMask ", ' ', "0.9");

    ValueArg<std::string> inArg("i","input","input image",true,"result","string");
    cmd.add( inArg );
    ValueArg<std::string> outArg("o","output","output image", true,"","string");
    cmd.add( outArg );

    ValueArg<float> tArg("t","thresh","threshold", true, 0.5,"float");
    cmd.add(tArg);

    ValueArg<int> rArg("r","refslice","reference slice", true, 0.5,"int");
    cmd.add(rArg);

    // Parse the args.
    cmd.parse( argc, argv );

    CmdLineObj.InIm = inArg.getValue();
    CmdLineObj.OutputIm = outArg.getValue();
    CmdLineObj.Thresh = tArg.getValue();
    CmdLineObj.RefSlice = rArg.getValue();
    }
  catch (ArgException &e)  // catch any exceptions
    {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    }
}

////////////////////////////////////////////////////////

template <class ImType>
void hMatch(const CmdLineType &CmdLineObj)
{
  typedef typename itk::Image<typename ImType::PixelType, ImType::ImageDimension - 1 > ImTypeSl;
  typename ImType::Pointer inputIm = readIm<ImType>(CmdLineObj.InIm);
  
  using ExtractFilterType = typename itk::ExtractImageFilter< ImType, ImTypeSl >;
  typename ExtractFilterType::Pointer extractFilter = ExtractFilterType::New();
  extractFilter->SetDirectionCollapseToSubmatrix();

  typename ImType::RegionType inputRegion = inputIm->GetBufferedRegion();
  typename ImType::SizeType size = inputRegion.GetSize();
  int zsize = size[2];
  size[2] = 0; // we extract along z direction
  typename ImType::IndexType start = inputRegion.GetIndex();
  start[2] = CmdLineObj.RefSlice;
  typename ImType::RegionType desiredRegion;
  desiredRegion.SetSize(  size  );
  desiredRegion.SetIndex( start );
  extractFilter->SetInput(inputIm);
  extractFilter->SetExtractionRegion( desiredRegion );

  typename ImTypeSl::Pointer RefSlice = extractFilter->GetOutput();
  extractFilter->Update();
  RefSlice->DisconnectPipeline();

  typedef typename itk::HistogramMatchingImageFilter<ImTypeSl, ImTypeSl> HMType;
  typename HMType::Pointer hm = HMType::New();
  hm->SetReferenceImage( RefSlice );
  hm->ThresholdAtMeanIntensityOff();
  hm->SetReferenceThreshold( CmdLineObj.Thresh );
  hm->SetSourceThreshold( CmdLineObj.Thresh );

  hm->SetNumberOfMatchPoints( 5 );
  hm->SetNumberOfHistogramLevels( 512 );

  using SlicerType = itk::SliceBySliceImageFilter<ImType, ImType, HMType>;
  typename SlicerType::Pointer slicer = SlicerType::New();

  slicer->SetInput(inputIm);
  slicer->SetFilter(hm);
  writeIm<ImType>(slicer->GetOutput(), CmdLineObj.OutputIm);
}


////////////////////////////////////////////////////////
int main(int argc, char * argv[])
{

  const int dim = 3;
  CmdLineType CmdLineObj;
  ParseCmdLine(argc, argv, CmdLineObj);

  typedef itk::Image<short, dim> ProbImType;

  hMatch<ProbImType>(CmdLineObj);
  return EXIT_SUCCESS;
}
