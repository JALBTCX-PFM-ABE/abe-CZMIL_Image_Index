
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.

*********************************************************************************************/

#ifndef CZMIL_IMAGE_CONVERT_H
#define CZMIL_IMAGE_CONVERT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#include "nvutility.h"
#include "nvutility.hpp"
#include "czmil.h"


#include <QtCore>
#include <QSharedMemory>

#include "version.hpp"


class CZMIL_Image_Index : QObject
{
  Q_OBJECT


public:

  CZMIL_Image_Index (int32_t argc = 0, char **argv = NULL);
  ~CZMIL_Image_Index ();


protected:

  void usage ();


protected slots:


};

#endif
