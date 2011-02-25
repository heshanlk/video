<?php
/**
 * 
 * @file
 * Theme functions for the videoftp module.
 */

/**
 * FormAPI theme function. Theme the output of an video ftp field.
 */
function theme_videoftp_widget($element) {
  return theme('form_element', $element, $element['#children']);
}

function theme_videoftp_widget_item($element) {
  // Put the upload button directly after the upload field.
  $element['ftpselect']['#field_suffix'] = drupal_render($element['videoftp_attach']);
  $element['ftpselect']['#theme'] = 'videoftp_widget_file';
  $output = '<div class="filefield-element clear-block">';

  if ($element['fid']['#value'] != 0) {
    $output .= '<div class="widget-preview">';
    $output .= drupal_render($element['preview']);
    $output .= '</div>';
  }

  $output .= '<div class="widget-edit">';
  $output .=  drupal_render($element);
  $output .= '</div>';
  $output .= '</div>';

  return $output;
}

/**
 * Custom theme function for VideoFTP upload elements.
 *
 * This function allows us to put the "Attach" button immediately after the
 * select field by respecting the #field_suffix property.
 */
function theme_videoftp_widget_file($element) {
  $output .= '<div class="filefield-upload clear-block">';
  if (isset($element['#field_prefix'])) {
    $output .= $element['#field_prefix'];
  }
  $size = $element['#size'] ? ' size="'. $element['#size'] .'"' : '';
  _form_set_class($element, array('form-select'));
  $multiple = $element['#multiple'];
  $output .= '<select name="'. $element['#name'] .''. ($multiple ? '[]' : '') .'"'. ($multiple ? ' multiple="multiple" ' : '') . drupal_attributes($element['#attributes']) .' id="'. $element['#id'] .'" '. $size .'>'. form_select_options($element) ."</select>\n";
  if (isset($element['#field_suffix'])) {
    $output .= $element['#field_suffix'];
  }
  $output .= '</div>';

  return theme('form_element', $element, $output);
}