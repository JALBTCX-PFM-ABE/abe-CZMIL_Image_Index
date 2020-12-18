
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

#include "CZMIL_Image_Index.hpp"
#include "version.hpp"

#define WEEK_OFFSET  7.0L * 86400.0L

/***************************************************************************\
*                                                                           *
*   Module Name:        CZMIL_Image_Index                                   *
*                                                                           *
*   Programmer(s):      Jan C. Depner (PFM Software)                        *
*                                                                           *
*   Date Written:       February 21, 2015                                   *
*                                                                           *
*   Purpose:            Down-sample all CZMIL .jpg files in the Camera      *
*                       folder to make them smaller and place them in a     *
*                       folder adjacent to the LiDAR data folder.  Also,    *
*                       computes a timestamp and makes a duplicate camera   *
*                       sync file with timestamp appended.                  *
*                                                                           *
\***************************************************************************/

int32_t main (int32_t argc, char **argv)
{
  new CZMIL_Image_Index (argc, argv);

  return (0);
}


void CZMIL_Image_Index::usage ()
{
  fprintf (stderr, "\nUsage: CZMIL_Image_Index DATA_FOLDER CAMERA_FOLDER\n");
  fprintf (stderr, "\nWhere:\n\n");
  fprintf (stderr, "\tDATA_FOLDER = Folder containing the CZMIL LiDAR data files\n");
  fprintf (stderr, "\t(i.e. *.cpf, *.cwf, *.csf, and *.cif files).\n");
  fprintf (stderr, "\tCAMERA_FOLDER = Folder containing the CZMIL camera images\n");
  fprintf (stderr, "\tand the CameraSync file.\n\n");
  fprintf (stderr, "IMPORTANT NOTE: Do not include a trailing file separator in the\n");
  fprintf (stderr, "DATA_FOLDER or CAMERA_FOLDER names!\n\n");
  fflush (stderr);
}


