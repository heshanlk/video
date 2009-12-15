<?php
// $Id$

/**
 * @file
 * uploadfield widget hooks and callbacks.
 */

/**
 * Implementation of CCK's hook_widget_settings($op = 'form').
 */
function uploadfield_widget_settings_form($widget) {
  $form = module_invoke('filefield', 'widget_settings', 'form', $widget);

  if ($form['file_extensions']['#default_value'] == 'txt') {
    $form['file_extensions']['#default_value'] = 'mp4 mpeg avi mpg wmv';
  }

  // Default video player settings.
  $form['player'] = array(
      '#type' => 'fieldset',
      '#title' => t('Video Player Settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#weight' => 9
  );

  $form['player']['default_resolution'] = array(
      '#type' => 'textfield',
      '#title' => t('Default Video Resolution'),
      '#default_value' => !empty($widget['default_resolution']) ? $widget['default_resolution'] : '16:9',
      '#size' => 15,
      '#maxlength' => 5,
      '#description' => t('Default player resolution WIDTHXHEIGHT in px. eg : 16:9 for widescreen or 4:3 for general screen'),
  );

  $form['player']['default_width'] = array(
      '#type' => 'textfield',
      '#title' => t('Default Video Player Width'),
      '#default_value' => !empty($widget['default_width']) ? $widget['default_width'] : 640,
      '#size' => 15,
      '#maxlength' => 5,
      '#description' => t('Default player WIDTHXHEIGHT in px. eg : 640 for 640X480 player size if resolution it 4:3'),
  );


  // Default image settings.
  $form['default'] = array(
      '#type' => 'fieldset',
      '#title' => t('Video Thumbnail Settings'),
      '#element_validate' => array('_uploadfield_widget_settings_default_validate'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#weight' => 10
  );

  // Present a video image of the current default image.
  if (!empty($widget['default_video_thumb'])) {
    $form['default']['default_video_thumbnail'] = array(
        '#type' => 'markup',
        '#value' => theme('uploadfield_image', $widget['default_video_thumb'], '', '', array('width' => '150'), FALSE),
        '#prefix' => '<div class="video_thumbnail">',
        '#suffix' => '</div>'
    );
  }

  $form['default']['default_video_thumb_upload'] = array(
      '#type'  => 'file',
      '#title' => empty($widget['default_video_thumb']) ? t('Upload video thumbnail') :  t('Replace video thumbnail with'),
      '#description' => t('Choose a image that will be used as default video thumbnail.'),
  );

  // We set this value on 'validate' so we can get CCK to add it
  // as a standard field setting.
  $form['default_video_thumb'] = array(
      '#type' => 'value',
      '#value' => $widget['default_video_thumb'],
  );

  return $form;
}

/**
 * Element specific validation for uploadfield default value.
 *
 * Validated in a separate function from uploadfield_field() to get access
 * to the $form_state variable.
 */
function _uploadfield_widget_settings_default_validate($element, &$form_state) {
// Verify the destination exists
  $destination = file_directory_path() .'/default_video_thumb';
  if (!field_file_check_directory($destination, FILE_CREATE_DIRECTORY)) {
    form_set_error('default_video_thumb', t('The default image could not be uploaded. The destination %destination does not exist or is not writable by the server.', array('%destination' => dirname($destination))));
    return;
  }

  $validators = array(
      'file_validate_is_image' => array(),
  );

  // We save the upload here because we can't know the correct path until the file is saved.
  if (!$file = file_save_upload('default_video_thumb_upload', $validators, $destination)) {
  // No upload to save we hope... or file_save_upload() reported an error on its own.
    return;
  }

  // Remove old image (if any) & clean up database.
  $old_default = $form_state['values']['default_video_thumb'];
  if (!empty($old_default['fid'])) {
    if (file_delete(file_create_path($old_default['filepath']))) {
      db_query('DELETE FROM {files} WHERE fid=%d', $old_default['fid']);
    }
  }

  // Make the file permanent and store it in the form.
  file_set_status($file, FILE_STATUS_PERMANENT);
  $file->timestamp = time();
  $form_state['values']['default_video_thumb'] = (array)$file;
}

/**
 * Implementation of CCK's hook_widget_settings($op = 'validate').
 */
function uploadfield_widget_settings_validate($widget) {
// Check that only web images are specified in the callback.
  $extensions = array_filter(explode(' ', $widget['file_extensions']));
  $web_extensions = array('mp4', 'mpeg', 'mpg', 'wmv', 'avi', 'flv');
  if (count(array_diff($extensions, $web_extensions))) {
    form_set_error('file_extensions', t('Only web-standard videos (mp4, mpeg, mpg, wmv and avi) are supported through the image widget. If needing to upload other types of files, change the widget to use a standard file upload.'));
  }

  // Check that set resolutions are valid.
  foreach (array('default_width') as $numerics) {
    if (!empty($widget[$numerics]) && !preg_match('/^[0-9]+$/', $widget[$numerics])) {
      form_set_error($numerics, t('Please specify default width in integers only (e.g. 640).'));
    }
  }

  // Check that set resolutions are valid.
  foreach (array('default_resolution') as $resolution) {
    if (!empty($widget[$resolution]) && !preg_match('/^[0-9]+:[0-9]+$/', $widget[$resolution])) {
      form_set_error($resolution, t('Please specify a resolution in the format WIDTH:HEIGHT (e.g. 16:9).'));
    }
  }
}

/**
 * Implementation of CCK's hook_widget_settings($op = 'save').
 */
function uploadfield_widget_settings_save($widget) {
  $filefield_settings = module_invoke('filefield', 'widget_settings', 'save', $widget);
  return array_merge($filefield_settings, array('default_video_thumb', 'use_default_video_thumb', 'use_custom_width', 'default_width'));
}

/**
 * Element #value_callback function.
 */
function uploadfield_widget_value($element, $edit = FALSE) {
  $item = filefield_widget_value($element, $edit);
  if ($edit) {
    $item['alt'] = isset($edit['alt']) ? $edit['alt'] : '';
    $item['title'] = isset($edit['title']) ? $edit['title'] : '';
    $item['default_video_thumb'] = isset($edit['default_video_thumb']['fid']) ? $edit['default_video_thumb']['fid'] : 'novalue';
  }
  else {
    $item['alt'] = '';
    $item['title'] = '';
    $item['default_video_thumb'] = '';
  }
  return $item;
}

/**
 * Element #process callback function.
 */
function uploadfield_widget_process($element, $edit, &$form_state, $form) {
// TODO : get play duration/ image thumbnails
  $file = $element['#value'];
  $field = content_fields($element['#field_name'], $element['#type_name']);
  //  print_r($field['widget']);
  //  exit;
  $element['#theme'] = 'imagefield_widget_item';

  if (isset($element['preview']) && $element['#value']['fid'] != 0) {
  //TODO : implement preview video
    $element['preview']['#suffix'] = '<div class="video_thumbnail">' .
        theme('uploadfield_image', $field['widget']['default_video_thumb'], '', '', array('width' => '150'), FALSE) .
        '</div>';
  }

  // Check if using the default width and replace tokens.
  $default_width = user_access('override player width');
    $element['data']['width'] = array(
        '#type' => $default_width ? 'textfield' : 'value',
        '#title' => t('Width for Player'),
        '#default_value' => $default_width ? $field['widget']['default_width'] : $file['data']['default_width'],
        '#description' => t('Set player width(in pix) here, make sure your video have good resolution to fix on larger values.'),
        '#size' => 15,
        '#maxlength' => 5,
        '#attributes' => array('class' => 'video-width-text'),
    );
    //  echo $field['widget']['default_width'];
    // #value must be hard-coded if #type = 'value'.
    if ($default_width) {
      $element['data']['width']['#value'] = $field['widget']['default_width'];
    }

  // default image default_image
  // use default thumb
  // TODO : extend permission to override node wise thumbnail
  //  $default_vid_thumb = (!$field['widget']['use_default_video_thumb'] || (empty($file['status']) && empty($file['data']['use_default_video_thumb'])));
  // user access for default thumbnail
  //  echo $file['data']['use_default_video_thumb'];
  if (user_access('use default thumbnail')) {
    $element['data']['use_default_video_thumb'] = array(
        '#type' => 'checkbox',
        '#title' => t('Use Default Video Thumbnail'),
        '#default_value' => empty($file['data']['use_default_video_thumb']) ? 1 : $file['data']['use_default_video_thumb'],
        '#description' => t('If you want to use default image instead of using actual thumbnail of video then check.'),
    );
  }

  //bypass auto conversion
  if (user_access('bypass conversion video')) {
    $element['data']['bypass_autoconversion'] = array(
        '#type' => 'checkbox',
        '#title' => t('Bypass auto conversion'),
        '#default_value' => !empty($file['data']['bypass_autoconversion']) ? $file['data']['bypass_autoconversion'] : FALSE,
        '#description' => t('This will bypass your auto conversion of videos.'),
    );
  }

  return $element;
}

/**
 * FormAPI theme function. Theme the output of an image field.
 */
function theme_uploadfield_widget($element) {
  drupal_add_css(drupal_get_path('module', 'uploadfield') .'/uploadfield.css');
  return theme('form_element', $element, $element['#children']);
}
