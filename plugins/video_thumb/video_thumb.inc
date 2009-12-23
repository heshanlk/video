<?php
//$Id$

/**
 * @file
 * Enable image support for video module.
 *
 * @author Heshan Wanigasooriya <heshan at heidisoft.com><heshanmw at gmail dot com>
 */

function video_thumb_process_api(&$element) {
  $file = $element['#value'];
  $field = content_fields($element['#field_name'], $element['#type_name']);
  if (isset($element['preview']) && $file['fid'] != 0) {
    module_load_include('inc', 'video', '/plugins/video_ffmpeg/ffmpeg');
    $thumbs = ffmpeg_auto_thumbnail($file);
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

  // default image default_image
  // use default thumb
  // TODO : extend permission to override node wise thumbnail
  if (user_access('use default thumbnail') && !empty($field['widget']['default_video_thumb'])) {
    $element['data']['use_default_video_thumb'] = array(
        '#type' => 'checkbox',
        '#title' => t('Override Video Thumbnail with Default'),
        '#default_value' => !empty($file['data']['use_default_video_thumb']) ? $file['data']['use_default_video_thumb'] : FALSE,
        '#description' => t('If you want to use default image instead of using actual thumbnail of video then check.'),
        '#weight' => 9,
        '#attributes' => array('class' => 'video-default-thumbnail'),
    );
  }
 }