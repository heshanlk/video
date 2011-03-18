<?php

/**
 *
 * @file
 * Theme functions for the video module.
 *
 */
function theme_video_thumbnails($file, $alt = '', $title = '', $attributes = NULL, $getsize = TRUE) {
  $file = (array) $file;
  //  return $file['filepath'];
  if (!is_file($file['filepath'])) {
    return '<!-- File not found: ' . $file['filepath'] . ' -->';
  }

  if ($getsize) {
    // Use cached width and height if available.
    if (!empty($file['data']['width']) && !empty($file['data']['height'])) {
      $attributes['width'] = $file['data']['width'];
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
  return '<span></span><img ' . $attributes . ' />';
}

function theme_video_widget_preview($item) {
  $output .= '<div class="image-preview">';
  $output .= drupal_render($item['preview']);
  $output .= '</div>';
  return $item;
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
  $file = (array) $file;
  //if this is imagecache skip this as the file might not be created yet
  if (!$imagecache && !is_file($file['filepath'])) {
    return '<!-- File not found: ' . str_replace("--", "-", $file['filepath']) . '" -->';
  }

  if ($getsize && $imagecache && ($image = image_get_info($file['filepath']))) {
    $attributes['width'] = $image['width'];
    $attributes['height'] = $image['height'];
  } elseif ($getsize) {
    // Use cached width and height if available.
    if (!empty($file['data']['width']) && !empty($file['data']['height'])) {
      $attributes['width'] = $file['data']['width'];
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
  return '<img ' . $attributes . ' />';
}

/**
 * Returns HTML for an video field widget.
 *
 * @param $variables
 *   An associative array containing:
 *   - element: A render element representing the image field widget.
 *
 * @ingroup themeable
 */
function theme_video_widget($variables) {
  $element = $variables['element'];
  $output = '';
  $output .= '<div class="video-widget form-managed-file clearfix">';

  if (isset($element['preview'])) {
    $output .= '<div class="video-preview">';
    $output .= drupal_render($element['preview']);
    $output .= '</div>';
  }

  $output .= '<div class="video-widget-data">';
  if ($element['fid']['#value'] != 0) {
    $element['filename']['#markup'] .= ' <span class="file-size">(' . format_size($element['#file']->filesize) . ')</span> ';
  }
  $output .= drupal_render_children($element);
  $output .= '</div>';
  $output .= '</div>';

  return $output;
}

/**
 * Returns HTML for an image using a specific image style.
 *
 * @param $variables
 *   An associative array containing:
 *   - style_name: The name of the style to be used to alter the original image.
 *   - path: The path of the image file relative to the Drupal files directory.
 *     This function does not work with images outside the files directory nor
 *     with remotely hosted images.
 *   - alt: The alternative text for text-based browsers.
 *   - title: The title text is displayed when the image is hovered in some
 *     popular browsers.
 *   - attributes: Associative array of attributes to be placed in the img tag.
 *   - getsize: If set to TRUE, the image's dimension are fetched and added as
 *     width/height attributes.
 *
 * @ingroup themeable
 */
function theme_video_thumb_style($variables) {
  $style_name = $variables['style_name'];
  $path = $variables['path'];

  // theme_image() can only honor the $getsize parameter with local file paths.
  // The derivative image is not created until it has been requested so the file
  // may not yet exist, in this case we just fallback to the URL.
  $style_path = image_style_path($style_name, $path);
  if (!file_exists($style_path)) {
    $style_path = image_style_url($style_name, $path);
  }
  $variables['path'] = $style_path;
  return theme('image', $variables);
}