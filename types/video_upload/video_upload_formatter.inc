<?php
// $Id$

/**
 * @file
 * video_upload formatter hooks and callbacks.
 */

function theme_video_upload_formatter_video_plain($element) {
  // Inside a view $element may contain null data. In that case, just return.
  if (empty($element['#item']['fid'])) {
    return '';
  }

  $field = content_fields($element['#field_name']);
  $item = $element['#item'];

  $item['data']['alt'] = isset($item['data']['alt']) ? $item['data']['alt'] : '';
  $item['data']['title'] = isset($item['data']['title']) ? $item['data']['title'] : NULL;

  $class = 'video_upload video_upload-'. $field['field_name'];
  return  theme('video_upload_image', $item, $item['data']['alt'], $item['data']['title'], array('class' => $class));
}

function theme_video_upload_formatter_video_nodelink($element) {
  // Inside a view $element may contain null data. In that case, just return.
  if (empty($element['#item']['fid'])) {
    return '';
  }

  $node = $element['#node'];
  $imagetag = theme('video_upload_formatter_video_plain', $element);
  $class = 'video_upload video_upload-nodelink video_upload-'. $element['#field_name'];
  return l($imagetag, 'node/'. $node->nid, array('attributes' => array('class' => $class), 'html' => TRUE));
}

function theme_video_upload_formatter_video_imagelink($element) {
  // Inside a view $element may contain null data. In that case, just return.
  if (empty($element['#item']['fid'])) {
    return '';
  }

  $item = $element['#item'];
  $imagetag = theme('video_upload_formatter_video_plain', $element);
  $original_image_url = file_create_url($item['filepath']);
  $class = 'video_upload video_upload-imagelink video_upload-'. $element['#field_name'];
  return l($imagetag, $original_image_url, array('attributes' => array('class' => $class), 'html' => TRUE));
}
