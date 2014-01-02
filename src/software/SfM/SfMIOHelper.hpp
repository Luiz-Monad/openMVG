
// Copyright (c) 2012, 2013 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENMVG_SFM_IO_H
#define OPENMVG_SFM_IO_H

#include "openMVG/numeric/numeric.h"
#include "openMVG/split/split.hpp"

#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace openMVG{
namespace SfMIO{

struct CameraInfo
{
  std::string m_sImageName;
  size_t m_intrinsicId;
};

struct IntrinsicCameraInfo
{
  size_t m_w, m_h;
  float m_focal;
  Mat3 m_K;
  bool m_bKnownIntrinsic; // true if 11 or 6, else false
  std::string m_sCameraMaker, m_sCameraModel;

  bool operator() (IntrinsicCameraInfo const &ci1, IntrinsicCameraInfo const &ci2)const
  {
    bool bequal = false;
    if ( ci1.m_sCameraMaker.compare("") != 0  && ci1.m_sCameraModel.compare("") != 0 )
    {
      if ( ci1.m_sCameraMaker.compare(ci2.m_sCameraMaker) == 0
          && ci1.m_sCameraModel.compare(ci2.m_sCameraModel) == 0
          && ci1.m_w == ci2.m_w
          && ci1.m_h == ci2.m_h
          && ci1.m_focal == ci2.m_focal )
      {
        bequal = true;
      }
      else
      {
        if(m_bKnownIntrinsic)
          bequal = ci1.m_K == ci2.m_K;
      }
    }
    return !bequal;
  }
};



// Load an image file list
// One basename per line.
// It handle different scenario based on the intrinsic info of the tested image
// - a camera without exif data
// - a camera with exif data found in the database
// - a camera with exif data not found in the database
// - a camera with known intrinsic
static bool loadImageList( std::vector<CameraInfo> & vec_camImageName,
                           std::vector<IntrinsicCameraInfo> & vec_focalGroup,
                           std::string sFileName,
                           bool bVerbose = true )
{
  typedef std::set<IntrinsicCameraInfo, IntrinsicCameraInfo> setIntrinsicCameraInfo;
  setIntrinsicCameraInfo set_focalGroup;

  std::ifstream in(sFileName.c_str());
  if(!in.is_open())  {
    std::cerr << std::endl
      << "Impossible to read the specified file." << std::endl;
  }
  std::string sValue;
  std::vector<std::string> vec_str;
  while(getline( in, sValue ) )
  {
    vec_str.clear();
    IntrinsicCameraInfo intrinsicCamInfo;
    split( sValue, ";", vec_str );
    if (vec_str.size() == 1)
    {
      std::cerr << "Invalid input file" << std::endl;
      in.close();
      return false;
    }
    std::stringstream oss;
    oss.clear(); oss.str(vec_str[1]);
    size_t width, height;
    oss >> width;
    oss.clear(); oss.str(vec_str[2]);
    oss >> height;

    intrinsicCamInfo.m_w = width;
    intrinsicCamInfo.m_h = height;

    switch ( vec_str.size() )
    {
      case 3 : // a camera without exif data
      {
         intrinsicCamInfo.m_focal = -1;
         intrinsicCamInfo.m_bKnownIntrinsic = false;
         intrinsicCamInfo.m_sCameraMaker = "";
         intrinsicCamInfo.m_sCameraModel = "";
      }
      break;
      case 5 : // a camera with exif data found in the database
      {
         intrinsicCamInfo.m_focal = -1;
         intrinsicCamInfo.m_bKnownIntrinsic = false;
         intrinsicCamInfo.m_sCameraMaker = vec_str[3];
         intrinsicCamInfo.m_sCameraModel = vec_str[4];
      }
      break;
      case  6 : // a camera with exif data not found in the database
      {
         oss.clear(); oss.str(vec_str[3]);
         double focal;
         oss >> focal;
         intrinsicCamInfo.m_focal = focal;
         intrinsicCamInfo.m_bKnownIntrinsic = true;
         intrinsicCamInfo.m_sCameraMaker = vec_str[4];
         intrinsicCamInfo.m_sCameraModel = vec_str[5];

         Mat3 K;
         K << focal, 0, width / 2,
              0, focal, height / 2,
              0, 0, 1;
         intrinsicCamInfo.m_K = K;

      }
      break;
      case 12 : // a camera with known intrinsic
      {
        intrinsicCamInfo.m_bKnownIntrinsic = true;
        intrinsicCamInfo.m_sCameraMaker = intrinsicCamInfo.m_sCameraModel = "";

        Mat3 K = Mat3::Identity();

        oss.clear(); oss.str(vec_str[3]);
        oss >> K(0,0);
        oss.clear(); oss.str(vec_str[4]);
        oss >> K(0,1);
        oss.clear(); oss.str(vec_str[5]);
        oss >> K(0,2);
        oss.clear(); oss.str(vec_str[6]);
        oss >> K(1,0);
        oss.clear(); oss.str(vec_str[7]);
        oss >> K(1,1);
        oss.clear(); oss.str(vec_str[8]);
        oss >> K(1,2);
        oss.clear(); oss.str(vec_str[9]);
        oss >> K(2,0);
        oss.clear(); oss.str(vec_str[10]);
        oss >> K(2,1);
        oss.clear(); oss.str(vec_str[11]);
        oss >> K(2,2);

        intrinsicCamInfo.m_K = K;
        intrinsicCamInfo.m_focal = K(0,0); // unkown sensor size;
      }
      break;
      default :
      {
        std::cerr << "Invalid line : wrong number of arguments" << std::endl;
      }
    }

    std::pair<setIntrinsicCameraInfo::iterator, bool> ret = set_focalGroup.insert(intrinsicCamInfo);
    if ( ret.second )
    {
      vec_focalGroup.push_back(intrinsicCamInfo);
    }
    size_t id = std::distance( ret.first, set_focalGroup.end()) - 1;
    CameraInfo camInfo;
    camInfo.m_sImageName = vec_str[0];
    camInfo.m_intrinsicId = id;
    vec_camImageName.push_back(camInfo);

    vec_str.clear();
  }
  in.close();
  return !(vec_camImageName.empty());
}

} // namespace SfMIO
} // namespace openMVG

#endif // OPENMVG_SFM_INCREMENTAL_ENGINE_H

