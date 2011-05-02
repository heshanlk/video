<?php

/*
 * @file
 * Class file used to create our video and thumbnail objects.
 * @todo : remove video_zencoder validation form the class
 *
 */

class video_helper {

  public function video_object($element) {
    $field = content_fields($element['#field_name'], $element['#type_name']);
    //setup our width x height
    $dimensions = explode("x", $element['#item']['data']['dimensions']);
    $player_dimensions = explode("x", $element['#item']['data']['player_dimensions']);
    if (!isset($dimensions[0]) || !isset($dimensions[1])) {
      $dimensions = explode("x", $field['widget']['default_dimensions']);
      if (!isset($dimensions[0]) || !isset($dimensions[1])) {
        drupal_set_message(t('Something is wrong with your dimensions.  Make sure you enter dimensions in the form of WxH.'), 'error');
      }
    }
    if (!isset($player_dimensions[0]) || !isset($player_dimensions[1])) {
      $player_dimensions = explode("x", $field['widget']['default_player_dimensions']);
      if (!isset($player_dimensions[0]) || !isset($player_dimensions[1])) {
        drupal_set_message(t('Something is wrong with your player dimensions.  Make sure you enter the player dimensions in the form of WxH.'), 'error');
      }
    }

    // Build our video object for all types.
    $video = new stdClass();
    $video->fid = $element['#item']['fid'];
    $video->original = $element['#item'];
    $extension = strtolower(pathinfo($element['#item']['filename'], PATHINFO_EXTENSION));
    $video->files->{$extension}->filename = pathinfo($element['#item']['filepath'], PATHINFO_FILENAME) . '.' . $extension;
    $video->files->{$extension}->filepath = $element['#item']['filepath'];
    $video->files->{$extension}->url = file_create_url($element['#item']['filepath']);
    $video->files->{$extension}->extension = $extension;
    $video->player = strtolower(pathinfo($element['#item']['filename'], PATHINFO_EXTENSION));
    $video->width = trim($dimensions[0]);
    $video->height = trim($dimensions[1]);
    $video->player_width = trim($player_dimensions[0]);
    $video->player_height = trim($player_dimensions[1]);
    $video->thumbnail = $this->thumbnail_object($element);
    $video->formatter = $element['#formatter'];
    $video->autoplay = variable_get('video_autoplay', TRUE);
    $video->autobuffering = variable_get('video_autobuffering', TRUE);
    $video->theora_player = variable_get('video_ogg_player', 'http://theora.org/cortado.jar');
    // lets find out if we have transcoded this file and update our paths.
    if (isset($field['widget']['autoconversion']) && $field['widget']['autoconversion']
        && !$element['#item']['data']['bypass_autoconversion']) {
      // discard all existing file data
      $video->files = new stdClass();
      module_load_include('inc', 'video', '/includes/conversion');
      $conversion = new video_conversion;
      $conversion->load_completed_job($video);
    }
//    echo '<pre>';
//    print_r($video);
//    die();
    // Let othere module to load the video files by referance
    // Lets find out if we have pushed this file to the cdn if enabled.
    // @TODO : add correct filesystem load to this
    $filesystem = variable_get('vid_filesystem', 'drupal');
    if ($filesystem != 'drupal' && !module_exists('video_zencoder')) {
      module_load_include('inc', 'video', '/includes/filesystem');
      $filesystem = new video_filesystem();
      $filesystem->load_file($video);
    }


    // Moved to last to recheck incase we changed our extension above.
    $video->flash_player = variable_get('video_extension_' . $video->player . '_flash_player', '');
    $video->html5_player = variable_get('video_extension_' . $video->player . '_html5_player', '');

    // Return our object
    return $video;
  }

  public function thumbnail_object($element) {
    $field = content_fields($element['#field_name'], $element['#type_name']);
    // Build our thumbnail object
    $thumbnail = new stdClass();
    $thumbnail->filepath = '';
    $thumbnail->url = '';
    //@todo future enhancements for our thumbnails
    $thumbnail->alt = '';
    $thumbnail->title = '';
    $thumbnail->description = '';

    // Setup our thumbnail path.
    $use_default_img = isset($element['#item']['data']['use_default_video_thumb']) ? $element['#item']['data']['use_default_video_thumb'] : false;
    if ($use_default_img && !empty($field['widget']['default_video_thumb']['filepath'])) {
      $thumbnail->filepath = $field['widget']['default_video_thumb']['filepath'];
    } elseif (isset($element['#item']['data']['video_thumb']) ? $element['#item']['data']['video_thumb'] : false) {
      $thumbnail->filepath = $element['#item']['data']['video_thumb'];
    } else {
      //need some type of default if nothing is present
      //drupal_set_message(t('No thumbnail has been configured for the video.'), 'error');
    }
    //lets check for an imagecache preset
    if (isset($element['imagecache_preset'])) {
      $thumbnail->url = imagecache_create_url($element['imagecache_preset'], $thumbnail->filepath);
      $thumbnail->filepath = imagecache_create_path($element['imagecache_preset'], $thumbnail->filepath);
    } else {
      $thumbnail->url = file_create_url($thumbnail->filepath);
    }

    //swftools appends sites/default/files to the front of our path...
    //@todo Is this a setting?  Need to figure this out.
    $thumbnail->swfthumb = $thumbnail->filepath;
    // filemime
    $thumbnail->filemime = file_get_mimetype($thumbnail->filepath);
    // Return our object
    return $thumbnail;
  }

}
