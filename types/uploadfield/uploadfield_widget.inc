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
    $form['file_extensions']['#default_value'] = 'mp4 mpeg avi mpg wmv flv mov';
  }

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
  if(!video_web_extensions($widget['file_extensions'])) {
    form_set_error('file_extensions', t('Only web-standard videos are supported through the videoftp widget. If needing to upload other types of files, change the widget to use a standard file upload.'));
  }
}

/**
 * Implementation of CCK's hook_widget_settings($op = 'save').
 */
function uploadfield_widget_settings_save($widget) {
  $filefield_settings = module_invoke('filefield', 'widget_settings', 'save', $widget);
  return array_merge($filefield_settings, array('default_video_thumb', 'default_dimensions', 'autoconversion', 'autothumbnail'));
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
          //lets set our node status to unpublished if our video is not converted.
          if ($element['#unpublish']) {
            $form_state['values']['status'] = 0;
          }
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