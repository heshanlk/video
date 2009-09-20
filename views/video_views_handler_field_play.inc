<?php
//$Id$
/**
* Implementation of hook_views_tables
*
* @return
*   array - Enables support in the video module for views integration
 * @author Glen Marianko Twitter@demoforum <glenm at demoforum dot com>
 * @todo
**/

/**
 * Field handler to display the play length of the video.
 *
 * @ingroup views_field_handlers
 */
class video_views_handler_field_play extends views_handler_field {
  /**
   * Render field output to the browser.
   */
  function render($values) {
    return _video_views_handler_field_play($values);
  }
}
/**
* Handler to to render the "Play" link field
**/
function _video_views_handler_field_play($values) {
  if($values->node_type &&  $values->node_type != 'video') return NULL;
  return l(t('Play'), "node/$values->nid/play", array());
}
