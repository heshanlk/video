<?php
// $Id$

/**
 * @file
 * Video formatter hooks and callbacks.
 */

/*
 * Default video cck formatter.  Makes sure the video being displayed exists, has been converted (if in que).
 * If not or the video is processed, then it will get the default player for the specific video type for output.
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
    return video_get_player($element);
  }
}

/*
 * Renders the video thumbnail as a link to the node page.
 */
function theme_video_formatter_video_nodelink($element) {
  $field = content_fields($element['#field_name'], $element['#type_name']);
  // Inside a view $element may contain null data. In that case, just return.
  if (empty($element['#item']['fid'])) return '';
  $node = $element['#node'];
  $use_default_img = $element['#item']['data']['use_default_video_thumb'];
  if($use_default_img) {
    $imagetag = theme('video_widget_video_thumb', $field['widget']['default_video_thumb']);
  }
  elseif ($element['#item']['data']['video_thumb']) {
    $thumb = array('filepath' => $element['#item']['data']['video_thumb']);
    $imagetag = theme('video_image', $thumb, '', '', '', FALSE);
  }
  $class = 'popups video video-nodelink video-'. $element['#field_name'];
  return l($imagetag, 'node/'. $node->nid, array('attributes' => array('class' => $class), 'html' => TRUE));
}

/*
 * Renders the video thumbnail linked to the absolute filepath of the video.  Colorbox is introduced to handle
 * the video in an overlay.
 * 
 * @todo Very unstable, need better methods of integration.  Seems hackish right now to find out what flv player is being
 * used.  We are also using jmedia for all other filetypes.
 */
function theme_video_formatter_video_colorbox($element) {
  global $base_path;
  if (!module_exists('colorbox')) {
    drupal_set_message(t('You must download and enable !colorbox for this formatter.', array('!colorbox' => l(t('Colorbox'), 'http://www.drupal.org/project/colorbox'))), 'error');
    return theme('video_formatter_video_nodelink', $element);
  }
  
  //load up our media plugins
  drupal_add_js(drupal_get_path('module', 'video') .'/js/jquery.media.js');
  drupal_add_js(drupal_get_path('module', 'video') .'/js/jquery.metadata.js');
  drupal_add_js(drupal_get_path('module', 'video') .'/js/flowplayer-3.2.0.min.js');
  
  $field = content_fields($element['#field_name'], $element['#type_name']);
  //setup our resolution and fix our height.
  $resolution = array_filter(explode(':', $field['widget']['default_resolution']));
  $element['#item']['data']['height'] = $element['#item']['data']['width'] * ($resolution[1]/$resolution[0]);
  
  $video = file_create_url($element['#item']['filepath']);
  $width = isset($element['#item']['data']['width']) && !empty($element['#item']['data']['width']) ? $element['#item']['data']['width'] : '350';
  $height = isset($element['#item']['data']['height']) && !empty($element['#item']['data']['height']) ? $element['#item']['data']['height'] : '285';
  
  $action = swftools_get_action($video);
  $player = swftools_get_player($action);
  $path = explode("_", $player);
  $player_url = $base_path . swftools_get_player_path() .'/'. $path[0] .'/'. variable_get($player .'_file', '');
  if(stristr($player, 'flowplayer')) {
    $player = 'flowplayer';
  }
  //add our default settings to the Drupal.settings object
  $settings = array('video' => array(
    'flvplayer' => $player_url, 
    'autoplay' => variable_get('video_autoplay', TRUE),
    'autobuffer' => variable_get('video_autobuffering', TRUE),
    'player' => $player,
  ));
  drupal_add_js($settings, 'setting');
  
  
  // Inside a view $element may contain null data. In that case, just return.
  if (empty($element['#item']['fid'])) return '';
  $node = $element['#node'];
  $use_default_img = $element['#item']['data']['use_default_video_thumb'];
  if($use_default_img) {
    $thumb = $field['widget']['default_video_thumb'];
  }
  elseif ($element['#item']['data']['video_thumb']) {
    $thumb = array('filepath' => $element['#item']['data']['video_thumb']);
  }
  else {
    //need some type of default if nothing is present
    drupal_set_message(t('No thumbnail has been configured for the video.'), 'error');
  }
  $imagetag = theme('video_image', $thumb, '', '', '', FALSE);
  $class = 'video-box video-'. $element['#field_name'] .'{width:\''.$width.'px\', height:\''.$height.'px\', player:\''.$player.'\'}';
  return l($imagetag, $video, array('attributes' => array('class' => $class), 'html' => TRUE));
}

/*
 * We are using the jMedia library to output our video files.
 * 
 * @todo Does not work with flv files as this requires an actual flv player.  Need to figure out how to best integrate the player
 * into this function.
 * 
 * We are outputing an anchor to the videofile.  The jMedia functions will overtake this anchor and setup our object/embed tags.
 */
function theme_video_formatter_video_media_js($element) {
  drupal_add_js(drupal_get_path('module', 'video') .'/js/jquery.media.js');
  drupal_add_js(drupal_get_path('module', 'video') .'/js/jquery.metadata.js');
  //lets get our width/height and video url and pass them as arguments
  $video = file_create_url($element['#item']['filepath']);
  //@todo change default width and height to be configuration settings.
  $width = isset($element['#item']['data']['width']) && !empty($element['#item']['data']['width']) ? $element['#item']['data']['width'] : '350';
  $height = isset($element['#item']['data']['height']) && !empty($element['#item']['data']['height']) ? $element['#item']['data']['height'] : '285';
  //lets output the link to be overtaken by jmedia
  $link = l($element['#item']['filename'], $video, array('attributes' => array('class' => 'jmedia {width: '. $width .', height: '. $height .', autoplay: '. variable_get('video_autoplay', TRUE) .'}')));
  return $link;
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

function video_video_flv($video, $node) {
  if ($video->flash_player == 'swftools') {
    $options = array(
      'params' => array(
        'width' => $video->width,
        'height' => $video->height,
      ),
      'othervars' => array(
        'image' => $video->thumbnail,
      ),
    );
    $output = swf($video->url, $options);
  }
  elseif ($video->flash_player == 'flowplayer') {
    $options = array(
      'clip' => array(
        'url' => $video->url,
        'autoPlay' => $video->autoplay,
        'autoBuffering' => $video->autobuffering,
      ),
    );
    $output = theme(
      'flowplayer', 
      $options,
      $video->formatter,
      array('style' => 'width: '. $width .'px; height: '. $height .'px;')
    );
  }
  else {
    $output = t('No flash player has been setup.');
  }
  return theme('video_play_flv', $output, $node);
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