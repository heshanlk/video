<?php
/**
 * @file
 * This file will be used to keep all utility functions data structures.
 */

/**
 * Adds a preview of thumbnails for you to select when uploading videos.
 */
function video_thumb_process(&$element, &$form_state) {
  $file = $element['#value'];
  $fid = $file['fid'];
  $field = field_widget_field($element, $form_state);

  if (isset($element['preview']) && $fid != 0) {
    $gen_fail = FALSE;

    if (in_array($field['settings']['autothumbnail'], array('auto', 'auto_fallback'))) {
      $default_thumb = array();
      $transcoder = new Transcoder;
      if ($thumbs = $transcoder->extractFrames($file)) {
        $default_thumb = array_rand($thumbs);
        if (!empty($thumbs)) {
          foreach ($thumbs as $img) {
            // if file object contain url then use file name to identify object
            $thumbss[$img->fid] = theme('image_style', array('style_name' => $field['settings']['preview_video_thumb_style'], 'path' => $img->uri));
          }
        }
      }

      if (!empty($thumbss)) {
        $element['thumbnail'] = array(
          '#type' => 'radios',
          '#title' => t('Video thumbnails'),
          '#options' => $thumbss,
          '#default_value' => !empty($file['thumbnail']) ? $file['thumbnail'] : $thumbs[$default_thumb]->fid,
          '#weight' => 10,
          '#attributes' => array('class' => array('video-thumbnails'), 'onchange' => 'Drupal.behaviors.videoEdit()', 'rel' => 'video_large_thumbnail-' . $fid),
        );
      }
      else {
        $gen_fail = TRUE;
      }
    }

    // if fail creating thumbnails fall back to manual or manual upload
    if ($gen_fail || $field['settings']['autothumbnail'] == 'auto_fallback' || $field['settings']['autothumbnail'] == 'manual_upload') {
      $file += (array) file_load($fid);
      $scheme = file_uri_scheme($file['uri']);
      $element['thumbnail'] = array(
        '#title' => t('Video thumbnail'),
        '#type' => 'managed_file',
        '#description' => t('The uploaded image will be used as video thumbnail on this video.'),
        '#default_value' => NULL,
        '#upload_location' => $scheme . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $fid,
        '#weight' => 10,
      );

      // Set the current thumbnail fid, if it is not the default one
      if (!empty($file['thumbnail'])) {
        if (empty($field['settings']['default_video_thumbnail']) || $file['thumbnail'] != $field['settings']['default_video_thumbnail']['fid']) {
          $element['thumbnail']['#default_value'] = $file['thumbnail'];
        }
        else {
          // Unset the form_state value for the thumbnail field if it is the default thumbnail.
          // This prevents the "may not be referenced" error.
          form_set_value($element, NULL, $form_state);
        }
      }
    }

    // if no thumbnail creation
    if ($field['settings']['autothumbnail'] == 'no') {
      $element['thumbnail'] = array(
        '#type' => 'value',
        '#value' => NULL,
      );
    }

    // Setup our large thumbnail that is on the left.
    $large_thumb = NULL;
    if (isset($file['thumbnail']['fid']) && is_array($file['thumbnail'])) {
      $large_thumb = file_load($file['thumbnail']['fid']);
    }
    elseif (isset($file['thumbnail']['fid']) && !empty($file['thumbnail'])) {
      $large_thumb = file_load($file['thumbnail']);
    }
    elseif (!empty($field['settings']['default_video_thumbnail']['fid'])) {
      $large_thumb = file_load($field['settings']['default_video_thumbnail']['fid']);
    }

    if (!empty($large_thumb)) {
      $element['preview']['#suffix'] = '<div class="video_large_thumbnail-' . $fid . '">' . theme('image_style', array('style_name' => $field['settings']['preview_video_thumb_style'], 'path' => $large_thumb->uri)) . '</div>';
    }
  }
}

/**
 * Process elements loads on settings
 */
