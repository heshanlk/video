<?php
/**
 *
 * @file
 * Theme functions for the video module.
 *
 */

function theme_video_thumbnails($file, $alt = '', $title = '', $attributes = NULL, $getsize = TRUE) {
  $file = (array)$file;
  //  return $file['filepath'];
  if (!is_file($file['filepath'])) {
    return '<!-- File not found: '. $file['filepath'] .' -->';
  }

  if ($getsize) {
    // Use cached width and height if available.
    if (!empty($file['data']['width']) && !empty($file['data']['height'])) {
      $attributes['width']  = $file['data']['width'];
      $attributes['height'] = $file['data']['height'];
    }
    // Otherwise pull the width and height from the file.
    elseif (list($width, $height, $type, $image_attributes) = @getimagesize($file['filepath'])) {
      $attributes['width'] = $width;
      $attributes['height'] = $height;
    }
  }

  if (!empty($title)) {
    $attributes['title'] = $title;
  }

  // Alt text should be added even if it is an empty string.
  $attributes['alt'] = $alt;

  // Add a timestamp to the URL to ensure it is immediately updated after editing.
  $query_string = '';
  if (isset($file['timestamp'])) {
    $query_character = (variable_get('file_downloads', FILE_DOWNLOADS_PUBLIC) == FILE_DOWNLOADS_PRIVATE && variable_get('clean_url', '0') == '0') ? '&' : '?';
    $query_string = $query_character . $file['timestamp'];
  }

  $url = file_create_url($file['filepath']) . $query_string;
  $attributes['src'] = $url;
  $attributes = drupal_attributes($attributes);
  return '<span></span><img '. $attributes .' />';
}

function theme_video_widget_preview($item) {
  return theme('filefield_widget_preview', $item);
}

function theme_video_widget_video_thumb($item = NULL) {
  return '<div class="video-thumb">' . theme('video_image', $item, '', '', '', FALSE) . '</div>';
}

/**
 * @defgroup "Theme Callbacks"
 * @{
 * @see uploadfield_theme().
 */
function theme_video_image($file, $alt = '', $title = '', $attributes = NULL, $getsize = TRUE, $imagecache = FALSE) {
  $file = (array)$file;
  //if this is imagecache skip this as the file might not be created yet
  if (!$imagecache && !is_file($file['filepath'])) {
    return '<!-- File not found: '. str_replace("--", "-", $file['filepath']) .'" -->';
  }

  if ($getsize && $imagecache && ($image = image_get_info($file['filepath']))) {
    $attributes['width'] = $image['width'];
    $attributes['height'] = $image['height'];
  }
  elseif ($getsize) {
  // Use cached width and height if available.
    if (!empty($file['data']['width']) && !empty($file['data']['height'])) {
      $attributes['width']  = $file['data']['width'];
      $attributes['height'] = $file['data']['height'];
    }
    // Otherwise pull the width and height from the file.
    elseif (list($width, $height, $type, $image_attributes) = @getimagesize($file['filepath'])) {
      $attributes['width'] = $width;
      $attributes['height'] = $height;
    }
  }

  if (!empty($title)) {
    $attributes['title'] = $title;
  }

  // Alt text should be added even if it is an empty string.
  $attributes['alt'] = $alt;

  // Add a timestamp to the URL to ensure it is immediately updated after editing.
  $query_string = '';
  if (isset($file['timestamp'])) {
    $query_character = (variable_get('file_downloads', FILE_DOWNLOADS_PUBLIC) == FILE_DOWNLOADS_PRIVATE && variable_get('clean_url', '0') == '0') ? '&' : '?';
    $query_string = $query_character . $file['timestamp'];
  }

  $url = file_create_url($file['filepath']) . $query_string;
  $attributes['src'] = $url;
  $attributes = drupal_attributes($attributes);
  return '<img '. $attributes .' />';
}