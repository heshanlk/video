<?php


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
    $form['file_extensions']['#default_value'] = 'mp4 mpeg avi mpg wmv flv mov';
  }
  //fix our path settings
  $form['path_settings']['file_path']['#default_value'] = ltrim(ltrim($form['path_settings']['file_path']['#default_value'], "videos"), "/"); //wierd i had to break this into two ltrims...
  $form['path_settings']['file_path']['#description'] = t('Optional subdirectory within the "<em>files/videos/</em>" directory where files will be stored. Do not include preceding or trailing slashes.');
  array_unshift($form['path_settings']['file_path']['#element_validate'], 'video_widget_settings_file_path_validate');

  //default settings
  $default = video_default_widget_settings($widget);
  $form = $form + $default;
  return $form;
}

/**
 * Implementation of CCK's hook_widget_settings($op = 'validate').
 */
function uploadfield_widget_settings_validate($widget) {
  // Check that only web images are specified in the callback.
  if (!video_web_extensions($widget['file_extensions'])) {
    form_set_error('file_extensions', t('Only web-standard videos are supported through the videoftp widget. If needing to upload other types of files, change the widget to use a standard file upload.'));
  }
}

/**
 * Implementation of CCK's hook_widget_settings($op = 'save').
 */
function uploadfield_widget_settings_save($widget) {
  $filefield_settings = module_invoke('filefield', 'widget_settings', 'save', $widget);
  return array_merge($filefield_settings, array('default_video_thumb', 'default_dimensions', 'default_player_dimensions', 'autoconversion', 'autothumbnail'));
}

/**
 * Element #value_callback function.
 */
function uploadfield_widget_value($element, $edit = FALSE) {
  //copied from filefield_widget_value with one change to reset our data array
  if (!$edit) {
    $file = field_file_load($element['#default_value']['fid']);
    $item = $element['#default_value'];
  } else {
    $item = array_merge($element['#default_value'], $edit);
    if (isset($element['#default_value']['data']) && isset($edit['data'])) {
      $item['data'] = array_merge($element['#default_value']['data'], $edit['data']);
    }
    $field = content_fields($element['#field_name'], $element['#type_name']);

    // Uploads take priority over value of fid text field.
    if ($fid = filefield_save_upload($element)) {
      $item['fid'] = $fid;
      $item['data'] = array(); //reset our data array for thumbnails and values.
    }
    // Check for #filefield_value_callback values.
    // Because FAPI does not allow multiple #value_callback values like it does
    // for #element_validate and #process, this fills the missing functionality
    // to allow FileField to be extended purely through FAPI.
    elseif (isset($element['#filefield_value_callback'])) {
      foreach ($element['#filefield_value_callback'] as $callback) {
        $callback($element, $item);
      }
    }

    // Load file if the FID has changed so that it can be saved by CCK.
    $file = field_file_load($item['fid']);

    // If the file entry doesn't exist, don't save anything.
    if (empty($file)) {
      $item = array();
    }

    // Checkboxes loose their value when empty.
    // If the list field is present make sure its unchecked value is saved.
    if (!empty($field['list_field']) && empty($edit['list'])) {
      $item['list'] = 0;
    }
  }
  // Merge file and item data so it is available to all widgets.
  if (isset($item['data']) && isset($file['data'])) {
    $file['data'] = array_merge($item['data'], $file['data']);
  }
  $item = array_merge($item, $file);

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

  // Create our thumbnails
  if ($field['widget']['autothumbnail']) {
    video_thumb_process($element);
  }

  // Add our extra fields if in preview mode
  if (!empty($item['fid'])) {
    video_widget_element_settings($element);
  }

  // Lets use the clicked_button #submit[0] value here instead and see how that works out for now...
  if ($form_state['submitted'] == 1) {
    video_widget_process($element, $form_state);
  }
  return $element;
}