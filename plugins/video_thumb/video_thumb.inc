<?php
//$Id$

/**
 * @file
 * Enable image support for video module.
 *
 * @author Heshan Wanigasooriya <heshan at heidisoft.com><heshanmw at gmail dot com>
 */



function video_thumb_auto_thumbnail($vidfile) {
  module_load_include('inc', 'video', '/plugins/video_ffmpeg/ffmpeg');
  if(module_hook('ffmpeg', 'auto_thumbnail')) {
    return ffmpeg_auto_thumbnail($vidfile);
  }
  return false;
 }

 