<?php
/**
 * @file
 * Implement an video field, based on the file module's file field.
 */

/**
 * Implements hook_field_info().
 */
function video_field_info() {
  return array(
    'video' => array(
      'label' => t('Video'),
      'description' => t('This field stores the ID of a video file as an integer value.'),
      'settings' => array(
        'uri_scheme' => variable_get('file_default_scheme', 'public'),
        'uri_scheme_converted' => variable_get('file_default_scheme', 'public'),
        'uri_scheme_thumbnails' => variable_get('file_default_scheme', 'public'),
        'thumbnail_format' => 'png',
        'autoconversion' => 0,
        'autothumbnail' => 'no',
        'default_video_thumbnail' => 0,
        'preview_video_thumb_style' => 'thumbnail',
      ),
      'instance_settings' => array(
        'file_extensions' => 'mp4 ogg avi mov wmv flv ogv webm',
        'file_directory' => 'videos/original',
        'max_filesize' => '',
        'default_dimensions' => '640x360',
      ),
      'default_widget' => 'video_upload',
      'default_formatter' => 'video_formatter_player',
    ),
  );
}

/**
 * Implements hook_field_settings_form().
 */
function video_field_settings_form($field, $instance, $has_data) {
  $defaults = field_info_field_settings($field['type']);
  $settings = array_merge($defaults, $field['settings']);

  // Copied from file_field_settings_form().
  $scheme_options = array();
  foreach (file_get_stream_wrappers(STREAM_WRAPPERS_WRITE_VISIBLE) as $scheme => $stream_wrapper) {
    $scheme_options[$scheme] = $stream_wrapper['name'];
  }
  $form['uri_scheme'] = array(
    '#type' => 'radios',
    '#title' => t('Upload destination of original file'),
    '#options' => $scheme_options,
    '#default_value' => $settings['uri_scheme'],
    '#description' => t('Select where the original video files should be stored. Private file storage has significantly more overhead than public files, but allows restricted access to files within this field.'),
  );
  $form['uri_scheme_converted'] = array(
    '#type' => 'radios',
    '#title' => t('Upload destination of converted files'),
    '#options' => $scheme_options,
    '#default_value' => $settings['uri_scheme_converted'],
    '#description' => t('Select where the converted video files should be stored. Private file storage has significantly more overhead than public files, but allows restricted access to files within this field.'),
  );
  $form['uri_scheme_thumbnails'] = array(
    '#type' => 'radios',
    '#title' => t('Upload destination of thumbnails'),
    '#options' => $scheme_options,
    '#default_value' => $settings['uri_scheme_thumbnails'],
    '#description' => t('Select where the generated thumbnails should be stored. Private file storage has significantly more overhead than public files, but allows restricted access to files within this field.'),
  );

  // Add warnings for S3
  if (isset($scheme_options['s3'])) {
    $factory = new TranscoderAbstractionAbstractFactory();
    $transcoder = $factory->getProduct();
    $value = $transcoder->getValue();

    if ($value == 'TranscoderAbstractionFactoryFfmpeg') {
      $form['uri_scheme']['#description'] .= '<br/><strong>';
      $form['uri_scheme']['#description'] .= t('You are using the FFmpeg transcoder. Using @scheme to store videos is not advised because FFmpeg is not able to transcode remote files. Every time FFmpeg needs access to the video, it will be copied to a temporary location.', array('@scheme' => $scheme_options['s3']));
      $form['uri_scheme']['#description'] .= '</strong>';
    }

    $form['uri_scheme_thumbnails']['#description'] .= '<br/>';
    $form['uri_scheme_thumbnails']['#description'] .= t('Be aware that using @scheme to store thumbnails increases the overhead of dynamic image manipulation by the Image module.', array('@scheme' => $scheme_options['s3']));
  }

  $form['autoconversion'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable auto video conversion'),
    '#description' => t('Convert videos automatically using FFmpeg or Zencoder. You can define presets at !preset to automatically convert videos to web compatible formats eg. FLV, MP4. Make sure to configure your !settings to make this work properly.', array(
      '!settings' => l(t('transcoder settings'), 'admin/config/media/video/transcoders'),
      '!preset' => l(t('preset settings'), 'admin/config/media/video/presets'),
    )),
    '#default_value' => isset($settings['autoconversion']) ? $settings['autoconversion'] : '',
  );

  $thumb_options = array(
    'auto' => 'Automatically extract thumbnails from video (with fallback to manual upload)',
    'manual_upload' => 'Manually upload a thumbnail',
    'no' => 'Don\'t create thumbnail',
  );

  $form['thumbnail_format'] = array(
    '#title' => t('Thumbnail format'),
    '#type' => 'radios',
    '#options' => array('png' => 'PNG', 'jpg' => 'JPEG'),
    '#default_value' => !empty($settings['thumbnail_format']) ? $settings['thumbnail_format'] : 'png',
  );

  $form['autothumbnail'] = array(
    '#type' => 'radios',
    '#title' => t('Video thumbnails'),
    '#options' => $thumb_options,
    '#description' => t('If you choose <i>Automatically extract thumbnails from video</i> then please make sure to configure your !settings to make this work properly.', array('!settings' => l(t('transcoder settings'), 'admin/config/media/video/transcoders'))),
    '#default_value' => isset($settings['autothumbnail']) ? $settings['autothumbnail'] : 'no',
  );

  $form['default_video_thumbnail'] = array(
    '#title' => t('Default video thumbnail'),
    '#type' => 'managed_file',
    '#element_validate' => array('video_field_default_thumbnail_validate'),
    '#description' => t('You can use a default thumbnail for all videos or videos from which a thumbnail can\'t be extracted. Settings to use default video thumbnails will be available on node edit. You can change the permissions for other users too.'),
    '#default_value' => !empty($settings['default_video_thumbnail']['fid']) ? $settings['default_video_thumbnail']['fid'] : '',
    '#upload_location' => 'public://videos/thumbnails/default',
  );

  $form['preview_video_thumb_style'] = array(
    '#title' => t('Preview thumbnail style'),
    '#type' => 'select',
    '#options' => image_style_options(FALSE),
    '#empty_option' => '<' . t('no preview') . '>',
    '#default_value' => !empty($settings['preview_video_thumb_style']) ? $settings['preview_video_thumb_style'] : '',
    '#description' => t('This image style will be used to show extracted video thumbnails on video node edit. Extracted thumbnail preview will also use this style.'),
  );

  return $form;
}

