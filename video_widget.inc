<?php
/**
* @file
* Common widget functions
*/

/**
 * Process elements loads on settings
 * @param array $element
 */
function video_widget_element_settings(&$element) {
  $file = $element['#value'];
  $delta = $element['#delta'];
  $field = content_fields($element['#field_name'], $element['#type_name']);
  // Check if using the default width and replace tokens.
  $default_dimensions = user_access('override player dimensions');
  $description = t('Set your video dimensions.  This will create your player with these dimensions.');
  //setup our default dimensions.
  $dimensions = $field['widget']['default_dimensions'];
  $player_dimensions = $field['widget']['default_player_dimensions'];
  // Lets figure out our dimensions for our video and add astericks next to our options.
  $options = video_explode("\n", variable_get("video_metadata_dimensions", video_default_dimensions()));
  if ($field['widget']['autoconversion'] && isset($element['preview']) && $file['fid'] != 0 && $default_dimensions) {
    $video_info = _video_dimensions_options($options, $file['filepath']);
    $description = t('Set your video dimensions. This will create your player and transcode your video with these dimensions.');
    if (!empty($video_info)) {
      $description .= t('Your video size is !size, if you choose a higher resolution, this could cause video distortion. You are shown dimensions that match your aspect ratio, if you choose dimensions that do not match your ratio, we will pad your video by adding black bars on either the top or bottom while maintaining your videos original aspect ratio.', array('!size' => $video_info['width'] .'x'. $video_info['height']));
    }

    //setup our default display of dimensions.
    //lets go through our options looking for a matching resolution
    foreach ($options as $key => $value) {
      if (stristr($value, t('(Matches Resolution)')) == TRUE) {
        $dimensions = $key;
        break;
      }
    }
  }
  // Override our dimensions to the user selected.
  if (isset($file['data']['dimensions']) && !empty($file['data']['dimensions'])) {
    $dimensions = $file['data']['dimensions'];
  }

  // Override our player dimensions to the user selected.
  if (isset($file['data']['player_dimensions']) && !empty($file['data']['player_dimensions'])) {
    $player_dimensions = $file['data']['player_dimensions'];
  }

  $element['data']['dimensions'] = array(
    '#type' => 'select',
    '#title' => t('Dimensions for Video Transcoding'),
    '#default_value' => $dimensions,
    '#description' => $description,
    '#options' => $options,
  );
  $element['data']['player_dimensions'] = array(
    '#type' => 'select',
    '#title' => t('Dimensions for Video Player'),
    '#default_value' => $player_dimensions,
    '#description' => t('WxH of your video player.'),
    '#options' => $options,
  );
  // If users cannot change the default dimensions, lets change this to a value.
  if (!$default_dimensions) {
    $element['data']['dimensions']['#type'] = 'value';
    $element['data']['dimensions']['#value'] = $dimensions;
    $element['data']['player_dimensions']['#type'] = 'value';
    $element['data']['player_dimensions']['#value'] = $player_dimensions;
  }

  // only in preview mode and then create thumbnails
  if ($field['widget']['autoconversion']) {
    $bypass = isset($file['data']['bypass_autoconversion']) ? $file['data']['bypass_autoconversion'] : variable_get('video_bypass_conversion', FALSE);
    if (user_access('bypass conversion video')) {
      $element['data']['bypass_autoconversion'] = array(
        '#type' => 'checkbox',
        '#title' => t('Bypass auto conversion'),
        '#default_value' => $bypass,
        '#description' => t('This will bypass your auto conversion of videos.'),
        '#attributes' => array('class' => 'video-bypass-auto-conversion'),
      );

      if ($file['status'] == FILE_STATUS_TEMPORARY) {
        // Checkbox #default_value does not work when the checkbox is inserted using AHAH
        $element['data']['bypass_autoconversion']['#value'] = $bypass;
      }
    }
    else {
      $element['data']['bypass_autoconversion'] = array(
        '#type' => 'value',
        '#value' => $bypass,
      );
    }

    $convert = isset($file['data']['convert_video_on_save']) ? $file['data']['convert_video_on_save'] : variable_get('video_convert_on_save', FALSE);
    if (user_access('convert on submission')) {
      $element['data']['convert_video_on_save'] = array(
        '#type' => 'checkbox',
        '#title' => t('Convert video on save'),
        '#default_value' => $convert,
        '#description' => t('This will convert your video to flv format when you save, instead of scheduling it for cron.'),
        '#attributes' => array('class' => 'video-convert-video-on-save'),
      );

      if ($file['status'] == FILE_STATUS_TEMPORARY) {
        // Checkbox #default_value does not work when the checkbox is inserted using AHAH
        $element['data']['convert_video_on_save']['#value'] = $convert;
      }
    } else {
      $element['data']['convert_video_on_save'] = array(
        '#type' => 'value',
        '#value' => $convert,
      );
    }

    $default_thumb = isset($file['data']['use_default_video_thumb']) ? $file['data']['use_default_video_thumb'] : variable_get('video_use_default_thumb', FALSE);
    if (user_access('use default thumb')) {
      $element['data']['use_default_video_thumb'] = array(
        '#type' => 'checkbox',
        '#title' => t('Use the default thumbnail for this video?'),
        '#default_value' => $default_thumb,
        '#description' => t('This will set a flag for this video to use the default video thumbnail when outputed..'),
        '#attributes' => array('class' => 'video-use-default-video-thumb'),
      );

      if ($file['status'] == FILE_STATUS_TEMPORARY) {
        // Checkbox #default_value does not work when the checkbox is inserted using AHAH
        $element['data']['use_default_video_thumb']['#value'] = $default_thumb;
      }
    } else {
      $element['data']['use_default_video_thumb'] = array(
        '#type' => 'value',
        '#value' => $default_thumb,
      );
    }
  }
}

