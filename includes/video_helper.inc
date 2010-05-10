<?php
//$Id$
/*
 * @file
 * Class file used to create our video and thumbnail objects.
 *
 */

class video_helper {

  public function video_object($element) {
    $field = content_fields($element['#field_name'], $element['#type_name']);
    //setup our resolution and fix our height.
    $resolution = array_filter(explode(':', $field['widget']['default_resolution']));
    $element['#item']['data']['height'] = $element['#item']['data']['width'] * ($resolution[1]/$resolution[0]);
    //get our extension
    $file = pathinfo($element['#item']['filename']);
    // Build our video object for all types.
    $video = new stdClass();
    $video->filepath = $element['#item']['filepath'];
    $video->url = file_create_url($element['#item']['filepath']);
    $video->extension = $file['extension'];
    $video->width = isset($element['#item']['data']['width']) && !empty($element['#item']['data']['width']) ? $element['#item']['data']['width'] : '350';
    $video->height = isset($element['#item']['data']['height']) && !empty($element['#item']['data']['height']) ? $element['#item']['data']['height'] : '285';
    $video->thumbnail = $this->thumbnail_object($element);
    $video->formatter = $element['#formatter'];
    $video->autoplay = variable_get('video_autoplay', TRUE);
    $video->autobuffering = variable_get('video_autobuffering', TRUE);
    $video->flash_player = variable_get('video_extension_'.$file['extension'].'_flash_player', '');
    $video->theora_player = variable_get('video_ogg_player', 'http://theora.org/cortado.jar');
    // Return our object
    return $video;
  }

  public function thumbnail_object($element) {
    $thumbnail = new stdClass();
    $thumbnail->filepath = '';
    $thumbnail->url = '';
    //@todo future enhancements for our thumbnails
    $thumbnail->alt = '';
    $thumbnail->title = '';
    $thumbnail->description = '';

    // Setup our thumbnail path.
    $use_default_img = $element['#item']['data']['use_default_video_thumb'];
    if ($use_default_img) {
      $thumbnail->filepath = $field['widget']['default_video_thumb'];
    }
    elseif ($element['#item']['data']['video_thumb']) {
      $thumbnail->filepath = $element['#item']['data']['video_thumb'];
    }
    else {
      //need some type of default if nothing is present
      //drupal_set_message(t('No thumbnail has been configured for the video.'), 'error');
    }
    $thumbnail->url = file_create_url($thumbnail->filepath);
    //swftools appends sites/default/files to the front of our path...
    //@todo Is this a setting?  Need to figure this out.
    $thumbnail->swfthumb = $thumbnail->filepath;
    // Return our object
    return $thumbnail;
  }
}