/**
 * Element specific validation for video default value.
 */
function video_field_default_thumbnail_validate($element, &$form_state) {
  $settings = $form_state['values']['field']['settings'];
  // Make the file permanent and store it in the form.
  if (!empty($settings['default_video_thumbnail']['fid'])) {
    $file = file_load($settings['default_video_thumbnail']['fid']);
    $file->status = FILE_STATUS_PERMANENT;
    $file = file_save($file);
    $form_state['values']['field']['settings']['default_video_thumbnail'] = (array) $file;
  }
}

/**
 * Implements hook_field_instance_settings_form().
 */
function video_field_instance_settings_form(array $field, array $instance) {
  $widget = $instance['settings'];

  // Use the file field instance settings form as a basis.
  $form = file_field_instance_settings_form($field, $instance);

  // Remove the description option.
  unset($form['description_field']);

  $form['default_dimensions'] = array(
    '#type' => 'select',
    '#title' => t('Output video dimensions'),
    '#default_value' => !empty($widget['default_dimensions']) ? $widget['default_dimensions'] : '',
    '#options' => video_utility::getDimensions(),
    '#description' => t('This setting can be overridden in node edit or preset settings. Select the most suitable values to use as output dimensions, as selecting a bad value could cause video conversion to fail.'),
    '#weight' => 16,
  );

  return $form;
}

/**
 * Implements hook_field_load().
 */
function video_field_load($entity_type, $entities, $field, $instances, $langcode, &$items, $age) {
  file_field_load($entity_type, $entities, $field, $instances, $langcode, $items, $age);
}

/**
 * Implements hook_field_presave().
 */
