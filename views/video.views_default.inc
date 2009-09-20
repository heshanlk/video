<?php
//$Id$
/**
* Implementation of hook_views_tables
*
* @return
*   array - Enables default video module Video Tracker vie
 * @author Glen Marianko Twitter@demoforum <glenm at demoforum dot com>
* @todo
**/

/**
* Provide a default view
*
* @return
    array - of views
**/
function video_views_default_views() {
    $views = array();
    //GMM: Set view->disabled to TRUE after pasting new export
    // view definition (Views 2)

    $view = new view;
    $view->name = 'video_tracker';
    $view->description = 'Shows all recent video activity (table format)';
    $view->tag = '';
    $view->view_php = '';
    $view->base_table = 'node';
    $view->is_cacheable = FALSE;
    $view->api_version = 2;
    $view->disabled = TRUE; /* Edit this to true to make a default view disabled initially */
    $handler = $view->new_display('default', 'Defaults', 'default');
    $handler->override_option('fields', array(
      'title' => array(
        'id' => 'title',
        'table' => 'node',
        'field' => 'title',
        'label' => 'Title',
        'link_to_node' => TRUE,
      ),
      'changed' => array(
        'id' => 'changed',
        'table' => 'node',
        'field' => 'changed',
        'label' => 'Last Updated',
        'date_format' => 'small',
      ),
      'name' => array(
        'id' => 'name',
        'table' => 'users',
        'field' => 'name',
        'label' => 'Author',
      ),
      'playtime_seconds' => array(
        'label' => 'Length',
        'alter' => array(
          'alter_text' => 0,
          'text' => '',
          'make_link' => 0,
          'path' => '',
          'link_class' => '',
          'alt' => '',
          'prefix' => '',
          'suffix' => '',
          'help' => '',
          'trim' => 0,
          'max_length' => '',
          'word_boundary' => 1,
          'ellipsis' => 1,
          'strip_tags' => 0,
          'html' => 0,
        ),
        'time_type' => 'hms',
        'exclude' => 0,
        'id' => 'playtime_seconds',
        'table' => 'video',
        'field' => 'playtime_seconds',
        'relationship' => 'none',
      ),
      'video_image' => array(
        'id' => 'video_image',
        'table' => 'video',
        'field' => 'video_image',
        'label' => 'Preview / Play',
      ),
    ));
    $handler->override_option('sorts', array(
      'changed' => array(
        'order' => 'DESC',
        'granularity' => 'second',
        'id' => 'changed',
        'table' => 'node',
        'field' => 'changed',
        'relationship' => 'none',
        'override' => array(
          'button' => 'Override',
        ),
      ),
    ));
    $handler->override_option('filters', array(
      'type' => array(
        'id' => 'type',
        'table' => 'node',
        'field' => 'type',
        'operator' => 'in',
        'value' => array(
          '0' => 'video',
        ),
      ),
      'status' => array(
        'id' => 'status',
        'table' => 'node',
        'field' => 'status',
        'value' => '1',
      ),
    ));
    $handler->override_option('access', array(
      'type' => 'none',
      'role' => array(),
      'perm' => '',
    ));
    $handler->override_option('cache', array(
      'type' => 'none',
    ));
    $handler->override_option('title', 'Recent video activity');
    $handler->override_option('header_format', '1');
    $handler->override_option('footer_format', '1');
    $handler->override_option('empty', 'There is no recent video activity');
    $handler->override_option('empty_format', '1');
    $handler->override_option('items_per_page', '20');
    $handler->override_option('use_pager', TRUE);
    $handler->override_option('style_plugin', 'table');
    $handler->override_option('style_options', array(
      'grouping' => '',
      'override' => 1,
      'sticky' => 0,
      'order' => 'desc',
      'columns' => array(
        'title' => 'title',
        'changed' => 'changed',
        'name' => 'name',
        'playtime_seconds' => 'playtime_seconds',
        'video_image' => 'video_image',
      ),
      'info' => array(
        'title' => array(
          'sortable' => 1,
          'separator' => '',
        ),
        'changed' => array(
          'sortable' => 1,
          'separator' => '',
        ),
        'name' => array(
          'sortable' => 1,
          'separator' => '',
        ),
        'playtime_seconds' => array(
          'sortable' => 1,
          'separator' => '',
        ),
        'video_image' => array(
          'separator' => '',
        ),
      ),
      'default' => 'changed',
    ));
    $handler = $view->new_display('page', 'Page', 'page_1');
    $handler->override_option('path', 'video/tracker');
    $handler->override_option('menu', array(
      'type' => 'none',
      'title' => '',
      'description' => '',
      'weight' => 0,
      'name' => 'navigation',
    ));
    $handler->override_option('tab_options', array(
      'type' => 'none',
      'title' => '',
      'description' => '',
      'weight' => 0,
    ));
    $handler = $view->new_display('block', 'Block', 'block_1');
    $handler->override_option('fields', array(
      'title' => array(
        'label' => '',
        'alter' => array(
          'alter_text' => 0,
          'text' => '',
          'make_link' => 0,
          'path' => '',
          'link_class' => '',
          'alt' => '',
          'prefix' => '',
          'suffix' => '',
          'help' => '',
          'trim' => 0,
          'max_length' => '',
          'word_boundary' => 1,
          'ellipsis' => 1,
          'strip_tags' => 0,
          'html' => 0,
        ),
        'link_to_node' => 1,
        'exclude' => 0,
        'id' => 'title',
        'table' => 'node',
        'field' => 'title',
        'override' => array(
          'button' => 'Use default',
        ),
        'relationship' => 'none',
      ),
      'video_image' => array(
        'label' => '',
        'alter' => array(
          'alter_text' => 0,
          'text' => '',
          'make_link' => 0,
          'path' => '',
          'link_class' => '',
          'alt' => '',
          'prefix' => '',
          'suffix' => '',
          'help' => '',
          'trim' => 0,
          'max_length' => '',
          'word_boundary' => 1,
          'ellipsis' => 1,
          'strip_tags' => 0,
          'html' => 0,
        ),
        'exclude' => 0,
        'id' => 'video_image',
        'table' => 'video',
        'field' => 'video_image',
        'override' => array(
          'button' => 'Use default',
        ),
        'relationship' => 'none',
      ),
    ));
    $handler->override_option('use_ajax', TRUE);
    $handler->override_option('items_per_page', 1);
    $handler->override_option('use_pager', 'mini');
    $handler->override_option('style_plugin', 'grid');
    $handler->override_option('style_options', array(
      'grouping' => '',
      'columns' => '1',
      'alignment' => 'horizontal',
    ));
    $handler->override_option('block_description', 'Video Tracker');
    $handler->override_option('block_caching', -1);

  // recent video node activity view
  $views[$view->name] = $view;
  return $views;
}

