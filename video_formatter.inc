<?php

/**
 * @file
 * Video formatter hooks and callbacks.
 */
/*
 * Default video cck formatter.  Makes sure the video being displayed exists, has been converted (if in que).
 * If not or the video is processed, then it will get the default player for the specific video type for output.
 */
function theme_video_formatter_video_plain($element) {
  if (empty($element['#item']['fid']))
    return '';
  // Get our field information to determine if we are checking conversion
  $field = content_fields($element['#field_name'], $element['#type_name']);
  if (!empty($field['list_field']) && !$element['#item']['list'])
    return '';
  // Only needs to be ran if they are converting videos
  if (isset($field['widget']['autoconversion']) && $field['widget']['autoconversion'] && !$element['#item']['data']['bypass_autoconversion']) {
    module_load_include('inc', 'video', '/includes/conversion');
    $conversion = new video_conversion;
    $video = $conversion->load_job($element['#item']['fid']);
    if ($video->video_status == VIDEO_RENDERING_ACTIVE || $video->video_status == VIDEO_RENDERING_PENDING) {
      return theme('video_inprogress');
    }
    else if ($video->video_status == VIDEO_RENDERING_FAILED) {
      return theme('video_encoding_failed');
    }
  }
  return video_get_player($element);
}

/*
 * Renders the video thumbnail as a link to the node page.
 */

function theme_video_formatter_video_nodelink($element, $imagecache = FALSE) {
  // Inside a view $element may contain null data. In that case, just return.
  if (empty($element['#item']['fid']))
    return '';
  //setup our thumbnail object
  module_load_include('inc', 'video', '/includes/video_helper');
  $video_helper = new video_helper;
  $thumbnail = $video_helper->thumbnail_object($element);
  //get our themed image
  $image = theme('video_image', $thumbnail, $thumbnail->alt, $thumbnail->title, '', TRUE, $imagecache);
  $class = 'popups video video-nodelink video-' . $element['#field_name'];
  return l($image, 'node/' . $element['#node']->nid, array('attributes' => array('class' => $class), 'html' => TRUE));
}

/*
 * Renders the video thumbnail with no inbuilt node link.
 */

function theme_video_formatter_video_nonodelink($element, $imagecache = FALSE) {
  // Inside a view $element may contain null data. In that case, just return.
  if (empty($element['#item']['fid']))
    return '';
  //setup our thumbnail object
  module_load_include('inc', 'video', '/includes/video_helper');
  $video_helper = new video_helper;
  $thumbnail = $video_helper->thumbnail_object($element);
  // return the themed image
  $output = '<div class="popups video video-nonodelink video-' . $element['#field_name'] . '">';
  $output .= theme('video_image', $thumbnail, $thumbnail->alt, $thumbnail->title, null, TRUE, $imagecache);
  $output .= '</div>';
  return $output;
}

/*
 * Renders the video thumbnail.
 */

function theme_video_formatter_video_thumbnail($element, $imagecache = FALSE) {
  // Inside a view $element may contain null data. In that case, just return.
  if (empty($element['#item']['fid']))
    return '';
  //setup our thumbnail object
  module_load_include('inc', 'video', '/includes/video_helper');
  $video_helper = new video_helper;
  $thumbnail = $video_helper->thumbnail_object($element);
  //get our themed image
  return theme('video_image', $thumbnail, $thumbnail->alt, $thumbnail->title, '', TRUE, $imagecache);
}

/*
 * Renders the video thumbnail linked to the absolute filepath of the video.  Colorbox is introduced to handle
 * the video in an overlay.
 *
 * @todo Very unstable, need better methods of integration.  Seems hackish right now to find out what flv player is being
 * used.  We are also using jmedia for all other filetypes.
 */

function theme_video_formatter_video_colorbox($element, $imagecache = FALSE) {
  global $base_path;
  if (!module_exists('colorbox')) {
    drupal_set_message(t('You must download and enable !colorbox for this formatter.', array('!colorbox' => l(t('Colorbox'), 'http://www.drupal.org/project/colorbox'))), 'error');
    return theme('video_formatter_video_nodelink', $element);
  }

  // Inside a view $element may contain null data. In that case, just return.
  if (empty($element['#item']['fid']))
    return '';

  //load up our media plugins
  drupal_add_js(drupal_get_path('module', 'video') . '/js/jquery.media.js');
  drupal_add_js(drupal_get_path('module', 'video') . '/js/jquery.metadata.js');
  drupal_add_js(drupal_get_path('module', 'video') . '/js/flowplayer-3.2.0.min.js');

  //setup our video object
  module_load_include('inc', 'video', '/includes/video_helper');
  $video_helper = new video_helper;
  $video = $video_helper->video_object($element);

  $action = swftools_get_action($video->filepath);
  $player = swftools_get_player($action);
  $path = explode("_", $player);
  $player_url = $base_path . swftools_get_player_path() . '/' . $path[0] . '/' . variable_get($player . '_file', '');
  if (stristr($player, 'flowplayer')) {
    $player = 'flowplayer';
  }
  //add our default settings to the Drupal.settings object
  $settings = array('video' => array(
      'flvplayer' => $player_url,
      'autoplay' => $video->autoplay,
      'autobuffer' => $video->autobuffering,
      'player' => $player,
      ));
  drupal_add_js($settings, 'setting');
  $image = theme('video_image', $video->thumbnail, $video->thumbnail->alt, $video->thumbnail->title, '', TRUE, $imagecache);
  $class = 'video-box video-' . $element['#field_name'] . '{width:\'' . $video->player_width . 'px\', height:\'' . $video->player_height . 'px\', player:\'' . $player . '\'}';
  return l($image, $video->files->{$video->player}->url, array('attributes' => array('class' => $class), 'html' => TRUE));
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
  //#913928
  $field = content_fields($element['#field_name'], $element['#type_name']);
  if (!empty($field['list_field']) && !$element['#item']['list'])
    return '';

  drupal_add_js(drupal_get_path('module', 'video') . '/js/jquery.media.js');
  drupal_add_js(drupal_get_path('module', 'video') . '/js/jquery.metadata.js');
  //setup our video object
  module_load_include('inc', 'video', '/includes/video_helper');
  $video_helper = new video_helper;
  $video = $video_helper->video_object($element);
  //lets output the link to be overtaken by jmedia
  $link = l($video->filename, $video->files->{$video->player}->url, array('attributes' => array('class' => 'jmedia {width: ' . $video->player_width . ', height: ' . $video->player_height . ', autoplay: ' . $video->autoplay . '}')));
  return $link;
}

