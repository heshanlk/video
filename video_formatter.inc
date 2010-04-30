<?php
// $Id$

/**
 * @file
 * Video formatter hooks and callbacks.
 */

function theme_video_formatter_video_plain($element) {
  //module_load_include('inc', 'video', '/includes/common');
  if (empty($element['#item']['fid'])) return '';
  $result = db_query('SELECT vr.status FROM {video_rendering} vr INNER JOIN {files} f ON vr.fid = f.fid WHERE vr.fid = f.fid AND f.status = %d AND f.fid = %d', FILE_STATUS_PERMANENT, $element['#item']['fid']);
  $status = db_fetch_object($result);
  if($status->status == VIDEO_RENDERING_ACTIVE || $status->status == VIDEO_RENDERING_PENDING) {
    return theme('video_inprogress');
  }
  else if ($status->status == VIDEO_RENDERING_FAILED) {
    return theme('video_encoding_failed');
  }
  else {
    //lets get our width/height and video url and pass them as arguments instead of the full element object
    $video = file_create_url($element['#item']['filepath']);
    //@todo change default width and height to be configuration settings.
    $width = isset($element['#item']['data']['width']) && !empty($element['#item']['data']['width']) ? $element['#item']['data']['width'] .'px' : '350px';
    $height = isset($element['#item']['data']['height']) && !empty($element['#item']['data']['height']) ? $element['#item']['data']['height'] .'px' : '285px';
    return video_get_player($element);
  }
}

/**
 *
 * @param <type> $element
 * @return <type>
 */

function theme_video_formatter_video_nodelink($element) {
  $field = content_fields($element['#field_name'], $element['#type_name']);
  // Inside a view $element may contain null data. In that case, just return.
  if (empty($element['#item']['fid'])) return '';
  $node = $element['#node'];
  $use_default_img = $element['#item']['data']['use_default_video_thumb'];
  if($use_default_img) {
    $imagetag = theme('uploadfield_widget_video_thumb', $field['widget']['default_video_thumb']);
  }
  else if($element['#item']['data']['video_thumb']) {
    //    print_r($field['widget']['default_video_thumb']);
    $thumb = array('filepath' => $element['#item']['data']['video_thumb']);
    $imagetag = theme('uploadfield_widget_video_thumb', $thumb);
  }
  $class = 'popups uploadfield uploadfield-nodelink uploadfield-'. $element['#field_name'];
  return l($imagetag, 'node/'. $node->nid, array('attributes' => array('class' => $class), 'html' => TRUE));

}

/**
 * use image cache profiles for video thumb display
 * @param object $element
 * @return HTML image file
 */

function theme_uploadfield_formatter_videolinked($element) {
  // Inside a view $element may contain NULL data. In that case, just return.
  if (empty($element['#item']['fid'])) {
    return '';
  }

  // Extract the preset name from the formatter name.
  $presetname = substr($element['#formatter'], 0, strrpos($element['#formatter'], '_'));
  $style = 'linked';

  $item = $element['#item'];
  $item['data']['alt'] = isset($item['data']['alt']) ? $item['data']['alt'] : '';
  $item['data']['title'] = isset($item['data']['title']) ? $item['data']['title'] : NULL;

  $node = $element['#node'];
  //  $imagetag = theme('uploadfield_formatter_video_plain', $element);
  $use_default_img = $element['#item']['data']['use_default_video_thumb'];
  if($use_default_img) {
    $thumb_path = $field['widget']['default_video_thumb'];
    //    $imagetag = theme('uploadfield_widget_video_thumb', $field['widget']['default_video_thumb']);
  }
  else if($element['#item']['data']['video_thumb']) {
    //    print_r($field['widget']['default_video_thumb']);
    //      $thumb = array('filepath' => $element['#item']['data']['video_thumb']);
    $thumb_path = $element['#item']['data']['video_thumb'];
    //      $imagetag = theme('uploadfield_widget_video_thumb', $thumb);
  }
  //  die($presetname);
  $imagetag = theme('imagecache', $presetname, $thumb_path, $node->title, $node->title);
  $path = empty($item['nid']) ? '' : 'node/'. $item['nid'];
  $class = "imagecache imagecache-$presetname imagecache-$style imagecache-{$element['#formatter']}";
  return l($imagetag, $path, array('attributes' => array('class' => $class), 'html' => TRUE));
}

/**
 * Displays a "encoding in progress message"
 */
function theme_video_inprogress() {
  return '<div class="video-ffmpeg-helper-inprogress">'. t('This video is currently being processed. Please wait.') . '</div>';
}

/**
 * Display an "encoding failed" message"
 */
function theme_video_encoding_failed() {
  return '<div class="video-ffmpeg-helper-encoding-failed">'. t('The video conversion process has failed. You might want to submit a simpler video format like <em>mpeg</em> or <em>divx avi</em>.<br />If the problem persists please contact website administrators.') . '</div>';
}

function video_preprocess_video_play_flv(&$vars) {
  //lets determine what player to output
  $pathinfo = pathinfo($vars['video']);
  $player_id = variable_get('video_extension_'.$pathinfo['extension'].'_flash_player', '');
  if ($player_id == 'swftools') {
    $output = swf($vars['video'], array('params' => array('width' => $vars['width'], 'height' => $vars['height'])));
  }
  elseif ($player_id == 'flowplayer') {
    $output = theme('flowplayer', array(
      'clip' => array(
        'url' =>  $vars['video'],
        'autoPlay' => variable_get('video_autoplay', TRUE),
        'autoBuffering' => variable_get('video_autobuffering', TRUE),
      ),
    ),
    $vars['formatter'],
    array('style' => 'width: '. $vars['width'] .'px; height: '. $vars['height'] .'px;'));
  }
  else {
    $output = t('No flash player has been setup.');
  }
  $vars['player'] = $output;
}