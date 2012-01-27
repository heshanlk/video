<?php
/**
 * @file
 * Theme functions for the uploadfield module.
 */

/**
 * FormAPI theme function. Theme the output of an video upload field.
 */
function theme_uploadfield_widget($element) {
  return theme('form_element', $element, $element['#children']);
}

function theme_uploadfield_widget_item($element) {
  return theme('filefield_widget_item', $element);
}