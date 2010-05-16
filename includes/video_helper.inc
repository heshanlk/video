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
    //setup our width x height
    $dimensions = explode("x", $element['#item']['data']['dimensions']);
    if(!isset($dimensions[0]) || !isset($dimensions[1])) {
      $dimensions = explode("x", $field['widget']['default_dimensions']);
      if(!isset($dimensions[0]) || !isset($dimensions[1])) {
        drupal_set_message(t('Something is wrong with your dimensions.  Make sure you enter dimensions if the form of WxH.', 'error'));
      }  
    }
    // Build our video object for all types.
    $video = new stdClass();
    $video->fid = $element['#item']['fid'];
    $video->original = $element['#item'];
    $video->filepath = $element['#item']['filepath'];
    $video->url = file_create_url($element['#item']['filepath']);
    $video->extension = pathinfo($element['#item']['filename'], PATHINFO_EXTENSION);
    $video->width = $dimensions[0];
    $video->height = $dimensions[1];
    $video->thumbnail = $this->thumbnail_object($element);
    $video->formatter = $element['#formatter'];
    $video->autoplay = variable_get('video_autoplay', TRUE);
    $video->autobuffering = variable_get('video_autobuffering', TRUE);    
    $video->theora_player = variable_get('video_ogg_player', 'http://theora.org/cortado.jar');
    
    // Lets find out if we have pushed this file to the cdn if enabled.
    $cdn = false;
    if(variable_get('amazon_s3', FALSE)) {
      module_load_include('inc', 'video_s3', '/includes/amazon_s3');
      $s3 = new video_amazon_s3;
      if($amazon = $s3->get($video->fid)) {
        $cdn = true;
        // Fix our filepath
        $video->filepath = $amazon->filepath;
        $video->url = $amazon->filepath;
        $video->extension = pathinfo($amazon->filepath, PATHINFO_EXTENSION);
      }
    }
    // If no cdn, lets find out if we have transcoded this file and update our paths.
    if(!$cdn) {
      //lets find out if we are overriding this video with a converted one.
      if (isset($field['widget']['autoconversion']) && $field['widget']['autoconversion'] && !$element['#item']['data']['bypass_autoconversion']) {
        module_load_include('inc', 'video', '/includes/conversion');
        $conversion = new video_conversion;
        $converted = $conversion->load_converted_video($video->fid);
        $video->filepath = $converted->filepath;
        $video->url = file_create_url($converted->filepath);
        $video->extension = pathinfo($converted->filepath, PATHINFO_EXTENSION);
      }
    }
    // Moved to last to recheck incase we changed our extension above.
    $video->flash_player = variable_get('video_extension_'.$video->extension.'_flash_player', '');
    
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
    $use_default_img = $element['#item']['data']['use_default_video_thumb'];
    if ($use_default_img && !empty($field['widget']['default_video_thumb']['filepath'])) {
      $thumbnail->filepath = $field['widget']['default_video_thumb']['filepath'];
    }
    elseif ($element['#item']['data']['video_thumb']) {
      $thumbnail->filepath = $element['#item']['data']['video_thumb'];
    }
    else {
      //need some type of default if nothing is present
      //drupal_set_message(t('No thumbnail has been configured for the video.'), 'error');
    }
    //lets check for an imagecache preset
    if(isset($element['imagecache_preset'])) {
      $thumbnail->url = imagecache_create_url($element['imagecache_preset'], $thumbnail->filepath);
      $thumbnail->filepath = imagecache_create_path($element['imagecache_preset'], $thumbnail->filepath);
    } else {
      $thumbnail->url = file_create_url($thumbnail->filepath);  
    }
    
    //swftools appends sites/default/files to the front of our path...
    //@todo Is this a setting?  Need to figure this out.
    $thumbnail->swfthumb = $thumbnail->filepath;
    // Return our object
    return $thumbnail;
  }
}