/**
 * Function updates our options list to show matching aspect ratios and if we have a matching resolution.
 * 
 * We will update the options array by reference and return the aspect ratio of the file.
 */
function _video_dimensions_options(&$options, $video) {
  $aspect_ratio = _video_aspect_ratio($video);

  if (empty($aspect_ratio)) {
    return $aspect_ratio;
  }

  //loop through our options and find matching ratio's and also the exact width/height
  foreach ($options as $key => $value) {
    $wxh = explode('x', $value);
    //lets check our width and height first
    if ($aspect_ratio['width'] == $wxh[0] && $aspect_ratio['height'] == $wxh[1]) {
      $options[$key] = $value . ' ' . t('(Matches Resolution)');
    } else {
      //now lets check our ratio's
      $ratio = number_format($wxh[0] / $wxh[1], 4);
      if ($ratio == $aspect_ratio['ratio']) {
        $options[$key] = $value . ' ' . t('(Matches Ratio)');
      }
    }
  }

  return $aspect_ratio;
}

/**
 * Video_widget_process for API handlers for any video types.
 * @param array $element
 * @param array $form_state
 */
function video_widget_process(&$element, &$form_state) {
  $item = $element['#value'];
  $field = content_fields($element['#field_name'], $element['#type_name']);
  switch ($form_state['clicked_button']['#submit'][0]) {
    case 'node_form_submit':
      // Auto convert our video file
      if ($field['widget']['autoconversion']) {
        video_convert_process($element);
        //lets set our node status to unpublished if our video is not converted.
        if (isset($element['#unpublish']) && $element['#unpublish']) {
          //unpublish the node
          $form_state['values']['status'] = 0;
        }
      }

      // Save manually uploaded thumbs (if they exist) and add them to element
      $filename = $field['field_name'] . '_' . $element['#delta'] . '_thumbs';
      if (isset($_FILES['files']) && is_array($_FILES['files']['name']) && !empty($_FILES['files']['name'][$filename])) {
        video_upload_manual_thumb($element);
      }

      // Call hook_video_submit API
      video_module_invoke('insert', $element, $form_state);
      //
      //queue up the file id to update the node id in the video rendering / cdn tables.
      $form_state['values']['video_id'][] = $item['fid'];
      break;
    case 'node_form_build_preview':
      // Call hook_video_preview API
      video_module_invoke('preview', $element, $form_state);
      break;
    case 'node_form_delete_submit':
      //moved to hook_file_delete in video module.
      break;
  }
}

/**
 * Adds a video to the video rendering table.
 *
 * If auto converting, it will convert your video to flv right now.  We are passing the element by reference
 * just in case we ever want to add more to the element during this process.
 *
 * @param array $element Form element to get the video file from.
 */
