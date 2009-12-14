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

  // Default image settings.
  $form['default'] = array(
    '#type' => 'fieldset',
    '#title' => t('Default Video Thumbnail'),
    '#element_validate' => array('_uploadfield_widget_settings_default_validate'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 10
  );

  // Present a video image of the current default image.
  $form['default']['use_default_thumb'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use default video thumbnail'),
    '#default_value' =>  TRUE,
    '#description' => t('Choose a image as a video thumbnail.'),
  );

  if (!empty($widget['default_image'])) {
    $form['default']['default_image_thumbnail'] = array(
      '#type' => 'markup',
      '#value' => theme('uploadfield_image', $widget['default_image'], '', '', array('width' => '150'), FALSE),
    );
  }
  $form['default']['default_image_upload'] = array(
    '#type'  => 'file',
    '#title' => t('Upload image'),
    '#description' => t('Choose a image that will be used as default video thumbnail.'),
  );

  // We set this value on 'validate' so we can get CCK to add it
  // as a standard field setting.
  $form['default_image'] = array(
    '#type' => 'value',
    '#value' => $widget['default_image'],
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
  $destination = file_directory_path() .'/uploadfield_default_images';
  if (!field_file_check_directory($destination, FILE_CREATE_DIRECTORY)) {
    form_set_error('default_image', t('The default image could not be uploaded. The destination %destination does not exist or is not writable by the server.', array('%destination' => dirname($destination))));
    return;
  }

  $validators = array(
    'file_validate_is_image' => array(),
  );

  // We save the upload here because we can't know the correct path until the file is saved.
  if (!$file = file_save_upload('default_image_upload', $validators, $destination)) {
    // No upload to save we hope... or file_save_upload() reported an error on its own.
    return;
  }

  // Remove old image (if any) & clean up database.
  $old_default = $form_state['values']['default_image'];
  if (!empty($old_default['fid'])) {
    if (file_delete(file_create_path($old_default['filepath']))) {
      db_query('DELETE FROM {files} WHERE fid=%d', $old_default['fid']);
    }
  }

  // Make the file permanent and store it in the form.
  file_set_status($file, FILE_STATUS_PERMANENT);
  $file->timestamp = time();
  $form_state['values']['default_image'] = (array)$file;
 }

/**
 * Implementation of CCK's hook_widget_settings($op = 'validate').
 */
function uploadfield_widget_settings_validate($widget) {
  // Check that only web images are specified in the callback.
  $extensions = array_filter(explode(' ', $widget['file_extensions']));
  $web_extensions = array('mp4', 'mpeg', 'mpg', 'wmv', 'avi');
  if (count(array_diff($extensions, $web_extensions))) {
    form_set_error('file_extensions', t('Only web-standard videos (mp4, mpeg, mpg, wmv and avi) are supported through the image widget. If needing to upload other types of files, change the widget to use a standard file upload.'));
  }

  // Check that set resolutions are valid.
  foreach (array('min_resolution', 'max_resolution') as $resolution) {
    if (!empty($widget[$resolution]) && !preg_match('/^[0-9]+x[0-9]+$/', $widget[$resolution])) {
      form_set_error($resolution, t('Please specify a resolution in the format WIDTHxHEIGHT (e.g. 640x480).'));
    }
  }
}

/**
 * Implementation of CCK's hook_widget_settings($op = 'save').
 */
function uploadfield_widget_settings_save($widget) {
  $filefield_settings = module_invoke('filefield', 'widget_settings', 'save', $widget);
  return array_merge($filefield_settings, array('default_image', 'use_default_thumb'));
}

/**
 * Element #value_callback function.
 */
function uploadfield_widget_value($element, $edit = FALSE) {
  $item = filefield_widget_value($element, $edit);
  if ($edit) {
    $item['alt'] = isset($edit['alt']) ? $edit['alt'] : '';
    $item['title'] = isset($edit['title']) ? $edit['title'] : '';
    $item['default_image'] = isset($edit['default_image']['fid']) ? $edit['default_image']['fid'] : 'novalue';
  }
  else {
    $item['alt'] = '';
    $item['title'] = '';
    $item['default_image'] = 'noval';
  }
  return $item;
}

/**
 * Element #process callback function.
 */
function uploadfield_widget_process($element, $edit, &$form_state, $form) {
  $file = $element['#value'];
  $field = content_fields($element['#field_name'], $element['#type_name']);

  $element['#theme'] = 'uploadfield_widget_item';

  if (isset($element['preview']) && $element['#value']['fid'] != 0) {
    $element['preview']['#value'] = theme('uploadfield_widget_preview', $element['#value']);
  }

  // Check if using the default alt text and replace tokens.
  $default_thumb = ($field['widget']['use_default_thumb']);
  // #value must be hard-coded if #type = 'value'.
  if ($default_thumb) {
    $element['data']['default_image']['#value'] = $field['widget']['default_image']['fid'];
  }
//  $element['data']['default_image']['#value'] = $field['widget']['default_image']['fid'];
  return $element;
}

/**
 * FormAPI theme function. Theme the output of an image field.
 */
function theme_uploadfield_widget($element) {
  drupal_add_css(drupal_get_path('module', 'uploadfield') .'/uploadfield.css');
  return theme('form_element', $element, $element['#children']);
}
