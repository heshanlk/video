<?php
//$Id$
/**
* Provides views data and enumerates handlers for video.module
*
* @return
*   array - Enables support in the video module for views integration
* @author Glen Marianko Twitter@demoforum <glenm at demoforum dot com>
* @todo
**/

function video_views_data() {
  // Basic table information.
  // ----------------------------------------------------------------
  // views table
  $data['video']['table']['group']  = t('Video');
  $data['video']['table']['join'] = array(
    // ...to the node table
    'node' => array(
      'left_field' => 'nid',
      'field' => 'vid',
    ),
  );

 // Fields that can be inserted into a view
 // play counter
  $data['video']['play_counter'] = array(
    'title' => t('Play count'),
    'help' => t('This will display the number of times this has been played.'),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
     ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'title' => t('Play count'),
      'help' => t('Sort by the number of video plays.'),
      'handler' => 'views_handler_sort',
    ),
  );
  $data['video']['download_counter'] = array(
    'title' => t('Download count'),
    'help' => t('This will display the number of times this has been downloaded.'),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
     ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'title' => t('Download count'),
      'help' => t('Sort by the number of video downloads.'),
      'handler' => 'views_handler_sort',
    ),
  );    
  $data['video']['videox'] = array(
    'title' => t('Width (x)'),
    'help' => t('This will display the width (x) of the video'),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
     ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );         
  $data['video']['videoy'] = array(
    'title' => t('Height (y)'),
    'help' => t('This will display the height (y) of the video'),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
     ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );         
  $data['video']['playtime_seconds'] = array(
    'title' => t('Length'),
    'help' => t('This will display the play length of the video.'),
    'field' => array(
      'handler' => 'video_views_handler_field_playtime_seconds',
      'click sortable' => TRUE,
     ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'title' => t('Length'),
      'help' => t('Sort by the video length.'),
      'handler' => 'views_handler_sort',
    ),
  );
  $data['video']['download_link'] = array(
    'real field' => 'vidfile',
    'title' => t('Download link'),
    'help' => t('This will display a download link if the node allows it.'),
    'field' => array(
      'handler' => 'video_views_handler_field_download',
      'click sortable' => FALSE,
     )
  );
  $data['video']['play_link'] = array(
    'real field' => 'vidfile',
    'title' => t('Play link'),
    'help' => t('This will display a play link if the node allows it.'),
    'field' => array(
      'handler' => 'video_views_handler_field_play',
      'click sortable' => FALSE,
     ),
  );

  // Add video_image support only if the video_image module is enabled
  if (module_exists('video_image')) {
    $data['video']['video_image'] = array(
      'real field' => 'vidfile',
      'title' => t('Thumbnail'),
      'help' => t('This will display the thumbnail image for the video.'),
      'field' => array(
        'handler' => 'video_views_handler_field_image',
        'click sortable' => FALSE,
      ),
    );
  }
  return $data;
}
