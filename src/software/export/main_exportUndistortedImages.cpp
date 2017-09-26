// This file is part of the AliceVision project and is made available under
// the terms of the MPL2 license (see the COPYING.md file).

#include "aliceVision/sfm/sfm.hpp"
#include "aliceVision/image/image.hpp"

#include <boost/program_options.hpp>
#include <boost/progress.hpp>

#include <stdlib.h>

using namespace aliceVision;
using namespace aliceVision::camera;
using namespace aliceVision::geometry;
using namespace aliceVision::image;
using namespace aliceVision::sfm;
namespace po = boost::program_options;

int main(int argc, char *argv[])
{
  // command-line parameters

  std::string verboseLevel = system::EVerboseLevel_enumToString(system::Logger::getDefaultVerboseLevel());
  std::string sfmDataFilename;
  std::string outDirectory;

  po::options_description allParams(
    "Export undistorted images related to a sfm_data file.\n"
    "AliceVision exportUndistortedImages");

  po::options_description requiredParams("Required parameters");
  requiredParams.add_options()
    ("input,i", po::value<std::string>(&sfmDataFilename)->required(),
      "SfMData file.")
    ("output,o", po::value<std::string>(&outDirectory)->required(),
      "Output directory.");

  po::options_description logParams("Log parameters");
  logParams.add_options()
    ("verboseLevel,v", po::value<std::string>(&verboseLevel)->default_value(verboseLevel),
      "verbosity level (fatal,  error, warning, info, debug, trace).");

  allParams.add(requiredParams).add(logParams);

  po::variables_map vm;
  try
  {
    po::store(po::parse_command_line(argc, argv, allParams), vm);

    if(vm.count("help") || (argc == 1))
    {
      ALICEVISION_COUT(allParams);
      return EXIT_SUCCESS;
    }
    po::notify(vm);
  }
  catch(boost::program_options::required_option& e)
  {
    ALICEVISION_CERR("ERROR: " << e.what());
    ALICEVISION_COUT("Usage:\n\n" << allParams);
    return EXIT_FAILURE;
  }
  catch(boost::program_options::error& e)
  {
    ALICEVISION_CERR("ERROR: " << e.what());
    ALICEVISION_COUT("Usage:\n\n" << allParams);
    return EXIT_FAILURE;
  }

  // set verbose level
  system::Logger::get()->setLogLevel(verboseLevel);

  // Create output dir
  if (!stlplus::folder_exists(outDirectory))
    stlplus::folder_create( outDirectory );

  SfMData sfm_data;
  if (!Load(sfm_data, sfmDataFilename, ESfMData(VIEWS|INTRINSICS))) {
    std::cerr << std::endl
      << "The input SfMData file \""<< sfmDataFilename << "\" cannot be read." << std::endl;
    return EXIT_FAILURE;
  }

  bool bOk = true;
  {
    // Export views as undistorted images (those with valid Intrinsics)
    Image<RGBColor> image, image_ud;
    boost::progress_display my_progress_bar( sfm_data.GetViews().size() );
    for(Views::const_iterator iter = sfm_data.GetViews().begin();
      iter != sfm_data.GetViews().end(); ++iter, ++my_progress_bar)
    {
      const View * view = iter->second.get();
      bool bIntrinsicDefined = view->getIntrinsicId() != UndefinedIndexT &&
        sfm_data.GetIntrinsics().find(view->getIntrinsicId()) != sfm_data.GetIntrinsics().end();

      Intrinsics::const_iterator iterIntrinsic = sfm_data.GetIntrinsics().find(view->getIntrinsicId());

      const std::string srcImage = stlplus::create_filespec(sfm_data.s_root_path, view->getImagePath());
      const std::string dstImage = stlplus::create_filespec(
        outDirectory, stlplus::filename_part(srcImage));

      const IntrinsicBase * cam = iterIntrinsic->second.get();
      if (cam->isValid() && cam->have_disto())
      {
        // undistort the image and save it
        if (ReadImage( srcImage.c_str(), &image))
        {
          UndistortImage(image, cam, image_ud, BLACK);
          bOk &= WriteImage(dstImage.c_str(), image_ud);
        }
      }
      else // (no distortion)
      {
        // copy the image since there is no distortion
        stlplus::file_copy(srcImage, dstImage);
      }
    }
  }

  // Exit program
  if (bOk)
    return( EXIT_SUCCESS );
  else
    return( EXIT_FAILURE );
}