function video_widget_element_settings(&$element, &$form_state) {
  $file = $element['#value'];
  $delta = $element['#delta'];

  $field = field_widget_field($element, $form_state);
  $instance = field_widget_instance($element, $form_state);
  $settings = $instance['settings'];
  // Check if using the default width and replace tokens.
  $default_dimensions = user_access('override player dimensions');
  $description = t('Set your video dimensions.  This will create your player with these dimensions.');
  // Setup our default dimensions.
  $dimensions = $settings['default_dimensions'];
  // Lets figure out our dimensions for our video and add astericks next to our options.
  $options = video_explode("\n", variable_get("video_metadata_dimensions", video_default_dimensions()));
  if ($field['settings']['autoconversion'] && isset($element['preview']) &&
      $file['fid'] != 0 && $default_dimensions) {
    $file_object = file_load($file['fid']);
    $video_info = _video_dimensions_options($options, $file_object);
    $description = t('Set your video dimensions.  This will create your player
      and transcode your video with these dimensions.  Your video size is <b>!size</b>,
      if you choose a higher resolution, this could cause video distortion.
      You are shown dimensions that match your aspect ratio,
      if you choose dimensions that do not match your ratio, we will pad your
      video by adding black bars on either the top or bottom while maintaining
      your videos original aspect ratio.', array('!size' => $video_info['width'] . 'x' . $video_info['height']));
    // setup our default display of dimensions.
    // lets go through our options looking for a matching resolution
    foreach ($options as $key => $value) {
      if (stristr($value, t('(Matches Resolution)')) == TRUE) {
        $dimensions = $key;
        break;
      }
    }
  }
  // Override our dimensions to the user selected.
  if (isset($file['dimensions']) && !empty($file['dimensions'])) {
    $dimensions = $file['dimensions'];
  }

  // show only enabled the autoconversion
  if ($field['settings']['autoconversion']) {
    $element['dimensions'] = array(
      '#type' => 'select',
      '#title' => t('Output video dimensions'),
      '#default_value' => $dimensions,
      '#description' => $description,
      '#options' => $options,
    );
  }
  // If users cannot change the default dimensions, lets change this to a value.
  if (!$default_dimensions) {
    $element['dimensions']['#type'] = 'value';
    $element['dimensions']['#value'] = $dimensions;
  }

  // only in preview mode and then create thumbnails
  if ($field['settings']['autoconversion']) {
    // check if already converted or failed
    $factory = new TranscoderAbstractionAbstractFactory();
    $transcoder = $factory->getProduct();
    $transcoder->setInput($file);
    $video = $transcoder->loadJob();
    if (user_access('re convert video') && isset($video->video_status) && ($video->video_status == VIDEO_RENDERING_COMPLETE || $video->video_status == VIDEO_RENDERING_FAILED)) {
      $status = array(VIDEO_RENDERING_COMPLETE => 'completed', VIDEO_RENDERING_FAILED => 'failed');
      $element['re_convert_video'] = array(
        '#type' => 'checkbox',
        '#title' => t('Video conversion has been <b>' . $status[$video->video_status] . '!</b>. Re-queue video?.'),
        '#description' => t('This will re-convert your video to output formats when you save, or schedule it for cron.'),
        '#attributes' => array('class' => array('video-re-convert', 'video-' . $video->video_status)),
      );
    }
    if (user_access('bypass conversion video')) {
      $element['bypass_autoconversion'] = array(
        '#type' => 'checkbox',
        '#title' => t('Bypass auto conversion'),
        '#default_value' => isset($file['bypass_autoconversion']) ? $file['bypass_autoconversion'] : variable_get('video_bypass_conversion', FALSE),
        '#description' => t('This will bypass the auto-conversion of videos.'),
        '#attributes' => array('class' => array('video-bypass-auto-conversion')),
      );
    }
    elseif (variable_get('video_bypass_conversion', FALSE)) {
      $element['bypass_autoconversion'] = array(
        '#type' => 'value',
        '#value' => variable_get('video_bypass_conversion', FALSE),
      );
    }
    // check this to convert the video on save
    if (user_access('convert on submission')) {
      $element['convert_video_on_save'] = array(
        '#type' => 'checkbox',
        '#title' => t('Convert video on save'),
        '#default_value' => variable_get('video_convert_on_save', FALSE),
        '#description' => t('This will convert your video to flv format when you save, instead of scheduling it for cron.'),
        '#attributes' => array('class' => array('video-convert-video-on-save')),
      );
    }
    elseif (variable_get('video_convert_on_save', FALSE)) {
      $element['convert_video_on_save'] = array(
        '#type' => 'value',
        '#value' => variable_get('video_convert_on_save', FALSE),
      );
    }
  }

  // use of default thumbnail
  if (!empty($field['settings']['default_video_thumbnail']['fid'])) {
    $defaultfid = $field['settings']['default_video_thumbnail']['fid'];

    $default_thumb = (isset($file['thumbnail']) && $defaultfid == $file['thumbnail']) || variable_get('video_use_default_thumb', FALSE);
    if (user_access('use default thumb')) {
      $element['use_default_video_thumb'] = array(
        '#type' => 'checkbox',
        '#title' => t('Use the default thumbnail for this video'),
        '#default_value' => $default_thumb,
        '#description' => t('This will set a flag for this video to use the default video thumbnail when output.'),
        '#attributes' => array('class' => array('video-use-default-video-thumb')),
      );
      if ($default_thumb) {
        $element['use_default_video_thumb']['#attributes']['checked'] = 'checked';
      }
    }
    else {
      $element['use_default_video_thumb'] = array(
        '#type' => 'value',
        '#value' => $default_thumb,
      );
    }
  }
}

/**
 * Compares passed extensions with normal video web extensions.
 */
function video_web_extensions($ext) {
  $extensions = array_filter(explode(' ', $ext));
  $web_extensions = array(
    'mov', 'mp4', '3gp', '3g2', 'mpg', 'mpeg', 'm4v', // quicktime
    'divx', 'mkv', // divx
    'rm', // realplayer
    'flv', 'f4v', // flash player
    'swf', // swf player
    'dir', 'dcr', // dcr player
    'asf', 'wmv', 'avi', 'mpg', 'mpeg', // windows media
    'ogg', 'ogv', 'webm' // ogg/ogv theora
  );

  return count(array_diff($extensions, $web_extensions)) == 0;
}

/**
 * Updates our option list to show matching aspect ratios matching resolutions
 *
 * We will update the options array by reference and return the aspect ratio
 * of the file.
 */

function _video_dimensions_options(&$options, $video) {
  $aspect_ratio = _video_aspect_ratio($video);
  // loop through our options and find matching ratio's and also the exact width/height
  foreach ($options as $key => $value) {
    $wxh = explode('x', $value);
    // lets check our width and height first
    if ($aspect_ratio['width'] == $wxh[0] && $aspect_ratio['height'] == $wxh[1]) {
      $options[$key] = $value . ' ' . t('(Matches Resolution)');
    }
    else {
      // now lets check our ratio's
      $ratio = number_format($wxh[0] / $wxh[1], 4);
      if ($ratio == $aspect_ratio['ratio']) {
        $options[$key] = $value . ' ' . t('(Matches Ratio)');
      }
    }
  }
  return $aspect_ratio;
}

/**
 * Returns the width/height and aspect ratio of the video
 *
 * @todo: move this to the transcoder class instead?
 */
function _video_aspect_ratio($video) {
  // lets get our video dimensions from the file
  $factory = new TranscoderAbstractionAbstractFactory();
  $transcoder = $factory->getProduct();
  $transcoder->setInput((array) $video);
  $wxh = $transcoder->getFileInfo();
  $width = $wxh['video']['dimensions']['width'];
  $height = $wxh['video']['dimensions']['height'];
  if (!$width || !$height) {
    // no width and height found just return.
    watchdog('video_conversion', 'We could not determine the height and width of the video', array(), WATCHDOG_DEBUG);
//    drupal_set_message(t('The system counld not determine the width and height of your video: !video.  If transcoding, the system could have problems.', array('!video' => $video)));
    return;
  }

  // now lets get aspect ratio and compare our options in the select dropdown then add an asterick if any to each option representing a matching aspect ratio.
  $ratio = number_format($width / $height, 4);
  $aspect_ratio = array(
    'width' => $width,
    'height' => $height,
    'ratio' => $ratio,
  );
  return $aspect_ratio;
}

/*
 * Return our list of video extensions and their associated player.
 */

function video_video_extensions() {
  $extensions = array(
    'divx' => 'video_play_divx',
    'mkv' => 'video_play_divx',
    'mov' => 'video_play_quicktime',
    '3gp' => 'video_play_quicktime',
    '3g2' => 'video_play_quicktime',
    'mp4' => 'video_play_quicktime',
    'm4v' => 'video_play_quicktime',
    'rm' => 'video_play_realmedia',
    'f4v' => 'video_play_flv',
    'flv' => 'video_play_flv',
    'swf' => 'video_play_flash',
    'dir' => 'video_play_dcr',
    'dcr' => 'video_play_dcr',
    'asf' => 'video_play_windowsmedia',
    'wmv' => 'video_play_windowsmedia',
    'avi' => 'video_play_windowsmedia',
    'mpg' => 'video_play_windowsmedia',
    'mpeg' => 'video_play_windowsmedia',
    'ogg' => 'video_play_theora',
    'ogv' => 'video_play_theora',
    'webm' => 'video_play_theora'
  );
  return $extensions;
}

/*
 * Return our supported video players.
 */

function video_video_players() {
  $players = array(
    'video_play_html5' => t('HTML5 Player'),
    'video_play_divx' => t('Divx Player'),
    'video_play_quicktime' => t('Quicktime'),
    'video_play_realmedia' => t('Real Media Player'),
    'video_play_flv' => t('FLV Flash Players'),
    'video_play_flash' => t('SWF Flash Player'),
    'video_play_dcr' => t('Director/Shockwave'),
    'video_play_windowsmedia' => t('Windows Media Player'),
    'video_play_theora' => t('Theora Player'),
  );
  return $players;
}

/**
 * Return our possible flash players.
 *
 * When extending this method, also change the error message in
 * video_players_admin_settings().
 */
function video_video_flv_players() {
  $options = array();
  if (module_exists('flowplayer')) {
    $options['flowplayer'] = t('Flowplayer');
  }
  if (module_exists('jw_player')) {
    $options['jwplayer'] = t('JW Player');
  }
  return $options;
}

/*
 * Return our possible flash players.
 */

function video_video_html5_players() {
  $options = array();
  if (module_exists('videojs')) {
    $options['videojs'] = t('Video.js');
  }
  if (module_exists('video')) {
    $options['video'] = t('Video for everybody');
  }
  return $options;
}

/**
 * Field instances settings
 */
function video_default_instance_settings($widget) {
  $form = array();
  // Default video settings.
  $form['default_dimensions'] = array(
    '#type' => 'select',
    '#title' => t('Output video dimensions'),
    '#default_value' => !empty($widget['default_dimensions']) ? $widget['default_dimensions'] : '',
    '#options' => video_explode("\n", variable_get("video_metadata_dimensions", video_default_dimensions())),
    '#description' => t('This setting can be overridden in node edit or preset settings. Select the most suitable values to use as output dimensions, as selecting a bad value could cause video conversion to fail.'),
    '#weight' => 16,
  );
  return $form;
}

/**
 * Default video field settings.
 */
function video_default_field_settings($settings) {
  $form = array();
  // Video auto conversion.
  $form['autoconversion'] = array(
    '#type' => 'checkbox',
    '#title' => t('<b>Enable auto video conversion.</b>'),
    '#description' => t('Convert videos automatically using FFmpeg or Zencoder. You can define presets at !preset to automatically convert videos to web compatible formats eg. FLV, MP4. Make sure to configure your !settings to make this work properly.', array(
      '!settings' => l(t('transcoder settings'), 'admin/config/media/video/transcoders'),
      '!preset' => l(t('preset settings'), 'admin/config/media/video/presets'),
    )),
    '#default_value' => isset($settings['autoconversion']) ? $settings['autoconversion'] : '',
    '#weight' => 17,
  );
  // video auto thumbnails.
  $thumb_options = array(
    'auto' => 'Automatically extract thumbnails from video (with fallback to manual upload)',
//    'auto_fallback' => 'Automatically extract thumbnails, with fallback to manual upload (If not thumbnails extracted from video)',
    'manual_upload' => 'Manually upload a thumbnail',
    'no' => 'Don\'t create thumbnail',
  );

  $form['autothumbnail'] = array(
    '#type' => 'radios',
    '#title' => t('Video thumbnails'),
    '#options' => $thumb_options,
    '#description' => t('If you choose <i>Automatically extract thumbnails from video</i> then please make sure to configure your !settings to make this work properly.', array('!settings' => l(t('transcoder settings'), 'admin/config/media/video/transcoders'))),
    '#default_value' => isset($settings['autothumbnail']) ? $settings['autothumbnail'] : 'no',
    '#weight' => 18,
  );

  $form['default_video_thumbnail'] = array(
    '#title' => t('Default video thumbnail'),
    '#type' => 'managed_file',
    '#element_validate' => array('video_field_default_thumbnail_validate'),
    '#description' => t('You can use a default thumbnail for all videos or videos from which a thumbnail can\'t be extracted. Settings to use default video thumbnails will be available on node edit. You can change the permissions for other users too.'),
    '#default_value' => !empty($settings['default_video_thumbnail']['fid']) ? $settings['default_video_thumbnail']['fid'] : '',
    '#upload_location' => 'public://videos/thumbnails/default',
    '#weight' => 19,
  );
  $form['preview_video_thumb_style'] = array(
    '#title' => t('Preview thumbnail style'),
    '#type' => 'select',
    '#options' => image_style_options(FALSE),
    '#empty_option' => '<' . t('no preview') . '>',
    '#default_value' => !empty($settings['preview_video_thumb_style']) ? $settings['preview_video_thumb_style'] : '',
    '#description' => t('This image style will be used to show extracted video thumbnails on video node edit. Extracted thumbnail preview will also use this style.'),
    '#weight' => 20,
  );
  return $form;
}

/*
 * #options helper function to set our key=value for the form api.
 */

function video_explode($delimeter, $dimensions) {
  $options = array();
  $values = explode($delimeter, $dimensions);
  foreach ($values as $value) {
    // lets check we have a value and its in the right format
    if (!empty($value) && video_format_right($value)) {
      $options[trim($value)] = trim($value);
    }
  }
  return $options;
}

function video_format_right($value) {
  $format = explode("x", $value);
  if (!isset($format[0]) || !is_numeric(trim($format[0])))
    return FALSE;
  if (!isset($format[1]) || !is_numeric(trim($format[1])))
    return FALSE;
  return TRUE;
}

/*
 * Default video dimensions.
 */

function video_default_dimensions() {
  return "176x144\n352x288\n704x576\n1408x1152\n128x96\n160x120\n320x240\n640x480\n800x600\n1024x768\n1600x1200\n2048x1024\n1280x1024\n2560x2048\n5120x4096\n852x480\n1366x768\n1600x1024\n1920x1200\n2560x1600\n3200x2048\n3840x2400\n6400x4096\n7680x4800\n320x200\n640x350\n852x480\n1280x720\n1920x1080\n640x360\n320x180";
}

/*
 * Utility function to remove all files and directories recursively.
 */

function rmdirr($dir) {
  if ($objs = glob($dir . "/*")) {
    foreach ($objs as $obj) {
      is_dir($obj) ? rmdirr($obj) : unlink($obj);
    }
  }
  @rmdir($dir);
}

/**
 * Helper funcations
 */
function _video_object_to_array($data) {
  if (is_array($data) || is_object($data)) {
    $result = array();
    foreach ($data as $key => $value) {
      $result[$key] = _video_object_to_array($value);
    }
    return $result;
  }
  return $data;
}