function video_convert_process(&$element) {
  $file = $element['#value'];

  // If the dimensions #value is empty, it is probably because the user
  // clicked the Save button directly and did not choose dimensions.
  // Take the default dimensions.
  if (empty($file['data']['dimensions']) && isset($element['data']['dimensions'])) {
    $file['data']['dimensions'] = $element['data']['dimensions']['#default_value'];
  }

  if (!empty($file['fid']) && empty($file['data']['bypass_autoconversion'])) {
    $convert = false;
    $fid = $file['fid'];
    //setup our conversion class and check for the fid existence.
    module_load_include('inc', 'video', '/includes/conversion');
    $video_conversion = new video_conversion;

    // Lets verify that we haven't added this video already.  Multiple validation fails will cause this to be ran more than once
    if (!$video = $video_conversion->load_job($fid)) {
      // Video has not been added to the queue yet so lets add it.
      $video = array('fid' => $fid, 'dimensions' => $file['data']['dimensions']);
      if (!($video_conversion->create_job($video)))
        drupal_set_message(t('Something went wrong with your video job creation.  Please check your recent log entries for further debugging.'), 'error');
      $convert = true;
      //lets queue our node status to unpublished.
      $element['#unpublish'] = true;
    } elseif ($video->video_status != VIDEO_RENDERING_COMPLETE) {
      //lets queue our node status to unpublished.
      $element['#unpublish'] = true;
    }

    // Our video should be in the database pending, lets see if we need to convert it now.
    // Check if we are going from unselected to selected or if this is a new video and we have checked the checkbox
    $convert_video_on_save = false;
    $element_data_convert_on_save = '';
    $file_date_convet_on_save = '';
    $convert_on_save = variable_get('video_convert_on_save', FALSE);
    if (isset($element['data']['convert_video_on_save']['#value']))
      $element_data_convert_on_save = $element['data']['convert_video_on_save']['#value'];
    if (isset($file['data']['convert_video_on_save']))
      $file_date_convet_on_save = $file['data']['convert_video_on_save'];
    $convert_video_on_save = $element_data_convert_on_save || $file_date_convet_on_save;
    if (((!isset($element['#default_value']['data']['convert_video_on_save']) || !$element['#default_value']['data']['convert_video_on_save'])
        && $convert_video_on_save) || ($convert && $convert_video_on_save) || $convert_on_save) {
      $return = $video_conversion->process($fid);
      if ($return === FALSE) {
        drupal_set_message(t('Something went wrong with your video conversion.  Please check your recent log entries for further debugging.'), 'error');
      } elseif ($return === TRUE) {
        //we are always unpublished until we are converted.
        unset($element['#unpublish']);
        drupal_set_message(t('Successfully converted your video.'));
      }
    } elseif ($convert) {
      drupal_set_message(t('Video submission queued for processing. Please wait: our servers are preparing your video for display.'));
    }
  }
}

/**
 * Handle saving of manual thumbs
 */
function video_upload_manual_thumb(&$element) {
  // TODO: test this
  $destination = video_thumb_path($element['#value']);
  if (!is_dir($destination)) {
    form_set_error('video_thumb_upload', t('The thumbnail image could not be uploaded. The destination %destination does not exist or is not writable by the server.', array('%destination' => $destination)));
    return;
  }

  $validators = array('file_validate_is_image' => array());
  if (!$file = file_save_upload($element['#field_name'] .'_'. $element['#delta'] .'_thumbs', $validators, $destination)) {
    // No upload to save we hope... or file_save_upload() reported an error on its own.
    return;
  }

  // Remove old image (if any) & clean up database.
  $old_thumb = $element['data']['video_thumb']['#value'];
  if (!empty($old_thumb)) {
    if (file_delete($old_thumb)) {
      db_query('DELETE FROM {files} WHERE filepath=\'%s\'', $old_thumb);
    }
  }
  // Make the file permanent and store it in the form.
  file_set_status($file, FILE_STATUS_PERMANENT);
  $element['data']['video_thumb']['#value'] = $file->filepath;
}