function video_field_presave($entity_type, $entity, $field, $instance, $langcode, &$items) {
  // change the thumbnails if default is checked
  foreach ($items as $delta => $item) {
    if (!empty($field['settings']['default_video_thumbnail']['fid'])) {
      if (!empty($item['use_default_video_thumb'])) {
        $items[$delta]['thumbnail'] = $field['settings']['default_video_thumbnail']['fid'];
      }
    }

    // delete from cache if exists
    $cid = 'video:video:' . md5($item['fid']);
    $cache = cache_get($cid);
    if (!empty($cache->data)) {
      cache_clear_all($cid, 'cache');
    }
  }

  file_field_presave($entity_type, $entity, $field, $instance, $langcode, $items);
}

/**
 * Implements hook_field_insert().
 */
function video_field_insert($entity_type, $entity, $field, $instance, $langcode, &$items) {
  file_field_insert($entity_type, $entity, $field, $instance, $langcode, $items);
  // calling function to handle conversion when auto conversion is enabled
  _video_field_file_autoconversion($entity_type, $entity, $field, $instance, $langcode, $items);

  // Update the thumbnails to be permanent and register with entity
  $thumbnails = _video_field_get_all_thumbnails($items);
  if (!empty($thumbnails)) {
    db_update('file_managed')
      ->fields(array('status' => FILE_STATUS_PERMANENT))
      ->condition('fid', array_keys($thumbnails), 'IN')->execute();
    file_field_insert($entity_type, $entity, $field, $instance, $langcode, $thumbnails);
  }

  // Update the thumbnails to be permanent and register with entity
  $converted = _video_field_get_all_converted($items);
  if (!empty($converted)) {
    file_field_insert($entity_type, $entity, $field, $instance, $langcode, $converted);
  }
}

/**
 * Implements hook_field_update().
 */
function video_field_update($entity_type, $entity, $field, $instance, $langcode, &$items) {
  file_field_update($entity_type, $entity, $field, $instance, $langcode, $items);
  _video_field_file_autoconversion($entity_type, $entity, $field, $instance, $langcode, $items);

  // Register the thumbnails in file_usage
  $thumbnails = _video_field_get_all_thumbnails($items);
  if (!empty($thumbnails)) {
    db_update('file_managed')
      ->fields(array('status' => FILE_STATUS_PERMANENT))
      ->condition('fid', array_keys($thumbnails), 'IN')->execute();
  }

  // Register the thumbnails in file_usage
  $converted = _video_field_get_all_converted($items);
  $derivedfiles = array_merge($converted, $thumbnails);

  // Modification of file_field_update().
  // That function can't be called because the logic for finding
  // existing fids is not the same.
  list($id, $vid, $bundle) = entity_extract_ids($entity_type, $entity);

  // On new revisions, all files are considered to be a new usage and no
  // deletion of previous file usages are necessary.
  if (!empty($entity->revision)) {
    foreach ($derivedfiles as $item) {
      $file = (object) $item;
      file_usage_add($file, 'file', $entity_type, $id);
    }
    return;
  }

  // Build a display of the current FIDs.
  $current_fids = array();
  foreach ($derivedfiles as $item) {
    $current_fids[] = $item['fid'];
  }

  // Create a bare-bones entity so that we can load its previous values.
  $original = entity_create_stub_entity($entity_type, array($id, $vid, $bundle));
  field_attach_load($entity_type, array($id => $original), FIELD_LOAD_CURRENT, array('field_id' => $field['id']));

  // Compare the original field values with the ones that are being saved.
  $original_fids = array();
  $delete_fids = array();
  if (!empty($original->{$field['field_name']}[$langcode])) {
    $original_thumbs = _video_field_get_all_thumbnails($original->{$field['field_name']}[$langcode]);
    $original_converted = _video_field_get_all_converted($original->{$field['field_name']}[$langcode]);
    $original_derivedfiles = array_merge($original_thumbs, $original_converted);
    foreach ($original_derivedfiles as $original_derivedfile) {
      $original_fids[] = $original_derivedfile['fid'];
      if (isset($original_derivedfile['fid']) && !in_array($original_derivedfile['fid'], $current_fids)) {
        // Decrement the file usage count by 1 and delete the file if possible.
        file_field_delete_file($original_derivedfile, $field, $entity_type, $id);
        $delete_fids[] = $original_derivedfile['fid'];
      }
    }
  }

  // Add new usage entries for newly added files.
  foreach ($derivedfiles as $item) {
    if (!in_array($item['fid'], $original_fids)) {
      $file = (object) $item;
      file_usage_add($file, 'file', $entity_type, $id);
    }
  }

  // Delete fids from video_thumbnails
  if (!empty($delete_fids)) {
    db_delete('video_thumbnails')->condition('thumbnailfid', $delete_fids, 'IN')->execute();
    db_delete('video_output')->condition('output_fid', $delete_fids, 'IN')->execute();
  }
}

