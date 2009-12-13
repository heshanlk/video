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

  $form['default_width'] = array(
    '#type' => 'textfield',
    '#title' => t('Default video player width'),
    '#default_value' => !empty($widget['default_width']) ? $widget['default_width'] : 640,
    '#size' => 15,
    '#maxlength' => 10,
    '#description' => t('The width of the video player.'),
    '#weight' => 2.1,
  );
  $form['min_resolution'] = array(
    '#type' => 'textfield',
    '#title' => t('Minimum resolution for Video Images'),
    '#default_value' => !empty($widget['min_resolution']) ? $widget['min_resolution'] : '100x100',
    '#size' => 15,
    '#maxlength' => 10,
    '#description' =>
    t('The minimum allowed image size expressed as WIDTHxHEIGHT (e.g. 640x480). Set to 0 for no restriction. If an image that is smaller than these dimensions is uploaded it will be rejected.'),
    '#weight' => 2.2,
  );

  // Default image settings.
  $form['default'] = array(
    '#type' => 'fieldset',
    '#title' => t('Default video image'),
    '#element_validate' => array('_uploadfield_widget_settings_default_validate'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 10
  );

  // Present a video image of the current default image.
  $form['default']['use_default_image'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use default image'),
    '#default_value' =>  TRUE,
    '#description' => t('When an video image is not uploaded, show a default image on display.'),
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
    '#description' => t('Choose a image that will be used as default.'),
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
  return array_merge($filefield_settings, array('max_resolution', 'min_resolution', 'alt',  'custom_alt', 'title', 'custom_title', 'title_type', 'default_image', 'use_default_image'));
}

/**
 * Element #value_callback function.
 */
function uploadfield_widget_value($element, $edit = FALSE) {
  $item = filefield_widget_value($element, $edit);
  if ($edit) {
    $item['alt'] = isset($edit['alt']) ? $edit['alt'] : '';
    $item['title'] = isset($edit['title']) ? $edit['title'] : '';
  }
  else {
    $item['alt'] = '';
    $item['title'] = '';
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
  $default_alt = (!$field['widget']['custom_alt'] || (empty($file['status']) && empty($file['data']['alt'])));
  if ($default_alt && function_exists('token_replace')) {
    $field['widget']['alt'] = empty($field['widget']['alt']) ? '' : token_replace($field['widget']['alt'], 'user', $GLOBALS['user']);
  }
  $element['data']['alt'] = array(
    '#title' => t('Alternate Text'),
    '#type' => $field['widget']['custom_alt'] ? 'textfield' : 'value',
    '#default_value' => $default_alt ? $field['widget']['alt'] : $file['data']['alt'],
    '#description' => t('This text will be used by screen readers, search engines, or when the image cannot be loaded.'),
    '#maxlength' => variable_get('uploadfield_alt_length', 80), // See http://www.gawds.org/show.php?contentid=28.
    '#attributes' => array('class' => 'uploadfield-text'),
  );
  // #value must be hard-coded if #type = 'value'.
  if ($default_alt) {
    $element['data']['alt']['#value'] = $field['widget']['alt'];
  }

  // Check if using the default title and replace tokens.
  $default_title = (!$field['widget']['custom_title'] || (empty($file['status']) && empty($file['data']['title'])));
  if ($default_title && function_exists('token_replace')) {
    $field['widget']['title'] = empty($field['widget']['title']) ? '' : token_replace($field['widget']['title'], 'user', $GLOBALS['user']);
  }

  // If the custom title is enabled, input type defaults to textfield.
  if (!empty($field['widget']['custom_title'])) {
    $title_type = !empty($field['widget']['title_type']) ? $field['widget']['title_type'] : 'textfield';
  }
  else {
    $title_type = 'value';
  }

  $element['data']['title'] = array(
    '#type' => $title_type,
    '#title' => t('Title'),
    '#default_value' => $default_title ? $field['widget']['title'] : $file['data']['title'],
    '#description' => t('The title is used as a tool tip when the user hovers the mouse over the image.'),
    '#maxlength' => variable_get('uploadfield_title_length', 500),
    '#attributes' => array('class' => 'uploadfield-text'),
  );
  // #value must be hard-coded if #type = 'value'.
  if ($default_title) {
    $element['data']['title']['#value'] = $field['widget']['title'];
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