function video_default_widget_settings($widget) {
  $form = array();
  // Default video settings.
  $form['plugins'] = array(
    '#type' => 'fieldset',
    '#title' => t('Video Advanced Settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#weight' => 10
  );
  $form['plugins']['default_dimensions'] = array(
    '#type' => 'select',
    '#title' => t('Default Video Resolution Dimensions'),
    '#default_value' => !empty($widget['default_dimensions']) ? $widget['default_dimensions'] : '',
    '#options' => video_explode("\n", variable_get("video_metadata_dimensions", video_default_dimensions())),
    '#description' => t('Default transcoding resolution WIDTHxHEIGHT, in px, that FFMPEG will use to transcode your video files.')
  );
  $form['plugins']['default_player_dimensions'] = array(
    '#type' => 'select',
    '#title' => t('Default Video Player Dimensions'),
    '#default_value' => !empty($widget['default_player_dimensions']) ? $widget['default_player_dimensions'] : '',
    '#options' => video_explode("\n", variable_get("video_metadata_dimensions", video_default_dimensions())),
    '#description' => t('Default player WIDTHxHEIGHT in px.  This is your actual player dimensions that your video will be playing in.')
  );
  $form['plugins']['autoconversion'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable video conversion.'),
    '#description' => t('Use ffmpeg(Default) to automatically convert videos to web compatible types eg. FLV, Please make sure to configure your transcoder settings.'),
    '#default_value' => $widget['autoconversion'],
  );

  // Default thumbnail settings.
  $form['default'] = array(
    '#type' => 'fieldset',
    '#title' => t('Video Thumbnail Settings'),
    '#element_validate' => array('video_default_widget_settings_validate'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#weight' => 11
  );

  $thumb_options = array(
    'auto' => 'Automatically generate thumbnails',
    'auto_fallback' => 'Automatically generate thumbnails, with fallback to manual upload',
    'manual_upload' => 'Manually upload a thumbnail',
    'no' => 'Don\'t create thumbnail',
  );

  $form['default']['autothumbnail'] = array(
    '#type' => 'radios',
    '#title' => t('Thumbnail Generation'),
    '#options' => $thumb_options,
    '#description' => t('To use ffmpeg(Default) to create thumbnails, Please make sure to configure your transcoder settings before using ffmpeg to create thumbnails.'),
    '#default_value' => isset($widget['autothumbnail']) ? $widget['autothumbnail'] : 'no',
  );

  // @TODO: Move this to the actual upload/attach when creating a node to allow the user to upload their own thumbnail for each video.
  // Present a video image of the current default image.
  if (!empty($widget['default_video_thumb'])) {
    $form['default']['default_video_thumbnail'] = array(
      '#type' => 'markup',
      '#value' => theme('video_image', $widget['default_video_thumb'], '', '', array('width' => '150'), FALSE),
      '#prefix' => '<div class="video_thumbnail">',
      '#suffix' => '</div>'
    );
    $form['default']['delete_default_video_thumbnail'] = array(
      '#type' => 'checkbox',
      '#title' => t('Delete the default thumbnail'),
    );
  }
  $form['default']['default_video_thumb_upload'] = array(
    '#type' => 'file',
    '#title' => empty($widget['default_video_thumb']) ? t('Upload default video thumbnail') : t('Replace default video thumbnail with'),
    '#description' => t('Choose a image that will be used as video thumbnail when you don\'t have video thumbnails for videos.'),
  );
  // We set this value on 'validate' so we can get CCK to add it
  // as a standard field setting.
  $form['default_video_thumb'] = array(
    '#type' => 'value',
    '#value' => isset($widget['default_video_thumb']) ? $widget['default_video_thumb'] : NULL,
  );
  return $form;
}

/**
 * Element specific validation for video default value.
 *
 */
function video_default_widget_settings_validate($element, &$form_state) {
  // Verify the destination exists
  $video_thumb_path = variable_get('video_thumb_path', 'video_thumbs');
  $destination = file_directory_path() . '/' . $video_thumb_path;
  if (!field_file_check_directory($destination, FILE_CREATE_DIRECTORY)) {
    form_set_error('default_video_thumb', t('The default image could not be uploaded. The destination %destination does not exist or is not writable by the server.', array('%destination' => dirname($destination))));
    return;
  }

  $oldthumb = $form_state['values']['default_video_thumb'];

  // We save the upload here because we can't know the correct path until the file is saved.
  $newthumb = file_save_upload('default_video_thumb_upload', array('file_validate_is_image' => array()), $destination);

  // Delete the current file if there is a new one or the delete_default_video_thumbnail checkbox is checked
  if (!empty($oldthumb['fid']) && ($newthumb || !empty($form_state['values']['delete_default_video_thumbnail']))) {
    if (file_delete(file_create_path($oldthumb['filepath']))) {
      db_query('DELETE FROM {files} WHERE fid=%d', $oldthumb['fid']);
    }
    $form_state['values']['default_video_thumb'] = array();
  }

  if ($newthumb) {
    // Make the file permanent and store it in the form.
    file_set_status($newthumb, FILE_STATUS_PERMANENT);
    $newthumb->timestamp = time();
    $form_state['values']['default_video_thumb'] = (array) $newthumb;
  }
}

function video_widget_settings_file_path_validate($element, &$form_state) {
  //lets prepend our video folder to the path settings.  first truncate videos/ off the end if it exists.
  // #848804
  if (!module_exists('filefield_paths')) {
    $form_state['values']['file_path'] = 'videos/'. $form_state['values']['file_path'];
  }
}

/**
 * Compares passed extensions with normal video web extensions.
 * 
 * @param string $ext
 * @return bool
 */
function video_web_extensions($ext) {
  $extensions = array_filter(explode(' ', $ext));
  $web_extensions = array(
    'mov', 'mp4', '3gp', '3g2', 'mpg', 'mpeg', // quicktime
    'divx', 'mkv', //divx
    'rm', // realplayer
    'flv', 'f4v', //flash player
    'swf', // swf player
    'dir', 'dcr', // dcr player
    'asf', 'wmv', 'avi', 'mpg', 'mpeg', // windows media
    'ogg', 'ogv', 'webm' // ogg/ogv theora
  );
  
  return count(array_diff($extensions, $web_extensions)) == 0;
}

/**
 * Utility function that will add a preview of thumbnails for you to select when uploading videos.
 */
function video_thumb_process(&$element) {
  // Developed for ffmpeg support
  $file = $element['#value'];
  $delta = $file['fid'];
  $field = content_fields($element['#field_name'], $element['#type_name']);
  $gen_fail = FALSE;

  if (isset($element['preview']) && $file['fid'] != 0) {
    if (in_array($field['widget']['autothumbnail'], array('auto', 'auto_fallback'))) {
      module_load_include('inc', 'video', '/includes/transcoder');
      $transcoder = new video_transcoder();
      $thumbs = $transcoder->generate_thumbnails($file);
      
      if (!empty($thumbs)) {
        drupal_add_js(drupal_get_path('module', 'video') .'/js/video.admin.js');
        
        $thumbss = array();
        foreach ($thumbs as $fid => $img) {
          $thumbss[$img->filepath] = theme('video_thumbnails', $img, '', '', array('width' => '50'), FALSE);
        }
        
        if (!empty($file['data']['video_thumb']) && isset($thumbss[$file['data']['video_thumb']])) {
          $currentthumb = $file['data']['video_thumb'];
        }
        else {
          $currentthumb = array_rand($thumbss);
        }
        
        $element['data']['video_thumb'] = array(
          '#type' => 'radios',
          '#title' => t('Video Thumbnail'),
          '#options' => $thumbss,
          '#default_value' => $currentthumb,
          '#weight' => 10,
          '#attributes' => array('class' => 'video-thumbnails', 'onchange' => 'videoftp_thumbnail_change()', 'rel' => 'video_large_thumbnail-' . $delta),
        );
      } else {
        $gen_fail = TRUE;
      }
    }

    if ((!empty($gen_fail) && $field['widget']['autothumbnail'] == 'auto_fallback') ||
        $field['widget']['autothumbnail'] == 'manual_upload') {

      $element['data']['video_thumb_file'] = array(
        '#name' => 'files[' . $element['#field_name'] . '_' . $element['#delta'] . '_thumbs]',
        '#type' => 'file',
        '#size' => '40',
        '#title' => !empty($file['data']['video_thumb']) ? t('Replace the video thumbnail') : t('Upload a video thumbnail'),
        '#description' => t('This thumbnail will be uploaded when the node is saved.'),
      );

      $element['data']['video_thumb'] = array(
        '#type' => 'value',
        '#value' => isset($file['data']['video_thumb']) ? $file['data']['video_thumb'] : false,
      );
    }

    // Setup our large thumbnail that is on the left.
    // @todo Add smaller video preview instead of thumbnail?
    if (!empty($file['data']['video_thumb'])) {
      $large_thumb = array('filepath' => $file['data']['video_thumb']);
    }
    elseif (!empty($field['widget']['default_video_thumb'])) {
      $large_thumb = $field['widget']['default_video_thumb'];
    }
    elseif (!empty($currentthumb)) {
      $large_thumb = array('filepath' => $currentthumb);
    }
    
    if (!empty($large_thumb)) {
      // @todo Integrate the thumbnails with imagecache.
      $element['preview']['#suffix'] = '<div class="video_large_thumbnail-'. $delta .'">'. theme('video_thumbnails', $large_thumb, '', '', array('width' => 150), FALSE) .'</div>';
    }
  }
}

/**
 * #options helper function to set our key=value for the form api.
 */
function video_explode($delimeter, $dimensions) {
  $options = array();
  $values = explode($delimeter, $dimensions);
  foreach ($values as $value) {
    //lets check we have a value and its in the right format
    if (!empty($value) && video_format_right($value)) {
      $options[trim($value)] = trim($value);
    }
  }
  return $options;
}

function video_format_right($value) {
  $format = explode('x', $value, 2);
  if (!isset($format[0]) || !is_numeric(trim($format[0]))) {
    return false;
  }
  if (!isset($format[1]) || !is_numeric(trim($format[1]))) {
    return false;
  }
  return true;
}
