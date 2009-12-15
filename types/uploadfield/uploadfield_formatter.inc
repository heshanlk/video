<?php
// $Id$

/**
 * @file
 * uploadfield formatter hooks and callbacks.
 */

//include common player file


/**
 *
 * @param <type> $element
 * @return <type> 
 */
function theme_uploadfield_formatter_video_plain($element) {
//  print_r($field = content_fields($element['#field_name'], $element['#type_name']););
  // Inside a view $element may contain null data. In that case, just return.
  module_load_include('inc', 'video', '/includes/common');

  if (empty($element['#item']['fid'])) {
    return '';
  }

  return _video_common_get_player($element);

//  $field = content_fields($element['#field_name']);
//  $item = $element['#item'];
//
//  $item['data']['alt'] = isset($item['data']['alt']) ? $item['data']['alt'] : '';
//  $item['data']['title'] = isset($item['data']['title']) ? $item['data']['title'] : NULL;
//
//  $class = 'uploadfield uploadfield-'. $field['field_name'];
//  return  theme('uploadfield_image', $item, $item['data']['alt'], $item['data']['title'], array('class' => $class));
}


/**
 *
 * @param <type> $element
 * @return <type> 
 */
 
function theme_uploadfield_formatter_video_nodelink($element) {
  $field = content_fields($element['#field_name'], $element['#type_name']);
  // Inside a view $element may contain null data. In that case, just return.
  if (empty($element['#item']['fid'])) {
    return '';
  }

  $node = $element['#node'];
//  $imagetag = theme('uploadfield_formatter_video_plain', $element);
  $imagetag = theme('uploadfield_widget_video_thumb', $field['widget']['default_video_thumb']);
  $class = 'uploadfield uploadfield-nodelink uploadfield-'. $element['#field_name'];
  return l($imagetag, 'node/'. $node->nid, array('attributes' => array('class' => $class), 'html' => TRUE));
}

/**
 *
 * @param <type> $element
 * @return <type> 
 */
function theme_uploadfield_formatter_video_imagelink($element) {
  // Inside a view $element may contain null data. In that case, just return.
  if (empty($element['#item']['fid'])) {
    return '';
  }

  $item = $element['#item'];
  $imagetag = theme('uploadfield_formatter_video_plain', $element);
  $original_image_url = file_create_url($item['filepath']);
  $class = 'uploadfield uploadfield-imagelink uploadfield-'. $element['#field_name'];
  return l($imagetag, $original_image_url, array('attributes' => array('class' => $class), 'html' => TRUE));
}
