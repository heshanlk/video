<?php
/**
* Implementation of hook_views_tables
*
* @return
*   array - Enables support in the video module for views integration
* porting to Drupal 6
* @author Heshan Wanigasooriya <heshan at heidisoft.com><heshanmw@gmail.com>
* @todo
**/

function video_views_tables() {
  $tables['video'] = array(
    'name' => 'video',
    'join' => array(
      'left' => array(
        'table' => 'node',
        'field' => 'vid'
      ),
      'right' => array(
        'field' => 'vid'
      )
    ),

 // Fields that can be inserted into a view
    'fields' => array(
       'play_counter' => array(
         'name' => t('Video: Play count'),
         'sortable' => true,
         'help' => t('This will display the number of times this has been played.'),
       ),
       'download_counter' => array(
         'name' => t('Video: Download count'),
         'sortable' => true,
         'help' => t('This will display the number of times this has been downloaded.'),
       ),
       'playtime_seconds' => array(
         'name' => t('Video: Length'),
         'handler' => 'video_views_handler_field_playtime_seconds',
         'sortable' => true,
         'help' => t('This will display the play length of the video.'),
       ),
       'download_link' => array(
         'name' => t('Video: Download link'),
         'handler' => 'video_views_handler_field_download',
         'notafield' => true,
         'sortable' => false,
         'help' => t('This will display a download link if the node allows it.'),
       ),
       'play_link' => array(
         'name' => t('Video: Play link'),
         'handler' => 'video_views_handler_field_play',
         'notafield' => true,
         'sortable' => false,
         'help' => t('This will display a play link if the node allows it.'),
       ),
       'videox' => array(
         'name' => t('Video: Width (x)'),
         'sortable' => true,
         'help' => t('This will display the width (x) of the video'),
       ),
       'videoy' =>array(
         'name' => t('Video: Height (y)'),
         'sortable' => true,
         'help' => t('This will display the height (y) of the video'),
       ),
    ),
    'sorts' => array(
      'play_counter' => array(
        'name' => t('Video: Play count'),
        'help' => t('Sort by the number of video plays.'),
       ),
      'download_counter' => array(
        'name' => t('Video: Download count'),
        'help' => t('Sort by the number of video downloads.'),
       ),
      'playtime_seconds' => array(
        'name' => t('Video: Length'),
        'help' => t('Sort by the video length.'),
      )
    )
  );

  // Add video_image support only if the video_image module is enabled
  if (module_exists('video_image')) {
    $tables['video']['fields']['video_image'] = array(
   'name' => t('Video: Thumbnail'),
      'notafield' => true,
      'handler' => 'video_views_handler_field_video_image',
      'sortable' => false,
      'help' => t('This will display the thumbnail image for the video.'),
      );
  }

  return $tables;
}


/**
* Provide a default view
*
* @return
    array - of views
**/
function video_views_default_views() {
  $views = array();

  // recent video node activity view
  $view = new stdClass();
  $view->name = 'video_tracker';
  $view->description = t('Shows all recent video activity (table format)');
  $view->access = array ();
  $view->view_args_php = '';
  $view->page = TRUE;
  $view->page_title = t('Recent video activity');
  $view->page_header = '';
  $view->page_header_format = '1';
  $view->page_footer = '';
  $view->page_footer_format = '1';
  $view->page_empty = t('There is no recent video activity');
  $view->page_empty_format = '1';
  $view->page_type = 'table';
  $view->url = 'video/tracker';
  $view->use_pager = TRUE;
  $view->nodes_per_page = '20';
  $view->sort = array ();
  $view->argument = array ();
  $view->field = array (
    array (
      'tablename' => 'node',
      'field' => 'title',
      'label' => t('Title'),
      'handler' => 'views_handler_field_nodelink',
      'sortable' => '1',
    ),
    array (
      'tablename' => 'node',
      'field' => 'changed',
      'label' => t('Last Updated'),
      'handler' => 'views_handler_field_date_small',
      'sortable' => '1',
      'defaultsort' => 'DESC',
    ),
    array (
      'tablename' => 'users',
      'field' => 'name',
      'label' => t('Author'),
    ),
    array (
      'tablename' => 'video',
      'field' => 'video_image',
      'label' => t('Preview / Play'),
    ),
  );
  $view->filter = array (
    array (
      'tablename' => 'node',
      'field' => 'type',
      'operator' => 'OR',
      'options' => '',
      'value' => array (
        0 => 'video',
      ),
    ),
    array (
      'tablename' => 'node',
      'field' => 'status',
      'operator' => '=',
      'options' => '',
      'value' => '1',
    ),
  );
  $view->exposed_filter = array ();
  $view->requires = array(node, users, video);
  $views[$view->name] = $view;

  return $views;
}


/**
* Handler to to render the "Download" link field
**/
function video_views_handler_field_download($fieldinfo, $fielddata, $value, $data) {
  $nid = $data->nid;
  return l(t('Download'), "node/$nid/download", array());
}


/**
* Handler to to render the "Play" link field
**/
function video_views_handler_field_play($fieldinfo, $fielddata, $value, $data) {
  $nid = $data->nid;
  return l(t('Play'), "node/$nid/play", array());
}


/**
* Handler to to render the correct playtime for the video in a field
**/
function video_views_handler_field_playtime_seconds($fieldinfo, $fielddata, $value, $data) {
  $seconds = $value;
  $hms = _video_sec2hms($seconds);

  // Pad the minutes / seconds with a leading "0", if
  // necessary
  if ($hms['hours'] > 0) {
    $hms['minutes'] = str_pad($hms['minutes'], 2, '0', STR_PAD_LEFT);
  }
  $hms['seconds'] = str_pad($hms['seconds'], 2, '0', STR_PAD_LEFT);

  $out = '';
  if ($hms['hours'] > 0) {
 $out .= $hms['hours'].":";
  }
  $out .= $hms['minutes'].":".$hms['seconds'];

  return t($out);
}


/**
* Handler to render the preview image associated with a video
**/
function video_views_handler_field_video_image($fieldinfo, $fielddata, $value, $data) {
 $node = node_load($data->nid);
 if($node->iid && $image = node_load($node->iid)) {
   $output = theme('video_image_teaser', $image, $node);
 }
 return $output;
}