/**
 * Implements hook_field_delete().
 */
function video_field_delete($entity_type, $entity, $field, $instance, $langcode, &$items) {
  file_field_delete($entity_type, $entity, $field, $instance, $langcode, $items);

  // Deregister the thumbnails in file_usage
  $thumbnails = _video_field_get_all_thumbnails($items);
  if (!empty($thumbnails)) {
    file_field_delete($entity_type, $entity, $field, $instance, $langcode, $thumbnails);
    db_delete('video_thumbnails')->condition('thumbnailfid', array_keys($thumbnails), 'IN')->execute();
  }

  // Deregister the converted files in file_usage
  $converted = _video_field_get_all_converted($items);
  if (!empty($converted)) {
    file_field_delete($entity_type, $entity, $field, $instance, $langcode, $converted);
    db_delete('video_output')->condition('output_fid', array_keys($converted), 'IN')->execute();
  }
}

/**
 * Implements hook_field_delete_revision().
 */
function video_field_delete_revision($entity_type, $entity, $field, $instance, $langcode, &$items) {
  file_field_delete_revision($entity_type, $entity, $field, $instance, $langcode, $items);

  // Deregister the thumbnails in file_usage
  $thumbnails = _video_field_get_all_thumbnails($items);
  file_field_delete_revision($entity_type, $entity, $field, $instance, $langcode, $thumbnails);

  // Deregister the converted files in file_usage
  $converted = _video_field_get_all_converted($items);
  file_field_delete($entity_type, $entity, $field, $instance, $langcode, $converted);
}

/**
 * Implements hook_field_is_empty().
 */
function video_field_is_empty($item, $field) {
  return file_field_is_empty($item, $field);
}

/**
 * Widget
 */

/**
 * Implements hook_field_widget_info().
 */
function video_field_widget_info() {
  return array(
    'video_upload' => array(
      'label' => t('Video Upload'),
      'field types' => array('video'),
      'settings' => array(
        'progress_indicator' => 'throbber',
      ),
      'behaviors' => array(
        'multiple values' => FIELD_BEHAVIOR_CUSTOM,
        'default value' => FIELD_BEHAVIOR_NONE,
      ),
    ),
  );
}

/**
 * Implements hook_field_widget_settings_form().
 */
function video_field_widget_settings_form($field, $instance) {
  // Use the file widget settings form.
  return file_field_widget_settings_form($field, $instance);
}

/**
 * Implements hook_field_widget_form().
 */
