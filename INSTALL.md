# Installing Video Module 5 for Drupal 6

## Requirements

### Required Drupal modules

- [CCK](http://drupal.org/project/cck)
- [Filefield](http://drupal.org/project/filefield)
- [Chaos Tools](http://drupal.org/project/ctools)
- [Libraries API](http://drupal.org/project/libraries), for Zencoder or S3

### Recommended Drupal modules

- [Flowplayer API](http://drupal.org/project/flowplayer)
- [VideoJS](http://drupal.org/project/videojs)
- [SWF Tools](http://drupal.org/project/swftools)
- [ImageCache](http://drupal.org/project/imagecache)

### Optional Drupal modules

- [FileField Sources](http://drupal.org/project/filefield_sources)

### Required software

- [FFmpeg](http://ffmpeg.org/) or another local transcoder, unless the Zencoder submodule is used, see instructions below

### Recommended software

- [flvtool2](http://www.inlet-media.de/flvtool2/)
- qt-faststart, provided by FFmpeg

## Installing the module

1. Download the module and extract it to sites/all/modules
2. Download and install optional modules, such as Flowplayer and VideoJS
3. Enable the Video module and Video FTP and / or Video Upload
4. Optionally, enable Amazon S3 on Video and / or Zencoder API on Video5. See below for installing additional libraries.
5. Go to Administer › Site configuration › Video 
6. Change the General settings to match your preferences
7. Setup players for the video types you would like to use
8. Select and configure your transcoder
9. If you are going to use transcoding, select at least one preset or add a new preset
10. Configure S3 storage if you intend to use it
11. Configure cron. If you do not want to use cron to convert videos, see below for an alternative

## Installing the Zencoder library

1. Go to https://github.com/zencoder/zencoder-php/tags
2. Download the Zencoder API library to sites/all/libraries/zencoder
3. Make sure the file sites/all/libraries/zencoder/Services/Zencoder.php exists
4. The Documentation directory is not needed
5. The version that is known to be compatible with the Zencoder module is 2.0.2 (2012-01-11)

## Installing the Amazon S3 library

1. Go to http://aws.amazon.com/sdkforphp/
2. Download the AWS SDK for PHP to sites/all/libraries/awssdk
3. Make sure the file sites/all/libraries/awssdk/sdk.class.php exists
4. The version that is known to be compatible with the Amazon S3 module is 1.5.0.1 (2011-12-21)

## Tips for a proper FFmpeg installation

When you intend to use FFmpeg to transcode videos, make sure your FFmpeg installation handles all 
the file formats before installing the module. Linux distributions using precompiled binaries, 
such as Ubuntu, usually do not provide the best compile options and it is advised to compile 
FFmpeg yourself.

The following guides will help you to get a good FFmpeg installation:

- [Ubuntu](http://ubuntuforums.org/showthread.php?t=786095): FFmpeg compilation guide
- [Ubuntu](http://ubuntuforums.org/showthread.php?t=1117283): guide to enhance your FFmpeg installation using precompiled packages
- [Gentoo](http://www.gentoo-portage.com/media-video/ffmpeg): make sure to setup the right USE flags before installing
- [Windows](http://www.videohelp.com/tools/ffmpeg): Windows packages

## Alternative for cron

Instead of the Drupal cron system, you can execute the video_scheduler.php script regularly
to process pending transcode jobs.

Be sure to run the crontab as same user account that the webserver process uses to allow
the website to delete or modify the files after transcoding.

On Ubuntu or Debian, this user is www-data. On other systems, it could also be apache or apache2.

Execute the following command to edit the cron tab for the web user:

    crontab -e -u WEBSERVERUSER

The crontab entry should look something like this:

    */20	*	*	*	*	cd /absolute/path/to/drupal/ ; php video_scheduler.php http://www.example.com/path_to_drupal

This will execute the script every 20 minutes. Make sure the script does not run too often.

Troubleshooting
---------------

Configuring and installing FFmpeg in a web server environment might be pretty
difficult. In order to help you troubleshoot the transcoding process the FFmpeg
helper puts debugging informations on the drupal logs. I strongly suggest to
have a look at them if you are experiencing problems with transcoding.

All FFmpeg commands are added to the Drupal log. You might try to rerun them on a 
command shell in order understand what went wrong.
