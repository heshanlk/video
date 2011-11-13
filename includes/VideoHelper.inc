<?php

/*
 * @file
 * Class file used to create our video and thumbnail objects.
 *
 */

class VideoHelper {

  public function video_object($variables) {
    $cid = 'video:video:' . md5($variables['item']['fid']);
    $cache = cache_get($cid);
    if (!empty($cache->data)) {
      return $cache->data;
    }
    else {
      //@todo cache the video object
      $field_settings = $variables['field']['settings'];
      $instance_settings = $variables['instance']['settings'];
      //setup our width x height
      $player_dimensions = explode("x", $variables['player_dimensions']);
      // Build our video object for all types.
      $video = new stdClass();
      $video->fid = $variables['item']['fid'];
      $video->original = $variables['item'];
      $extension = strtolower(pathinfo($variables['item']['filename'], PATHINFO_EXTENSION));
      $video->files->{$extension}->filename = pathinfo($variables['item']['filename'], PATHINFO_FILENAME) . '.' . $extension;
      $video->files->{$extension}->filepath = $variables['item']['uri'];
      $video->files->{$extension}->url = file_create_url($variables['item']['uri']);
      $video->files->{$extension}->uri = $variables['item']['uri'];
      $video->files->{$extension}->filemime = $variables['item']['filemime'];
      $video->files->{$extension}->extension = $extension;
      // set the player to play
      $video->player = $extension;
      // load thumbnail object
      $video->thumbnail = $this->thumbnail_object($variables);
      $video->formatter = $variables['instance']['display']['default']['type'];
      $video->autoplay = variable_get('video_autoplay', FALSE);
      $video->autobuffering = variable_get('video_autobuffering', TRUE);
      $video->theora_player = variable_get('video_ogg_player', 'http://theora.org/cortado.jar');
      // lets find out if we have transcoded this file and update our paths.
      if (isset($field_settings['autoconversion']) && $field_settings['autoconversion']) {
        // transcoder factory
        $factory = new TranscoderAbstractionAbstractFactory();
        $transcoder = $factory->getProduct();
        $transcoder->setInput($variables['item']);
        if ($transcoder->loadJob()) {
          // reset the video files object and add converted videos in to it
          $video->files = new stdClass();
          $video->files = $transcoder->loadOutput($video);
        }
        // add player to the video object
        if (!empty($video->files)) {
          //get the preset and check for output file type to set as player
          $video_preset = new Preset();
          $presets = $video_preset->properties();
          foreach ($presets as $preset) {
            $video->player = $preset['settings']['video_extension'];
          }
        }
      }
      // set cache
      cache_set($cid, $video);
      // Return our object
      return $video;
    }
  }

  public function thumbnail_object($variables) {
    $field_settings = $variables['field']['settings'];
    $instance_settings = $variables['instance']['settings'];
    // Build our thumbnail object
    $thumbnail = new stdClass();
    $thumbnail->filepath = '';
    $thumbnail->url = '';
    $thumbnail->alt = $variables['entity']->title;
    $thumbnail->title = $variables['entity']->title;
    $use_default_img = isset($variables['item']['use_default_video_thumb']) ? TRUE : FALSE;
    if ($use_default_img && !empty($field_settings['default_video_thumbnail'])) {
      $default_thumbnail = file_load($field_settings['default_video_thumbnail']['fid']);
      // Check the checkbox to use default thumbnail on node
      $thumbnail->filepath = $default_thumbnail->uri;
    }
    elseif (isset($variables['item']['thumbnail']) ? TRUE : FALSE) {
      // actual video thumbnails is present
      $thumbnail_load = file_load($variables['item']['thumbnail']);
      if (isset($thumbnail_load->uri) && !empty($thumbnail_load->uri)) {
        $thumbnail->filepath = $thumbnail_load->uri;
      }
      elseif (!empty($field_settings['default_video_thumbnail']['fid'])) {
        // if thumbnail is missing or not found use default one if uploadded
        $default_thumbnail = file_load($field_settings['default_video_thumbnail']['fid']);
        $thumbnail->filepath = $default_thumbnail->uri;
      }
      else
        return;
    }
    else {
      return;
    }
    $thumbnail->url = file_create_url($thumbnail->filepath);
    // uri
    $thumbnail->uri = $thumbnail->filepath;
    //mime
    $thumbnail->filemime = file_get_mimetype($thumbnail->filepath);
    //swftools appends sites/default/files to the front of our path...
    //@todo Is this a setting?  Need to figure this out.
    $thumbnail->swfthumb = $thumbnail->filepath;
    // Return our object
    return $thumbnail;
  }

}