function video_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  // Add display_field setting to field because file_field_widget_form() assumes it is set.
  $field['settings']['display_field'] = 0;

  $elements = file_field_widget_form($form, $form_state, $field, $instance, $langcode, $items, $delta, $element);
  $settings = $instance['settings'];

  foreach (element_children($elements) as $delta) {
    // If not using custom extension validation, ensure this is an video.
    $supported_extensions = array_keys(video_utility::getVideoExtensions());
    $extensions = isset($elements[$delta]['#upload_validators']['file_validate_extensions'][0]) ? $elements[$delta]['#upload_validators']['file_validate_extensions'][0] : implode(' ', $supported_extensions);
    $extensions = array_intersect(explode(' ', $extensions), $supported_extensions);
    $elements[$delta]['#upload_validators']['file_validate_extensions'][0] = implode(' ', $extensions);

    // Add all extra functionality provided by the video widget.
    $elements[$delta]['#process'][] = 'video_field_widget_process';

    // Add thumbnail stub to prevent errors in file_ajax_upload().
    $elements[$delta]['thumbnail'] = array(
      '#type' => 'value',
      '#value' => NULL,
    );
  }

  if ($field['cardinality'] == 1) {
    // If there's only one field, return it as delta 0.
    if (empty($elements[0]['#default_value']['fid'])) {
      $elements[0]['#description'] = theme('file_upload_help', array('description' => $instance['description'], 'upload_validators' => $elements[0]['#upload_validators']));
    }
  }
  else {
    $elements['#file_upload_description'] = theme('file_upload_help', array('upload_validators' => $elements[0]['#upload_validators']));
  }
  return $elements;
}

/**
 * An element #process callback for the video field type.
 */
function video_field_widget_process($element, &$form_state, $form) {
  $item = $element['#value'];
  $item['fid'] = $element['fid']['#value'];
  $field = field_widget_field($element, $form_state);

  $element['#theme'] = 'video_widget';
  $element['preview'] = array();

  // Title is not necessary for each individual field.
  if ($field['cardinality'] != 1) {
    unset($element['#title']);
  }

  // Add our extra fields if in preview mode
  if (empty($item['fid'])) {
    return $element;
  }

  video_widget_element_settings($element, $form_state, $field);
  video_thumb_process($element, $form_state, $field);

  return $element;
}

/**
 * Adds a preview of thumbnails for you to select when uploading videos.
 */
function video_thumb_process(array &$element, array &$form_state, array $field) {
  $file = $element['#value'];
  $fid = intval($file['fid']);
  $defaultthumbfid = isset($field['settings']['default_video_thumbnail']) ? intval($field['settings']['default_video_thumbnail']['fid']) : 0;
  $thumbnailfid = !isset($file['thumbnail']) ? 0 : (is_array($file['thumbnail']) ? intval($file['thumbnail']['fid']) : intval($file['thumbnail']));

  if (isset($element['preview']) && $fid != 0) {
    $gen_fail = FALSE;

    if ($field['settings']['autothumbnail'] == 'auto') {
      $transcoder = new Transcoder();
      $thumbs = $transcoder->extractFrames($file, $field);

      if (!empty($thumbs)) {
        $thumbss = array();
        foreach ($thumbs as $img) {
          $thumbss[$img->fid] = theme('image_style', array('style_name' => $field['settings']['preview_video_thumb_style'], 'path' => $img->uri));
        }

        if ($thumbnailfid > 0 && isset($thumbss[$thumbnailfid])) {
          $currentthumb = $thumbnailfid;
        }
        else {
          $currentthumb = array_rand($thumbss);
        }

        $element['thumbnail'] = array(
          '#type' => 'radios',
          '#title' => t('Video thumbnail'),
          '#options' => $thumbss,
          '#default_value' => $currentthumb,
          '#weight' => 10,
          '#attributes' => array('class' => array('video-thumbnails'), 'onchange' => 'Drupal.behaviors.videoEdit()', 'rel' => 'video_large_thumbnail-' . $fid),
        );
      }
      elseif($thumbs === FALSE) {
        $gen_fail = TRUE;
      }
    }

    if ($gen_fail || $field['settings']['autothumbnail'] == 'manual_upload') {
      $file += (array) file_load($fid);
      $scheme = isset($field['settings']['uri_scheme_thumbnails']) ? $field['settings']['uri_scheme_thumbnails'] : 'public';
      $element['thumbnail'] = array(
        '#title' => t('Video thumbnail'),
        '#type' => 'managed_file',
        '#description' => t('The uploaded image will be used as video thumbnail on this video.'),
        '#default_value' => NULL,
        '#upload_location' => $scheme . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $fid,
        '#weight' => 10,
      );

      // Set the current thumbnail fid, if it is not the default one
      if ($thumbnailfid > 0) {
        if ($defaultthumbfid == 0 || $thumbnailfid != $defaultthumbfid) {
          $element['thumbnail']['#default_value'] = $thumbnailfid;
        }
        else {
          // Unset the form_state value for the thumbnail field if it is the default thumbnail.
          // This prevents the "may not be referenced" error.
          form_set_value($element, NULL, $form_state);
        }
      }
    }

    // Setup our large thumbnail that is on the left.
    $large_thumb = NULL;
    if (!empty($currentthumb)) {
      $large_thumb = file_load($currentthumb);
    }
    elseif ($thumbnailfid > 0) {
      $large_thumb = file_load($thumbnailfid);
    }
    elseif ($defaultthumbfid > 0) {
      $large_thumb = file_load($defaultthumbfid);
    }

    if (!empty($large_thumb)) {
      $element['preview']['#suffix'] = '<div class="video_large_thumbnail-' . $fid . '">' . theme('image_style', array('style_name' => $field['settings']['preview_video_thumb_style'], 'path' => $large_thumb->uri)) . '</div>';
    }
  }
}

