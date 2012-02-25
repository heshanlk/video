<?php

/**
 * @file
 * Provide views data and handlers for statistics.module
 */
/**
 * @defgroup views_statistics_module statistics.module handlers
 *
 * Includes the ability to create views of just the statistics table.
 * @{
 */

/**
 * Implements hook_views_data().
 */
function video_views_data() {
  // Basic table information.
  // ----------------------------------------------------------------
  // video_output table

  $data['video_queue']['table']['group'] = t('Video');

  $data['video_queue']['table']['join'] = array(
    // ...to the node table
    'node' => array(
      'left_field' => 'nid',
      'field' => 'entity_id',
    ),
  );

  // dimensions
  $data['video_queue']['dimensions'] = array(
    'title' => t('Dimensions'),
    'help' => t('The widthXheight of the video.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_string',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  // status
  $data['video_queue']['status'] = array(
    'title' => t('Status'),
    'help' => t('Video conversion status.'),
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

  // duration
  $data['video_queue']['duration'] = array(
    'title' => t('Duration'),
    'help' => t('Video playtime.'),
    'field' => array(
      'handler' => 'video_handler_field_video_duration',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  // started
  $data['video_queue']['started'] = array(
    'title' => t('Video conversion start'),
    'help' => t('The time that conversion process started.'),
    'field' => array(
      'handler' => 'views_handler_field_date',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_date',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  // completed
  $data['video_queue']['completed'] = array(
    'title' => t('Video conversion end'),
    'help' => t('The time that the conversion process completed.'),
    'field' => array(
      'handler' => 'views_handler_field_date',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_date',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  return $data;
}