CZMIL_Image_Index::CZMIL_Image_Index (int32_t argc, char **argv)
{
  char               file_name[256], jpg_file[1024], scaled_jpg_file[1024], cam_file[1024], tim_file[1024], data_folder[1024], camera_folder[1024],
                     string[1024], ndx_camera_folder[1024];
  FILE               *cfp = NULL, *tfp = NULL;
  int32_t            percent = 0, old_percent = -1;


  if (argc < 3)
    {
      usage ();
      exit (-1);
    }


  fprintf (stdout, "\n\n %s \n\n\n", VERSION);
  fflush (stdout);


  strcpy (data_folder, argv[1]);
  strcpy (camera_folder, argv[2]);

  int32_t ld_index = QString (data_folder).lastIndexOf ("LD", -1);
  QString ncf = QString (data_folder).replace (ld_index, 2, "DC");
  strcpy (ndx_camera_folder, ncf.toLatin1 ());


  //  Make sure that we at least have matching day and time for the dataset folders.

  QString dDataSet = QString (gen_basename (data_folder)).section ('_', 3, 4);
  QString cDataSet = QString (gen_basename (camera_folder)).section ('_', 3, 4);


  if (cDataSet != dDataSet)
    {
      fprintf (stderr, "\nError, data and camera folder dates/times do not match!\n");
      strcpy (string, dDataSet.toLatin1 ());
      fprintf (stderr, "Data folder: %s\n", string);
      strcpy (string, cDataSet.toLatin1 ());
      fprintf (stderr, "Camera folder: %s\n\n", string);
      exit (-1);
    }


  dDataSet = QString (gen_basename (data_folder)).section ('_', 3, 5);
  cDataSet = QString (gen_basename (camera_folder)).section ('_', 3, 5);


  //  Get the CameraSync file for this dataset.

  strcpy (string, cDataSet.toLatin1 ());

  sprintf (cam_file, "%s%1cCameraSync_%s_0.dat", camera_folder, SEPARATOR, string);

  if ((cfp = fopen (cam_file, "r")) == NULL)
    {
      perror (cam_file);
      exit (-1);
    }


  //  Quickly read the CameraSync file to count the records so we can provide a percentage spinner.

  int32_t num_recs = 0;
  while (ngets (string, sizeof (string), cfp) != NULL)
    {
      num_recs++;
    }
  fseek (cfp, 0, SEEK_SET);


  //  Open the output CameraSync + timestamp file in the indexed camera folder (which may be the same as the camera_folder but
  //  not necessarily).

  QDir ndxCamDir (ndx_camera_folder);
  if (!ndxCamDir.exists ()) ndxCamDir.mkpath (ndx_camera_folder);

  strcpy (string, dDataSet.toLatin1 ());

  sprintf (tim_file, "%s%1cCameraSync_%s_T.dat", ndx_camera_folder, SEPARATOR, string);

  if ((tfp = fopen (tim_file, "w")) == NULL)
    {
      perror (tim_file);
      exit (-1);
    }


  //  Get the year, month, and day (for the start week computation) from the camera folder name.

  char short_name[256];
  strcpy (short_name, gen_basename (camera_folder));

  int32_t year, month, mday;
  char yymmdd[40];

  strcpy (yymmdd, QString (short_name).section ('_', 3, 3).toLatin1 ());
  sscanf (yymmdd, "%2d%2d%2d", &year, &month, &mday);


#ifdef NVWIN3X
 #ifdef __MINGW64__
  putenv("TZ=GMT");
  tzset();
 #else
  _putenv("TZ=GMT");
  _tzset();
 #endif
#else
  putenv((char *)"TZ=GMT");
  tzset();
#endif


  //  Read through the CameraSync file to get the actual time (in GPS seconds, yuck) at which each picture was taken.  We also get the 
  //  JPG file name from the CaameraSync file.

  double gps_seconds;
  int64_t prev_time = -1;
  char tcat[128];
  struct tm tm;
  time_t tv_sec, start_week = -1;
  uint8_t first = NVTrue, midnight = NVFalse;
  int32_t count = 0;

  while (ngets (string, sizeof (string), cfp) != NULL)
    {
      //  The JPG filename is the second field of the record.

      strcpy (file_name, QString (string).simplified ().section (' ', 1, 1).toLatin1 ());


      sprintf (jpg_file, "%s%1c%s", camera_folder, SEPARATOR, file_name);


      QImage *full_res_image = new QImage ();

      if (!full_res_image->load (QString (jpg_file)))
        {
          fprintf (stderr, "%s %s %s %d - %s - %s\n", argv[0], __FILE__, __FUNCTION__, __LINE__, jpg_file, strerror (errno));
        }
      else
        {
          //  Get the scaled size.

          int32_t new_width = 1024;
          int32_t new_height = NINT (((float) full_res_image->height () / (float) full_res_image->width ()) * (float) new_width);


          QImage scaled_image = full_res_image->scaled (new_width, new_height, Qt::KeepAspectRatio, Qt::SmoothTransformation);


          //  Write out the jpeg file.

          QFileInfo jpgFileInfo = QFileInfo (jpg_file);

          char fname[256];
          strcpy (fname, jpgFileInfo.baseName ().toLatin1 ());

          sprintf (scaled_jpg_file, "%s%1c%s_scaled.jpeg", ndx_camera_folder, SEPARATOR, fname);

          scaled_image.save (scaled_jpg_file, "jpg");

          delete (full_res_image);
        }


      gps_seconds = QString (string).simplified ().section (' ', 12, 12).toDouble ();


      //  The first record needs to define our start week time so we can handle week end changes.

      if (first)
        {
          first = NVFalse;


          //  tm struct wants years since 1900!!!

          tm.tm_year = year + 100;
          tm.tm_mon = month - 1;
          tm.tm_mday = mday;
          tm.tm_hour = 0.0;
          tm.tm_min = 0.0;
          tm.tm_sec = 0.0;
          tm.tm_isdst = -1;


          //  Get seconds from the epoch (01-01-1970) for the date in the filename. This will also give us the day of the week for the GPS seconds of
          //  week calculation.

          tv_sec = mktime (&tm);


          //  Subtract the number of days since Saturday midnight (Sunday morning) in seconds.

          tv_sec = tv_sec - (tm.tm_wday * 86400);
          start_week = tv_sec;
        }


      int64_t picture_time = (int64_t) (((double) start_week + gps_seconds) * 1000000.0);


      //  Check for end of week rollover.

      if (picture_time < prev_time) midnight = NVTrue;

      prev_time = picture_time;


      if (midnight) picture_time += ((int64_t) WEEK_OFFSET * 1000000);


      sprintf (tcat, "    ""%" PRId64, picture_time);
      strcat (string, tcat);
      fprintf (tfp, "%s\n", string);


      count++;


      //  Print a percent complete message

      percent = NINT (((float) count / (float) num_recs) * 100.0);
      if (percent != old_percent)
        {
          fprintf (stdout, "%03d%% of files converted\r", percent);
          fflush (stdout);
          old_percent = percent;
        }
    }


  fclose (cfp);
  fclose (tfp);


  fprintf (stdout, "100%% of files converted\n\n");
  fflush (stdout);
}


CZMIL_Image_Index::~CZMIL_Image_Index ()
{
}