/**
 * Process elements loads on settings
 */
function video_widget_element_settings(array &$element, array &$form_state, array $field) {
  $file = $element['#value'];
  $delta = $element['#delta'];

  $instance = field_widget_instance($element, $form_state);
  $dimensions = $instance['settings']['default_dimensions'];
  $default_dimensions = user_access('override player dimensions');
  $description = t('Set your video dimensions. This will create your player with these dimensions.');

  // Lets figure out our dimensions for our video and add astericks next to our options.
  $options = video_utility::getDimensions();
  if ($field['settings']['autoconversion'] && isset($element['preview']) && $file['fid'] != 0 && $default_dimensions) {
    $file_object = file_load($file['fid']);
    $video_info = _video_dimensions_options($options, $file_object);
    if (!empty($video_info['width']) && !empty($video_info['height'])) {
      $description .= ' ' . t('Your video size is <b>!size</b>, if you choose a higher resolution, this could cause video distortion. You are shown dimensions that match your aspect ratio, if you choose dimensions that do not match your ratio, we will pad your video by adding black bars on either the top or bottom while maintaining your videos original aspect ratio.', array('!size' => $video_info['width'] . 'x' . $video_info['height']));
    }

    // setup our default display of dimensions.
    // lets go through our options looking for a matching resolution
    foreach ($options as $key => $value) {
      if (stristr($value, t('(Matches Resolution)'))) {
        $dimensions = $key;
        break;
      }
    }
  }
  // Override our dimensions to the user selected.
  if (!empty($file['dimensions'])) {
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
 * Updates options list to show matching aspect ratios and matching resolutions
 *
 * We will update the options array by reference and return the aspect ratio of
 * the file.
 */
function _video_dimensions_options(&$options, $video) {
  $aspect_ratio = _video_aspect_ratio($video);

  if (empty($aspect_ratio)) {
    return $aspect_ratio;
  }

  // Loop through our options and find matching ratio's and also the exact width/height
  foreach ($options as $key => $value) {
    $wxh = explode('x', $value);
    // Lets check our width and height first
    if ($aspect_ratio['width'] == $wxh[0] && $aspect_ratio['height'] == $wxh[1]) {
      $options[$key] = $value . ' ' . t('(Matches Resolution)');
    }
    else {
      // Now lets check our ratio's
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
 */
function _video_aspect_ratio($video) {
  $factory = new TranscoderAbstractionAbstractFactory();
  $transcoder = $factory->getProduct();
  $transcoder->setInput((array) $video);
  $wxh = $transcoder->getDimensions();

  if (empty($wxh) || empty($wxh['width']) || empty($wxh['height'])) {
    // No width and height found. This may be because the transcoder does not support retrieving dimensions.
    return NULL;
  }

  return array(
    'width' => $wxh['width'],
    'height' => $wxh['height'],
    'ratio' => number_format($wxh['width'] / $wxh['height'], 4),
  );
}

/**
 * Formatters
 */

/**
 * Implements CCK's hook_field_formatter_info().
 */
function video_field_formatter_info() {
  $formatters = array(
    'video_formatter_player' => array(
      'label' => t('Video player'),
      'field types' => array('video'),
      'settings' => array(
        'widthxheight' => '640x360',
      ),
    ),
    'video_formatter_thumbnail' => array(
      'label' => t('Video thumbnail'),
      'field types' => array('video'),
      'settings' => array(
        'image_style' => '',
        'image_link' => ''
      ),
    ),
  );
  return $formatters;
}

/**
 * Implements hook_field_formatter_settings_form().
 */
function video_field_formatter_settings_form($field, $instance, $view_mode, $form, &$form_state) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];

  if ($display['type'] == 'video_formatter_thumbnail') {
    $image_styles = image_style_options(FALSE);
    $element['image_style'] = array(
      '#title' => t('Video thumbnail style'),
      '#type' => 'select',
      '#default_value' => $settings['image_style'],
      '#empty_option' => t('None (original video thumbnail)'),
      '#options' => $image_styles,
    );

    $link_types = array(
      'content' => t('Content'),
      'file' => t('File'),
    );
    $element['image_link'] = array(
      '#title' => t('Link video or video thumbanil to'),
      '#type' => 'select',
      '#default_value' => $settings['image_link'],
      '#empty_option' => t('Nothing'),
      '#options' => $link_types,
    );
  }
  // set the player wxh
  if ($display['type'] == 'video_formatter_player') {
    $element['widthxheight'] = array(
      '#title' => t('Dimensions'),
      '#type' => 'select',
      '#default_value' => $settings['widthxheight'],
      '#description' => t('Select the desired widthxheight of the video player. You can add your own dimensions from !settings.', array('!settings' => l(t('video metadata settings'), 'admin/config/media/video/metadata'))),
      '#options' => video_utility::getDimensions(),
    );
  }
  return $element;
}

/**
 * Implements hook_field_formatter_settings_summary().
 */
function video_field_formatter_settings_summary($field, $instance, $view_mode) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];
  $summary = array();
  $image_styles = image_style_options(FALSE);
  // Unset possible 'No defined styles' option.
  unset($image_styles['']);
  // Styles could be lost because of enabled/disabled modules that defines
  // their styles in code.
  if ($display['type'] == 'video_formatter_thumbnail') {
    if (isset($image_styles[$settings['image_style']])) {
      $summary[] = t('Video thumbnail style: @style', array('@style' => $image_styles[$settings['image_style']]));
    }
    else {
      $summary[] = t('Original video thumbnail');
    }

    $link_types = array(
      'content' => t('Linked to content'),
      'file' => t('Linked to video file'),
    );
    // Display this setting only if image is linked.
    if (isset($link_types[$settings['image_link']])) {
      $summary[] = $link_types[$settings['image_link']];
    }
  }
  if ($display['type'] == 'video_formatter_player') {
    $summary[] = t('WidthxHeight of the player: @widthxheight', array('@widthxheight' => $settings['widthxheight']));
  }

  return implode('<br />', $summary);
}

/**
 * Implements hook_field_formatter_view().
 */
function video_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  $element = array();
  $settings = $display['settings'];
  // set the display
  $theme = $display['type'];
  foreach ($items as $delta => $item) {
    if ($display['type'] == 'video_formatter_thumbnail') {
      // Check if the formatter involves a link.
      if ($settings['image_link'] == 'content') {
        $uri = entity_uri($entity_type, $entity);
      }
      elseif ($settings['image_link'] == 'file') {
        $link_file = TRUE;
      }
      if (isset($link_file)) {
        $uri = array(
          'path' => file_create_url($item['uri']),
          'options' => array(),
        );
      }
      $element[$delta] = array(
        '#theme' => $theme,
        '#item' => $item,
        '#image_style' => $settings['image_style'],
        '#path' => isset($uri) ? $uri : '',
        '#entity' => $entity,
        '#field' => $field,
        '#instance' => $instance
      );
    }
    if ($display['type'] == 'video_formatter_player') {
      $element[$delta] = array(
        '#theme' => $theme,
        '#item' => $item,
        '#entity' => $entity,
        '#field' => $field,
        '#instance' => $instance,
        '#player_dimensions' => $settings['widthxheight']
      );
    }
  }
  return $element;
}

