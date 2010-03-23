<?php
//$Id$

/**
 * @file
 * Enable image support for video module.
 *
 * @author Heshan Wanigasooriya <heshan at heidisoft dot com, heshanmw at gmail dot com>
 */

function video_auto_thumb_process (&$element) {
//developed for ffmpeg support
  $file = $element['#value'];
//  $field = content_fields($element['#field_name'], $element['#type_name']);
  if (isset($element['preview']) && $file['fid'] != 0) {
  // TODO : change this to use any transcode to generate thumbnails
    $transcoder = variable_get('vid_convertor', 'ffmpeg');
    module_load_include('inc', 'video', '/plugins/' . $transcoder);
    $function = variable_get('vid_convertor', 'ffmpeg') . '_auto_thumbnail';
    if (function_exists($function)) {
    //    $thumbs = ffmpeg_auto_thumbnail($file);
      $thumbs = $function($file);
    }
    else {
      drupal_set_message(t('Transcoder not configured properly'), 'error');
    }
    $default_thumb = '';
    $rnd_img = rand(1, variable_get('no_of_video_thumbs', 5));
    $default_thumb = $thumbs[$rnd_img]->filepath;

    if(is_array($thumbs)) {
      foreach($thumbs as $fid => $img) {
        $thumbss[$img->filepath] = theme('uploadfield_image', $img, '', '', array('width' => '50'), FALSE);
      }
    }
  }

  $element['data']['video_thumb'] = array(
      '#type' => 'radios',
      '#title' => t('Video Thumbnails'),
      '#options' => $thumbss,
      '#default_value' => !empty($file['data']['video_thumb']) ? $file['data']['video_thumb'] : $default_thumb,
      //      '#description' => t('Pleaes choose a thumbnail from above'),
      '#weight' => 10,
      '#attributes' => array('class' => 'video-thumbnails'),
  );
 }