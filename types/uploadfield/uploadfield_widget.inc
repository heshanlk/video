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
    $form['file_extensions']['#default_value'] = 'mp4 mpeg avi mpg wmv flv';
  }

  // Default video settings.
  $form['plugins'] = array(
    '#type' => 'fieldset',
    '#title' => t('Video Advanced Settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#weight' => 10
  );
  $form['plugins']['default_resolution'] = array(
      '#type' => 'textfield',
      '#title' => t('Default Video Resolution'),
      '#default_value' => !empty($widget['default_resolution']) ? $widget['default_resolution'] : '16:9',
      '#size' => 15,
      '#maxlength' => 5,
      '#description' => t('Default player resolution WIDTH:HEIGHT in px. eg : 16:9 for widescreen or 4:3 for general screen'),
  );
  $form['plugins']['default_width'] = array(
      '#type' => 'textfield',
      '#title' => t('Default Video Player Width'),
      '#default_value' => !empty($widget['default_width']) ? $widget['default_width'] : 640,
      '#size' => 15,
      '#maxlength' => 5,
      '#description' => t('Default player WIDTH:HEIGHT in px. eg : 640 for 640X480 player size if resolution it 4:3'),
  );
  $form['plugins']['autoconversion'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable video conversion.'),
    '#description' => t('Use ffmpeg(Default) to automatically convert videos to web compatible types eg. FLV, Please make sure to configure your transcoder settings.'),
    '#default_value' => $widget['autoconversion'],
  );
  $form['plugins']['autothumbnail'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable thumbnail creation.'),
    '#description' => t('Use ffmpeg(Default) to create thumbnails, Please make sure to configure your transcoder settings.'),
    '#default_value' => $widget['autothumbnail'],
  );


  // Default thumbnail settings.
  $form['default'] = array(
      '#type' => 'fieldset',
      '#title' => t('Video Thumbnail Settings'),
      '#element_validate' => array('_uploadfield_widget_settings_default_validate'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
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
  $destination = file_directory_path() .'/video_thumb';
  if (!field_file_check_directory($destination, FILE_CREATE_DIRECTORY)) {
    form_set_error('default_video_thumb', t('The default image could not be uploaded. The destination %destination does not exist or is not writable by the server.', array('%destination' => dirname($destination))));
    return;
  }

  $validators = array (
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
  if(!video_web_extensions($widget['file_extensions'])) {
    form_set_error('file_extensions', t('Only web-standard videos are supported through the videoftp widget. If needing to upload other types of files, change the widget to use a standard file upload.'));
  }

  // Check that set video resolutions are valid.
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
  return array_merge($filefield_settings, array('default_video_thumb', 'default_width', 'default_resolution', 'autoconversion', 'autothumbnail'));
}

/**
 * Element #value_callback function.
 */
function uploadfield_widget_value($element, $edit = FALSE) {
  $item = filefield_widget_value($element, $edit);
  return $item;
}

/**
 * Element #process callback function.
 */
function uploadfield_widget_process($element, $edit, &$form_state, $form) {
  $item = $element['#value'];
  $field_name = $element['#field_name'];
  $delta = $element['#delta'];
  $field = content_fields($element['#field_name'], $element['#type_name']);
  $element['#theme'] = 'uploadfield_widget_item';
  
  if (isset($element['preview']) && $element['#value']['fid'] != 0) {
    $element['preview']['#value'] = theme('video_widget_preview', $element['#value']);
  }

  // Title is not necessary for each individual field.
  if ($field['multiple'] > 0) {
    unset($element['#title']);
  }
  
  // Add our extra fields if in preview mode
  if(!empty($item['fid'])) {
    video_widget_element_settings($element);
  }
  
  // Create our thumbnails
  if($field['widget']['autothumbnail']) {
    video_thumb_process($element);
  }

  // Lets use the clicked_button #submit[0] value here instead and see how that works out for now...
  if($form_state['submitted'] == 1) {
    switch($form_state['clicked_button']['#submit'][0]) {      
      case 'node_form_submit':
        // Auto convert our video file
        if($field['widget']['autoconversion']) {
          video_convert_process($element);
        }
        break;
      case 'node_form_build_preview':
        break;
      case 'node_form_delete_submit':
        //moved to hook_file_delete in video module.
        break;
    }
  }
  return $element;
}