/**
 * Implements hook_filefield_sources_widgets().
 */
function video_filefield_sources_widgets() {
  return array('video_upload');
}

/**
 * Video file save to the video_queue table for conversions
 */
function _video_field_file_autoconversion($entity_type, $entity, $field, $instance, $langcode, &$items) {
  if (empty($field['settings']['autoconversion'])) {
    return;
  }

  // Find the entity id
  list($entity_id, $entity_vid, $bundle) = entity_extract_ids($entity_type, $entity);

  $hasfilefieldpaths = module_exists('filefield_paths') && !empty($instance['settings']['filefield_paths']);

  $video_conversion = new Conversion();
  $factory = new TranscoderAbstractionAbstractFactory();
  $transcoder = $factory->getProduct();

  foreach ($items as $delta => &$item) {
    // When filefield_paths is active, _video_field_file_autoconversion is invoked twice:
    // the first time before filefield_paths has changed the path,
    // the second time after the file has been moved (or not)
    // We only process the item in the second round of processing.
    if ($hasfilefieldpaths && !isset($item['origname'])) {
      continue;
    }

    // set input file
    $transcoder->setInput($item);
    // skip adding entry if bypass conversion is checked
    if (!empty($item['bypass_autoconversion'])) {
      // delete the conversion job if any
      $transcoder->deleteJob();
      return;
    }

    // Try to load the job
    $video = $transcoder->loadJob();

    // Create the job if it doesn't exist
    if (!$video) {
      if (!($transcoder->createJob($item['fid'], $item['dimensions'], $entity_id, $entity_type, $field['field_name'], $langcode, $delta))) {
        drupal_set_message(t('Something went wrong with your video job creation. Please check your recent log entries for further debugging.'), 'error');
        return;
      }
      // Load a fresh copy of the job
      $video = $transcoder->loadJob();
    }

    // re queue for video conversion
    if (!empty($item['re_convert_video'])) {
      $video_conversion->changeStatus($video->vid, VIDEO_RENDERING_PENDING);
      $video->video_status = VIDEO_RENDERING_PENDING;
    }

    // Convert on save if the job is pending and if it is requested
    if (!empty($item['convert_video_on_save']) && $video->video_status == VIDEO_RENDERING_PENDING) {
      if ($video_conversion->process($video)) {
        drupal_set_message(t('Transcoding job submission was successful. Your video will be converted and published very soon.'));
      }
      else {
        $video_conversion->changeStatus($video->vid, VIDEO_RENDERING_FAILED);
        drupal_set_message(t('Something went wrong with your video transcoding. Please check your recent log entries for further debugging.'), 'error');
      }
    }
  }
}

function _video_field_get_all_thumbnails(array $items) {
  if (empty($items)) {
    return array();
  }

  $videofids = array();
  foreach ($items as $item) {
    if (!empty($item['fid'])) {
      $videofids[] = intval($item['fid']);
    }
  }

  if (empty($videofids)) {
    return array();
  }

  $thumbnailfids = db_select('video_thumbnails', 't')
    ->fields('t', array('thumbnailfid'))->condition('videofid', $videofids, 'IN')
    ->execute()->fetchCol(0);

  return video_utility::objectToArray(file_load_multiple($thumbnailfids));
}

function _video_field_get_all_converted(array $items) {
  if (empty($items)) {
    return array();
  }

  $videofids = array();
  foreach ($items as $item) {
    if (!empty($item['fid'])) {
      $videofids[] = intval($item['fid']);
    }
  }

  if (empty($videofids)) {
    return array();
  }

  $convertedfids = db_select('video_output', 't')
    ->fields('t', array('output_fid'))->condition('original_fid', $videofids, 'IN')
    ->execute()->fetchCol(0);

  return video_utility::objectToArray(file_load_multiple($convertedfids));
}