/**
 * Displays a "encoding in progress message"
 */
function theme_video_inprogress() {
  return '<div class="video-ffmpeg-helper-inprogress">' . t('This video is currently being processed. Please wait.') . '</div>';
}

/**
 * Display an "encoding failed" message"
 */
function theme_video_encoding_failed() {
  return '<div class="video-ffmpeg-helper-encoding-failed">' . t('The video conversion process has failed. You might want to submit a simpler video format like <em>mpeg</em> or <em>divx avi</em>.<br />If the problem persists please contact website administrators.') . '</div>';
}

/**
 * Callback to FLV
 */
function theme_video_flv($video, $node) {
  if ($video->flash_player == 'swftools') {
    $options = array(
      'params' => array(
        'width' => $video->player_width,
        'height' => $video->player_height,
      ),
      'othervars' => array(
        //@todo: swftools bug, can't enable this until they fix their pathing for the images.
        'image' => $video->thumbnail->swfthumb,
      ),
    );
    $themed_output = swf($video->files->{$video->player}->url, $options);
  }
  elseif ($video->flash_player == 'flowplayer') {
    // kjh: use a playlist to display the thumbnail if not auto playing
    if (!$video->autoplay && $video->thumbnail->url) {
      $options = array(
        'playlist' => array($video->thumbnail->url,
          array('url' => urlencode($video->files->{$video->player}->url),
            'autoPlay' => $video->autoplay,
            'autoBuffering' => $video->autobuffering,
          ),),);
    }
    else {
      $options = array(
        'clip' => array('url' => urlencode($video->files->{$video->player}->url),
          'autoPlay' => $video->autoplay,
          'autoBuffering' => $video->autobuffering,
        ),);
    }

    $themed_output = theme(
        'flowplayer', $options, $video->formatter,
        // adding 24px to height #973636
        array('style' => 'width:' . $video->player_width . 'px;height:' . ($video->player_height + 24) . 'px;')
    );
  }
  else {
    $themed_output = t('No flash player has been setup. ' . l(t('Please select a player to play Flash videos.'), 'admin/settings/video/players'));
  }
  return theme('video_play_flv', $video, $node, $themed_output);
}

/**
 * Play HTML5 videos
 */
function theme_video_html5($video, $node) {
  $themed_output = NULL;
  $video->html5_player = variable_get('video_extension_' . $video->player . '_html5_player', '');
  switch ($video->html5_player) {
    case 'video':
      return theme('video_play_html5', $video, $node, $themed_output);
      break;
    case 'videojs':
      $items = _video_object_to_array($video->files);
      $items +=array('thumbnail' => (array) $video->thumbnail);
      $attributes = array();
      $attributes['width'] = $video->width;
      $attributes['height'] = $video->height;
      return theme('videojs', $items, 'video-' . $video->fid, $attributes);
      break;
    default:
      return t('No HTML5 player has been setup. ' . l(t('Please select a player to play HTML5 videos.'), 'admin/settings/video/players'));
      break;
  }
}

/**
 * Helper funcations
 */
function _video_object_to_array($data) {
  if (is_array($data) || is_object($data)) {
    $result = array();
    foreach ($data as $key => $value) {
      $result[$key] = _video_object_to_array($value);
    }
    return $result;
  }
  return $data;
}

function theme_video_formatter_imagecache($element) {
  // Inside a view $element may contain NULL data. In that case, just return.
  if (empty($element['#item']['fid'])) {
    return '';
  }

  // Extract the preset name from the formatter name.
  list($namespace, $theme_function) = explode('__', $element['#formatter'], 2);
  if ($preset = imagecache_preset_by_name($namespace)) {
    $element['imagecache_preset'] = $namespace;
  }
  //return $theme_function;
  return theme('video_formatter_' . $theme_function, $element, TRUE);
}
