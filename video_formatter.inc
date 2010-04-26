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
    return video_get_player($element);
  }
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
  //  print_r($field);
  //  exit;
  $node = $element['#node'];
  //  $imagetag = theme('uploadfield_formatter_video_plain', $element);
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

/*
 * Use SWF Tools to play flash video files.
 * //@todo Break this out to allow the user to be able to use other flash players and not limit this to just swftools.
 */
function theme_video_play_flash($element) {
  //TODO : remove item height set in here
  $video = file_create_url($element['#item']['filepath']);
  $width = isset($element['#item']['data']['width']) && !empty($element['#item']['data']['width']) ? $element['#item']['data']['width'] .'px' : '350px';
  $height = isset($element['#item']['data']['height']) && !empty($element['#item']['data']['height']) ? $element['#item']['data']['height'] .'px' : '285px';

  if (module_exists('flowplayer')) {
    $options = array('clip' => array('url' =>  $video, 'autoPlay' => variable_get('video_autoplay', TRUE), 'autoBuffering' => variable_get('video_autobuffering', TRUE)), $element['#formatter'], array('style' => "width: $width; height: $height"));
    $output = theme('flowplayer', $options);
  }
  else {
    $output = t('Download or enable the !flowplayer module (<b>flowplayer</b>) to play FLV files.', array('!flowplayer' => l(t('Flowplayer'), 'http://drupal.org/project/flowplayer')));
  }
  return $output;
}

/**
 * Play videos from in Quicktime format
 *
 * @see http://developer.apple.com/internet/ieembedprep.html
 * @param $element
 *   object with element information
 *
 * @return
 *   string of content to display
 */
function theme_video_play_quicktime($element) {
  // Increase the height to accommodate the player controls on the bottom.
  $video = file_create_url($element['#item']['filepath']);
  $width = isset($element['#item']['data']['width']) && !empty($element['#item']['data']['width']) ? $element['#item']['data']['width'] .'px' : '350px';
  $height = isset($element['#item']['data']['height']) && !empty($element['#item']['data']['height']) ? $element['#item']['data']['height'] .'px' : '285px';

  // this wont be ran by Internet Explorer browsers
  $output = '<!--[if !IE]> <--><object type="video/quicktime" width="'. $width .'" height="'. $height .'" data="'. $video .'"><!--> <![endif]-->' . "\n";
  // this will be executed by Internet Explorer
  $output .=  '<!--[if IE]><object classid="clsid:02BF25D5-8C17-4B23-BC80-D3488ABDDC6B" codebase="http://www.apple.com/qtactivex/qtplugin.cab" width="'. $width .'" height="'. $height .'" scale="tofit" ><![endif]-->' . "\n";
  // params will be passed to all browsers
  //GMM: kioskmode enabled so users don't bypass download security video through player
  $output .= '<param name="src" value="'. $video .'" />
    <param name="AUTOPLAY" value="'.(variable_get('video_autoplay', TRUE) ? TRUE : FALSE).'" />
    <param name="KIOSKMODE" value="true" />
    <param name="CONTROLLER" value="true" />
    <p>'. t('Your browser is not able to display this multimedia content.') .'</p>
    </object>';

  return $output;
}

/**
 * Play videos from in WindowsMediaVideo format
 *
 * @param $node
 *   object with node information
 *
 * @return
 *   string of content to display
 */
function theme_video_play_windowsmedia($element) {
  // Windows Media's embeded player includes the controls in the height
  $video = file_create_url($element['#item']['filepath']);
  $width = isset($element['#item']['data']['width']) && !empty($element['#item']['data']['width']) ? $element['#item']['data']['width'] .'px' : '350px';
  $height = isset($element['#item']['data']['height']) && !empty($element['#item']['data']['height']) ? $element['#item']['data']['height'] .'px' : '285px';

  // this wont be ran by Internet Explorer browsers
  $output = '<!--[if !IE]> <--><object type="application/x-mplayer2" width="'. $width .'" height="'. $height .'" data="'. $video .'"><!--> <![endif]-->' . "\n";
  // this will be executed by Internet Explorer
  $output .=  '<!--[if IE]><object type="application/x-oleobject" width="'. $width .'" height="'. $height .'" classid="clsid:6BF52A52-394A-11d3-B153-00C04F79FAA6" ><![endif]-->' . "\n";
  // params will be passed to all browsers
  $output .= '<param name="src" value="'. $video .'" />
    <param name="URL" value="'.$video.'" />
    <param name="animationatStart" value="true" />
    <param name="transparentatStart" value="true" />
    <param name="autoStart" value="'.(variable_get('video_autoplay', TRUE) ? 'true' : 'false').'" />
    <param name="showControls" value="true" />
    <param name="loop" value="true" />
    <p>'. t('Your browser is not able to display this multimedia content.') .'</p>
    </object>';

  return $output;
}