<?php
/**
 * @file
 * Class file used to create our video and thumbnail objects.
 */

class VideoHelper {
  public function video_object($variables) {
    $cid = 'video:video:' . md5($variables['item']['fid']);
    $cache = cache_get($cid);
    if (!empty($cache->data)) {
      return $cache->data;
    }

    $field_settings = $variables['field']['settings'];
    $instance_settings = $variables['instance']['settings'];
    // Setup our width x height
    $player_dimensions = explode('x', $variables['player_dimensions']);
    // Build our video object for all types.
    $video = new stdClass();
    $video->fid = $variables['item']['fid'];
    $video->original = $variables['item'];
    $extension = strtolower(pathinfo($variables['item']['filename'], PATHINFO_EXTENSION));
    $video->files = new stdClass();
    $video->files->{$extension} = new stdClass();
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
        // get the preset and check for output file type to set as player
        $video_preset = new Preset();
        $presets = $video_preset->properties();
        foreach ($presets as $preset) {
          $video->player = $preset['settings']['video_extension'];
        }
      }
    }

    cache_set($cid, $video, 'cache', time() + 7 * 24 * 3600);
    return $video;
  }

  public function thumbnail_object($variables) {
    if (empty($variables['item']['thumbnail'])) {
      return NULL;
    }

    $file = file_load($variables['item']['thumbnail']);
    if (empty($file)) {
      return NULL;
    }

    // Build our thumbnail object
    $thumbnail = new stdClass();
    $thumbnail->filepath = $file->uri;
    $thumbnail->url = file_create_url($thumbnail->filepath);
    $thumbnail->uri = $thumbnail->filepath;
    $thumbnail->filemime = file_get_mimetype($thumbnail->filepath);
    $thumbnail->alt = isset($variables['entity']->title) ? $variables['entity']->title : '';
    $thumbnail->title = $thumbnail->alt;

    // Return our object
    return $thumbnail;
  }
}
